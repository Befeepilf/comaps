#include "testing/testing.hpp"

#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"
#include "map/street_stats_db.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "indexer/data_source.hpp"

#include "kml/types.hpp"

#include "platform/settings.hpp"

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

namespace
{
class HistoricalImportBreadcrumbCleanup
{
public:
  HistoricalImportBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~HistoricalImportBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class HistoricalImportFixture
{
public:
  HistoricalImportFixture()
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

kml::MultiGeometry::LineT ShortLineAt(double lat, double lon)
{
  auto const [lat2, lon2] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 0.0, 10.0);
  return {geometry::PointWithAltitude(mercator::FromLatLon(lat, lon)),
          geometry::PointWithAltitude(mercator::FromLatLon(lat2, lon2))};
}

geometry::PointWithAltitude PointAt(double lat, double lon)
{
  return geometry::PointWithAltitude(mercator::FromLatLon(lat, lon));
}
}  // namespace

UNIT_TEST(HistoricalImport_FirstImportEverLiveClear)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.SetupPixels({{pixelA, false}});

  fixture.Manager().ImportHistoricalTrack({ShortLineAt(lat, lon)});

  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(pixelA), ());
}

UNIT_TEST(HistoricalImport_LiveThenImportRemainsEverLive)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.SetupPixels({{pixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(pixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(pixelA), ());

  fixture.Manager().ImportHistoricalTrack({ShortLineAt(lat, lon)});

  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(pixelA), ());
}

UNIT_TEST(HistoricalImport_MultiSegmentGapIsNotFilled)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [latA, lonA] =
      street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(latA, lonA);
  auto const [latB, lonB] = street_pixels_tests::OffsetLatLonByMeters(latA, lonA, 1000.0, 0.0);
  auto const pixelB = street_pixels_tests::PixelIdForLatLon(latB, lonB);
  auto const [latMid, lonMid] = street_pixels_tests::OffsetLatLonByMeters(latA, lonA, 500.0, 0.0);
  auto const pixelMid = street_pixels_tests::PixelIdForLatLon(latMid, lonMid);
  TEST_NOT_EQUAL(pixelA, pixelB, ());
  TEST_NOT_EQUAL(pixelA, pixelMid, ());
  TEST_NOT_EQUAL(pixelB, pixelMid, ());

  fixture.SetupPixels({{pixelA, false}, {pixelB, false}, {pixelMid, false}});

  auto const [latBCentre, lonBCentre] = street_pixels_tests::LatLonForPixelId(pixelB);
  fixture.Manager().ImportHistoricalTrack({ShortLineAt(latA, lonA), ShortLineAt(latBCentre, lonBCentre)});

  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelB), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelMid), ());
}

UNIT_TEST(HistoricalImport_DuplicateGeometryHashSkipsSecondPaint)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  std::string const countryId = "sp081_dup";
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.21, 16.38));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  auto const segments = std::vector<kml::MultiGeometry::LineT>{ShortLineAt(lat, lon)};

  street_stats::StreetStatsDB::Instance().DeleteProcessedTracksForCountry(countryId);
  fixture.Manager().SetStreetPixelsOverlayForTesting(countryId, street_pixels_tests::MakePixelSet({{pixelA, false}}));

  fixture.Manager().ImportHistoricalTrack(segments);
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  auto const geometryHash = fixture.Manager().ComputeHistoricalGeometryHashForTesting(segments);
  TEST(street_stats::StreetStatsDB::Instance().IsTrackProcessed(geometryHash, countryId), ());

  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());

  size_t const marked = fixture.Manager().ImportHistoricalTrack(segments);
  TEST_EQUAL(marked, 0, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());

  street_stats::StreetStatsDB::Instance().DeleteProcessedTracksForCountry(countryId);
}

UNIT_TEST(HistoricalImport_UpdateExploredPixelsDoesNotPaint)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.SetupPixels({{pixelA, false}});

  fixture.Manager().UpdateExploredPixels();

  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}

UNIT_TEST(HistoricalImport_InvalidCoordinatesAreSkipped)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [latA, lonA] =
      street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(latA, lonA);
  auto const [latB, lonB] = street_pixels_tests::OffsetLatLonByMeters(latA, lonA, 1000.0, 0.0);
  auto const pixelB = street_pixels_tests::PixelIdForLatLon(latB, lonB);
  auto const [latMid, lonMid] = street_pixels_tests::OffsetLatLonByMeters(latA, lonA, 500.0, 0.0);
  auto const pixelMid = street_pixels_tests::PixelIdForLatLon(latMid, lonMid);

  fixture.SetupPixels({{pixelA, false}, {pixelB, false}, {pixelMid, false}});

  kml::MultiGeometry::LineT line;
  line.push_back(PointAt(latA, lonA));
  line.emplace_back(m2::PointD(std::numeric_limits<double>::quiet_NaN(), 0.0));
  line.push_back(PointAt(latB, lonB));

  auto const validHash =
      fixture.Manager().ComputeHistoricalGeometryHashForTesting({{PointAt(latA, lonA), PointAt(latB, lonB)}});
  auto const skippedHash = fixture.Manager().ComputeHistoricalGeometryHashForTesting({line});
  TEST_EQUAL(skippedHash, validHash, ());

  fixture.Manager().ImportHistoricalTrack({line});

  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelB), ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelMid), ());
}

UNIT_TEST(HistoricalImport_EmptyOrInvalidGeometryDoesNotPaint)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.SetupPixels({{pixelA, false}});

  TEST_EQUAL(fixture.Manager().ImportHistoricalTrack({}), 0, ());
  TEST_EQUAL(fixture.Manager().ImportHistoricalTrack({{}}), 0, ());

  kml::MultiGeometry::LineT invalidOnly;
  invalidOnly.emplace_back(m2::PointD(std::numeric_limits<double>::quiet_NaN(),
                                      std::numeric_limits<double>::infinity()));
  TEST_EQUAL(fixture.Manager().ImportHistoricalTrack({invalidOnly}), 0, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}
