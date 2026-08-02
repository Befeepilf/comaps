#include "testing/testing.hpp"

#include "map/gps_track.hpp"
#include "map/live_sample_acceptance_filter.hpp"
#include "map/recording_pause_resume.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"
#include "map/track_recording_geometry.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
class PauseResumeBreadcrumbCleanup
{
public:
  PauseResumeBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~PauseResumeBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class PauseResumeFixture
{
public:
  static std::int64_t constexpr kPixelA = 1000;
  static std::int64_t constexpr kPixelB = 500000000;
  static std::int64_t constexpr kPixelC = 1000000000;
  static std::int64_t constexpr kPixelD = 1500000000;

  PauseResumeFixture()
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

  location::GpsInfo GpsAtPixel(std::int64_t pixelId, double accuracyM, double timestampSec) const
  {
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(pixelId);
    return street_pixels_tests::MakeGpsInfo(lat, lon, accuracyM, timestampSec);
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};

std::string GetTestGpsTrackPath()
{
  return base::JoinPath(GetPlatform().WritableDir(), "sp010_gpstrack_test.bin");
}

location::GpsInfo MakeTrackPoint(double timestamp, double lat, double lon)
{
  return street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, timestamp);
}

class GpsTrackWaiter
{
public:
  void OnUpdate(std::vector<std::pair<size_t, location::GpsInfo>> &&, std::pair<size_t, size_t> const &,
                TrackStatistics const &)
  {
    std::lock_guard lg(m_mutex);
    m_gotCallback = true;
    m_cv.notify_one();
  }

  void Reset()
  {
    std::lock_guard lg(m_mutex);
    m_gotCallback = false;
  }

  bool Wait(std::chrono::seconds timeout = std::chrono::seconds(5))
  {
    std::unique_lock ul(m_mutex);
    return m_cv.wait_for(ul, timeout, [this]() { return m_gotCallback; });
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  bool m_gotCallback = false;
};

void AttachGpsTrackCallback(GpsTrack & track, GpsTrackWaiter & waiter)
{
  track.SetCallback(
      [&waiter](std::vector<std::pair<size_t, location::GpsInfo>> && toAdd, std::pair<size_t, size_t> const & toRemove,
                TrackStatistics const & stats) { waiter.OnUpdate(std::move(toAdd), toRemove, stats); });
}

bool WaitForSegmentBoundaries(GpsTrack const & track, size_t expectedCount,
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
}  // namespace

UNIT_TEST(PauseResume_Paused_CollectsZeroPixels)
{
  PauseResumeBreadcrumbCleanup cleanup;
  PauseResumeFixture fixture;
  fixture.SetupPixels({{PauseResumeFixture::kPixelA, false}, {PauseResumeFixture::kPixelB, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(PauseResumeFixture::kPixelA, 5.0, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(PauseResumeFixture::kPixelA), ());

  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(PauseResumeFixture::kPixelB, 5.0, ts + 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(PauseResumeFixture::kPixelB), ());

  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());
}

UNIT_TEST(PauseResume_ImpossibleSpeedAcrossPause_FirstSampleAccepted)
{
  PauseResumeBreadcrumbCleanup cleanup;
  PauseResumeFixture fixture;
  auto const [baseLat, baseLon] = street_pixels_tests::LatLonForPixelId(PauseResumeFixture::kPixelA);
  fixture.SetupPixels({{PauseResumeFixture::kPixelA, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(PauseResumeFixture::kPixelA, 5.0, ts));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::None, ());

  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());

  auto const [farLat, farLon] = street_pixels_tests::OffsetLatLonByMeters(baseLat, baseLon, 500.0, 0.0);
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(farLat, farLon, 5.0, ts + 1.0));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::None, ());
}

UNIT_TEST(PauseResume_ThreeCycles_CollectionMatchesIntervals)
{
  PauseResumeBreadcrumbCleanup cleanup;
  PauseResumeFixture fixture;
  fixture.SetupPixels({{PauseResumeFixture::kPixelA, false},
                       {PauseResumeFixture::kPixelB, false},
                       {PauseResumeFixture::kPixelC, false},
                       {PauseResumeFixture::kPixelD, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(PauseResumeFixture::kPixelA, 5.0, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(PauseResumeFixture::kPixelA), ());

  for (int cycle = 0; cycle < 3; ++cycle)
  {
    TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
    fixture.SetupPixels({{PauseResumeFixture::kPixelB, false}});
    fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(PauseResumeFixture::kPixelB, 5.0, ts + 10.0 + cycle));
    TEST(!fixture.Manager().IsPixelExploredForTesting(PauseResumeFixture::kPixelB), ());

    TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());
    std::int64_t const resumePixel =
        (cycle == 0) ? PauseResumeFixture::kPixelC
                     : ((cycle == 1) ? PauseResumeFixture::kPixelD : PauseResumeFixture::kPixelA);
    fixture.SetupPixels({{resumePixel, false}});
    fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(resumePixel, 5.0, ts + 20.0 + cycle));
    TEST(fixture.Manager().IsPixelExploredForTesting(resumePixel), ());
  }
}

UNIT_TEST(PauseResume_TrackBoundary_SaveProducesSeparateLines)
{
  std::string const filePath = GetTestGpsTrackPath();
  SCOPE_GUARD(deleter, std::bind(&FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  GpsTrack track(filePath);
  GpsTrackWaiter waiter;
  AttachGpsTrackCallback(track, waiter);

  track.AddPoint(MakeTrackPoint(1000.0, 10.0, 20.0));
  track.AddPoint(MakeTrackPoint(1001.0, 10.001, 20.001));
  TEST(waiter.Wait(), ());

  track.SetAppendSuspended(true);
  track.MarkSegmentBoundary();
  TEST(WaitForSegmentBoundaries(track, 1), ());

  track.AddPoint(MakeTrackPoint(1100.0, 11.0, 21.0));
  TEST(track.IsAppendSuspended(), ());
  auto boundariesWhileSuspended = track.GetSegmentBoundaryIndices();
  TEST_EQUAL(boundariesWhileSuspended.size(), 1, ());

  track.SetAppendSuspended(false);
  waiter.Reset();
  track.AddPoint(MakeTrackPoint(1101.0, 11.001, 21.001));
  track.AddPoint(MakeTrackPoint(1102.0, 11.002, 21.002));
  TEST(waiter.Wait(), ());

  auto const boundaries = track.GetSegmentBoundaryIndices();
  TEST_EQUAL(boundaries.size(), 1, ());
  TEST_EQUAL(boundaries[0], 2, ());

  std::vector<location::GpsInfo> points;
  track.ForEachPoint([&points](location::GpsInfo const & pt, size_t) -> bool
  {
    points.push_back(pt);
    return true;
  });
  TEST_EQUAL(points.size(), 4, ());

  auto const geometry = MakeTrackRecordingGeometry(points, boundaries);
  TEST_EQUAL(geometry.m_lines.size(), 2, ());
  TEST_EQUAL(geometry.m_lines[0].size(), 2, ());
  TEST_EQUAL(geometry.m_lines[1].size(), 2, ());
  TEST_EQUAL(geometry.m_timestamps.size(), 2, ());
}

UNIT_TEST(PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly)
{
  std::string const filePath = GetTestGpsTrackPath();
  SCOPE_GUARD(deleter, std::bind(&FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  GpsTrack track(filePath);
  GpsTrackWaiter waiter;
  AttachGpsTrackCallback(track, waiter);

  track.AddPoint(MakeTrackPoint(1000.0, 10.0, 20.0));
  track.AddPoint(MakeTrackPoint(1001.0, 10.001, 20.001));
  TEST(waiter.Wait(), ());

  track.SetAppendSuspended(true);
  track.MarkSegmentBoundary();

  track.SetAppendSuspended(false);
  waiter.Reset();
  track.AddPoint(MakeTrackPoint(1101.0, 11.001, 21.001));
  track.AddPoint(MakeTrackPoint(1102.0, 11.002, 21.002));
  TEST(waiter.Wait(), ());
  TEST(WaitForSegmentBoundaries(track, 1), ());

  auto const boundaries = track.GetSegmentBoundaryIndices();
  TEST_EQUAL(boundaries.size(), 1, ());
  TEST_EQUAL(boundaries[0], 2, ());

  std::vector<location::GpsInfo> points;
  track.ForEachPoint([&points](location::GpsInfo const & pt, size_t) -> bool
  {
    points.push_back(pt);
    return true;
  });
  TEST_EQUAL(points.size(), 4, ());

  auto const geometry = MakeTrackRecordingGeometry(points, boundaries);
  TEST_EQUAL(geometry.m_lines.size(), 2, ());
  TEST_EQUAL(geometry.m_lines[0].size(), 2, ());
  TEST_EQUAL(geometry.m_lines[1].size(), 2, ());
}

UNIT_TEST(PauseResume_FinishFromPaused_TrackDataValid)
{
  std::string const filePath = GetTestGpsTrackPath();
  SCOPE_GUARD(deleter, std::bind(&FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  PauseResumeBreadcrumbCleanup cleanup;
  RecordingSession session;
  GpsTrack track(filePath);
  GpsTrackWaiter waiter;
  AttachGpsTrackCallback(track, waiter);

  session.SetStateListener([&track](RecordingSession::State previous, RecordingSession::State current)
  {
    using State = RecordingSession::State;
    if (previous == State::Recording && current == State::Paused)
    {
      track.SetAppendSuspended(true);
      track.MarkSegmentBoundary();
    }
    else if (previous == State::Paused && current == State::Recording)
    {
      track.SetAppendSuspended(false);
    }
    else if (previous == State::Paused &&
             (current == State::Finished || current == State::Discarded))
    {
      track.SetAppendSuspended(false);
    }
  });

  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  track.AddPoint(MakeTrackPoint(2000.0, 30.0, 40.0));
  track.AddPoint(MakeTrackPoint(2001.0, 30.001, 40.001));
  TEST(waiter.Wait(), ());

  TEST_EQUAL(session.Pause(), RecordingSession::TransitionResult::Ok, ());
  TEST(WaitForSegmentBoundaries(track, 1), ());
  track.AddPoint(MakeTrackPoint(2500.0, 35.0, 45.0));
  TEST(track.IsAppendSuspended(), ());

  TEST_EQUAL(session.Finish(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(session.GetState(), RecordingSession::State::Finished, ());
  TEST(!track.IsAppendSuspended(), ());

  std::vector<location::GpsInfo> points;
  track.ForEachPoint([&points](location::GpsInfo const & pt, size_t) -> bool
  {
    points.push_back(pt);
    return true;
  });
  TEST_EQUAL(points.size(), 2, ());

  auto const boundaries = track.GetSegmentBoundaryIndices();
  TEST_EQUAL(boundaries.size(), 1, ());
  auto const geometry = MakeTrackRecordingGeometry(points, boundaries);
  TEST_EQUAL(geometry.m_lines.size(), 1, ());
  TEST_EQUAL(geometry.m_lines[0].size(), 2, ());
  TEST(!geometry.m_lines[0].empty(), ());
}

UNIT_TEST(PauseResume_MultipleBoundaries_SaveSplitsAll)
{
  std::vector<location::GpsInfo> points = {
      MakeTrackPoint(1.0, 1.0, 1.0), MakeTrackPoint(2.0, 1.1, 1.1), MakeTrackPoint(3.0, 1.2, 1.2),
      MakeTrackPoint(4.0, 2.0, 2.0), MakeTrackPoint(5.0, 2.1, 2.1), MakeTrackPoint(6.0, 3.0, 3.0),
  };
  std::vector<size_t> const boundaries = {2, 5};
  auto const geometry = MakeTrackRecordingGeometry(points, boundaries);
  TEST_EQUAL(geometry.m_lines.size(), 3, ());
  TEST_EQUAL(geometry.m_lines[0].size(), 2, ());
  TEST_EQUAL(geometry.m_lines[1].size(), 3, ());
  TEST_EQUAL(geometry.m_lines[2].size(), 1, ());
}
