#include "map/competition_hint.hpp"

#include "map/identity_store.hpp"

#include "platform/settings.hpp"

#include <string>

namespace street_pixels
{
namespace
{
constexpr char kHintCollectedKey[] = "StreetPixels.CompetitionHintCollected";
constexpr char kHintCompleteKey[] = "StreetPixels.CompetitionHintComplete";
constexpr char kHintPresentedKey[] = "StreetPixels.CompetitionHintPresented";
}  // namespace

void CompetitionHintTracker::LoadUnlocked() const
{
  if (m_loaded)
    return;
  settings::TryGet(kHintCompleteKey, m_complete);
  settings::TryGet(kHintPresentedKey, m_presented);
  uint64_t collected = 0;
  settings::TryGet(kHintCollectedKey, collected);
  if (collected > kCompetitionHintLivePixelThreshold)
    collected = kCompetitionHintLivePixelThreshold;
  m_collected = static_cast<uint32_t>(collected);
  if (m_complete)
    m_collected = kCompetitionHintLivePixelThreshold;
  m_loaded = true;
}

void CompetitionHintTracker::SaveUnlocked() const
{
  settings::Set(kHintCompleteKey, m_complete);
  settings::Set(kHintPresentedKey, m_presented);
  settings::Set(kHintCollectedKey, static_cast<uint64_t>(m_collected));
}

CompetitionHintProgress CompetitionHintTracker::Snapshot() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  LoadUnlocked();
  CompetitionHintProgress progress;
  progress.m_collected = m_collected;
  progress.m_threshold = kCompetitionHintLivePixelThreshold;
  progress.m_complete = m_complete;
  progress.m_presented = m_presented;
  return progress;
}

bool CompetitionHintTracker::AddNewlyExploredLivePixels(uint32_t count)
{
  if (count == 0)
    return false;
  std::lock_guard<std::mutex> lock(m_mutex);
  LoadUnlocked();
  if (m_presented)
    return false;
  bool const alreadyComplete = m_complete;
  if (!m_complete)
  {
    uint32_t const room = kCompetitionHintLivePixelThreshold - m_collected;
    uint32_t const add = count > room ? room : count;
    uint32_t const next = m_collected + add;
    if (next >= kCompetitionHintLivePixelThreshold)
    {
      m_collected = kCompetitionHintLivePixelThreshold;
      m_complete = true;
    }
    else
    {
      m_collected = next;
    }
    SaveUnlocked();
  }
  if (!m_complete || alreadyComplete || m_presented)
    return false;
  if (IdentityStore::HasCompetitionConsent())
    return false;
  return true;
}

void CompetitionHintTracker::MarkPresented()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  LoadUnlocked();
  m_presented = true;
  SaveUnlocked();
}

void CompetitionHintTracker::ResetForTesting()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ClearPersistedForTesting();
  m_loaded = false;
  m_complete = false;
  m_presented = false;
  m_collected = 0;
}

void CompetitionHintTracker::ClearPersistedForTesting()
{
  settings::Delete(kHintCompleteKey);
  settings::Delete(kHintPresentedKey);
  settings::Delete(kHintCollectedKey);
}

std::string DebugPrint(CompetitionHintProgress const & progress)
{
  return std::to_string(progress.m_collected) + "/" + std::to_string(progress.m_threshold) +
         (progress.m_complete ? " complete" : "") + (progress.m_presented ? " presented" : "");
}
}  // namespace street_pixels
