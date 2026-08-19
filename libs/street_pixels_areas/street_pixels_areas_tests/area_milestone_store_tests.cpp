#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/area_completion_cache.hpp"
#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <string>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

std::string MilestoneDbPath(std::string const & leaf)
{
  return base::JoinPath(GetPlatform().WritableDir(), leaf + ".db");
}

struct MilestoneFixture
{
  std::string m_path;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  std::vector<int64_t> m_universe;
  CountryPolicy m_policy;
};

AreaCompletionCache BuildDistrictCityCache(MilestoneFixture const & fx, std::vector<int64_t> const & explored)
{
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());
  return AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
}

MilestoneFixture MakeDistrictCityFixture(std::string const & leaf)
{
  auto const config = FinlandConfig();
  MilestoneFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.1, 60.1),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 20, 30};
  fx.m_params.m_mapDataVersion = 340;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = leaf;
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

MilestoneFixture MakeTwoPixelDistrictFixture(std::string const & leaf)
{
  auto fx = MakeDistrictCityFixture(leaf);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.6, 60.6),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 15, 30};
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

MilestoneFixture MakeEmptyDistrictFixture(std::string const & leaf)
{
  auto const config = FinlandConfig();
  MilestoneFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "EmptyDistrict", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.1, 60.1),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {20, 30};
  fx.m_params.m_mapDataVersion = 341;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = leaf;
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}
}  // namespace

UNIT_TEST(AreaMilestone_FireOncePerThreshold)
{
  auto const dbPath = MilestoneDbPath("sp063_fire_once");
  Platform::RemoveFileIfExists(dbPath);

  auto fx = MakeTwoPixelDistrictFixture("sp063_fire_once_spa");
  AreaMilestoneStore store(dbPath);

  auto cache50 = BuildDistrictCityCache(fx, {10});
  auto crossings = store.EvaluateAndRecordFires(cache50, 1000);
  TEST_EQUAL(crossings.size(), 2, ());
  TEST_EQUAL(crossings[0].m_threshold, AreaMilestoneThreshold::P50, ());
  TEST_EQUAL(crossings[1].m_threshold, AreaMilestoneThreshold::P25, ());

  auto record = store.Get(10);
  TEST(record.has_value(), ());
  TEST_EQUAL(record->m_firedMask, kAreaMilestoneMask25 | kAreaMilestoneMask50, ());

  crossings = store.EvaluateAndRecordFires(cache50, 1001);
  TEST_EQUAL(crossings.size(), 0, ());

  auto cache100 = BuildDistrictCityCache(fx, {10, 15});
  crossings = store.EvaluateAndRecordFires(cache100, 1002);
  TEST_EQUAL(crossings.size(), 1, ());
  TEST_EQUAL(crossings[0].m_threshold, AreaMilestoneThreshold::P100, ());

  record = store.Get(10);
  TEST(record.has_value(), ());
  TEST_EQUAL(record->m_firedMask, kAreaMilestoneMask25 | kAreaMilestoneMask50 | kAreaMilestoneMask100, ());
  TEST(record->m_completed100At.has_value(), ());
  TEST_EQUAL(*record->m_completed100At, 1002, ());

  Platform::RemoveFileIfExists(dbPath);
  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaMilestone_TripleCrossOneUpdate)
{
  auto const dbPath = MilestoneDbPath("sp063_triple");
  Platform::RemoveFileIfExists(dbPath);

  auto fx = MakeDistrictCityFixture("sp063_triple_spa");
  AreaMilestoneStore store(dbPath);

  auto cache = BuildDistrictCityCache(fx, {10});
  auto crossings = store.EvaluateAndRecordFires(cache, 2000);
  TEST_EQUAL(crossings.size(), 3, ());
  TEST_EQUAL(crossings[0].m_threshold, AreaMilestoneThreshold::P100, ());
  TEST_EQUAL(crossings[1].m_threshold, AreaMilestoneThreshold::P50, ());
  TEST_EQUAL(crossings[2].m_threshold, AreaMilestoneThreshold::P25, ());

  Platform::RemoveFileIfExists(dbPath);
  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaMilestone_NoRefireAfterDrop)
{
  auto const dbPath = MilestoneDbPath("sp063_norefire");
  Platform::RemoveFileIfExists(dbPath);

  auto fx = MakeDistrictCityFixture("sp063_norefire_spa");
  AreaMilestoneStore store(dbPath);

  auto full = BuildDistrictCityCache(fx, {10});
  store.EvaluateAndRecordFires(full, 3000);

  auto partial = BuildDistrictCityCache(fx, {});
  auto crossings = store.EvaluateAndRecordFires(partial, 3001);
  TEST_EQUAL(crossings.size(), 0, ());

  auto record = store.Get(10);
  TEST(record.has_value(), ());
  TEST((record->m_firedMask & kAreaMilestoneMask100) != 0, ());
  TEST(record->m_completed100At.has_value(), ());
  TEST_EQUAL(*record->m_completed100At, 3000, ());
  TEST(store.WasPreviouslyCompletedBelow100(10, 0.0), ());

  Platform::RemoveFileIfExists(dbPath);
  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaMilestone_ZeroTotalDoesNotFire)
{
  auto const dbPath = MilestoneDbPath("sp063_zero");
  Platform::RemoveFileIfExists(dbPath);

  auto fx = MakeEmptyDistrictFixture("sp063_zero_spa");
  AreaMilestoneStore store(dbPath);

  auto cache = BuildDistrictCityCache(fx, {});
  auto crossings = store.EvaluateAndRecordFires(cache, 4000);
  TEST_EQUAL(crossings.size(), 0, ());
  TEST(!store.Get(10).has_value(), ());

  Platform::RemoveFileIfExists(dbPath);
  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaMilestone_ConsumePendingCrossings)
{
  auto const dbPath = MilestoneDbPath("sp063_pending");
  Platform::RemoveFileIfExists(dbPath);

  auto fx = MakeDistrictCityFixture("sp063_pending_spa");
  AreaMilestoneStore store(dbPath);

  auto cache = BuildDistrictCityCache(fx, {10});
  store.EvaluateAndRecordFires(cache, 5000);
  auto pending = store.ConsumePendingCrossings();
  TEST_EQUAL(pending.size(), 3, ());
  TEST_EQUAL(store.ConsumePendingCrossings().size(), 0, ());

  Platform::RemoveFileIfExists(dbPath);
  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaMilestone_OsmIdStableAcrossCacheRebuild)
{
  auto const dbPath = MilestoneDbPath("sp063_osm");
  Platform::RemoveFileIfExists(dbPath);

  auto fx = MakeDistrictCityFixture("sp063_osm_spa");
  AreaMilestoneStore store(dbPath);

  auto cache = BuildDistrictCityCache(fx, {10});
  store.EvaluateAndRecordFires(cache, 6000);

  fx.m_params.m_mapDataVersion = 341;
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  auto rebuilt = BuildDistrictCityCache(fx, {10});
  auto crossings = store.EvaluateAndRecordFires(rebuilt, 6001);
  TEST_EQUAL(crossings.size(), 0, ());

  auto record = store.Get(10);
  TEST(record.has_value(), ());
  TEST((record->m_firedMask & kAreaMilestoneMask100) != 0, ());

  Platform::RemoveFileIfExists(dbPath);
  RemoveIfExists(fx.m_path);
}
