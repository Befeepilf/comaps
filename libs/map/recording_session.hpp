#pragma once

#include <cstdint>
#include <functional>
#include <string>

class RecordingSession
{
public:
  enum class State
  {
    Idle,
    Recording,
    Paused,
    Finished,
    Discarded,
  };

  enum class TransitionResult
  {
    Ok,
    Rejected,
  };

  using StateChangedFn = std::function<void(State previous, State current)>;

  TransitionResult Start();
  TransitionResult Pause();
  TransitionResult Resume();
  TransitionResult Finish();
  TransitionResult Discard();
  TransitionResult Reset();

  State GetState() const;
  bool IsRecording() const;
  uint64_t GetSessionId() const;
  uint64_t GetStartTimestampSec() const;
  bool HasActiveSessionBreadcrumb() const;

  void SetStateListener(StateChangedFn const & fn);

private:
  TransitionResult TransitionTo(State next);
  void SetActiveSessionBreadcrumb(bool active);
  void ClearSessionMetadata();

  State m_state = State::Idle;
  uint64_t m_sessionId = 0;
  uint64_t m_startTimestampSec = 0;
  StateChangedFn m_stateListener;
};

std::string DebugPrint(RecordingSession::State state);
std::string DebugPrint(RecordingSession::TransitionResult result);
