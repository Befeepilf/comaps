#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
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
std::string AcPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void AcRemove(std::string const & path) { Platform::RemoveFileIfExists(path); }

std::vector<m2::PointD> AcLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput AcMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct AcMgrFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string spxPath;
  int64_t mapDataVersion = 42;
  uint32_t policyVersion = 1;
  std::vector<m2::PointD> samples;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
};

AcMgrFixture MakeAcFixture(std::string const & leaf)
{
  AcMgrFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = AcPath(leaf + ".pix");
  fx.spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  AcRemove(fx.spaPath);
  AcRemove(fx.pixPath);
  AcRemove(fx.spxPath);

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
  for (auto const & input : {AcMakeAdmin(10, 10, "District", AcLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             AcMakeAdmin(8, 8, "City", AcLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  // Real HEALPix nest ids whose centres fall in district / city-only / outside.
  int64_t const districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  int64_t const cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  int64_t const outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);
  TEST(districtId != cityOnlyId, ());
  TEST(districtId != outsideId, ());

  // Dense assign slots follow ascending universe order (SP-028).
  std::vector<std::pair<int64_t, m2::PointD>> universeRows = {
      {districtId, mercator::FromLatLon(60.5, 24.5)},
      {cityOnlyId, mercator::FromLatLon(60.1, 24.1)},
      {outsideId, mercator::FromLatLon(70.0, 30.0)},
  };
  std::sort(universeRows.begin(), universeRows.end(),
            [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  fx.samples.clear();
  for (auto const & row : universeRows)
  {
    universeIds.push_back(row.first);
    fx.samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = fx.policyVersion;
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, fx.samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{districtId, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  fx.districtId = districtId;
  fx.cityOnlyId = cityOnlyId;
  fx.outsideId = outsideId;
  return fx;
}

void CleanupAc(AcMgrFixture const & fx)
{
  AcRemove(fx.spaPath);
  AcRemove(fx.pixPath);
  AcRemove(fx.spxPath);
}
}  // namespace

UNIT_TEST(AreaCompletionManager_RebuildAndFraction)
{
  auto fx = MakeAcFixture("sp034_mgr_rebuild");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.IsAreaCompletionCacheValid(), ());

  auto district = manager.GetAreaCompletion(0);
  TEST(district.has_value(), ());
  TEST_EQUAL(district->m_compactIndex, 0u, ());
  TEST_EQUAL(district->m_osmId, 10u, ());
  TEST_EQUAL(district->m_total, 1u, ());
  TEST_EQUAL(district->m_explored, 1u, ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(0), 1.0, ());

  auto city = manager.GetAreaCompletion(1);
  TEST(city.has_value(), ());
  TEST_EQUAL(city->m_total, 1u, ());
  TEST_EQUAL(city->m_explored, 0u, ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(1), 0.0, ());

  CleanupAc(fx);
}

UNIT_TEST(AreaCompletionManager_ImportIncrementsWithoutInvalidating)
{
  auto fx = MakeAcFixture("sp034_mgr_import");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.IsAreaCompletionCacheValid(), ());
  TEST(manager.GetAreaCompletion(1).has_value(), ());
  TEST_EQUAL(manager.GetAreaCompletion(1)->m_explored, 0u, ());

  manager.SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false),
  });
  TEST(manager.SetFocusedArea(1, fx.spaPath), ());
  TEST(manager.GetFocusedAreaProgress().m_fractionValid, ());
  TEST_EQUAL(manager.GetFocusedAreaProgress().m_fraction, 0.0, ());

  manager.MarkImportedPixelsForTesting({fx.cityOnlyId});
  TEST(manager.IsAreaCompletionCacheValid(), ());
  TEST(manager.GetAreaCompletion(0).has_value(), ());
  TEST(manager.GetAreaCompletion(1).has_value(), ());
  TEST_EQUAL(manager.GetAreaCompletion(0)->m_explored, 1u, ());
  TEST_EQUAL(manager.GetAreaCompletion(1)->m_explored, 1u, ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(1), 1.0, ());
  auto progress = manager.GetFocusedAreaProgress();
  TEST(progress.m_fractionValid, ());
  TEST_EQUAL(progress.m_fraction, 1.0, ());
  TEST(progress.m_areaCompleted, ());

  manager.MarkImportedPixelsForTesting({fx.cityOnlyId});
  TEST_EQUAL(manager.GetAreaCompletion(1)->m_explored, 1u, ());

  manager.MarkImportedPixelsForTesting({fx.outsideId});
  TEST(manager.IsAreaCompletionCacheValid(), ());
  TEST_EQUAL(manager.GetAreaCompletion(1)->m_explored, 1u, ());

  CleanupAc(fx);
}

UNIT_TEST(AreaCompletionManager_InvalidateOnRematch)
{
  auto fx = MakeAcFixture("sp034_mgr_rematch");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.IsAreaCompletionCacheValid(), ());

  TEST(manager.RematchStreetPixelsWithNewUniverseForTesting(
           fx.leaf, {fx.districtId, fx.cityOnlyId, fx.outsideId, fx.outsideId + 1}, fx.mapDataVersion + 1),
       ());
  TEST(!manager.IsAreaCompletionCacheValid(), ());

  CleanupAc(fx);
}

UNIT_TEST(AreaCompletionManager_InvalidateOnExplicitClear)
{
  auto fx = MakeAcFixture("sp034_mgr_clear");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.IsAreaCompletionCacheValid(), ());

  manager.InvalidateAreaCompletionCache();
  TEST(!manager.IsAreaCompletionCacheValid(), ());

  CleanupAc(fx);
}

UNIT_TEST(AreaCompletionManager_PolicyBumpRebuilds)
{
  auto fx = MakeAcFixture("sp034_mgr_policy");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.IsAreaCompletionCacheValid(), ());

  auto const config = street_pixels::CountryConfig::LoadFromString(R"({
  "policy_version": 2,
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
  for (auto const & input : {AcMakeAdmin(10, 10, "District", AcLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             AcMakeAdmin(8, 8, "City", AcLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }
  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = 2;
  params.m_isoCode = "FI";
  params.m_mwmId = fx.leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, fx.samples, policy, params);

  TEST(manager.RematerializeAssignmentsOnPolicyBump(fx.leaf, fx.spaPath, fx.mapDataVersion, 2), ());
  TEST(manager.IsAreaCompletionCacheValid(), ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(0), 1.0, ());

  CleanupAc(fx);
}
