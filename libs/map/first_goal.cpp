#include "map/first_goal.hpp"

#include "platform/settings.hpp"

#include <string>

namespace street_pixels
{
namespace
{
constexpr char kCollectedKey[] = "StreetPixels.FirstGoalCollected";
constexpr char kCompleteKey[] = "StreetPixels.FirstGoalComplete";
}  // namespace

void FirstGoalTracker::LoadUnlocked() const
{
  if (m_loaded)
    return;
  settings::TryGet(kCompleteKey, m_complete);
  uint64_t collected = 0;
  settings::TryGet(kCollectedKey, collected);
  if (collected > kFirstGoalLivePixelThreshold)
    collected = kFirstGoalLivePixelThreshold;
  m_collected = static_cast<uint32_t>(collected);
  if (m_complete)
    m_collected = kFirstGoalLivePixelThreshold;
  m_loaded = true;
}

void FirstGoalTracker::SaveUnlocked() const
{
  settings::Set(kCompleteKey, m_complete);
  settings::Set(kCollectedKey, static_cast<uint64_t>(m_collected));
}

FirstGoalProgress FirstGoalTracker::Snapshot(bool sessionActive) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  LoadUnlocked();
  FirstGoalProgress progress;
  progress.m_collected = m_collected;
  progress.m_threshold = kFirstGoalLivePixelThreshold;
  if (m_complete)
    progress.m_state = FirstGoalState::Complete;
  else if (sessionActive)
    progress.m_state = FirstGoalState::InProgress;
  else
    progress.m_state = FirstGoalState::Hidden;
  return progress;
}

bool FirstGoalTracker::AddNewlyExploredLivePixels(uint32_t count)
{
  if (count == 0)
    return false;
  std::lock_guard<std::mutex> lock(m_mutex);
  LoadUnlocked();
  if (m_complete)
    return false;
  uint32_t const room = kFirstGoalLivePixelThreshold - m_collected;
  uint32_t const add = count > room ? room : count;
  uint32_t const next = m_collected + add;
  if (next >= kFirstGoalLivePixelThreshold)
  {
    m_collected = kFirstGoalLivePixelThreshold;
    m_complete = true;
    SaveUnlocked();
    return true;
  }
  m_collected = next;
  SaveUnlocked();
  return false;
}

void FirstGoalTracker::ResetForTesting()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ClearPersistedForTesting();
  m_loaded = false;
  m_complete = false;
  m_collected = 0;
}

void FirstGoalTracker::ClearPersistedForTesting()
{
  settings::Delete(kCompleteKey);
  settings::Delete(kCollectedKey);
}

std::string DebugPrint(FirstGoalState state)
{
  switch (state)
  {
  case FirstGoalState::Hidden: return "Hidden";
  case FirstGoalState::InProgress: return "InProgress";
  case FirstGoalState::Complete: return "Complete";
  }
  return "UnknownFirstGoalState";
}

std::string DebugPrint(FirstGoalProgress const & progress)
{
  return DebugPrint(progress.m_state) + " " + std::to_string(progress.m_collected) + "/" +
         std::to_string(progress.m_threshold);
}
}  // namespace street_pixels
