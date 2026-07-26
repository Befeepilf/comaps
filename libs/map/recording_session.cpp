#include "map/recording_session.hpp"

#include "platform/settings.hpp"

#include "base/timer.hpp"

namespace
{
char constexpr kActiveSessionKey[] = "RecordingSessionActive";

uint64_t & NextSessionId()
{
  static uint64_t s_nextSessionId = 0;
  return s_nextSessionId;
}
}  // namespace

RecordingSession::TransitionResult RecordingSession::Start()
{
  if (m_state != State::Idle)
    return TransitionResult::Rejected;

  m_sessionId = ++NextSessionId();
  m_startTimestampSec = base::SecondsSinceEpoch();
  SetActiveSessionBreadcrumb(true);
  return TransitionTo(State::Recording);
}

RecordingSession::TransitionResult RecordingSession::Pause()
{
  if (m_state != State::Recording)
    return TransitionResult::Rejected;

  return TransitionTo(State::Paused);
}

RecordingSession::TransitionResult RecordingSession::Resume()
{
  if (m_state != State::Paused)
    return TransitionResult::Rejected;

  return TransitionTo(State::Recording);
}

RecordingSession::TransitionResult RecordingSession::Finish()
{
  if (m_state != State::Recording && m_state != State::Paused)
    return TransitionResult::Rejected;

  SetActiveSessionBreadcrumb(false);
  return TransitionTo(State::Finished);
}

RecordingSession::TransitionResult RecordingSession::Discard()
{
  if (m_state != State::Recording && m_state != State::Paused)
    return TransitionResult::Rejected;

  SetActiveSessionBreadcrumb(false);
  return TransitionTo(State::Discarded);
}

RecordingSession::TransitionResult RecordingSession::Reset()
{
  if (m_state != State::Finished && m_state != State::Discarded)
    return TransitionResult::Rejected;

  ClearSessionMetadata();
  return TransitionTo(State::Idle);
}

RecordingSession::State RecordingSession::GetState() const { return m_state; }

bool RecordingSession::IsRecording() const { return m_state == State::Recording; }

uint64_t RecordingSession::GetSessionId() const { return m_sessionId; }

uint64_t RecordingSession::GetStartTimestampSec() const { return m_startTimestampSec; }

bool RecordingSession::HasActiveSessionBreadcrumb() const
{
  bool active = false;
  return settings::Get(kActiveSessionKey, active) && active;
}

void RecordingSession::SetStateListener(StateChangedFn const & fn) { m_stateListener = fn; }

RecordingSession::TransitionResult RecordingSession::TransitionTo(State next)
{
  State const previous = m_state;
  m_state = next;
  if (m_stateListener)
    m_stateListener(previous, next);
  return TransitionResult::Ok;
}

void RecordingSession::SetActiveSessionBreadcrumb(bool active)
{
  if (active)
    settings::Set(kActiveSessionKey, true);
  else
    settings::Delete(kActiveSessionKey);
}

void RecordingSession::ClearSessionMetadata()
{
  m_sessionId = 0;
  m_startTimestampSec = 0;
}

std::string DebugPrint(RecordingSession::State state)
{
  switch (state)
  {
  case RecordingSession::State::Idle: return "Idle";
  case RecordingSession::State::Recording: return "Recording";
  case RecordingSession::State::Paused: return "Paused";
  case RecordingSession::State::Finished: return "Finished";
  case RecordingSession::State::Discarded: return "Discarded";
  }
  return "Unknown";
}

std::string DebugPrint(RecordingSession::TransitionResult result)
{
  switch (result)
  {
  case RecordingSession::TransitionResult::Ok: return "Ok";
  case RecordingSession::TransitionResult::Rejected: return "Rejected";
  }
  return "Unknown";
}
