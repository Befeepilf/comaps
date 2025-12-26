#pragma once

#include "indexer/mwm_set.hpp"
#include "kml/type_utils.hpp"
#include "storage/storage_defines.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace street_stats
{
// Manages a persistent SQLite database for storing street exploration statistics.
// This class is a singleton and is thread-safe.
class StreetStatsDB
{
public:
  static StreetStatsDB & Instance();

  using Bitmask = std::vector<uint8_t>;

  std::optional<Bitmask> GetBitmask(MwmSet::MwmId const & mwmId, uint32_t featureId);

  void SaveBitmask(MwmSet::MwmId const & mwmId, uint32_t featureId, Bitmask const & bitmask);

  void DeleteMwmData(std::string const & mwmName);

  bool IsTrackProcessed(std::int64_t const geometryHash, storage::CountryId const & countryId);
  void MarkTrackProcessed(std::int64_t const geometryHash, storage::CountryId const & countryId);

  template <typename TFn>
  void WithTransaction(TFn && fn)
  {
    if (!m_db)
      return;
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    BeginTransaction();
    fn();
    EndTransaction();
  }

private:
  StreetStatsDB();
  ~StreetStatsDB();

  StreetStatsDB(StreetStatsDB const &) = delete;
  StreetStatsDB & operator=(StreetStatsDB const &) = delete;

  void InitSchema();
  void BeginTransaction();
  void EndTransaction();

  sqlite3 * m_db = nullptr;
  std::string m_dbPath;
  std::recursive_mutex m_mutex;
};
}  // namespace street_stats