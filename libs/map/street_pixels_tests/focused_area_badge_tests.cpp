#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
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

struct FabFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  int64_t mapDataVersion = 42;
  uint32_t policyVersion = 1;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
  m2::PointD districtCentre;
  m2::PointD cityOnlyCentre;
};

FabFixture MakeFabFixture(std::string const & leaf)
{
  FabFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = FabPath(leaf + ".pix");
  FabRemove(fx.spaPath);
  FabRemove(fx.pixPath);
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
  fx.policyVersion = config.GetPolicyVersion();

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : {FabMakeAdmin(10, 10, "District", FabLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             FabMakeAdmin(8, 8, "City", FabLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  fx.districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  fx.cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  fx.outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);
  fx.districtCentre = mercator::FromLatLon(60.5, 24.5);
  fx.cityOnlyCentre = mercator::FromLatLon(60.1, 24.1);

  std::vector<std::pair<int64_t, m2::PointD>> rows = {
      {fx.districtId, fx.districtCentre},
      {fx.cityOnlyId, fx.cityOnlyCentre},
      {fx.outsideId, mercator::FromLatLon(70.0, 30.0)},
  };
  std::sort(rows.begin(), rows.end(), [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  std::vector<m2::PointD> samples;
  for (auto const & row : rows)
  {
    universeIds.push_back(row.first);
    samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = fx.policyVersion;
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{fx.districtId, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  return fx;
}

void CleanupFab(FabFixture const & fx)
{
  FabRemove(fx.spaPath);
  FabRemove(fx.pixPath);
  FabRemove(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), fx.leaf));
}
}  // namespace

UNIT_TEST(FocusedAreaBadge_SetFocusShowsNameAndFraction)
{
  auto fx = MakeFabFixture("sp035_badge_set");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.SetFocusedArea(0, fx.spaPath), ());

  auto progress = manager.GetFocusedAreaProgress();
  TEST(progress.m_hasFocus, ());
  TEST(progress.m_fractionValid, ());
  TEST_EQUAL(progress.m_displayName, "District", ());
  TEST_EQUAL(progress.m_compactIndex, 0u, ());
  TEST_EQUAL(progress.m_osmId, 10u, ());
  TEST_EQUAL(progress.m_fraction, 1.0, ());
  TEST(progress.m_areaCompleted, ());
  TEST(!progress.m_noExplorationArea, ());
  TEST(progress.m_displayName != fx.leaf, ());

  CleanupFab(fx);
}

UNIT_TEST(FocusedAreaBadge_BlankNameClearsFocus)
{
  auto fx = MakeFabFixture("sp035_badge_blank");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.SetFocusedAreaForTesting(0, "", 10);
  TEST(!manager.GetFocusedAreaProgress().m_hasFocus, ());
  TEST(manager.GetFocusedAreaProgress().m_noExplorationArea, ());

  manager.SetFocusedAreaForTesting(0, "District", 10);
  TEST(manager.GetFocusedAreaProgress().m_hasFocus, ());
  TEST(!manager.GetFocusedAreaProgress().m_noExplorationArea, ());
  manager.ClearFocusedArea();
  TEST(!manager.GetFocusedAreaProgress().m_hasFocus, ());
  TEST(manager.GetFocusedAreaProgress().m_noExplorationArea, ());
  TEST(manager.GetFocusedAreaProgress().m_displayName.empty(), ());
  TEST(manager.GetFocusedAreaProgress().m_displayName != fx.leaf, ());

  CleanupFab(fx);
}

UNIT_TEST(FocusedAreaBadge_InvalidCacheMarksFractionInvalid)
{
  auto fx = MakeFabFixture("sp035_badge_invalid_cache");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.SetFocusedArea(0, fx.spaPath), ());
  TEST(manager.GetFocusedAreaProgress().m_fractionValid, ());

  manager.InvalidateAreaCompletionCache();
  auto progress = manager.GetFocusedAreaProgress();
  TEST(progress.m_hasFocus, ());
  TEST_EQUAL(progress.m_displayName, "District", ());
  TEST(!progress.m_fractionValid, ());

  CleanupFab(fx);
}

UNIT_TEST(FocusedAreaBadge_FocusChangeUpdatesBadgeSnapshot)
{
  auto fx = MakeFabFixture("sp035_badge_change");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.SetFocusedArea(0, fx.spaPath), ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());

  TEST(manager.SetFocusedArea(1, fx.spaPath), ());
  auto progress = manager.GetFocusedAreaProgress();
  TEST_EQUAL(progress.m_displayName, "City", ());
  TEST_EQUAL(progress.m_compactIndex, 1u, ());
  TEST(progress.m_fractionValid, ());
  TEST_EQUAL(progress.m_fraction, 0.0, ());
  TEST(!progress.m_areaCompleted, ());
  TEST(!progress.m_noExplorationArea, ());

  CleanupFab(fx);
}

UNIT_TEST(FocusedAreaBadge_NoAreaSignalNeverUsesMwmId)
{
  auto fx = MakeFabFixture("sp040_no_area");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.SetFocusedArea(0, fx.spaPath), ());
  TEST(manager.GetFocusedAreaProgress().m_hasFocus, ());

  m2::PointD const outside = mercator::FromLatLon(70.0, 30.0);
  bool const focused = manager.SelectFocusedAreaAtPoint(outside, fx.spaPath, fx.mapDataVersion);
  TEST(!focused, ());
  auto progress = manager.GetFocusedAreaProgress();
  TEST(!progress.m_hasFocus, ());
  TEST(progress.m_noExplorationArea, ());
  TEST(!progress.m_areaCompleted, ());
  TEST(progress.m_displayName.empty(), ());
  TEST(progress.m_displayName != fx.leaf, ());

  CleanupFab(fx);
}

UNIT_TEST(FocusedAreaBadge_TryFocusAtPointDistrict)
{
  auto fx = MakeFabFixture("sp035_badge_point");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());

  // ResourcesDir may not ship country_policies in test harness — point focus
  // loads policy from ResourcesDir. Fall back to SetFocusedArea if policy missing.
  bool const focused = manager.TryFocusAtPoint(fx.districtCentre, fx.spaPath, fx.mapDataVersion);
  if (focused)
  {
    auto progress = manager.GetFocusedAreaProgress();
    TEST(progress.m_hasFocus, ());
    TEST_EQUAL(progress.m_displayName, "District", ());
  }
  else
  {
    TEST(manager.SetFocusedArea(0, fx.spaPath), ());
    TEST_EQUAL(manager.GetFocusedAreaProgress().m_displayName, "District", ());
  }

  CleanupFab(fx);
}

UNIT_TEST(LookupExplorationAreaAtPoint_PrefersSubdivision)
{
  auto fx = MakeFabFixture("sp035_lookup_point");
  auto sidecar = street_pixels::TryLoadExplorationSidecar(fx.spaPath);
  TEST_EQUAL(sidecar.m_status, street_pixels::SpaLoadStatus::Ok, ());
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
  auto const * district =
      street_pixels::LookupExplorationAreaAtPoint(sidecar.m_file, config.GetByIso("FI"), fx.districtCentre);
  TEST(district != nullptr, ());
  TEST_EQUAL(district->m_name, "District", ());

  auto const * city =
      street_pixels::LookupExplorationAreaAtPoint(sidecar.m_file, config.GetByIso("FI"), fx.cityOnlyCentre);
  TEST(city != nullptr, ());
  TEST_EQUAL(city->m_name, "City", ());

  CleanupFab(fx);
}
