#include "testing/testing.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/live_recency_store.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <string>

namespace
{
using namespace street_pixels;

std::string RecencyDbPath(std::string const & leaf)
{
  return base::JoinPath(GetPlatform().WritableDir(), leaf + ".db");
}

void RemoveRecencyDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}
}  // namespace

UNIT_TEST(LiveRecency_SeedInsertOrIgnoreKeepsFirstTimestamp)
{
  auto const dbPath = RecencyDbPath("sp072_seed_ignore");
  RemoveRecencyDb(dbPath);
  LiveRecencyStore store(dbPath);

  store.SeedEverLive({10, 20}, 1000);
  auto t10 = store.GetLastLiveVisit(10);
  auto t20 = store.GetLastLiveVisit(20);
  TEST(t10.has_value(), ());
  TEST(t20.has_value(), ());
  TEST_EQUAL(*t10, 1000, ());
  TEST_EQUAL(*t20, 1000, ());

  store.SeedEverLive({10, 30}, 2000);
  t10 = store.GetLastLiveVisit(10);
  t20 = store.GetLastLiveVisit(20);
  auto t30 = store.GetLastLiveVisit(30);
  TEST(t10.has_value(), ());
  TEST_EQUAL(*t10, 1000, ());
  TEST(t20.has_value(), ());
  TEST_EQUAL(*t20, 1000, ());
  TEST(t30.has_value(), ());
  TEST_EQUAL(*t30, 2000, ());

  RemoveRecencyDb(dbPath);
}

UNIT_TEST(LiveRecency_TouchOverwritesTimestamp)
{
  auto const dbPath = RecencyDbPath("sp072_touch_overwrite");
  RemoveRecencyDb(dbPath);
  LiveRecencyStore store(dbPath);

  store.SeedEverLive({10}, 1000);
  TEST_EQUAL(*store.GetLastLiveVisit(10), 1000, ());

  store.TouchLiveVisits({10}, 2500);
  TEST_EQUAL(*store.GetLastLiveVisit(10), 2500, ());

  store.TouchLiveVisits({10, 11}, 3000);
  TEST_EQUAL(*store.GetLastLiveVisit(10), 3000, ());
  TEST_EQUAL(*store.GetLastLiveVisit(11), 3000, ());

  RemoveRecencyDb(dbPath);
}

UNIT_TEST(LiveRecency_DefaultPathIsLiveRecencyNotMilestones)
{
  auto const recency = LiveRecencyStore::DefaultDbPath();
  auto const milestones = AreaMilestoneStore::DefaultDbPath();
  TEST_EQUAL(recency, GetPlatform().WritablePathForFile("live_recency.db"), ());
  TEST_NOT_EQUAL(recency, milestones, ());
  TEST(recency.find("live_recency.db") != std::string::npos, ());
  TEST(recency.find("area_milestones.db") == std::string::npos, ());
}

UNIT_TEST(LiveRecency_TempDbRemovesWalAndShm)
{
  auto const dbPath = RecencyDbPath("sp072_temp_wal");
  RemoveRecencyDb(dbPath);
  {
    LiveRecencyStore store(dbPath);
    store.TouchLiveVisits({42}, 9);
    TEST_EQUAL(*store.GetLastLiveVisit(42), 9, ());
  }
  RemoveRecencyDb(dbPath);
  TEST(!Platform::IsFileExistsByFullPath(dbPath), ());
  TEST(!Platform::IsFileExistsByFullPath(dbPath + "-wal"), ());
  TEST(!Platform::IsFileExistsByFullPath(dbPath + "-shm"), ());
}

UNIT_TEST(LiveRecency_GetLastLiveVisitsBatch)
{
  auto const dbPath = RecencyDbPath("sp072_batch");
  RemoveRecencyDb(dbPath);
  LiveRecencyStore store(dbPath);
  store.TouchLiveVisits({1, 2}, 50);
  auto const visits = store.GetLastLiveVisits({2, 3, 1});
  TEST_EQUAL(visits.size(), 3, ());
  TEST(visits[0].has_value(), ());
  TEST_EQUAL(*visits[0], 50, ());
  TEST(!visits[1].has_value(), ());
  TEST(visits[2].has_value(), ());
  TEST_EQUAL(*visits[2], 50, ());
  RemoveRecencyDb(dbPath);
}

UNIT_TEST(LiveRecency_ReopenSwitchesPath)
{
  auto const dbA = RecencyDbPath("sp072_reopen_a");
  auto const dbB = RecencyDbPath("sp072_reopen_b");
  RemoveRecencyDb(dbA);
  RemoveRecencyDb(dbB);
  LiveRecencyStore store(dbA);
  store.TouchLiveVisits({7}, 11);
  TEST_EQUAL(*store.GetLastLiveVisit(7), 11, ());
  store.Reopen(dbB);
  TEST(!store.GetLastLiveVisit(7).has_value(), ());
  store.TouchLiveVisits({7}, 22);
  TEST_EQUAL(*store.GetLastLiveVisit(7), 22, ());
  RemoveRecencyDb(dbA);
  RemoveRecencyDb(dbB);
}

UNIT_TEST(LiveRecency_ClearAllRemovesRows)
{
  auto const dbPath = RecencyDbPath("sp089_clear_all");
  RemoveRecencyDb(dbPath);
  LiveRecencyStore store(dbPath);
  store.TouchLiveVisits({10, 20}, 4000);
  TEST_EQUAL(*store.GetLastLiveVisit(10), 4000, ());
  TEST_EQUAL(*store.GetLastLiveVisit(20), 4000, ());
  store.ClearAll();
  TEST(!store.GetLastLiveVisit(10).has_value(), ());
  TEST(!store.GetLastLiveVisit(20).has_value(), ());
  RemoveRecencyDb(dbPath);
}
