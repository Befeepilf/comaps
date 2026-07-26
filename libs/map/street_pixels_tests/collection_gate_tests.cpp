#include "testing/testing.hpp"

#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

#include <initializer_list>

namespace
{
class CollectionGateBreadcrumbCleanup
{
public:
  CollectionGateBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~CollectionGateBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class CollectionGateFixture
{
public:
  static std::int64_t constexpr kPixelA = 1000;
  static std::int64_t constexpr kPixelB = 500000000;
  static std::int64_t constexpr kPixelC = 1000000000;

  CollectionGateFixture()
    : m_manager(m_dataSource)
  {
    m_manager.SetRecordingSession(&m_session);
  }

  void SetupPixels(std::initializer_list<std::pair<std::int64_t, bool>> idsAndExplored)
  {
    m_manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet(idsAndExplored));
  }

  location::GpsInfo GpsAtPixel(std::int64_t pixelId, double timestampSec) const
  {
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(pixelId);
    return street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, timestampSec);
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};
}  // namespace

UNIT_TEST(CollectionGate_Idle_CollectsNothing)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());
}

UNIT_TEST(CollectionGate_Paused_CollectsNothing)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());
}

UNIT_TEST(CollectionGate_Recording_CollectsExpected)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());
}

UNIT_TEST(CollectionGate_StartMidSequence_CollectsFromStartOnward)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;

  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());

  fixture.SetupPixels({{CollectionGateFixture::kPixelB, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelB, 2.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelB), ());
}

UNIT_TEST(CollectionGate_PauseMidSequence_CollectsUntilPause)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());

  fixture.SetupPixels({{CollectionGateFixture::kPixelB, false}});
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelB, 2.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelB), ());

  fixture.SetupPixels({{CollectionGateFixture::kPixelC, false}});
  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelC, 3.0));
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelC), ());
}

UNIT_TEST(CollectionGate_TrackReplay_MarksRegardlessOfSession)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().MarkTrackPixelsForTesting({CollectionGateFixture::kPixelA});
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());
}

UNIT_TEST(CollectionGate_Rejected_NoVibration)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  size_t vibrationCalls = 0;
  fixture.Manager().SetVibrationHandler([&](size_t) { ++vibrationCalls; });

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST_EQUAL(vibrationCalls, 0, ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.SetupPixels({{CollectionGateFixture::kPixelB, false}});
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelB, 2.0));
  TEST_EQUAL(vibrationCalls, 0, ());
}

UNIT_TEST(CollectionGate_Finished_CollectsNothing)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Finish(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());
}

UNIT_TEST(CollectionGate_Discarded_CollectsNothing)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Discard(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionGateFixture::kPixelA), ());
}

UNIT_TEST(CollectionGate_Recording_TriggersVibration)
{
  CollectionGateBreadcrumbCleanup cleanup;
  CollectionGateFixture fixture;
  fixture.SetupPixels({{CollectionGateFixture::kPixelA, false}});
  size_t vibrationCalls = 0;
  fixture.Manager().SetVibrationHandler([&](size_t) { ++vibrationCalls; });

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(CollectionGateFixture::kPixelA, 1.0));
  TEST_EQUAL(vibrationCalls, 1, ());
}
