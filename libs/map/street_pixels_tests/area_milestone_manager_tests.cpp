#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string AmPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void AmRemove(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> AmLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput AmMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct AmFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string dbPath;
  int64_t mapDataVersion = 42;
  std::vector<m2::PointD> samples;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
};

AmFixture MakeAmFixture(std::string const & leaf)
{
  AmFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = AmPath(leaf + ".pix");
  fx.dbPath = AmPath(leaf + "_milestones.db");
  AmRemove(fx.spaPath);
  AmRemove(fx.pixPath);
  AmRemove(fx.dbPath);

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
  for (auto const & input : {AmMakeAdmin(10, 10, "District", AmLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             AmMakeAdmin(8, 8, "City", AmLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  int64_t const districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  int64_t const cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  int64_t const outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);

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
  params.m_policyVersion = config.GetPolicyVersion();
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

void CleanupAm(AmFixture const & fx)
{
  AmRemove(fx.spaPath);
  AmRemove(fx.pixPath);
  AmRemove(fx.dbPath);
  street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
}
}  // namespace

UNIT_TEST(AreaMilestoneManager_FiresOnRebuild)
{
  auto fx = MakeAmFixture("sp063_mgr_fire");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());

  auto record = manager.GetAreaMilestoneRecord(10);
  TEST(record.has_value(), ());
  TEST((record->m_firedMask & street_pixels::kAreaMilestoneMask100) != 0, ());
  TEST(!manager.GetAreaMilestoneRecord(8).has_value(), ());

  auto crossings = manager.ConsumePendingAreaMilestoneCrossings();
  TEST_EQUAL(crossings.size(), 3, ());
  TEST_EQUAL(crossings[0].m_osmId, 10u, ());

  CleanupAm(fx);
}

UNIT_TEST(AreaMilestoneManager_NoRefireAfterInvalidateRebuild)
{
  auto fx = MakeAmFixture("sp063_mgr_norefire");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  manager.ConsumePendingAreaMilestoneCrossings();

  manager.InvalidateAreaCompletionCache();
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(manager.ConsumePendingAreaMilestoneCrossings().size(), 0, ());

  auto record = manager.GetAreaMilestoneRecordByCompactIndex(0);
  TEST(record.has_value(), ());
  TEST((record->m_firedMask & street_pixels::kAreaMilestoneMask100) != 0, ());

  CleanupAm(fx);
}

UNIT_TEST(AreaMilestoneManager_ImportCanCrossThreshold)
{
  auto fx = MakeAmFixture("sp063_mgr_import");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(1), 0.0, ());
  TEST(!manager.GetAreaMilestoneRecord(8).has_value(), ());

  std::set<int64_t> const universe = {fx.districtId, fx.cityOnlyId, fx.outsideId};
  street_pixels_file::ExploredEverLiveMap explored{
      {fx.districtId, true},
      {fx.cityOnlyId, false},
  };
  TEST(street_pixels_file::SaveRematchedUniverse(fx.pixPath, universe, explored, fx.mapDataVersion), ());

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(1), 1.0, ());

  auto record = manager.GetAreaMilestoneRecord(8);
  TEST(record.has_value(), ());
  TEST((record->m_firedMask & street_pixels::kAreaMilestoneMask100) != 0, ());

  CleanupAm(fx);
}

UNIT_TEST(AreaMilestoneManager_PreviouslyCompletedBelow100)
{
  auto fx = MakeAmFixture("sp063_mgr_prev");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  manager.ConsumePendingAreaMilestoneCrossings();
  TEST(!manager.WasAreaPreviouslyCompletedBelow100(0), ());

  std::set<int64_t> const universe = {fx.districtId, fx.cityOnlyId, fx.outsideId};
  street_pixels_file::ExploredEverLiveMap explored{{fx.cityOnlyId, false}};
  TEST(street_pixels_file::SaveRematchedUniverse(fx.pixPath, universe, explored, fx.mapDataVersion), ());

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(0), 0.0, ());
  TEST(manager.WasAreaPreviouslyCompletedBelow100(0), ());

  auto record = manager.GetAreaMilestoneRecordByCompactIndex(0);
  TEST(record.has_value(), ());
  TEST(record->m_completed100At.has_value(), ());

  CleanupAm(fx);
}
