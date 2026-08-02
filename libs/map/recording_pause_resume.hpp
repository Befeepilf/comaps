#pragma once

#include "map/recording_session.hpp"

#include <cstdint>

class GpsTracker;
class StreetPixelsManager;

inline constexpr uint64_t kRecordingInterruptionGapSeconds = 60;

inline bool IsRecordingLocationGapAnInterruption(uint64_t gapSeconds)
{
  return gapSeconds >= kRecordingInterruptionGapSeconds;
}

void ApplyRecordingPauseResumeEffects(RecordingSession::State previous, RecordingSession::State current,
                                      GpsTracker * tracker, StreetPixelsManager * manager);

void ApplyRecordingInterruptionEffects(GpsTracker * tracker, StreetPixelsManager * manager);
