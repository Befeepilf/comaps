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

void AreaMilestonePresenter::Enqueue(std::vector<AreaMilestoneCrossing> const & crossings, NameLookup const & names)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto const & crossing : crossings)
  {
    std::string name = names(crossing.m_compactIndex, crossing.m_osmId);
    if (name.empty())
      continue;
    if (ContainsUnlocked(crossing.m_osmId, crossing.m_threshold))
      continue;
    AreaMilestonePresentation item;
    item.m_osmId = crossing.m_osmId;
    item.m_compactIndex = crossing.m_compactIndex;
    item.m_threshold = crossing.m_threshold;
    item.m_displayName = std::move(name);
    if (m_competitionLineFn)
      item.m_competitionLine = m_competitionLineFn(crossing.m_osmId);
    m_queue.push_back(std::move(item));
  }
  SortUnlocked();
}

std::optional<AreaMilestonePresentation> AreaMilestonePresenter::Peek() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return std::nullopt;
  return m_queue.front();
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
    if (item.m_osmId == osmId && item.m_threshold == threshold)
      return true;
  }
  return false;
}

void AreaMilestonePresenter::SortUnlocked()
{
  std::sort(m_queue.begin(), m_queue.end(),
            [](AreaMilestonePresentation const & a, AreaMilestonePresentation const & b)
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
