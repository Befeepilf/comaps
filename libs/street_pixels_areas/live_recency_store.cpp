#include "street_pixels_areas/live_recency_store.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <sqlite3.h>

#include <utility>

namespace street_pixels
{
namespace
{
constexpr char kLiveRecencyDatabaseFileName[] = "live_recency.db";

bool StepBoundPair(sqlite3_stmt * stmt, int64_t healpixId, int64_t unixSec)
{
  sqlite3_reset(stmt);
  sqlite3_clear_bindings(stmt);
  sqlite3_bind_int64(stmt, 1, healpixId);
  sqlite3_bind_int64(stmt, 2, unixSec);
  return sqlite3_step(stmt) == SQLITE_DONE;
}
}  // namespace

std::string LiveRecencyStore::DefaultDbPath()
{
  return GetPlatform().WritablePathForFile(kLiveRecencyDatabaseFileName);
}

LiveRecencyStore & LiveRecencyStore::Instance()
{
  static LiveRecencyStore instance;
  return instance;
}

LiveRecencyStore::LiveRecencyStore(std::string dbPath)
  : m_dbPath(std::move(dbPath))
{
  if (m_dbPath.empty())
    m_dbPath = DefaultDbPath();
}

LiveRecencyStore::~LiveRecencyStore()
{
  CloseDb();
}

void LiveRecencyStore::CloseDb() const
{
  if (!m_db)
    return;
  sqlite3_close_v2(m_db);
  m_db = nullptr;
}

void LiveRecencyStore::Reopen(std::string const & dbPath)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  CloseDb();
  m_dbPath = dbPath;
  EnsureOpen();
}

bool LiveRecencyStore::EnsureOpen() const
{
  if (m_db)
    return true;
  sqlite3 * db = nullptr;
  if (sqlite3_open_v2(m_dbPath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
  {
    LOG(LERROR, ("Can't open live recency database:", m_dbPath, "reason:", sqlite3_errmsg(db)));
    sqlite3_close_v2(db);
    return false;
  }
  m_db = db;
  InitSchema();
  return true;
}

void LiveRecencyStore::InitSchema() const
{
  ASSERT(m_db, ());
  sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(m_db, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
  char * errMsg = nullptr;
  char const * sql =
      "CREATE TABLE IF NOT EXISTS live_recency ("
      "healpix_id INTEGER PRIMARY KEY, "
      "last_live_visit INTEGER NOT NULL);"
      "CREATE TABLE IF NOT EXISTS recency_meta ("
      "key TEXT PRIMARY KEY, "
      "value INTEGER NOT NULL);";
  if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to initialize live recency schema:", errMsg));
    sqlite3_free(errMsg);
  }
}

std::optional<int64_t> LiveRecencyStore::LoadVisitUnlocked(int64_t healpixId) const
{
  ASSERT(m_db, ());
  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT last_live_visit FROM live_recency WHERE healpix_id = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return std::nullopt;

  sqlite3_bind_int64(stmt, 1, healpixId);
  if (sqlite3_step(stmt) != SQLITE_ROW)
    return std::nullopt;
  return sqlite3_column_int64(stmt, 0);
}

void LiveRecencyStore::RunVisitBatch(char const * sql, std::vector<int64_t> const & ids, int64_t unixSec)
{
  ASSERT(sql, ());
  if (ids.empty())
    return;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return;

  if (sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK)
    return;

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
    return;
  }

  for (int64_t const id : ids)
  {
    if (!StepBoundPair(stmt, id, unixSec))
    {
      sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
      return;
    }
  }

  if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
}

void LiveRecencyStore::SeedEverLive(std::vector<int64_t> const & ids, int64_t consentUnixSec)
{
  RunVisitBatch("INSERT OR IGNORE INTO live_recency(healpix_id, last_live_visit) VALUES(?,?);", ids,
                consentUnixSec);
}

void LiveRecencyStore::TouchLiveVisits(std::vector<int64_t> const & ids, int64_t nowUnixSec)
{
  RunVisitBatch(
      "INSERT INTO live_recency(healpix_id, last_live_visit) VALUES(?,?) "
      "ON CONFLICT(healpix_id) DO UPDATE SET last_live_visit=excluded.last_live_visit;",
      ids, nowUnixSec);
}

std::optional<int64_t> LiveRecencyStore::GetLastLiveVisit(int64_t healpixId) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return std::nullopt;
  return LoadVisitUnlocked(healpixId);
}

std::vector<std::optional<int64_t>> LiveRecencyStore::GetLastLiveVisits(std::vector<int64_t> const & ids) const
{
  std::vector<std::optional<int64_t>> out;
  out.reserve(ids.size());

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
  {
    out.assign(ids.size(), std::nullopt);
    return out;
  }

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT last_live_visit FROM live_recency WHERE healpix_id = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    out.assign(ids.size(), std::nullopt);
    return out;
  }

  for (int64_t const id : ids)
  {
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
      out.emplace_back(sqlite3_column_int64(stmt, 0));
    else
      out.emplace_back(std::nullopt);
  }
  return out;
}

void LiveRecencyStore::ClearAll()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return;
  char * errMsg = nullptr;
  if (sqlite3_exec(m_db, "DELETE FROM live_recency;", nullptr, nullptr, &errMsg) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to clear live recency:", errMsg));
    sqlite3_free(errMsg);
  }
}
}  // namespace street_pixels
