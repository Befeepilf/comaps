#include "street_pixels_areas/weekly_city_live_store.hpp"

#include "street_pixels_areas/weekly_city_week.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <utility>

namespace street_pixels
{
namespace
{
constexpr char kWeeklyCityLiveDatabaseFileName[] = "weekly_city_live.db";
}  // namespace

std::string WeeklyCityLiveStore::DefaultDbPath()
{
  return GetPlatform().WritablePathForFile(kWeeklyCityLiveDatabaseFileName);
}

WeeklyCityLiveStore & WeeklyCityLiveStore::Instance()
{
  static WeeklyCityLiveStore instance;
  return instance;
}

WeeklyCityLiveStore::WeeklyCityLiveStore(std::string dbPath)
  : m_dbPath(std::move(dbPath))
{
  if (m_dbPath.empty())
    m_dbPath = DefaultDbPath();
}

WeeklyCityLiveStore::~WeeklyCityLiveStore()
{
  CloseDb();
}

void WeeklyCityLiveStore::CloseDb() const
{
  if (!m_db)
    return;
  sqlite3_close_v2(m_db);
  m_db = nullptr;
}

void WeeklyCityLiveStore::Reopen(std::string const & dbPath)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  CloseDb();
  m_dbPath = dbPath;
  EnsureOpen();
}

bool WeeklyCityLiveStore::EnsureOpen() const
{
  if (m_db)
    return true;
  sqlite3 * db = nullptr;
  if (sqlite3_open_v2(m_dbPath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
  {
    LOG(LERROR, ("Can't open weekly city live database:", m_dbPath, "reason:", sqlite3_errmsg(db)));
    sqlite3_close_v2(db);
    return false;
  }
  m_db = db;
  InitSchema();
  return true;
}

void WeeklyCityLiveStore::InitSchema() const
{
  ASSERT(m_db, ());
  sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(m_db, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
  char * errMsg = nullptr;
  char const * sql =
      "CREATE TABLE IF NOT EXISTS weekly_city_counts ("
      "city_osm_id INTEGER NOT NULL, "
      "week_id INTEGER NOT NULL, "
      "new_live_count INTEGER NOT NULL, "
      "PRIMARY KEY (city_osm_id, week_id));"
      "CREATE TABLE IF NOT EXISTS city_tz ("
      "city_osm_id INTEGER PRIMARY KEY, "
      "iana_tz TEXT NOT NULL);";
  if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to initialize weekly city live schema:", errMsg));
    sqlite3_free(errMsg);
  }
}

std::string WeeklyCityLiveStore::LoadTzUnlocked(int64_t cityOsmId) const
{
  ASSERT(m_db, ());
  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT iana_tz FROM city_tz WHERE city_osm_id = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return {};

  sqlite3_bind_int64(stmt, 1, cityOsmId);
  if (sqlite3_step(stmt) != SQLITE_ROW)
    return {};
  unsigned char const * text = sqlite3_column_text(stmt, 0);
  if (text == nullptr)
    return {};
  return reinterpret_cast<char const *>(text);
}

int64_t WeeklyCityLiveStore::LoadCountUnlocked(int64_t cityOsmId, int64_t weekId) const
{
  ASSERT(m_db, ());
  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql =
      "SELECT new_live_count FROM weekly_city_counts WHERE city_osm_id = ? AND week_id = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;

  sqlite3_bind_int64(stmt, 1, cityOsmId);
  sqlite3_bind_int64(stmt, 2, weekId);
  if (sqlite3_step(stmt) != SQLITE_ROW)
    return 0;
  return sqlite3_column_int64(stmt, 0);
}

CompetitionWeeklyCityQuery WeeklyCityLiveStore::QueryUnlocked(int64_t cityOsmId, int64_t nowUnix) const
{
  CompetitionWeeklyCityQuery query;
  query.m_cityOsmId = cityOsmId;
  auto const bounds = WeekBoundsFromUnix(nowUnix, LoadTzUnlocked(cityOsmId));
  query.m_weekId = bounds.m_weekId;
  query.m_weekEndUnix = bounds.m_weekEndUnix;
  query.m_remainingSeconds = RemainingSeconds(bounds, nowUnix);
  query.m_usedUtcFallback = bounds.m_usedUtcFallback;
  query.m_newLiveCount = LoadCountUnlocked(cityOsmId, bounds.m_weekId);
  return query;
}

void WeeklyCityLiveStore::RecordFirstLive(std::vector<int64_t> const & cityOsmIds, int64_t nowUnix)
{
  if (cityOsmIds.empty())
    return;

  std::vector<int64_t> sorted = cityOsmIds;
  std::sort(sorted.begin(), sorted.end());

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return;

  if (sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK)
    return;

  bool committed = false;
  SCOPE_GUARD(rollback, [&]()
  {
    if (!committed)
      sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
  });

  {
    sqlite3_stmt * stmt = nullptr;
    SCOPE_GUARD(finalize, [&stmt]()
    {
      if (stmt)
        sqlite3_finalize(stmt);
    });

    char const * sql =
        "INSERT INTO weekly_city_counts(city_osm_id, week_id, new_live_count) VALUES(?,?,?) "
        "ON CONFLICT(city_osm_id, week_id) DO UPDATE SET "
        "new_live_count = new_live_count + excluded.new_live_count;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
      return;

    size_t i = 0;
    while (i < sorted.size())
    {
      int64_t const city = sorted[i];
      int64_t delta = 0;
      while (i < sorted.size() && sorted[i] == city)
      {
        ++delta;
        ++i;
      }
      if (city == 0 || delta <= 0)
        continue;
      auto const bounds = WeekBoundsFromUnix(nowUnix, LoadTzUnlocked(city));
      sqlite3_reset(stmt);
      sqlite3_clear_bindings(stmt);
      sqlite3_bind_int64(stmt, 1, city);
      sqlite3_bind_int64(stmt, 2, bounds.m_weekId);
      sqlite3_bind_int64(stmt, 3, delta);
      if (sqlite3_step(stmt) != SQLITE_DONE)
        return;
    }
  }

  if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
    return;
  committed = true;
}

CompetitionWeeklyCityQuery WeeklyCityLiveStore::Query(int64_t cityOsmId, int64_t nowUnix) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
  {
    CompetitionWeeklyCityQuery query;
    query.m_cityOsmId = cityOsmId;
    auto const bounds = WeekBoundsFromUnix(nowUnix, {});
    query.m_weekId = bounds.m_weekId;
    query.m_weekEndUnix = bounds.m_weekEndUnix;
    query.m_remainingSeconds = RemainingSeconds(bounds, nowUnix);
    query.m_usedUtcFallback = bounds.m_usedUtcFallback;
    return query;
  }
  return QueryUnlocked(cityOsmId, nowUnix);
}

std::vector<WeeklyCityLiveCountRow> WeeklyCityLiveStore::ListCurrentWeekCounts(int64_t nowUnix) const
{
  std::vector<WeeklyCityLiveCountRow> out;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return out;

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT city_osm_id, week_id, new_live_count FROM weekly_city_counts;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return out;

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    int64_t const city = sqlite3_column_int64(stmt, 0);
    int64_t const weekId = sqlite3_column_int64(stmt, 1);
    int64_t const count = sqlite3_column_int64(stmt, 2);
    auto const bounds = WeekBoundsFromUnix(nowUnix, LoadTzUnlocked(city));
    if (weekId != bounds.m_weekId)
      continue;
    WeeklyCityLiveCountRow row;
    row.m_cityOsmId = city;
    row.m_weekId = weekId;
    row.m_newLiveCount = count;
    out.push_back(row);
  }
  return out;
}

void WeeklyCityLiveStore::SetCityIanaTz(int64_t cityOsmId, std::string const & ianaTz)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return;

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql =
      "INSERT INTO city_tz(city_osm_id, iana_tz) VALUES(?,?) "
      "ON CONFLICT(city_osm_id) DO UPDATE SET iana_tz = excluded.iana_tz;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return;

  sqlite3_bind_int64(stmt, 1, cityOsmId);
  sqlite3_bind_text(stmt, 2, ianaTz.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
}

std::string WeeklyCityLiveStore::GetCityIanaTz(int64_t cityOsmId) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return {};
  return LoadTzUnlocked(cityOsmId);
}
}  // namespace street_pixels
