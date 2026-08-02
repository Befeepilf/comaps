#include "testing/testing.hpp"

#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/settings.hpp"

#include <cmath>
#include <initializer_list>

namespace
{
class CollectionRadiusBreadcrumbCleanup
{
public:
  CollectionRadiusBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~CollectionRadiusBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class CollectionRadiusFixture
{
public:
  static std::int64_t constexpr kPixel = 1000;

  CollectionRadiusFixture()
    : m_manager(m_dataSource)
  {
    m_manager.SetRecordingSession(&m_session);
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(kPixel);
    m_pixelLat = lat;
    m_pixelLon = lon;
  }

  void SetupPixels(std::initializer_list<std::pair<std::int64_t, bool>> idsAndExplored)
  {
    m_manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet(idsAndExplored));
  }

  location::GpsInfo GpsOffsetNorth(double northMeters, double timestampSec) const
  {
    auto const [lat, lon] =
        street_pixels_tests::OffsetLatLonByMeters(m_pixelLat, m_pixelLon, northMeters, 0.0);
    return street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, timestampSec);
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
  double m_pixelLat = 0.0;
  double m_pixelLon = 0.0;
};

void CollectAtNorthOffset(CollectionRadiusFixture & fixture, double northMeters)
{
  fixture.SetupPixels({{CollectionRadiusFixture::kPixel, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsOffsetNorth(northMeters, street_pixels_tests::CurrentTimestampSec()));
}
}  // namespace

UNIT_TEST(CollectionRadius_24m_Collected)
{
  CollectionRadiusBreadcrumbCleanup cleanup;
  CollectionRadiusFixture fixture;
  CollectAtNorthOffset(fixture, 24.0);
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionRadiusFixture::kPixel), ());
}

UNIT_TEST(CollectionRadius_26m_NotCollected)
{
  CollectionRadiusBreadcrumbCleanup cleanup;
  CollectionRadiusFixture fixture;
  CollectAtNorthOffset(fixture, 26.0);
  TEST(!fixture.Manager().IsPixelExploredForTesting(CollectionRadiusFixture::kPixel), ());
}

UNIT_TEST(CollectionRadius_22m_Collected)
{
  CollectionRadiusBreadcrumbCleanup cleanup;
  CollectionRadiusFixture fixture;
  CollectAtNorthOffset(fixture, 22.0);
  TEST(fixture.Manager().IsPixelExploredForTesting(CollectionRadiusFixture::kPixel), ());
}

UNIT_TEST(CollectionRadius_RadiansDerived)
{
  double constexpr kExpectedRadiusRads = 25.0 / 6371000.0;
  TEST_ALMOST_EQUAL_ABS(street_pixels_tests::kExploreRadiusRads, kExpectedRadiusRads, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(street_pixels_tests::kExploreRadiusMeters, 25.0, 1e-12, ());
}
