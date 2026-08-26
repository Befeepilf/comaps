#pragma once

#include "street_pixels_areas/competition_presentation.hpp"

#include <cstdint>
#include <mutex>
#include <string>

namespace street_pixels
{
struct CompetitionHintProgress
{
  uint32_t m_collected = 0;
  uint32_t m_threshold = kCompetitionHintLivePixelThreshold;
  bool m_complete = false;
  bool m_presented = false;
};

inline bool operator==(CompetitionHintProgress const & lhs, CompetitionHintProgress const & rhs)
{
  return lhs.m_collected == rhs.m_collected && lhs.m_threshold == rhs.m_threshold &&
         lhs.m_complete == rhs.m_complete && lhs.m_presented == rhs.m_presented;
}

inline bool operator!=(CompetitionHintProgress const & lhs, CompetitionHintProgress const & rhs)
{
  return !(lhs == rhs);
}

inline bool ShouldPresentCompetitionHint(bool routingFollowing, bool hasConsent, bool presented)
{
  return !routingFollowing && !hasConsent && !presented;
}

class CompetitionHintTracker
{
public:
  CompetitionHintProgress Snapshot() const;
  bool AddNewlyExploredLivePixels(uint32_t count);
  void MarkPresented();
  void ResetForTesting();
  static void ClearPersistedForTesting();

private:
  void LoadUnlocked() const;
  void SaveUnlocked() const;

  mutable std::mutex m_mutex;
  mutable bool m_loaded = false;
  mutable bool m_complete = false;
  mutable bool m_presented = false;
  mutable uint32_t m_collected = 0;
};

std::string DebugPrint(CompetitionHintProgress const & progress);
}  // namespace street_pixels
