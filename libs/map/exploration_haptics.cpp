#include "map/exploration_haptics.hpp"

#include "platform/settings.hpp"
#include "platform/vibration.hpp"

#include <cstddef>
#include <string>

namespace street_pixels
{
bool ShouldPlayExplorationHaptic(ExplorationHapticGate const & gate)
{
  return gate.recording && gate.foreground && gate.toggleOn;
}

bool ShouldPlayCollectionPulse(ExplorationHapticGate const & gate, size_t newlyExploredPixels)
{
  return ShouldPlayExplorationHaptic(gate) && newlyExploredPixels >= 1;
}

bool ExplorationHapticsToggleEnabled()
{
  bool enabled = true;
  settings::TryGet(kExplorationHapticsSettingsKey, enabled);
  return enabled;
}

void PlayExplorationHapticWaveform(ExplorationHapticKind kind)
{
  switch (kind)
  {
  case ExplorationHapticKind::Collection:
    platform::Vibrate(kCollectionPulseMs);
    return;
  case ExplorationHapticKind::FirstGoalComplete:
    platform::VibratePattern(kFirstGoalDurations, kFirstGoalDelays, std::size(kFirstGoalDurations));
    return;
  case ExplorationHapticKind::FiftyPercent:
    platform::VibratePattern(kFiftyDurations, kFiftyDelays, std::size(kFiftyDurations));
    return;
  case ExplorationHapticKind::HundredPercent:
    platform::VibratePattern(kHundredDurations, kHundredDelays, std::size(kHundredDurations));
    return;
  }
}

std::string DebugPrint(ExplorationHapticKind kind)
{
  switch (kind)
  {
  case ExplorationHapticKind::Collection: return "Collection";
  case ExplorationHapticKind::FirstGoalComplete: return "FirstGoalComplete";
  case ExplorationHapticKind::FiftyPercent: return "FiftyPercent";
  case ExplorationHapticKind::HundredPercent: return "HundredPercent";
  }
  return "UnknownExplorationHapticKind";
}
}  // namespace street_pixels
