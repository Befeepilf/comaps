#include "testing/testing.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/live_recency_store.hpp"
#include "street_pixels_areas/weekly_city_live_store.hpp"
#include "street_pixels_areas/weekly_city_week.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <algorithm>
#include <string>

namespace
{
using namespace street_pixels;

int64_t constexpr kUtcMonday2026Mar23 = 1774224000;

std::string WeeklyDbPath(std::string const & leaf)
{
  return base::JoinPath(GetPlatform().WritableDir(), leaf + ".db");
}

void RemoveWeeklyDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}
}  // namespace

UNIT_TEST(WeeklyCityLive_Increment)
{
  auto const dbPath = WeeklyDbPath("sp073_increment");
  RemoveWeeklyDb(dbPath);
  WeeklyCityLiveStore store(dbPath);

  store.RecordFirstLive({8}, kUtcMonday2026Mar23);
  auto q = store.Query(8, kUtcMonday2026Mar23);
  TEST_EQUAL(q.m_cityOsmId, 8, ());
  TEST_EQUAL(q.m_newLiveCount, 1, ());
  TEST_EQUAL(q.m_weekId, kUtcMonday2026Mar23, ());
  TEST(q.m_usedUtcFallback, ());

  store.RecordFirstLive({8, 8}, kUtcMonday2026Mar23);
  q = store.Query(8, kUtcMonday2026Mar23);
  TEST_EQUAL(q.m_newLiveCount, 3, ());

  RemoveWeeklyDb(dbPath);
}

UNIT_TEST(WeeklyCityLive_TwoCities)
{
  auto const dbPath = WeeklyDbPath("sp073_two_cities");
  RemoveWeeklyDb(dbPath);
  WeeklyCityLiveStore store(dbPath);

  store.RecordFirstLive({8, 18, 8}, kUtcMonday2026Mar23);
  auto const a = store.Query(8, kUtcMonday2026Mar23);
  auto const b = store.Query(18, kUtcMonday2026Mar23);
  TEST_EQUAL(a.m_newLiveCount, 2, ());
  TEST_EQUAL(b.m_newLiveCount, 1, ());
  TEST_EQUAL(a.m_weekId, b.m_weekId, ());

  auto rows = store.ListCurrentWeekCounts(kUtcMonday2026Mar23);
  TEST_EQUAL(rows.size(), 2, ());
  std::sort(rows.begin(), rows.end(),
            [](WeeklyCityLiveCountRow const & l, WeeklyCityLiveCountRow const & r)
            { return l.m_cityOsmId < r.m_cityOsmId; });
  TEST_EQUAL(rows[0].m_cityOsmId, 8, ());
  TEST_EQUAL(rows[0].m_newLiveCount, 2, ());
  TEST_EQUAL(rows[1].m_cityOsmId, 18, ());
  TEST_EQUAL(rows[1].m_newLiveCount, 1, ());

  RemoveWeeklyDb(dbPath);
}

UNIT_TEST(WeeklyCityLive_TzChangesWeekIdVsUtc)
{
  auto const dbPath = WeeklyDbPath("sp073_tz_week");
  RemoveWeeklyDb(dbPath);
  WeeklyCityLiveStore store(dbPath);

  int32_t constexpr kUtcPlus2 = 7200;
  int64_t const sundayEveningUtc = kUtcMonday2026Mar23 - 3600;
  auto const utcBounds = WeekBoundsFromUnix(sundayEveningUtc, {});
  auto const offsetBounds = WeekBoundsAtFixedOffset(sundayEveningUtc, kUtcPlus2);
  TEST_NOT_EQUAL(utcBounds.m_weekId, offsetBounds.m_weekId, ());

  store.RecordFirstLive({8}, sundayEveningUtc);
  auto const beforeTz = store.Query(8, sundayEveningUtc);
  TEST_EQUAL(beforeTz.m_weekId, utcBounds.m_weekId, ());
  TEST_EQUAL(beforeTz.m_newLiveCount, 1, ());
  TEST(beforeTz.m_usedUtcFallback, ());

  store.SetCityIanaTz(8, "Etc/GMT-2");
  TEST_EQUAL(store.GetCityIanaTz(8), "Etc/GMT-2", ());
  auto const afterTz = store.Query(8, sundayEveningUtc);
  TEST_EQUAL(afterTz.m_weekId, utcBounds.m_weekId, ());
  TEST_NOT_EQUAL(afterTz.m_weekId, offsetBounds.m_weekId, ());
  TEST_EQUAL(afterTz.m_newLiveCount, 1, ());
  TEST(afterTz.m_usedUtcFallback, ());

  RemoveWeeklyDb(dbPath);
}

UNIT_TEST(WeeklyCityLive_DefaultDbPathFilename)
{
  auto const weekly = WeeklyCityLiveStore::DefaultDbPath();
  auto const recency = LiveRecencyStore::DefaultDbPath();
  auto const milestones = AreaMilestoneStore::DefaultDbPath();
  TEST_EQUAL(weekly, GetPlatform().WritablePathForFile("weekly_city_live.db"), ());
  TEST_NOT_EQUAL(weekly, recency, ());
  TEST_NOT_EQUAL(weekly, milestones, ());
  TEST(weekly.find("weekly_city_live.db") != std::string::npos, ());
  TEST(weekly.find("live_recency.db") == std::string::npos, ());
  TEST(weekly.find("area_milestones.db") == std::string::npos, ());
}

UNIT_TEST(WeeklyCityLive_TempDbRemovesWalAndShm)
{
  auto const dbPath = WeeklyDbPath("sp073_temp_wal");
  RemoveWeeklyDb(dbPath);
  {
    WeeklyCityLiveStore store(dbPath);
    store.RecordFirstLive({42}, kUtcMonday2026Mar23);
    TEST_EQUAL(store.Query(42, kUtcMonday2026Mar23).m_newLiveCount, 1, ());
  }
  RemoveWeeklyDb(dbPath);
  TEST(!Platform::IsFileExistsByFullPath(dbPath), ());
  TEST(!Platform::IsFileExistsByFullPath(dbPath + "-wal"), ());
  TEST(!Platform::IsFileExistsByFullPath(dbPath + "-shm"), ());
}

UNIT_TEST(WeeklyCityLive_UnknownCityUtcZero)
{
  auto const dbPath = WeeklyDbPath("sp073_unknown");
  RemoveWeeklyDb(dbPath);
  WeeklyCityLiveStore store(dbPath);
  auto const q = store.Query(999, kUtcMonday2026Mar23);
  TEST_EQUAL(q.m_cityOsmId, 999, ());
  TEST_EQUAL(q.m_newLiveCount, 0, ());
  TEST_EQUAL(q.m_weekId, kUtcMonday2026Mar23, ());
  TEST(q.m_usedUtcFallback, ());
  TEST_EQUAL(store.GetCityIanaTz(999), "", ());
  RemoveWeeklyDb(dbPath);
}
