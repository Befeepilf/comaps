#include "testing/testing.hpp"

#include "map/live_sample_acceptance_filter.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

namespace
{
class SampleAcceptanceManagerBreadcrumbCleanup
{
public:
  SampleAcceptanceManagerBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~SampleAcceptanceManagerBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class SampleAcceptanceManagerFixture
{
public:
  static std::int64_t constexpr kPixelA = 1000;
  static std::int64_t constexpr kPixelB = 500000000;

  SampleAcceptanceManagerFixture()
    : m_manager(m_dataSource)
  {
    m_manager.SetRecordingSession(&m_session);
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
}  // namespace

UNIT_TEST(SampleAcceptanceManager_PoorAccuracy_CollectsNothing)
{
  SampleAcceptanceManagerBreadcrumbCleanup cleanup;
  SampleAcceptanceManagerFixture fixture;
  fixture.SetupPixels({{SampleAcceptanceManagerFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(SampleAcceptanceManagerFixture::kPixelA, 26.0, ts));
  TEST(!fixture.Manager().IsPixelExploredForTesting(SampleAcceptanceManagerFixture::kPixelA), ());
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::Accuracy, ());
}

UNIT_TEST(SampleAcceptanceManager_GoodAccuracy_Collects)
{
  SampleAcceptanceManagerBreadcrumbCleanup cleanup;
  SampleAcceptanceManagerFixture fixture;
  fixture.SetupPixels({{SampleAcceptanceManagerFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(SampleAcceptanceManagerFixture::kPixelA, 5.0, ts));
  TEST(fixture.Manager().IsPixelExploredForTesting(SampleAcceptanceManagerFixture::kPixelA), ());
}

UNIT_TEST(SampleAcceptanceManager_Rejected_NoVibration)
{
  SampleAcceptanceManagerBreadcrumbCleanup cleanup;
  SampleAcceptanceManagerFixture fixture;
  fixture.SetupPixels({{SampleAcceptanceManagerFixture::kPixelA, false}});
  size_t vibrationCalls = 0;
  fixture.Manager().SetVibrationHandler([&](size_t) { ++vibrationCalls; });
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(SampleAcceptanceManagerFixture::kPixelA, 26.0, ts));
  TEST_EQUAL(vibrationCalls, 0, ());
}

UNIT_TEST(SampleAcceptanceManager_RejectThenAccept_ComparesToLastAccepted)
{
  SampleAcceptanceManagerBreadcrumbCleanup cleanup;
  SampleAcceptanceManagerFixture fixture;
  auto const [baseLat, baseLon] = street_pixels_tests::LatLonForPixelId(SampleAcceptanceManagerFixture::kPixelA);
  fixture.SetupPixels({{SampleAcceptanceManagerFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(SampleAcceptanceManagerFixture::kPixelA, 5.0, ts));

  double const dtSec = 8.0;
  double const fastDistanceM = 55.0 * 1000.0 / 3600.0 * dtSec;
  auto const [fastLat, fastLon] = street_pixels_tests::OffsetLatLonByMeters(baseLat, baseLon, fastDistanceM, 0.0);
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(fastLat, fastLon, 5.0, ts + dtSec));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::ImpliedSpeed, ());

  double const walkDistanceM = 5.0 * 1000.0 / 3600.0 * dtSec;
  auto const [walkLat, walkLon] = street_pixels_tests::OffsetLatLonByMeters(baseLat, baseLon, walkDistanceM, 0.0);
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(walkLat, walkLon, 5.0, ts + dtSec));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::None, ());
}

UNIT_TEST(SampleAcceptanceManager_ResetReference_ClearsSpeedBarrier)
{
  SampleAcceptanceManagerBreadcrumbCleanup cleanup;
  SampleAcceptanceManagerFixture fixture;
  auto const [baseLat, baseLon] = street_pixels_tests::LatLonForPixelId(SampleAcceptanceManagerFixture::kPixelA);
  fixture.SetupPixels({{SampleAcceptanceManagerFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(SampleAcceptanceManagerFixture::kPixelA, 5.0, ts));
  fixture.Manager().ResetSampleAcceptanceReference();

  auto const [farLat, farLon] = street_pixels_tests::OffsetLatLonByMeters(baseLat, baseLon, 500.0, 0.0);
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(farLat, farLon, 5.0, ts + 1.0));
  TEST_EQUAL(fixture.Manager().GetLastSampleRejectReason(), SampleRejectReason::None, ());
}
