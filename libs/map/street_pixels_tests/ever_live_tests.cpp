#include "testing/testing.hpp"

#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

#include <cstdint>
#include <initializer_list>
#include <set>

namespace
{
class EverLiveBreadcrumbCleanup
{
public:
  EverLiveBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~EverLiveBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class EverLiveFixture
{
public:
  static std::int64_t constexpr kPixelA = 1000;
  static std::int64_t constexpr kPixelB = 500000000;

  EverLiveFixture()
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

UNIT_TEST(EverLive_FirstLiveSetsEverLive)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(EverLiveFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));

  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
}

UNIT_TEST(EverLive_FirstImportedClearThenLiveSets)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}});

  fixture.Manager().MarkImportedPixelsForTesting({EverLiveFixture::kPixelA});
  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(EverLiveFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));

  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
}

UNIT_TEST(EverLive_LiveThenImportedRemainsSet)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(EverLiveFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());

  fixture.Manager().MarkImportedPixelsForTesting({EverLiveFixture::kPixelA});
  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
}

UNIT_TEST(EverLive_TrackAloneLeavesClear)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}, {EverLiveFixture::kPixelB, false}});

  fixture.Manager().MarkTrackPixelsForTesting({EverLiveFixture::kPixelA});
  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelB), ());
}

UNIT_TEST(EverLive_TrackAfterLiveRemainsSet)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(EverLiveFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());

  fixture.Manager().MarkTrackPixelsForTesting({EverLiveFixture::kPixelA});
  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
}

UNIT_TEST(EverLive_LiveNeverClears)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(EverLiveFixture::kPixelA, ts));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(EverLiveFixture::kPixelA, ts + 1.0));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());

  fixture.Manager().MarkImportedPixelsForTesting({EverLiveFixture::kPixelA});
  fixture.Manager().MarkTrackPixelsForTesting({EverLiveFixture::kPixelA});
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
}

UNIT_TEST(EverLive_UpgradeDoesNotDoubleCount)
{
  EverLiveBreadcrumbCleanup cleanup;
  EverLiveFixture fixture;
  fixture.SetupPixels({{EverLiveFixture::kPixelA, false}});

  fixture.Manager().MarkImportedPixelsForTesting({EverLiveFixture::kPixelA});
  TEST_EQUAL(fixture.Manager().GetTotalExploredFraction(), 1.0, ());

  size_t vibrationNewlyExplored = 0;
  fixture.Manager().SetVibrationHandler(
      [&vibrationNewlyExplored](size_t newlyExplored) { vibrationNewlyExplored += newlyExplored; });

  uint32_t listenerNewPixels = 0;
  fixture.Manager().SetExplorationListener(
      [&listenerNewPixels](StreetPixelsManager::ExplorationDelta const & delta)
      { listenerNewPixels += delta.m_newPixels; });

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(EverLiveFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));

  TEST(fixture.Manager().IsPixelExploredForTesting(EverLiveFixture::kPixelA), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(EverLiveFixture::kPixelA), ());
  TEST_EQUAL(fixture.Manager().GetTotalExploredFraction(), 1.0, ());
  TEST_EQUAL(vibrationNewlyExplored, 0, ());
  TEST_EQUAL(listenerNewPixels, 0, ());
}

UNIT_TEST(EverLive_GetPixelIdMaskUnaffectedByFlags)
{
  auto const pixel = street_pixels_tests::MakeStreetPixel(EverLiveFixture::kPixelA, true, true);
  TEST_EQUAL(pixel.GetPixelId(), EverLiveFixture::kPixelA, ());
  TEST_EQUAL(pixel.GetPixelId() & static_cast<std::int64_t>(0xC000000000000000ULL), 0, ());
}
