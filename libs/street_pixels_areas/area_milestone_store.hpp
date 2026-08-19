#pragma once

#include "street_pixels_areas/area_completion_cache.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string>
#include <vector>

struct sqlite3;

namespace street_pixels
{
enum class AreaMilestoneThreshold : uint8_t
{
  P25 = 0,
  P50 = 1,
  P100 = 2,
};

inline std::string DebugPrint(AreaMilestoneThreshold threshold)
{
  switch (threshold)
  {
  case AreaMilestoneThreshold::P25: return "P25";
  case AreaMilestoneThreshold::P50: return "P50";
  case AreaMilestoneThreshold::P100: return "P100";
  }
  return "UnknownAreaMilestoneThreshold";
}

constexpr uint8_t kAreaMilestoneMask25 = 1u;
constexpr uint8_t kAreaMilestoneMask50 = 2u;
constexpr uint8_t kAreaMilestoneMask100 = 4u;

uint8_t AreaMilestoneMask(AreaMilestoneThreshold threshold);

struct AreaMilestoneRecord
{
  uint8_t m_firedMask = 0;
  std::optional<int64_t> m_completed100At;
};

struct AreaMilestoneCrossing
{
  uint64_t m_osmId = 0;
  uint32_t m_compactIndex = 0;
  AreaMilestoneThreshold m_threshold = AreaMilestoneThreshold::P25;
};

class AreaMilestoneStore
{
public:
  static AreaMilestoneStore & Instance();

  AreaMilestoneStore(std::string dbPath = {});
  ~AreaMilestoneStore();

  AreaMilestoneStore(AreaMilestoneStore const &) = delete;
  AreaMilestoneStore & operator=(AreaMilestoneStore const &) = delete;

  static std::string DefaultDbPath();

  std::vector<AreaMilestoneCrossing> EvaluateAndRecordFires(AreaCompletionCache const & cache,
                                                            int64_t nowSec);
  std::vector<AreaMilestoneCrossing> ConsumePendingCrossings();

  std::optional<AreaMilestoneRecord> Get(uint64_t osmId) const;
  bool WasPreviouslyCompletedBelow100(uint64_t osmId, double currentFraction) const;
  std::vector<uint64_t> ListAreasPreviouslyCompletedNowBelow(AreaCompletionCache const & cache) const;

  void Reopen(std::string const & dbPath);

private:
  bool EnsureOpen();
  void InitSchema();
  std::optional<AreaMilestoneRecord> LoadRecord(uint64_t osmId) const;
  bool UpsertRecord(uint64_t osmId, uint8_t firedMask, std::optional<int64_t> completed100At);
  void AppendPendingCrossings(std::vector<AreaMilestoneCrossing> const & crossings);

  std::string m_dbPath;
  sqlite3 * m_db = nullptr;
  mutable std::mutex m_mutex;
  std::vector<AreaMilestoneCrossing> m_pendingCrossings;
};
}  // namespace street_pixels
