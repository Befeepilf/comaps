#include "map/recording_pause_resume.hpp"

#include "map/gps_tracker.hpp"
#include "map/street_pixels_manager.hpp"

void ApplyRecordingPauseResumeEffects(RecordingSession::State previous, RecordingSession::State current,
                                      GpsTracker * tracker, StreetPixelsManager * manager)
{
  using State = RecordingSession::State;

  if (previous == State::Recording && current == State::Paused)
  {
    if (tracker != nullptr && tracker->IsEnabled())
    {
      tracker->SetAppendSuspended(true);
      tracker->MarkSegmentBoundary();
    }
    if (manager != nullptr)
      manager->ResetSampleAcceptanceReference();
  }
  else if (previous == State::Paused && current == State::Recording)
  {
    if (tracker != nullptr && tracker->IsEnabled())
      tracker->SetAppendSuspended(false);
    if (manager != nullptr)
      manager->ResetSampleAcceptanceReference();
  }
  else if (previous == State::Paused &&
           (current == State::Finished || current == State::Discarded))
  {
    if (tracker != nullptr && tracker->IsEnabled())
      tracker->SetAppendSuspended(false);
  }
}

void ApplyRecordingInterruptionEffects(GpsTracker * tracker, StreetPixelsManager * manager)
{
  if (tracker != nullptr && tracker->IsEnabled())
    tracker->MarkSegmentBoundary();
  if (manager != nullptr)
    manager->ResetSampleAcceptanceReference();
}
