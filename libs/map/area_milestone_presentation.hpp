#pragma once

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/completion_card.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
enum class AreaMilestoneHapticEvent : uint8_t
{
  FiftyPercent = 0,
  HundredPercent = 1,
};

struct AreaMilestonePresentation
{
  uint64_t m_osmId = 0;
  uint32_t m_compactIndex = 0;
  AreaMilestoneThreshold m_threshold = AreaMilestoneThreshold::P25;
  std::string m_displayName;
  std::string m_competitionLine;
};

inline bool operator==(AreaMilestonePresentation const & lhs, AreaMilestonePresentation const & rhs)
{
  return lhs.m_osmId == rhs.m_osmId && lhs.m_compactIndex == rhs.m_compactIndex &&
         lhs.m_threshold == rhs.m_threshold && lhs.m_displayName == rhs.m_displayName &&
         lhs.m_competitionLine == rhs.m_competitionLine;
}

inline bool operator!=(AreaMilestonePresentation const & lhs, AreaMilestonePresentation const & rhs)
{
  return !(lhs == rhs);
}

std::string DebugPrint(AreaMilestoneHapticEvent event);
std::string DebugPrint(AreaMilestonePresentation const & presentation);

class AreaMilestonePresenter
{
public:
  using NameLookup = std::function<std::string(uint32_t compactIndex, uint64_t osmId)>;
  using CompetitionLineFn = std::function<std::string(uint64_t osmId)>;
  using CardSourceLookup = std::function<std::optional<CompletionCardSource>(uint32_t compactIndex, uint64_t osmId)>;

  void SetCompetitionLineProvider(CompetitionLineFn const & fn);
  void Enqueue(std::vector<AreaMilestoneCrossing> const & crossings, NameLookup const & names,
               CardSourceLookup const & cards = {});
  std::optional<AreaMilestonePresentation> Peek() const;
  std::optional<CompletionCardSource> PeekCardSource() const;
  void Acknowledge();
  void ResetForTesting();

private:
  struct QueueItem
  {
    AreaMilestonePresentation presentation;
    std::optional<CompletionCardSource> cardSource;
  };

  bool ContainsUnlocked(uint64_t osmId, AreaMilestoneThreshold threshold) const;
  void SortUnlocked();

  mutable std::mutex m_mutex;
  std::vector<QueueItem> m_queue;
  CompetitionLineFn m_competitionLineFn;
};
}  // namespace street_pixels
