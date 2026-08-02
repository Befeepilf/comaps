#include "testing/testing.hpp"

#include "map/recording_session.hpp"

#include "platform/settings.hpp"

#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace
{
using State = RecordingSession::State;
using TransitionResult = RecordingSession::TransitionResult;

class BreadcrumbCleanup
{
public:
  BreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~BreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

void TEST_EQUAL_STATE(RecordingSession const & session, State expected)
{
  TEST_EQUAL(session.GetState(), expected, ());
}

void TEST_OK(TransitionResult result) { TEST_EQUAL(result, TransitionResult::Ok, ()); }

void TEST_REJECTED(TransitionResult result) { TEST_EQUAL(result, TransitionResult::Rejected, ()); }

struct StateTransition
{
  State previous;
  State current;
};

class ObserverRecorder
{
public:
  void Attach(RecordingSession & session)
  {
    session.SetStateListener([this](State previous, State current) { m_transitions.emplace_back(previous, current); });
  }

  std::vector<StateTransition> const & Transitions() const { return m_transitions; }

  void Clear() { m_transitions.clear(); }

private:
  std::vector<StateTransition> m_transitions;
};
}  // namespace

UNIT_TEST(RecordingSession_LegalTransitions_PauseResumeFinishReset)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_EQUAL_STATE(session, State::Idle);
  TEST_OK(session.Start());
  TEST_EQUAL_STATE(session, State::Recording);
  TEST(session.GetSessionId() != 0, ());
  TEST(session.GetStartTimestampSec() != 0, ());
  TEST(session.HasActiveSessionBreadcrumb(), ());

  TEST_OK(session.Pause());
  TEST_EQUAL_STATE(session, State::Paused);
  TEST(session.HasActiveSessionBreadcrumb(), ());

  TEST_OK(session.Resume());
  TEST_EQUAL_STATE(session, State::Recording);

  TEST_OK(session.Finish());
  TEST_EQUAL_STATE(session, State::Finished);
  TEST(!session.HasActiveSessionBreadcrumb(), ());
  TEST(session.GetSessionId() != 0, ());
  TEST(session.GetStartTimestampSec() != 0, ());

  TEST_OK(session.Reset());
  TEST_EQUAL_STATE(session, State::Idle);
  TEST_EQUAL(session.GetSessionId(), 0ULL, ());
  TEST_EQUAL(session.GetStartTimestampSec(), 0ULL, ());

  TEST_EQUAL(observer.Transitions().size(), 5, ());
}

UNIT_TEST(RecordingSession_LegalTransitions_DiscardReset)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_OK(session.Start());
  TEST_OK(session.Discard());
  TEST_EQUAL_STATE(session, State::Discarded);
  TEST(!session.HasActiveSessionBreadcrumb(), ());

  TEST_OK(session.Reset());
  TEST_EQUAL_STATE(session, State::Idle);
}

UNIT_TEST(RecordingSession_LegalTransitions_FinishFromRecording)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_OK(session.Start());
  TEST_OK(session.Finish());
  TEST_EQUAL_STATE(session, State::Finished);
  TEST_OK(session.Reset());
}

UNIT_TEST(RecordingSession_LegalTransitions_FinishFromPaused)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_OK(session.Start());
  TEST_OK(session.Pause());
  TEST_OK(session.Finish());
  TEST_EQUAL_STATE(session, State::Finished);
  TEST_OK(session.Reset());
}

UNIT_TEST(RecordingSession_LegalTransitions_DiscardFromPaused)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_OK(session.Start());
  TEST_OK(session.Pause());
  TEST_OK(session.Discard());
  TEST_EQUAL_STATE(session, State::Discarded);
  TEST_OK(session.Reset());
}

UNIT_TEST(RecordingSession_IllegalTransitions_FromIdle)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_REJECTED(session.Pause());
  TEST_REJECTED(session.Resume());
  TEST_REJECTED(session.Finish());
  TEST_REJECTED(session.Discard());
  TEST_REJECTED(session.Reset());
  TEST_EQUAL_STATE(session, State::Idle);
  TEST_EQUAL(observer.Transitions().size(), 0, ());
}

UNIT_TEST(RecordingSession_IllegalTransitions_FromRecording)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_OK(session.Start());
  observer.Clear();

  TEST_REJECTED(session.Start());
  TEST_REJECTED(session.Resume());
  TEST_REJECTED(session.Reset());
  TEST_EQUAL_STATE(session, State::Recording);
  TEST_EQUAL(observer.Transitions().size(), 0, ());
}

UNIT_TEST(RecordingSession_IllegalTransitions_FromPaused)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_OK(session.Start());
  TEST_OK(session.Pause());
  observer.Clear();

  TEST_REJECTED(session.Start());
  TEST_REJECTED(session.Pause());
  TEST_REJECTED(session.Reset());
  TEST_EQUAL_STATE(session, State::Paused);
  TEST_EQUAL(observer.Transitions().size(), 0, ());
}

UNIT_TEST(RecordingSession_IllegalTransitions_FromFinished)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_OK(session.Start());
  TEST_OK(session.Finish());
  observer.Clear();

  TEST_REJECTED(session.Start());
  TEST_REJECTED(session.Pause());
  TEST_REJECTED(session.Resume());
  TEST_REJECTED(session.Finish());
  TEST_REJECTED(session.Discard());
  TEST_EQUAL_STATE(session, State::Finished);
  TEST_EQUAL(observer.Transitions().size(), 0, ());
}

UNIT_TEST(RecordingSession_IllegalTransitions_FromDiscarded)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_OK(session.Start());
  TEST_OK(session.Discard());
  observer.Clear();

  TEST_REJECTED(session.Start());
  TEST_REJECTED(session.Pause());
  TEST_REJECTED(session.Resume());
  TEST_REJECTED(session.Finish());
  TEST_REJECTED(session.Discard());
  TEST_EQUAL_STATE(session, State::Discarded);
  TEST_EQUAL(observer.Transitions().size(), 0, ());
}

UNIT_TEST(RecordingSession_DistinctSessionIds)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_OK(session.Start());
  uint64_t const firstId = session.GetSessionId();
  TEST_OK(session.Finish());
  TEST_OK(session.Reset());

  TEST_OK(session.Start());
  uint64_t const secondId = session.GetSessionId();
  TEST(firstId != secondId, ());
}

UNIT_TEST(RecordingSession_ObserverNotifiesOnSuccessOnly)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;
  ObserverRecorder observer;
  observer.Attach(session);

  TEST_OK(session.Start());
  TEST_REJECTED(session.Start());
  TEST_OK(session.Pause());
  TEST_REJECTED(session.Pause());
  TEST_OK(session.Resume());
  TEST_REJECTED(session.Resume());
  TEST_OK(session.Finish());
  TEST_OK(session.Reset());

  TEST_EQUAL(observer.Transitions().size(), 5, ());
  TEST_EQUAL(observer.Transitions()[0].previous, State::Idle, ());
  TEST_EQUAL(observer.Transitions()[0].current, State::Recording, ());
  TEST_EQUAL(observer.Transitions()[1].previous, State::Recording, ());
  TEST_EQUAL(observer.Transitions()[1].current, State::Paused, ());
  TEST_EQUAL(observer.Transitions()[2].previous, State::Paused, ());
  TEST_EQUAL(observer.Transitions()[2].current, State::Recording, ());
  TEST_EQUAL(observer.Transitions()[3].previous, State::Recording, ());
  TEST_EQUAL(observer.Transitions()[3].current, State::Finished, ());
  TEST_EQUAL(observer.Transitions()[4].previous, State::Finished, ());
  TEST_EQUAL(observer.Transitions()[4].current, State::Idle, ());
}

UNIT_TEST(RecordingSession_IsRecording)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST(!session.IsRecording(), ());
  TEST_OK(session.Start());
  TEST(session.IsRecording(), ());
  TEST_OK(session.Pause());
  TEST(!session.IsRecording(), ());
  TEST_OK(session.Resume());
  TEST(session.IsRecording(), ());
}

UNIT_TEST(RecordingSession_BreadcrumbPersistenceAcrossRestart)
{
  BreadcrumbCleanup cleanup;
  {
    RecordingSession session;
    TEST_OK(session.Start());
    TEST(session.HasActiveSessionBreadcrumb(), ());
  }

  RecordingSession restartedSession;
  TEST_EQUAL_STATE(restartedSession, State::Idle);
  TEST(restartedSession.HasActiveSessionBreadcrumb(), ());
}

UNIT_TEST(RecordingSession_BreadcrumbClearedOnFinishAndDiscard)
{
  BreadcrumbCleanup cleanup;
  RecordingSession finishSession;
  TEST_OK(finishSession.Start());
  TEST(finishSession.HasActiveSessionBreadcrumb(), ());
  TEST_OK(finishSession.Finish());
  TEST(!finishSession.HasActiveSessionBreadcrumb(), ());

  RecordingSession discardSession;
  TEST_OK(discardSession.Start());
  TEST(discardSession.HasActiveSessionBreadcrumb(), ());
  TEST_OK(discardSession.Discard());
  TEST(!discardSession.HasActiveSessionBreadcrumb(), ());
}

UNIT_TEST(RecordingSession_PausedDuration_AccumulatesAcrossCycles)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_EQUAL(session.GetPausedDurationSec(), 0ULL, ());
  TEST_OK(session.Start());
  TEST_EQUAL(session.GetPausedDurationSec(), 0ULL, ());

  TEST_OK(session.Pause());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  uint64_t const duringFirstPause = session.GetPausedDurationSec();
  TEST(duringFirstPause >= 1ULL, (duringFirstPause));

  TEST_OK(session.Resume());
  uint64_t const afterFirstResume = session.GetPausedDurationSec();
  TEST(afterFirstResume >= 1ULL, (afterFirstResume));

  TEST_OK(session.Pause());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  TEST_OK(session.Resume());
  uint64_t const afterSecondResume = session.GetPausedDurationSec();
  TEST(afterSecondResume >= afterFirstResume + 1ULL, (afterSecondResume, afterFirstResume));

  TEST_OK(session.Pause());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  TEST_OK(session.Finish());
  uint64_t const afterFinishFromPaused = session.GetPausedDurationSec();
  TEST(afterFinishFromPaused >= afterSecondResume + 1ULL, (afterFinishFromPaused, afterSecondResume));

  TEST_OK(session.Reset());
  TEST_EQUAL(session.GetPausedDurationSec(), 0ULL, ());
}

UNIT_TEST(RecordingSession_PausedDuration_DiscardFromPausedAccumulates)
{
  BreadcrumbCleanup cleanup;
  RecordingSession session;

  TEST_OK(session.Start());
  TEST_OK(session.Pause());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  TEST_OK(session.Discard());
  TEST(session.GetPausedDurationSec() >= 1ULL, (session.GetPausedDurationSec()));
}
