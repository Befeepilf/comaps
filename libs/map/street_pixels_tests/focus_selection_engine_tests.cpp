#include "testing/testing.hpp"

#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/focus_selection_engine.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace
{
std::string FabPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void FabRemove(std::string const & path) { Platform::RemoveFileIfExists(path); }

std::vector<m2::PointD> FabLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput FabMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
                                               std::vector<m2::PointD> const & ring)
{
  street_pixels::AreaCandidateInput input;
  input.m_osmId = osmId;
  input.m_osmType = street_pixels::OsmObjectType::Relation;
  input.m_geometrySource = street_pixels::GeometrySource::TrueClosedRing;
  input.m_name = name;
  input.m_kind = "admin";
  input.m_adminLevel = adminLevel;
  input.m_lonLatRings = {ring};
  return input;
}

struct FocusFx
{
  std::string leaf;
  std::string spaPath;
  int64_t mapDataVersion = 42;
  m2::PointD districtCentre;
  m2::PointD cityOnlyCentre;
};

FocusFx MakeFocusFx(std::string const & leaf)
{
  FocusFx fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  FabRemove(fx.spaPath);
  FabRemove(FabPath(leaf + ".pix"));
  FabRemove(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf));

  auto const config = street_pixels::CountryConfig::LoadFromString(R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": { "enabled": true, "place_types": ["neighbourhood"] }
    }
  }
})");
  auto const policy = config.GetByIso("FI");

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : {FabMakeAdmin(10, 10, "District", FabLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             FabMakeAdmin(8, 8, "City", FabLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  fx.districtCentre = mercator::FromLatLon(60.5, 24.5);
  fx.cityOnlyCentre = mercator::FromLatLon(60.1, 24.1);

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  std::vector<m2::PointD> samples = {fx.districtCentre, fx.cityOnlyCentre, mercator::FromLatLon(70.0, 30.0)};
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);
  return fx;
}

void CleanupFocusFx(FocusFx const & fx)
{
  FabRemove(fx.spaPath);
  FabRemove(FabPath(fx.leaf + ".pix"));
  FabRemove(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), fx.leaf));
}

UNIT_TEST(FocusEngine_Manager_Rule3_ExplicitSelect)
{
  auto fx = MakeFocusFx("sp036_explicit");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.SelectFocusedAreaExplicit(0, fx.spaPath), ());
  auto p = manager.GetFocusedAreaProgress();
  TEST(p.m_hasFocus, ());
  TEST(!p.m_citySummary, ());
  TEST_EQUAL(p.m_displayName, "District", ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_ApplySelection_CitySummary)
{
  auto fx = MakeFocusFx("sp036_city");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);

  street_pixels::FocusSelectionRequest req;
  req.m_event = street_pixels::FocusEvent::ZoomChanged;
  req.m_atCityScale = true;
  req.m_cityCompactIndex = 1;
  TEST(manager.ApplyFocusSelection(req, fx.spaPath, fx.mapDataVersion), ());
  auto p = manager.GetFocusedAreaProgress();
  TEST(p.m_hasFocus, ());
  TEST(p.m_citySummary, ());
  TEST_EQUAL(p.m_displayName, "City", ());
  TEST_EQUAL(p.m_compactIndex, 1u, ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_ApplySelection_RecentreUser)
{
  auto fx = MakeFocusFx("sp036_recentre");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);

  street_pixels::FocusSelectionRequest req;
  req.m_event = street_pixels::FocusEvent::Recentre;
  req.m_userAreaCompactIndex = 0;
  req.m_mapCentreAreaCompactIndex = 1;
  TEST(manager.ApplyFocusSelection(req, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());
  CleanupFocusFx(fx);
}
}  // namespace
