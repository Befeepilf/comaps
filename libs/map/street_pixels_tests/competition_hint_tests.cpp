#include "testing/testing.hpp"

#include "map/competition_hint.hpp"
#include "map/first_goal.hpp"
#include "map/identity_store.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

#include <cstdint>
#include <vector>

namespace
{
class CompetitionHintCleanup
{
public:
  CompetitionHintCleanup()
  {
    settings::Delete("RecordingSessionActive");
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
    street_pixels::CompetitionHintTracker::ClearPersistedForTesting();
    IdentityStore::RevokeCompetitionConsent();
  }

  ~CompetitionHintCleanup()
  {
    IdentityStore::RevokeCompetitionConsent();
    street_pixels::CompetitionHintTracker::ClearPersistedForTesting();
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
    settings::Delete("RecordingSessionActive");
  }
};

class CompetitionHintFixture
{
public:
  CompetitionHintFixture() : m_manager(m_dataSource)
  {
    m_manager.ResetFirstGoalForTesting();
    m_manager.ResetCompetitionHintForTesting();
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

UNIT_TEST(CompetitionHint_FiresAtThirtyNewlyExploredLivePixels)
{
  CompetitionHintCleanup cleanup;
  street_pixels::CompetitionHintTracker tracker;
  TEST(!tracker.AddNewlyExploredLivePixels(29), ());
  TEST_EQUAL(tracker.Snapshot().m_collected, 29u, ());
  TEST(!tracker.Snapshot().m_complete, ());
  TEST(tracker.AddNewlyExploredLivePixels(1), ());
  TEST_EQUAL(tracker.Snapshot().m_collected, street_pixels::kCompetitionHintLivePixelThreshold, ());
  TEST(tracker.Snapshot().m_complete, ());
  TEST(!tracker.Snapshot().m_presented, ());
}

UNIT_TEST(CompetitionHint_ImportDoesNotAdvance)
{
  CompetitionHintCleanup cleanup;
  CompetitionHintFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().MarkImportedPixelsForTesting({fixture.PixelAt(0), fixture.PixelAt(1)});
  auto const hint = fixture.Manager().GetCompetitionHintProgress();
  TEST_EQUAL(hint.m_collected, 0u, ());
  TEST(!hint.m_complete, ());
}

UNIT_TEST(CompetitionHint_DoesNotResetFirstGoalTen)
{
  CompetitionHintCleanup cleanup;
  CompetitionHintFixture fixture;
  fixture.SetupUnexplored(32);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  bool hintFired = false;
  fixture.Manager().SetCompetitionHintReadyHandler([&hintFired]() { hintFired = true; });

  for (int i = 0; i < 10; ++i)
    fixture.Collect(i);
  auto const first = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(first.m_state, street_pixels::FirstGoalState::Complete, ());
  TEST_EQUAL(first.m_collected, street_pixels::kFirstGoalLivePixelThreshold, ());
  TEST_EQUAL(fixture.Manager().GetCompetitionHintProgress().m_collected, 10u, ());
  TEST(!hintFired, ());

  for (int i = 10; i < 29; ++i)
    fixture.Collect(i);
  TEST(!hintFired, ());
  TEST_EQUAL(fixture.Manager().GetCompetitionHintProgress().m_collected, 29u, ());

  fixture.Collect(29);
  TEST(hintFired, ());
  auto const hint = fixture.Manager().GetCompetitionHintProgress();
  TEST(hint.m_complete, ());
  TEST_EQUAL(hint.m_collected, street_pixels::kCompetitionHintLivePixelThreshold, ());
  auto const stillFirst = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(stillFirst.m_state, street_pixels::FirstGoalState::Complete, ());
  TEST_EQUAL(stillFirst.m_collected, street_pixels::kFirstGoalLivePixelThreshold, ());
}

UNIT_TEST(CompetitionHint_DoesNotPresentWhileRoutingFollowing)
{
  CompetitionHintCleanup cleanup;
  TEST(!street_pixels::ShouldPresentCompetitionHint(true, false, false), ());
  TEST(street_pixels::ShouldPresentCompetitionHint(false, false, false), ());
  street_pixels::CompetitionHintTracker tracker;
  TEST(tracker.AddNewlyExploredLivePixels(street_pixels::kCompetitionHintLivePixelThreshold), ());
  TEST(tracker.Snapshot().m_complete, ());
  TEST(!tracker.Snapshot().m_presented, ());
}

UNIT_TEST(CompetitionHint_SkippedWhenAlreadyConsented)
{
  CompetitionHintCleanup cleanup;
  IdentityStore::GrantCompetitionConsent();
  street_pixels::CompetitionHintTracker tracker;
  TEST(!tracker.AddNewlyExploredLivePixels(street_pixels::kCompetitionHintLivePixelThreshold), ());
  TEST(tracker.Snapshot().m_complete, ());
  TEST_EQUAL(tracker.Snapshot().m_collected, street_pixels::kCompetitionHintLivePixelThreshold, ());
  TEST(!tracker.Snapshot().m_presented, ());
}

UNIT_TEST(CompetitionHint_OncePerInstall)
{
  CompetitionHintCleanup cleanup;
  street_pixels::CompetitionHintTracker tracker;
  TEST(tracker.AddNewlyExploredLivePixels(street_pixels::kCompetitionHintLivePixelThreshold), ());
  tracker.MarkPresented();
  TEST(!tracker.AddNewlyExploredLivePixels(1), ());
  TEST(tracker.Snapshot().m_presented, ());
  TEST_EQUAL(tracker.Snapshot().m_collected, street_pixels::kCompetitionHintLivePixelThreshold, ());
}

UNIT_TEST(CompetitionHint_LiveVisitOfImportedPixelsDoesNotAdvance)
{
  CompetitionHintCleanup cleanup;
  CompetitionHintFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().MarkImportedPixelsForTesting({fixture.PixelAt(0), fixture.PixelAt(1)});
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(0)), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.PixelAt(0)), ());

  fixture.Collect(0);
  fixture.Collect(1);
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fixture.PixelAt(0)), ());
  TEST_EQUAL(fixture.Manager().GetCompetitionHintProgress().m_collected, 0u, ());
}
