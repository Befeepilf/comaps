#include "street_pixels_areas/area_milestone_store.hpp"

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
constexpr char kDatabaseFileName[] = "area_milestones.db";

constexpr double kThreshold25 = 0.25;
constexpr double kThreshold50 = 0.50;
constexpr double kThreshold100 = 1.0;

int CrossingPriority(AreaMilestoneThreshold threshold)
{
  switch (threshold)
  {
  case AreaMilestoneThreshold::P100: return 0;
  case AreaMilestoneThreshold::P50: return 1;
  case AreaMilestoneThreshold::P25: return 2;
  }
  return 3;
}

bool FractionCrosses(double fraction, AreaMilestoneThreshold threshold)
{
  switch (threshold)
  {
  case AreaMilestoneThreshold::P25: return fraction >= kThreshold25;
  case AreaMilestoneThreshold::P50: return fraction >= kThreshold50;
  case AreaMilestoneThreshold::P100: return fraction >= kThreshold100;
  }
  return false;
}

uint8_t MaskFor(AreaMilestoneThreshold threshold)
{
  switch (threshold)
  {
  case AreaMilestoneThreshold::P25: return kAreaMilestoneMask25;
  case AreaMilestoneThreshold::P50: return kAreaMilestoneMask50;
  case AreaMilestoneThreshold::P100: return kAreaMilestoneMask100;
  }
  return 0;
}

void SortCrossings(std::vector<AreaMilestoneCrossing> & crossings)
{
  std::sort(crossings.begin(), crossings.end(),
            [](AreaMilestoneCrossing const & a, AreaMilestoneCrossing const & b)
            {
              int const pa = CrossingPriority(a.m_threshold);
              int const pb = CrossingPriority(b.m_threshold);
              if (pa != pb)
                return pa < pb;
              if (a.m_osmId != b.m_osmId)
                return a.m_osmId < b.m_osmId;
              return a.m_compactIndex < b.m_compactIndex;
            });
}
}  // namespace

uint8_t AreaMilestoneMask(AreaMilestoneThreshold threshold) { return MaskFor(threshold); }

std::string AreaMilestoneStore::DefaultDbPath()
{
  return GetPlatform().WritablePathForFile(kDatabaseFileName);
}

AreaMilestoneStore & AreaMilestoneStore::Instance()
{
  static AreaMilestoneStore instance;
  return instance;
}

AreaMilestoneStore::AreaMilestoneStore(std::string dbPath)
  : m_dbPath(std::move(dbPath))
{
  if (m_dbPath.empty())
    m_dbPath = DefaultDbPath();
}

AreaMilestoneStore::~AreaMilestoneStore()
{
  CloseDb();
}

void AreaMilestoneStore::CloseDb() const
{
  if (!m_db)
    return;
  sqlite3_close_v2(m_db);
  m_db = nullptr;
}

void AreaMilestoneStore::Reopen(std::string const & dbPath)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  CloseDb();
  m_dbPath = dbPath;
  m_pendingCrossings.clear();
  EnsureOpen();
}

bool AreaMilestoneStore::EnsureOpen() const
{
  if (m_db)
    return true;
  sqlite3 * db = nullptr;
  if (sqlite3_open_v2(m_dbPath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
  {
    LOG(LERROR, ("Can't open area milestone database:", m_dbPath, "reason:", sqlite3_errmsg(db)));
    sqlite3_close_v2(db);
    return false;
  }
  m_db = db;
  InitSchema();
  return true;
}

void AreaMilestoneStore::InitSchema() const
{
  ASSERT(m_db, ());
  sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(m_db, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
  char * errMsg = nullptr;
  char const * sql =
      "CREATE TABLE IF NOT EXISTS area_milestones ("
      "osm_id INTEGER PRIMARY KEY, "
      "fired_mask INTEGER NOT NULL, "
      "completed_100_at INTEGER);";
  if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
  {
    LOG(LERROR, ("Failed to initialize area milestone schema:", errMsg));
    sqlite3_free(errMsg);
  }
}

std::optional<AreaMilestoneRecord> AreaMilestoneStore::LoadRecord(uint64_t osmId) const
{
  ASSERT(m_db, ());
  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql = "SELECT fired_mask, completed_100_at FROM area_milestones WHERE osm_id = ?;";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return std::nullopt;

  sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(osmId));
  if (sqlite3_step(stmt) != SQLITE_ROW)
    return std::nullopt;

  AreaMilestoneRecord record;
  record.m_firedMask = static_cast<uint8_t>(sqlite3_column_int(stmt, 0));
  if (sqlite3_column_type(stmt, 1) != SQLITE_NULL)
    record.m_completed100At = sqlite3_column_int64(stmt, 1);
  return record;
}

bool AreaMilestoneStore::UpsertRecord(uint64_t osmId, uint8_t firedMask,
                                      std::optional<int64_t> completed100At)
{
  ASSERT(m_db, ());
  sqlite3_stmt * stmt = nullptr;
  SCOPE_GUARD(finalize, [&stmt]()
  {
    if (stmt)
      sqlite3_finalize(stmt);
  });

  char const * sql =
      "INSERT INTO area_milestones (osm_id, fired_mask, completed_100_at) VALUES (?, ?, ?) "
      "ON CONFLICT(osm_id) DO UPDATE SET fired_mask = excluded.fired_mask, "
      "completed_100_at = COALESCE(area_milestones.completed_100_at, excluded.completed_100_at);";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(osmId));
  sqlite3_bind_int(stmt, 2, firedMask);
  if (completed100At)
    sqlite3_bind_int64(stmt, 3, *completed100At);
  else
    sqlite3_bind_null(stmt, 3);

  return sqlite3_step(stmt) == SQLITE_DONE;
}

void AreaMilestoneStore::AppendPendingCrossings(std::vector<AreaMilestoneCrossing> const & crossings)
{
  m_pendingCrossings.insert(m_pendingCrossings.end(), crossings.begin(), crossings.end());
}

std::vector<AreaMilestoneCrossing> AreaMilestoneStore::EvaluateAndRecordFires(AreaCompletionCache const & cache,
                                                                              int64_t nowSec)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<AreaMilestoneCrossing> newCrossings;
  if (!cache.IsValid() || !EnsureOpen())
    return newCrossings;

  if (sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK)
    return newCrossings;

  for (auto const & row : cache.Rows())
  {
    if (row.m_total == 0 || row.m_osmId == 0)
      continue;

    double const fraction = AreaCompletionFraction(row);
    auto record = LoadRecord(row.m_osmId);
    uint8_t firedMask = record ? record->m_firedMask : 0;
    std::optional<int64_t> completed100At = record ? record->m_completed100At : std::nullopt;

    bool changed = false;
    for (auto threshold :
         {AreaMilestoneThreshold::P25, AreaMilestoneThreshold::P50, AreaMilestoneThreshold::P100})
    {
      uint8_t const mask = MaskFor(threshold);
      if ((firedMask & mask) != 0)
        continue;
      if (!FractionCrosses(fraction, threshold))
        continue;

      firedMask = static_cast<uint8_t>(firedMask | mask);
      changed = true;
      newCrossings.push_back({row.m_osmId, row.m_compactIndex, threshold});
      if (threshold == AreaMilestoneThreshold::P100 && !completed100At)
        completed100At = nowSec;
    }

    if (changed && !UpsertRecord(row.m_osmId, firedMask, completed100At))
    {
      sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
      return {};
    }
  }

  if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
  {
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
    return {};
  }
  SortCrossings(newCrossings);
  AppendPendingCrossings(newCrossings);
  return newCrossings;
}

std::vector<AreaMilestoneCrossing> AreaMilestoneStore::ConsumePendingCrossings()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<AreaMilestoneCrossing> out = std::move(m_pendingCrossings);
  m_pendingCrossings.clear();
  return out;
}

std::optional<AreaMilestoneRecord> AreaMilestoneStore::Get(uint64_t osmId) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return std::nullopt;
  return LoadRecord(osmId);
}

bool AreaMilestoneStore::WasPreviouslyCompletedUnlocked(uint64_t osmId, double currentFraction) const
{
  auto const record = LoadRecord(osmId);
  if (!record)
    return false;
  if ((record->m_firedMask & kAreaMilestoneMask100) == 0)
    return false;
  return currentFraction < kThreshold100;
}

bool AreaMilestoneStore::WasPreviouslyCompletedBelow100(uint64_t osmId, double currentFraction) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return false;
  return WasPreviouslyCompletedUnlocked(osmId, currentFraction);
}

std::vector<uint64_t> AreaMilestoneStore::ListAreasPreviouslyCompletedNowBelow(
    AreaCompletionCache const & cache) const
{
  std::vector<uint64_t> result;
  if (!cache.IsValid())
    return result;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!EnsureOpen())
    return result;

  for (auto const & row : cache.Rows())
  {
    if (row.m_total == 0 || row.m_osmId == 0)
      continue;
    if (WasPreviouslyCompletedUnlocked(row.m_osmId, AreaCompletionFraction(row)))
      result.push_back(row.m_osmId);
  }
  return result;
}
}  // namespace street_pixels
