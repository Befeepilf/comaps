#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
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

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string Sp036Path(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void Sp036Remove(std::string const & path) { Platform::RemoveFileIfExists(path); }

std::vector<m2::PointD> Sp036LonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput Sp036MakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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
  Sp036Remove(fx.spaPath);
  Sp036Remove(Sp036Path(leaf + ".pix"));
  Sp036Remove(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf));

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
  for (auto const & input : {Sp036MakeAdmin(10, 10, "District", Sp036LonLatBox(24.2, 60.2, 24.8, 60.8)),
                             Sp036MakeAdmin(8, 8, "City", Sp036LonLatBox(24.0, 60.0, 25.0, 61.0))})
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
  Sp036Remove(fx.spaPath);
  Sp036Remove(Sp036Path(fx.leaf + ".pix"));
  Sp036Remove(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), fx.leaf));
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

UNIT_TEST(FocusEngine_Manager_SelectAtPoint_PrefersSubdivision)
{
  auto fx = MakeFocusFx("sp038_tap_district");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  bool const ok = manager.SelectFocusedAreaAtPoint(fx.districtCentre, fx.spaPath, fx.mapDataVersion);
  if (ok)
  {
    auto p = manager.GetFocusedAreaProgress();
    TEST(p.m_hasFocus, ());
    TEST_EQUAL(p.m_displayName, "District", ());
    TEST(!p.m_citySummary, ());
  }
  else
  {
    TEST(manager.SelectFocusedAreaExplicit(0, fx.spaPath), ());
    TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());
  }
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_SelectAtPoint_OutsideClears)
{
  auto fx = MakeFocusFx("sp038_tap_outside");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.SelectFocusedAreaExplicit(0, fx.spaPath), ());
  TEST(manager.GetFocusedAreaProgress().m_hasFocus, ());

  m2::PointD const outside = mercator::FromLatLon(70.0, 30.0);
  bool const focused = manager.SelectFocusedAreaAtPoint(outside, fx.spaPath, fx.mapDataVersion);
  if (!focused)
  {
    auto p = manager.GetFocusedAreaProgress();
    TEST(!p.m_hasFocus, ());
    TEST(p.m_noExplorationArea, ());
    TEST(p.m_displayName.empty(), ());
  }
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_IdlePanRefreshFollowsMapCentre)
{
  auto fx = MakeFocusFx("sp038_pan_centre");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RefreshFocusFromViewport(fx.districtCentre, std::nullopt, false, false, 16, fx.spaPath,
                                        fx.mapDataVersion),
       ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());

  TEST(manager.RefreshFocusFromViewport(fx.cityOnlyCentre, std::nullopt, false, false, 16, fx.spaPath,
                                        fx.mapDataVersion),
       ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "City", ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_HasExplorationAreaAtPoint)
{
  auto fx = MakeFocusFx("sp038_has_area");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.HasExplorationAreaAtPoint(fx.districtCentre, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.HasExplorationAreaAtPoint(fx.cityOnlyCentre, fx.spaPath, fx.mapDataVersion), ());
  TEST(!manager.HasExplorationAreaAtPoint(mercator::FromLatLon(70.0, 30.0), fx.spaPath, fx.mapDataVersion), ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_FollowRefreshUsesUserArea)
{
  auto fx = MakeFocusFx("sp035_follow_user");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RefreshFocusFromViewport(fx.cityOnlyCentre, fx.districtCentre, false, true, 16, fx.spaPath,
                                        fx.mapDataVersion),
       ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());
  TEST(!manager.GetFocusedAreaProgress().m_citySummary, ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_ExplicitStickyIgnoresIdlePanRefresh)
{
  auto fx = MakeFocusFx("sp038_sticky");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.SelectFocusedAreaExplicit(0, fx.spaPath), ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());

  TEST(manager.RefreshFocusFromViewport(fx.cityOnlyCentre, std::nullopt, false, false, 16, fx.spaPath,
                                        fx.mapDataVersion),
       ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_CityScaleRefreshUsesCitySummary)
{
  auto fx = MakeFocusFx("sp039_city_zoom_refresh");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RefreshFocusFromViewport(fx.districtCentre, std::nullopt, false, false, 10, fx.spaPath,
                                        fx.mapDataVersion),
       ());
  auto p = manager.GetFocusedAreaProgress();
  TEST(p.m_hasFocus, ());
  TEST(p.m_citySummary, ());
  TEST_EQUAL(p.m_displayName, "City", ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_CitySummaryFailClosedWithoutPix)
{
  auto fx = MakeFocusFx("sp039_city_failclosed");
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
  TEST(!p.m_fractionValid, ());
  CleanupFocusFx(fx);
}

UNIT_TEST(FocusEngine_Manager_CitySummaryUsesRollupFraction)
{
  // City-summary badge fraction = sum(explored)/sum(total) over settlement + contained
  // assignables — not settlement-alone and not an average of area percentages.
  auto fx = MakeFocusFx("sp039_city_rollup");
  int64_t const districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  int64_t const cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  int64_t const outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);
  TEST(districtId != cityOnlyId, ());
  TEST(districtId != outsideId, ());

  std::vector<std::pair<int64_t, m2::PointD>> universeRows = {
      {districtId, mercator::FromLatLon(60.5, 24.5)},
      {cityOnlyId, mercator::FromLatLon(60.1, 24.1)},
      {outsideId, mercator::FromLatLon(70.0, 30.0)},
  };
  std::sort(universeRows.begin(), universeRows.end(),
            [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  std::vector<m2::PointD> samples;
  for (auto const & row : universeRows)
  {
    universeIds.push_back(row.first);
    samples.push_back(row.second);
  }

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
  for (auto const & input : {Sp036MakeAdmin(10, 10, "District", Sp036LonLatBox(24.2, 60.2, 24.8, 60.8)),
                             Sp036MakeAdmin(8, 8, "City", Sp036LonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }
  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = fx.leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{districtId, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           Sp036Path(fx.leaf + ".pix"), std::set<int64_t>(universeIds.begin(), universeIds.end()), seed,
           fx.mapDataVersion),
       ());

  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());

  auto const city = manager.GetCityCompletion(1);
  TEST(city.has_value(), ());
  TEST_EQUAL(city->m_explored, 1u, ());
  TEST_EQUAL(city->m_total, 2u, ());
  TEST_EQUAL(manager.GetCityCompletionFraction(1), 0.5, ());

  auto const settlementOnly = manager.GetAreaCompletion(1);
  TEST(settlementOnly.has_value(), ());
  TEST_EQUAL(settlementOnly->m_explored, 0u, ());
  TEST_EQUAL(settlementOnly->m_total, 1u, ());

  street_pixels::FocusSelectionRequest req;
  req.m_event = street_pixels::FocusEvent::ZoomChanged;
  req.m_atCityScale = true;
  req.m_cityCompactIndex = 1;
  TEST(manager.ApplyFocusSelection(req, fx.spaPath, fx.mapDataVersion), ());
  auto p = manager.GetFocusedAreaProgress();
  TEST(p.m_hasFocus, ());
  TEST(p.m_citySummary, ());
  TEST_EQUAL(p.m_displayName, "City", ());
  TEST(p.m_fractionValid, ());
  TEST_EQUAL(p.m_fraction, 0.5, ());
  TEST_NOT_EQUAL(p.m_fraction, manager.GetAreaCompletionFraction(1),
                  ("City badge must not use settlement-only fraction"));

  CleanupFocusFx(fx);
}
}  // namespace
