#pragma once

#include "map/recording_session.hpp"

class GpsTracker;
class StreetPixelsManager;

void ApplyRecordingPauseResumeEffects(RecordingSession::State previous, RecordingSession::State current,
                                      GpsTracker * tracker, StreetPixelsManager * manager);
