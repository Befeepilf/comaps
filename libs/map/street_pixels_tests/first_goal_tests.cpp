#include "testing/testing.hpp"

#include "map/first_goal.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace
{
class FirstGoalCleanup
{
public:
  FirstGoalCleanup()
  {
    settings::Delete("RecordingSessionActive");
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
  }

  ~FirstGoalCleanup()
  {
    settings::Delete("RecordingSessionActive");
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
  }
};

class FirstGoalFixture
{
public:
  FirstGoalFixture() : m_manager(m_dataSource)
  {
    m_manager.ResetFirstGoalForTesting();
    m_manager.SetRecordingSession(&m_session);
  }

  std::int64_t PixelAt(int index) const
  {
    return street_pixels_tests::PixelIdForLatLon(50.0 + static_cast<double>(index), 10.0);
  }

  void SetupUnexplored(size_t count)
  {
    std::vector<df::StreetPixel> pixels;
    pixels.reserve(count);
    for (size_t i = 0; i < count; ++i)
      pixels.push_back(street_pixels_tests::MakeStreetPixel(PixelAt(static_cast<int>(i)), false));
    m_manager.SetStreetPixelsForTesting(std::move(pixels));
  }

  void Collect(int index)
  {
    m_manager.ResetSampleAcceptanceReference();
    m_manager.MarkInterpolationBarrier();
    m_manager.OnLocationUpdate(street_pixels_tests::MakeGpsInfo(
        50.0 + static_cast<double>(index), 10.0, 5.0, street_pixels_tests::CurrentTimestampSec() + index));
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};
}  // namespace

UNIT_TEST(FirstGoal_AppearsOnFirstRecordingStart)
{
  FirstGoalCleanup cleanup;
  FirstGoalFixture fixture;
  auto hidden = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(hidden.m_state, street_pixels::FirstGoalState::Hidden, ());
  TEST_EQUAL(hidden.m_collected, 0u, ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  auto shown = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(shown.m_state, street_pixels::FirstGoalState::InProgress, ());
  TEST_EQUAL(shown.m_collected, 0u, ());
  TEST_EQUAL(shown.m_threshold, street_pixels::kFirstGoalLivePixelThreshold, ());
}

UNIT_TEST(FirstGoal_CompletesAtTenNewlyExploredLivePixels)
{
  FirstGoalCleanup cleanup;
  FirstGoalFixture fixture;
  fixture.SetupUnexplored(12);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  size_t completeCalls = 0;
  fixture.Manager().SetFirstGoalCompleteHandler([&completeCalls]() { ++completeCalls; });

  for (int i = 0; i < 9; ++i)
  {
    fixture.Collect(i);
    auto p = fixture.Manager().GetFirstGoalProgress();
    TEST_EQUAL(p.m_state, street_pixels::FirstGoalState::InProgress, ());
    TEST_EQUAL(p.m_collected, static_cast<uint32_t>(i + 1), ());
  }
  TEST_EQUAL(completeCalls, 0u, ());

  fixture.Collect(9);
  auto done = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(done.m_state, street_pixels::FirstGoalState::Complete, ());
  TEST_EQUAL(done.m_collected, street_pixels::kFirstGoalLivePixelThreshold, ());
  TEST_EQUAL(completeCalls, 1u, ());
}

UNIT_TEST(FirstGoal_ImportDoesNotAdvance)
{
  FirstGoalCleanup cleanup;
  FirstGoalFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().MarkImportedPixelsForTesting({fixture.PixelAt(0), fixture.PixelAt(1)});
  auto p = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(p.m_state, street_pixels::FirstGoalState::InProgress, ());
  TEST_EQUAL(p.m_collected, 0u, ());
}

UNIT_TEST(FirstGoal_PauseDoesNotIncrement)
{
  FirstGoalCleanup cleanup;
  FirstGoalFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Collect(0);
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_collected, 1u, ());

  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Collect(1);
  auto paused = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(paused.m_state, street_pixels::FirstGoalState::InProgress, ());
  TEST_EQUAL(paused.m_collected, 1u, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(1)), ());

  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());
  fixture.Collect(1);
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_collected, 2u, ());
}

UNIT_TEST(FirstGoal_IncompleteSurvivesSecondSession)
{
  FirstGoalCleanup cleanup;
  FirstGoalFixture fixture;
  fixture.SetupUnexplored(5);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Collect(0);
  fixture.Collect(1);
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_collected, 2u, ());

  TEST_EQUAL(fixture.Session().Finish(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_state, street_pixels::FirstGoalState::Hidden, ());
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_collected, 2u, ());

  TEST_EQUAL(fixture.Session().Reset(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  auto again = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(again.m_state, street_pixels::FirstGoalState::InProgress, ());
  TEST_EQUAL(again.m_collected, 2u, ());
}

UNIT_TEST(FirstGoal_CompleteDoesNotReturn)
{
  FirstGoalCleanup cleanup;
  FirstGoalFixture fixture;
  fixture.SetupUnexplored(12);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  for (int i = 0; i < 10; ++i)
    fixture.Collect(i);
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_state, street_pixels::FirstGoalState::Complete, ());

  TEST_EQUAL(fixture.Session().Finish(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Reset(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Manager().GetFirstGoalProgress().m_state, street_pixels::FirstGoalState::Complete, ());
}
