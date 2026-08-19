#pragma once

#include <cstdint>
#include <mutex>
#include <string>

namespace street_pixels
{
uint32_t constexpr kFirstGoalLivePixelThreshold = 10;

enum class FirstGoalState : uint8_t
{
  Hidden = 0,
  InProgress = 1,
  Complete = 2,
};

struct FirstGoalProgress
{
  FirstGoalState m_state = FirstGoalState::Hidden;
  uint32_t m_collected = 0;
  uint32_t m_threshold = kFirstGoalLivePixelThreshold;
};

inline bool operator==(FirstGoalProgress const & lhs, FirstGoalProgress const & rhs)
{
  return lhs.m_state == rhs.m_state && lhs.m_collected == rhs.m_collected && lhs.m_threshold == rhs.m_threshold;
}

inline bool operator!=(FirstGoalProgress const & lhs, FirstGoalProgress const & rhs) { return !(lhs == rhs); }

class FirstGoalTracker
{
public:
  FirstGoalProgress Snapshot(bool sessionActive) const;
  bool AddNewlyExploredLivePixels(uint32_t count);
  void ResetForTesting();
  static void ClearPersistedForTesting();

private:
  void LoadUnlocked() const;
  void SaveUnlocked() const;

  mutable std::mutex m_mutex;
  mutable bool m_loaded = false;
  mutable bool m_complete = false;
  mutable uint32_t m_collected = 0;
};

std::string DebugPrint(FirstGoalState state);
std::string DebugPrint(FirstGoalProgress const & progress);
}  // namespace street_pixels
