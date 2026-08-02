#include "testing/testing.hpp"

#include "map/gps_track.hpp"
#include "map/gps_tracker.hpp"
#include "map/live_sample_acceptance_filter.hpp"
#include "map/recording_pause_resume.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"
#include "map/track_statistics.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <initializer_list>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace interrupted_session_tests_detail
{
double constexpr kInterruptedBaseLat = 48.2;
double constexpr kInterruptedBaseLon = 16.37;

class InterruptedSessionBreadcrumbCleanup
{
public:
  InterruptedSessionBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~InterruptedSessionBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class InterruptedSessionFixture
{
public:
  InterruptedSessionFixture()
    : m_manager(m_dataSource)
  {
    m_manager.SetRecordingSession(&m_session);
    m_session.SetStateListener([this](RecordingSession::State previous, RecordingSession::State current)
    {
      ApplyRecordingPauseResumeEffects(previous, current, nullptr /* tracker */, &m_manager);
    });
  }

  void SetupPixels(std::initializer_list<std::pair<std::int64_t, bool>> idsAndExplored)
  {
    m_manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet(idsAndExplored));
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};

struct InterruptedSegmentGeometry
{
  double startLat = 0.0;
  double startLon = 0.0;
  double midLat = 0.0;
  double midLon = 0.0;
  double endLat = 0.0;
  double endLon = 0.0;
  std::int64_t pixelStart = 0;
  std::int64_t pixelMid = 0;
  std::int64_t pixelEnd = 0;
};

InterruptedSegmentGeometry MakeInterruptedNorthSegment(double lengthMeters)
{
  InterruptedSegmentGeometry g;
  g.startLat = kInterruptedBaseLat;
  g.startLon = kInterruptedBaseLon;
  auto const mid =
      street_pixels_tests::OffsetLatLonByMeters(kInterruptedBaseLat, kInterruptedBaseLon, lengthMeters * 0.5, 0.0);
  g.midLat = mid.first;
  g.midLon = mid.second;
  auto const end =
      street_pixels_tests::OffsetLatLonByMeters(kInterruptedBaseLat, kInterruptedBaseLon, lengthMeters, 0.0);
  g.endLat = end.first;
  g.endLon = end.second;
  g.pixelStart = street_pixels_tests::PixelIdForLatLon(g.startLat, g.startLon);
  g.pixelMid = street_pixels_tests::PixelIdForLatLon(g.midLat, g.midLon);
  g.pixelEnd = street_pixels_tests::PixelIdForLatLon(g.endLat, g.endLon);
  return g;
}

location::GpsInfo InterruptedGpsAt(double lat, double lon, double timestampSec)
{
  return street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, timestampSec);
}

class GpsTrackerStateRestorer
{
public:
  GpsTrackerStateRestorer(GpsTracker & tracker, bool enabled, bool suspended)
    : m_tracker(tracker)
    , m_enabled(enabled)
    , m_suspended(suspended)
  {}

  ~GpsTrackerStateRestorer()
  {
    m_tracker.SetAppendSuspended(m_suspended);
    m_tracker.SetEnabled(m_enabled);
  }

private:
  GpsTracker & m_tracker;
  bool m_enabled;
  bool m_suspended;
};

bool WaitForGpsTrackBoundaries(GpsTrack const & track, size_t expectedCount,
                               std::chrono::seconds timeout = std::chrono::seconds(5))
{
  auto const deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline)
  {
    if (track.GetSegmentBoundaryIndices().size() >= expectedCount)
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return track.GetSegmentBoundaryIndices().size() >= expectedCount;
}

std::string GetInterruptedTestGpsTrackPath()
{
  return base::JoinPath(GetPlatform().WritableDir(), "sp013_gpstrack_test.bin");
}

location::GpsInfo MakeInterruptedTrackPoint(double timestamp, double lat, double lon)
{
  return street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, timestamp);
}
}  // namespace interrupted_session_tests_detail

UNIT_TEST(InterruptedSession_BreadcrumbPresent_Idle_ConsumeDetects)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  {
    RecordingSession session;
    TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
    TEST(session.HasActiveSessionBreadcrumb(), ());
  }

  RecordingSession restarted;
  TEST_EQUAL(restarted.GetState(), RecordingSession::State::Idle, ());
  TEST(restarted.HasActiveSessionBreadcrumb(), ());
  TEST(restarted.ConsumeActiveSessionBreadcrumb(), ());
  TEST(!restarted.HasActiveSessionBreadcrumb(), ());
  TEST(!restarted.ConsumeActiveSessionBreadcrumb(), ());
}

UNIT_TEST(InterruptedSession_BreadcrumbAbsent_NoDetection)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  RecordingSession session;
  TEST_EQUAL(session.GetState(), RecordingSession::State::Idle, ());
  TEST(!session.HasActiveSessionBreadcrumb(), ());
  TEST(!session.ConsumeActiveSessionBreadcrumb(), ());
}

UNIT_TEST(InterruptedSession_BreadcrumbClearedOnFinish_SubsequentClean)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  RecordingSession session;
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(session.Finish(), RecordingSession::TransitionResult::Ok, ());
  TEST(!session.HasActiveSessionBreadcrumb(), ());

  RecordingSession next;
  TEST(!next.HasActiveSessionBreadcrumb(), ());
  TEST(!next.ConsumeActiveSessionBreadcrumb(), ());
}

UNIT_TEST(InterruptedSession_BreadcrumbClearedOnDiscard_SubsequentClean)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  RecordingSession session;
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(session.Discard(), RecordingSession::TransitionResult::Ok, ());
  TEST(!session.HasActiveSessionBreadcrumb(), ());

  RecordingSession next;
  TEST(!next.HasActiveSessionBreadcrumb(), ());
  TEST(!next.ConsumeActiveSessionBreadcrumb(), ());
}

UNIT_TEST(InterruptedSession_GapThreshold_Boundary)
{
  TEST(!IsRecordingLocationGapAnInterruption(kRecordingInterruptionGapSeconds - 1), ());
  TEST(IsRecordingLocationGapAnInterruption(kRecordingInterruptionGapSeconds), ());
  TEST(IsRecordingLocationGapAnInterruption(kRecordingInterruptionGapSeconds + 1), ());
}

UNIT_TEST(InterruptedSession_AfterEffects_NoInterpolate_DiscCollected)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  interrupted_session_tests_detail::InterruptedSessionFixture fixture;
  auto const g = interrupted_session_tests_detail::MakeInterruptedNorthSegment(100.0);
  TEST_NOT_EQUAL(g.pixelStart, g.pixelMid, ());
  TEST_NOT_EQUAL(g.pixelMid, g.pixelEnd, ());
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      interrupted_session_tests_detail::InterruptedGpsAt(g.startLat, g.startLon, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());

  ApplyRecordingInterruptionEffects(nullptr /* tracker */, &fixture.Manager());
  TEST_EQUAL(fixture.Session().GetState(), RecordingSession::State::Recording, ());

  fixture.Manager().OnLocationUpdate(
      interrupted_session_tests_detail::InterruptedGpsAt(g.endLat, g.endLon, ts + 20.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
}

UNIT_TEST(InterruptedSession_AfterEffects_NextSampleNotRejectedForImpliedSpeed)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  interrupted_session_tests_detail::InterruptedSessionFixture fixture;
  auto const [baseLat, baseLon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(
      interrupted_session_tests_detail::kInterruptedBaseLat, interrupted_session_tests_detail::kInterruptedBaseLon));
  std::int64_t const pixelA = street_pixels_tests::PixelIdForLatLon(baseLat, baseLon);
  fixture.SetupPixels({{pixelA, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(interrupted_session_tests_detail::InterruptedGpsAt(baseLat, baseLon, ts));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::None, ());

  ApplyRecordingInterruptionEffects(nullptr /* tracker */, &fixture.Manager());

  auto const [farLat, farLon] = street_pixels_tests::OffsetLatLonByMeters(baseLat, baseLon, 500.0, 0.0);
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(farLat, farLon, 5.0, ts + 1.0));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::None, ());
}

UNIT_TEST(InterruptedSession_PixelsBeforeInterruptionIntact)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  interrupted_session_tests_detail::InterruptedSessionFixture fixture;
  auto const g = interrupted_session_tests_detail::MakeInterruptedNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      interrupted_session_tests_detail::InterruptedGpsAt(g.startLat, g.startLon, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());

  ApplyRecordingInterruptionEffects(nullptr /* tracker */, &fixture.Manager());
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
}

UNIT_TEST(InterruptedSession_TrackBeforeInterruptionIntact)
{
  std::string const filePath = interrupted_session_tests_detail::GetInterruptedTestGpsTrackPath();
  SCOPE_GUARD(deleter, std::bind(&FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  GpsTrack track(filePath);
  std::mutex mutex;
  std::condition_variable cv;
  bool gotCallback = false;
  track.SetCallback(
      [&](std::vector<std::pair<size_t, location::GpsInfo>> &&, std::pair<size_t, size_t> const &,
          TrackStatistics const &)
      {
        std::lock_guard lg(mutex);
        gotCallback = true;
        cv.notify_all();
      });

  auto waitCallback = [&]()
  {
    std::unique_lock ul(mutex);
    return cv.wait_for(ul, std::chrono::seconds(5), [&]() { return gotCallback; });
  };

  track.AddPoint(interrupted_session_tests_detail::MakeInterruptedTrackPoint(1000.0, 10.0, 20.0));
  track.AddPoint(interrupted_session_tests_detail::MakeInterruptedTrackPoint(1001.0, 10.001, 20.001));
  TEST(waitCallback(), ());

  track.MarkSegmentBoundary();
  TEST(interrupted_session_tests_detail::WaitForGpsTrackBoundaries(track, 1), ());
  TEST(!track.IsAppendSuspended(), ());

  {
    std::lock_guard lg(mutex);
    gotCallback = false;
  }
  track.AddPoint(interrupted_session_tests_detail::MakeInterruptedTrackPoint(1100.0, 11.0, 21.0));
  TEST(waitCallback(), ());

  std::vector<location::GpsInfo> points;
  track.ForEachPoint([&points](location::GpsInfo const & pt, size_t) -> bool
  {
    points.push_back(pt);
    return true;
  });
  TEST_EQUAL(points.size(), 3, ());
  auto const boundaries = track.GetSegmentBoundaryIndices();
  TEST_EQUAL(boundaries.size(), 1, ());
  TEST_EQUAL(boundaries[0], 2, ());
}

UNIT_TEST(InterruptedSession_PauseIsNotInterruption)
{
  interrupted_session_tests_detail::InterruptedSessionBreadcrumbCleanup cleanup;
  interrupted_session_tests_detail::InterruptedSessionFixture fixture;

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().GetState(), RecordingSession::State::Recording, ());

  ApplyRecordingInterruptionEffects(nullptr /* tracker */, &fixture.Manager());
  TEST_EQUAL(fixture.Session().GetState(), RecordingSession::State::Recording, ());

  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().GetState(), RecordingSession::State::Paused, ());

  auto & tracker = GpsTracker::Instance();
  interrupted_session_tests_detail::GpsTrackerStateRestorer restorer(tracker, tracker.IsEnabled(),
                                                                     tracker.IsAppendSuspended());

  tracker.SetEnabled(true);
  tracker.SetAppendSuspended(false);

  ApplyRecordingInterruptionEffects(&tracker, nullptr /* manager */);
  TEST(!tracker.IsAppendSuspended(), ());

  ApplyRecordingPauseResumeEffects(RecordingSession::State::Recording, RecordingSession::State::Paused, &tracker,
                                   nullptr /* manager */);
  TEST(tracker.IsAppendSuspended(), ());

  ApplyRecordingPauseResumeEffects(RecordingSession::State::Paused, RecordingSession::State::Recording, &tracker,
                                   nullptr /* manager */);
  TEST(!tracker.IsAppendSuspended(), ());
}
