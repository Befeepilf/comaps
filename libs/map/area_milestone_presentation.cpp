#include "map/area_milestone_presentation.hpp"

#include <algorithm>
#include <string>

namespace street_pixels
{
namespace
{
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
}  // namespace

void AreaMilestonePresenter::SetCompetitionLineProvider(CompetitionLineFn const & fn)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_competitionLineFn = fn;
}

void AreaMilestonePresenter::Enqueue(std::vector<AreaMilestoneCrossing> const & crossings, NameLookup const & names,
                                     CardSourceLookup const & cards)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto const & crossing : crossings)
  {
    std::string name = names(crossing.m_compactIndex, crossing.m_osmId);
    if (name.empty())
      continue;
    if (ContainsUnlocked(crossing.m_osmId, crossing.m_threshold))
      continue;
    QueueItem item;
    item.presentation.m_osmId = crossing.m_osmId;
    item.presentation.m_compactIndex = crossing.m_compactIndex;
    item.presentation.m_threshold = crossing.m_threshold;
    item.presentation.m_displayName = std::move(name);
    if (m_competitionLineFn)
      item.presentation.m_competitionLine = m_competitionLineFn(crossing.m_osmId);
    if (cards && crossing.m_threshold == AreaMilestoneThreshold::P100)
    {
      item.cardSource = cards(crossing.m_compactIndex, crossing.m_osmId);
      if (item.cardSource)
        item.cardSource->m_competitionLine = item.presentation.m_competitionLine;
    }
    m_queue.push_back(std::move(item));
  }
  SortUnlocked();
}

std::optional<AreaMilestonePresentation> AreaMilestonePresenter::Peek() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return std::nullopt;
  return m_queue.front().presentation;
}

std::optional<CompletionCardSource> AreaMilestonePresenter::PeekCardSource() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return std::nullopt;
  return m_queue.front().cardSource;
}

void AreaMilestonePresenter::Acknowledge()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return;
  m_queue.erase(m_queue.begin());
}

void AreaMilestonePresenter::ResetForTesting()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queue.clear();
}

bool AreaMilestonePresenter::ContainsUnlocked(uint64_t osmId, AreaMilestoneThreshold threshold) const
{
  for (auto const & item : m_queue)
  {
    if (item.presentation.m_osmId == osmId && item.presentation.m_threshold == threshold)
      return true;
  }
  return false;
}

void AreaMilestonePresenter::SortUnlocked()
{
  std::sort(m_queue.begin(), m_queue.end(),
            [](QueueItem const & a, QueueItem const & b)
            {
              int const pa = CrossingPriority(a.presentation.m_threshold);
              int const pb = CrossingPriority(b.presentation.m_threshold);
              if (pa != pb)
                return pa < pb;
              if (a.presentation.m_osmId != b.presentation.m_osmId)
                return a.presentation.m_osmId < b.presentation.m_osmId;
              return a.presentation.m_compactIndex < b.presentation.m_compactIndex;
            });
}

std::string DebugPrint(AreaMilestoneHapticEvent event)
{
  switch (event)
  {
  case AreaMilestoneHapticEvent::FiftyPercent: return "FiftyPercent";
  case AreaMilestoneHapticEvent::HundredPercent: return "HundredPercent";
  }
  return "UnknownAreaMilestoneHapticEvent";
}

std::string DebugPrint(AreaMilestonePresentation const & presentation)
{
  return DebugPrint(presentation.m_threshold) + " " + presentation.m_displayName + " " +
         std::to_string(presentation.m_osmId);
}
}  // namespace street_pixels
