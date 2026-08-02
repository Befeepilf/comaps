#include "testing/testing.hpp"

#include "map/live_sample_acceptance_filter.hpp"
#include "map/live_segment_interpolation.hpp"
#include "map/recording_pause_resume.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

namespace
{
double constexpr kInterpBaseLat = 48.2;
double constexpr kInterpBaseLon = 16.37;

class SegmentInterpolationBreadcrumbCleanup
{
public:
  SegmentInterpolationBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~SegmentInterpolationBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class SegmentInterpolationFixture
{
public:
  SegmentInterpolationFixture()
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

struct SegmentGeometry
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

SegmentGeometry MakeNorthSegment(double lengthMeters)
{
  SegmentGeometry g;
  g.startLat = kInterpBaseLat;
  g.startLon = kInterpBaseLon;
  auto const mid = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, lengthMeters * 0.5, 0.0);
  g.midLat = mid.first;
  g.midLon = mid.second;
  auto const end = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, lengthMeters, 0.0);
  g.endLat = end.first;
  g.endLon = end.second;
  g.pixelStart = street_pixels_tests::PixelIdForLatLon(g.startLat, g.startLon);
  g.pixelMid = street_pixels_tests::PixelIdForLatLon(g.midLat, g.midLon);
  g.pixelEnd = street_pixels_tests::PixelIdForLatLon(g.endLat, g.endLon);
  return g;
}

location::GpsInfo GpsAt(double lat, double lon, double timestampSec)
{
  return street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, timestampSec);
}

size_t CountInterpolationSamples(location::GpsInfo const & from, location::GpsInfo const & to)
{
  size_t count = 0;
  ForEachInterpolationSample(from, to, [&count](double, double) { ++count; });
  return count;
}
}  // namespace

UNIT_TEST(SegmentInterpolation_Barrier_AfterPause_NoMidpoint)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  TEST_NOT_EQUAL(g.pixelStart, g.pixelMid, ());
  TEST_NOT_EQUAL(g.pixelMid, g.pixelEnd, ());
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());

  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
}

UNIT_TEST(SegmentInterpolation_Barrier_AfterResume_NoMidpoint)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));

  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.midLat, g.midLon, ts + 5.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());

  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
}

UNIT_TEST(SegmentInterpolation_Barrier_AfterInterruption_NoMidpoint)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());

  fixture.Manager().MarkInterpolationBarrier();

  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
}

UNIT_TEST(SegmentInterpolation_Barrier_AfterRejection_NoMidpoint)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));

  auto const [rejectLat, rejectLon] =
      street_pixels_tests::OffsetLatLonByMeters(g.startLat, g.startLon, 80.0, 0.0);
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(rejectLat, rejectLon, 26.0, ts + 10.0));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::Accuracy, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());

  auto const [farLat, farLon] =
      street_pixels_tests::OffsetLatLonByMeters(g.startLat, g.startLon, 250.0, 0.0);
  fixture.Manager().OnLocationUpdate(GpsAt(farLat, farLon, ts + 15.0));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::Teleport, ());

  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
}

UNIT_TEST(SegmentInterpolation_Barrier_SessionStart_NoMidpoint)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());

  TEST_EQUAL(fixture.Session().Finish(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Reset(), RecordingSession::TransitionResult::Ok, ());
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_TimeGap_JustUnder)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 100.0, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + 29.0);
  TEST(MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_TimeGap_ExactMax)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 100.0, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + kMaxInterpolationGapSeconds);
  TEST(MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_TimeGap_JustOver)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 100.0, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + 31.0);
  TEST(!MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_Distance_JustUnder)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 199.0, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + 20.0);
  TEST(MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_Distance_JustOver)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 201.0, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + 20.0);
  TEST(!MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_Speed_JustUnder)
{
  double const distanceM = 100.0;
  double const dtSec = distanceM / (49.0 / 3.6);
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, distanceM, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + dtSec);
  TEST(MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_MayInterpolate_Speed_JustOver)
{
  double const distanceM = 100.0;
  double const dtSec = distanceM / (51.0 / 3.6);
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, distanceM, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + dtSec);
  TEST(!MayInterpolateSegment(from, to), ());
}

UNIT_TEST(SegmentInterpolation_WithinCaps_CollectsMidpoint)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));
  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));

  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
}

UNIT_TEST(SegmentInterpolation_TimeGapOver_EndpointOnly)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  auto const g = MakeNorthSegment(100.0);
  fixture.SetupPixels({{g.pixelStart, false}, {g.pixelMid, false}, {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));
  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 40.0));

  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(g.pixelMid), ());
}

UNIT_TEST(SegmentInterpolation_LongGap_BoundedByTwoDiscs)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;
  double constexpr kGapMeters = 10000.0;
  auto const g = MakeNorthSegment(kGapMeters);
  auto const nearStart =
      street_pixels_tests::OffsetLatLonByMeters(g.startLat, g.startLon, 10.0, 0.0);
  auto const nearEnd = street_pixels_tests::OffsetLatLonByMeters(g.endLat, g.endLon, -10.0, 0.0);
  std::int64_t const pixelNearStart = street_pixels_tests::PixelIdForLatLon(nearStart.first, nearStart.second);
  std::int64_t const pixelNearEnd = street_pixels_tests::PixelIdForLatLon(nearEnd.first, nearEnd.second);
  std::int64_t const pixelFarMid = g.pixelMid;

  fixture.SetupPixels({{g.pixelStart, false},
                       {pixelNearStart, false},
                       {pixelFarMid, false},
                       {pixelNearEnd, false},
                       {g.pixelEnd, false}});

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(GpsAt(g.startLat, g.startLon, ts));
  fixture.Manager().MarkInterpolationBarrier();
  fixture.Manager().ResetSampleAcceptanceReference();
  fixture.Manager().OnLocationUpdate(GpsAt(g.endLat, g.endLon, ts + 20.0));

  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelStart), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelNearStart), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(g.pixelEnd), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelNearEnd), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelFarMid), ());
}

UNIT_TEST(SegmentInterpolation_CyclingSequence_ContinuousCoverage)
{
  SegmentInterpolationBreadcrumbCleanup cleanup;
  SegmentInterpolationFixture fixture;

  double constexpr kStepMeters = 40.0;
  double constexpr kDtSec = 8.0;
  size_t constexpr kPoints = 6;
  std::vector<std::pair<double, double>> points;
  std::vector<std::int64_t> midPixels;
  points.reserve(kPoints);
  for (size_t i = 0; i < kPoints; ++i)
  {
    auto const p = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, kStepMeters * static_cast<double>(i), 0.0);
    points.push_back(p);
    if (i + 1 < kPoints)
    {
      auto const mid = street_pixels_tests::OffsetLatLonByMeters(
          kInterpBaseLat, kInterpBaseLon, kStepMeters * (static_cast<double>(i) + 0.5), 0.0);
      midPixels.push_back(street_pixels_tests::PixelIdForLatLon(mid.first, mid.second));
    }
  }

  std::vector<df::StreetPixel> pixelSet;
  pixelSet.reserve(points.size() + midPixels.size());
  for (auto const & p : points)
    pixelSet.push_back(street_pixels_tests::MakeStreetPixel(street_pixels_tests::PixelIdForLatLon(p.first, p.second), false));
  for (auto id : midPixels)
    pixelSet.push_back(street_pixels_tests::MakeStreetPixel(id, false));
  fixture.Manager().SetStreetPixelsForTesting(std::move(pixelSet));

  double const ts = street_pixels_tests::CurrentTimestampSec();
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  for (size_t i = 0; i < points.size(); ++i)
    fixture.Manager().OnLocationUpdate(GpsAt(points[i].first, points[i].second, ts + kDtSec * static_cast<double>(i)));

  for (auto id : midPixels)
    TEST(fixture.Manager().IsPixelExploredForTesting(id), ());
}

UNIT_TEST(SegmentInterpolation_StepSize_MatchesTenMeters)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 100.0, 0.0);
  auto const to = GpsAt(lat, lon, 1020.0);
  double const distMeters = ms::DistanceOnEarth(from.m_latitude, from.m_longitude, to.m_latitude, to.m_longitude);
  size_t const expectedSegments =
      std::max<size_t>(1, static_cast<size_t>(std::ceil(distMeters / kInterpolationStepMeters)));
  TEST_EQUAL(CountInterpolationSamples(from, to), expectedSegments + 1, ());
  TEST_ALMOST_EQUAL_ABS(kInterpolationStepMeters, 10.0, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(kMaxInterpolationGapSeconds, 30.0, 1e-12, ());
}

UNIT_TEST(SegmentInterpolation_PerUpdateCost_MaxGapSegment)
{
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 199.0, 0.0);
  auto const to = GpsAt(lat, lon, 1000.0 + 20.0);
  TEST(MayInterpolateSegment(from, to), ());

  double const distMeters = ms::DistanceOnEarth(from.m_latitude, from.m_longitude, to.m_latitude, to.m_longitude);
  size_t const expectedSegments =
      std::max<size_t>(1, static_cast<size_t>(std::ceil(distMeters / kInterpolationStepMeters)));
  size_t const expectedSamplesPerCall = expectedSegments + 1;

  auto const start = std::chrono::steady_clock::now();
  size_t samples = 0;
  for (int i = 0; i < 1000; ++i)
  {
    ForEachInterpolationSample(from, to, [&samples](double, double) { ++samples; });
  }
  auto const elapsed = std::chrono::steady_clock::now() - start;
  double const msPerCall =
      std::chrono::duration<double, std::milli>(elapsed).count() / 1000.0;
  TEST_EQUAL(samples, 1000 * expectedSamplesPerCall, ());
  TEST_LESS(msPerCall, 1.0, ("~200 m / 10 m sampling should be well under 1 ms per update"));
}

UNIT_TEST(SegmentInterpolation_OriginClearedByBarrier_NotByFilterReference)
{
  LiveSegmentInterpolation interp;
  auto const from = GpsAt(kInterpBaseLat, kInterpBaseLon, 1000.0);
  interp.SetInterpolationOrigin(from);
  TEST(interp.HasInterpolationOrigin(), ());

  auto const [lat, lon] = street_pixels_tests::OffsetLatLonByMeters(kInterpBaseLat, kInterpBaseLon, 50.0, 0.0);
  auto const to = GpsAt(lat, lon, 1010.0);
  TEST(interp.CanInterpolateTo(to), ());

  interp.MarkInterpolationBarrier();
  TEST(!interp.HasInterpolationOrigin(), ());
  TEST(!interp.CanInterpolateTo(to), ());
}
