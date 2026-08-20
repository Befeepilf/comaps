#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace street_pixels
{
inline constexpr char kExplorationHapticsSettingsKey[] = "StreetPixels.ExplorationHaptics";

enum class ExplorationHapticKind : uint8_t
{
  Collection = 0,
  FirstGoalComplete = 1,
  FiftyPercent = 2,
  HundredPercent = 3,
};

struct ExplorationHapticGate
{
  bool recording = false;
  bool foreground = false;
  bool toggleOn = true;
};

bool ShouldPlayExplorationHaptic(ExplorationHapticGate const & gate);
bool ShouldPlayCollectionPulse(ExplorationHapticGate const & gate, size_t newlyExploredPixels);
bool ExplorationHapticsToggleEnabled();

uint32_t constexpr kCollectionPulseMs = 50;
uint32_t constexpr kFirstGoalDurations[] = {80, 80};
uint32_t constexpr kFirstGoalDelays[] = {90, 90};
uint32_t constexpr kFiftyDurations[] = {90, 90};
uint32_t constexpr kFiftyDelays[] = {110, 110};
uint32_t constexpr kHundredDurations[] = {80, 80, 120};
uint32_t constexpr kHundredDelays[] = {70, 70, 70};

void PlayExplorationHapticWaveform(ExplorationHapticKind kind);
std::string DebugPrint(ExplorationHapticKind kind);
}  // namespace street_pixels
