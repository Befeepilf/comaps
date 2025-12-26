#include "map/street_stats_db.hpp"

#include "indexer/mwm_set.hpp"
#include "storage/storage_defines.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <mutex>

namespace
{
std::string const kDatabaseFileName = "street_stats.db";

// Helper function to get or insert mwm_id
int64_t GetMwmId(sqlite3 * db, MwmSet::MwmId const & mwmId)
{
  auto const & mwmInfo = mwmId.GetInfo();
  if (!mwmInfo)
    return -1;

  auto const & mwmName = mwmInfo->GetCountryName();

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT mwm_id FROM mwms WHERE mwm_name = ?;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text(stmt, 1, mwmName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
      return sqlite3_column_int64(stmt, 0);
  }
  sqlite3_finalize(stmt);
  stmt = nullptr;

  sql = "INSERT INTO mwms (mwm_name) VALUES (?);";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text(stmt, 1, mwmName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE)
      LOG(LERROR, ("Failed to insert MWM name:", mwmName, "reason:", sqlite3_errmsg(db)));
    else
      return sqlite3_last_insert_rowid(db);
  }

  LOG(LERROR, ("Failed to prepare SQL statement for MWM insertion."));
  return -1;
}
}  // namespace

namespace street_stats
{
// static
StreetStatsDB & StreetStatsDB::Instance()
{
  static StreetStatsDB instance;
  return instance;
}

StreetStatsDB::StreetStatsDB()
{
  LOG(LINFO, ("Initializing street stats database..."));
  m_dbPath = GetPlatform().WritablePathForFile(kDatabaseFileName);
  if (sqlite3_open_v2(m_dbPath.c_str(), &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
  {
    LOG(LERROR, ("Can't open street stats database:", m_dbPath, "reason:", sqlite3_errmsg(m_db)));
    sqlite3_close(m_db);
    m_db = nullptr;
    return;
  }
  InitSchema();
  LOG(LINFO, ("Street stats database initialized successfully"));
}

StreetStatsDB::~StreetStatsDB()
{
  if (m_db)
    sqlite3_close(m_db);
}

void StreetStatsDB::InitSchema()
{
  ASSERT(m_db, ("Street stats database is not initialized"));
  char * errMsg = nullptr;
  char const * sql =
      "CREATE TABLE IF NOT EXISTS mwms ("
      "mwm_id INTEGER PRIMARY KEY, "
      "mwm_name TEXT UNIQUE NOT NULL);"
      //
      "CREATE TABLE IF NOT EXISTS street_exploration ("
      "mwm_id INTEGER, "
      "feature_index INTEGER, "
      "pixel_bitmask BLOB, "
      "PRIMARY KEY (mwm_id, feature_index));"
      //
      "CREATE TABLE IF NOT EXISTS processed_tracks ("
      "geometry_hash INTEGER, "
      "country_id TEXT, "
      "PRIMARY KEY (geometry_hash, country_id));";

  if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to initialize street stats DB schema:", errMsg));
    sqlite3_free(errMsg);
  }
}

void StreetStatsDB::BeginTransaction()
{
  ASSERT(m_db, ("Street stats database is not initialized"));
  sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
}

void StreetStatsDB::EndTransaction()
{
  ASSERT(m_db, ("Street stats database is not initialized"));
  sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
}

std::optional<StreetStatsDB::Bitmask> StreetStatsDB::GetBitmask(MwmSet::MwmId const & mwmId, uint32_t featureId)
{
  ASSERT(m_db, ("Street stats database is not initialized"));

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  int64_t const internalMwmId = GetMwmId(m_db, mwmId);
  if (internalMwmId < 0)
    return {};

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });
  char const * sql = "SELECT pixel_bitmask FROM street_exploration WHERE mwm_id = ? AND feature_index = ?;";

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to prepare statement for GetBitmask:", sqlite3_errmsg(m_db)));
    return {};
  }

  sqlite3_bind_int64(stmt, 1, internalMwmId);
  sqlite3_bind_int(stmt, 2, featureId);

  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    int const blobSize = sqlite3_column_bytes(stmt, 0);
    uint8_t const * blobData = static_cast<uint8_t const *>(sqlite3_column_blob(stmt, 0));
    Bitmask bitmask;
    bitmask.assign(blobData, blobData + blobSize);
    return bitmask;
  }

  return {};
}

void StreetStatsDB::SaveBitmask(MwmSet::MwmId const & mwmId, uint32_t featureId, Bitmask const & bitmask)
{
  ASSERT(m_db, ("Street stats database is not initialized"));

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  int64_t const internalMwmId = GetMwmId(m_db, mwmId);
  if (internalMwmId < 0)
    return;

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql =
      "INSERT OR REPLACE INTO street_exploration (mwm_id, feature_index, pixel_bitmask) VALUES (?, ?, ?);";

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to prepare statement for SaveBitmask:", sqlite3_errmsg(m_db)));
    return;
  }

  sqlite3_bind_int64(stmt, 1, internalMwmId);
  sqlite3_bind_int(stmt, 2, featureId);
  sqlite3_bind_blob(stmt, 3, bitmask.data(), bitmask.size(), SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE)
    LOG(LERROR, ("Failed to save bitmask:", sqlite3_errmsg(m_db)));
}

void StreetStatsDB::DeleteMwmData(std::string const & mwmName)
{
  ASSERT(m_db, ("Street stats database is not initialized"));

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  int64_t internalMwmId = -1;
  char const * sqlSelect = "SELECT mwm_id FROM mwms WHERE mwm_name = ?;";
  if (sqlite3_prepare_v2(m_db, sqlSelect, -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_text(stmt, 1, mwmName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
      internalMwmId = sqlite3_column_int64(stmt, 0);
  }
  sqlite3_finalize(stmt);
  stmt = nullptr;

  if (internalMwmId < 0)
    return;

  char const * sqlDeleteExploration = "DELETE FROM street_exploration WHERE mwm_id = ?;";
  if (sqlite3_prepare_v2(m_db, sqlDeleteExploration, -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_int64(stmt, 1, internalMwmId);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);
  stmt = nullptr;

  char const * sqlDeleteMwm = "DELETE FROM mwms WHERE mwm_id = ?;";
  if (sqlite3_prepare_v2(m_db, sqlDeleteMwm, -1, &stmt, nullptr) == SQLITE_OK)
  {
    sqlite3_bind_int64(stmt, 1, internalMwmId);
    sqlite3_step(stmt);
  }
}

bool StreetStatsDB::IsTrackProcessed(std::int64_t const geometryHash, storage::CountryId const & countryId)
{
  ASSERT(m_db, ("Street stats database is not initialized"));

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT 1 FROM processed_tracks WHERE geometry_hash = ? AND country_id = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int64(stmt, 1, geometryHash);
  sqlite3_bind_text(stmt, 2, countryId.c_str(), -1, SQLITE_TRANSIENT);
  return sqlite3_step(stmt) == SQLITE_ROW;
}

void StreetStatsDB::MarkTrackProcessed(std::int64_t geometryHash, storage::CountryId const & countryId)
{
  ASSERT(m_db, ("Street stats database is not initialized"));

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "INSERT OR IGNORE INTO processed_tracks (geometry_hash, country_id) VALUES (?, ?);";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return;

  sqlite3_bind_int64(stmt, 1, geometryHash);
  sqlite3_bind_text(stmt, 2, countryId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
}

}  // namespace street_stats
