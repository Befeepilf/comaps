#include "testing/testing.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/live_recency_store.hpp"
#include "street_pixels_areas/weekly_city_live_store.hpp"
#include "street_pixels_areas/weekly_city_week.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <sqlite3.h>

namespace
{
using namespace street_pixels;

int64_t constexpr kWeeklyCityLiveUtcMonday2026Mar23 = 1774224000;

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

  store.RecordFirstLive({8}, kWeeklyCityLiveUtcMonday2026Mar23);
  auto q = store.Query(8, kWeeklyCityLiveUtcMonday2026Mar23);
  TEST_EQUAL(q.m_cityOsmId, 8, ());
  TEST_EQUAL(q.m_newLiveCount, 1, ());
  TEST_EQUAL(q.m_weekId, kWeeklyCityLiveUtcMonday2026Mar23, ());
  TEST(q.m_usedUtcFallback, ());

  store.RecordFirstLive({8, 8}, kWeeklyCityLiveUtcMonday2026Mar23);
  q = store.Query(8, kWeeklyCityLiveUtcMonday2026Mar23);
  TEST_EQUAL(q.m_newLiveCount, 3, ());

  RemoveWeeklyDb(dbPath);
}

UNIT_TEST(WeeklyCityLive_TwoCities)
{
  auto const dbPath = WeeklyDbPath("sp073_two_cities");
  RemoveWeeklyDb(dbPath);
  WeeklyCityLiveStore store(dbPath);

  store.RecordFirstLive({8, 18, 8}, kWeeklyCityLiveUtcMonday2026Mar23);
  auto const a = store.Query(8, kWeeklyCityLiveUtcMonday2026Mar23);
  auto const b = store.Query(18, kWeeklyCityLiveUtcMonday2026Mar23);
  TEST_EQUAL(a.m_newLiveCount, 2, ());
  TEST_EQUAL(b.m_newLiveCount, 1, ());
  TEST_EQUAL(a.m_weekId, b.m_weekId, ());

  auto rows = store.ListCurrentWeekCounts(kWeeklyCityLiveUtcMonday2026Mar23);
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
  int64_t const sundayEveningUtc = kWeeklyCityLiveUtcMonday2026Mar23 - 3600;
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
    store.RecordFirstLive({42}, kWeeklyCityLiveUtcMonday2026Mar23);
    TEST_EQUAL(store.Query(42, kWeeklyCityLiveUtcMonday2026Mar23).m_newLiveCount, 1, ());
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
  auto const q = store.Query(999, kWeeklyCityLiveUtcMonday2026Mar23);
  TEST_EQUAL(q.m_cityOsmId, 999, ());
  TEST_EQUAL(q.m_newLiveCount, 0, ());
  TEST_EQUAL(q.m_weekId, kWeeklyCityLiveUtcMonday2026Mar23, ());
  TEST(q.m_usedUtcFallback, ());
  TEST_EQUAL(store.GetCityIanaTz(999), "", ());
  RemoveWeeklyDb(dbPath);
}

UNIT_TEST(WeeklyCityLive_MondayBoundarySeparateWeeks)
{
  auto const dbPath = WeeklyDbPath("sp073_monday_boundary");
  RemoveWeeklyDb(dbPath);
  WeeklyCityLiveStore store(dbPath);

  int64_t const monday = kWeeklyCityLiveUtcMonday2026Mar23;
  store.RecordFirstLive({8}, monday - 1);
  store.RecordFirstLive({8}, monday);

  auto const before = store.Query(8, monday - 1);
  auto const after = store.Query(8, monday);
  TEST_EQUAL(before.m_newLiveCount, 1, ());
  TEST_EQUAL(after.m_newLiveCount, 1, ());
  TEST_NOT_EQUAL(before.m_weekId, after.m_weekId, ());
  TEST_EQUAL(after.m_weekId, monday, ());

  auto const current = store.ListCurrentWeekCounts(monday);
  TEST_EQUAL(current.size(), 1, ());
  TEST_EQUAL(current[0].m_cityOsmId, 8, ());
  TEST_EQUAL(current[0].m_weekId, monday, ());
  TEST_EQUAL(current[0].m_newLiveCount, 1, ());

  RemoveWeeklyDb(dbPath);
}

UNIT_TEST(WeeklyCityLive_SchemaHasNoGpsOrHealpix)
{
  auto const dbPath = WeeklyDbPath("sp073_schema");
  RemoveWeeklyDb(dbPath);
  {
    WeeklyCityLiveStore store(dbPath);
    store.RecordFirstLive({8}, kWeeklyCityLiveUtcMonday2026Mar23);
    store.SetCityIanaTz(8, "Etc/UTC");
  }

  sqlite3 * db = nullptr;
  TEST_EQUAL(sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr), SQLITE_OK, ());
  SCOPE_GUARD(closeDb, [&db]() { sqlite3_close_v2(db); });

  sqlite3_stmt * tables = nullptr;
  TEST_EQUAL(sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table';", -1, &tables, nullptr),
             SQLITE_OK, ());
  SCOPE_GUARD(finalizeTables, [&tables]() { sqlite3_finalize(tables); });

  std::vector<std::string> tableNames;
  while (sqlite3_step(tables) == SQLITE_ROW)
  {
    unsigned char const * text = sqlite3_column_text(tables, 0);
    TEST(text != nullptr, ());
    tableNames.emplace_back(reinterpret_cast<char const *>(text));
  }
  TEST(std::find(tableNames.begin(), tableNames.end(), "weekly_city_counts") != tableNames.end(), ());
  TEST(std::find(tableNames.begin(), tableNames.end(), "city_tz") != tableNames.end(), ());
  TEST(std::find(tableNames.begin(), tableNames.end(), "weekly_meta") == tableNames.end(), ());

  auto collectColumns = [&](char const * table)
  {
    std::vector<std::string> names;
    std::string const sql = std::string("PRAGMA table_info(") + table + ");";
    sqlite3_stmt * cols = nullptr;
    TEST_EQUAL(sqlite3_prepare_v2(db, sql.c_str(), -1, &cols, nullptr), SQLITE_OK, ());
    SCOPE_GUARD(finalizeCols, [&cols]() { sqlite3_finalize(cols); });
    while (sqlite3_step(cols) == SQLITE_ROW)
    {
      unsigned char const * text = sqlite3_column_text(cols, 1);
      TEST(text != nullptr, ());
      names.emplace_back(reinterpret_cast<char const *>(text));
    }
    return names;
  };

  auto const countCols = collectColumns("weekly_city_counts");
  TEST_EQUAL(countCols.size(), 3, ());
  TEST_EQUAL(countCols[0], "city_osm_id", ());
  TEST_EQUAL(countCols[1], "week_id", ());
  TEST_EQUAL(countCols[2], "new_live_count", ());

  auto const tzCols = collectColumns("city_tz");
  TEST_EQUAL(tzCols.size(), 2, ());
  TEST_EQUAL(tzCols[0], "city_osm_id", ());
  TEST_EQUAL(tzCols[1], "iana_tz", ());

  auto forbid = [](std::string const & name)
  {
    std::string lower = name;
    for (char & c : lower)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    TEST(lower != "lat" && lower != "lon" && lower != "latitude" && lower != "longitude", (name));
    TEST(lower.find("gps") == std::string::npos, (name));
    TEST(lower.find("healpix") == std::string::npos, (name));
    TEST(lower.find("pixel") == std::string::npos, (name));
    TEST(lower.find("nest") == std::string::npos, (name));
  };
  for (auto const & name : tableNames)
  {
    if (name.rfind("sqlite_", 0) == 0)
      continue;
    forbid(name);
  }
  for (auto const & name : countCols)
    forbid(name);
  for (auto const & name : tzCols)
    forbid(name);

  RemoveWeeklyDb(dbPath);
}
