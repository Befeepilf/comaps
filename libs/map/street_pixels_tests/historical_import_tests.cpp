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

#include "base/logging.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <string>
#include <sys/resource.h>
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

class ProcessedTracksCleanup
{
public:
  explicit ProcessedTracksCleanup(std::string countryId)
    : m_countryId(std::move(countryId))
  {
    street_stats::StreetStatsDB::Instance().DeleteProcessedTracksForCountry(m_countryId);
  }

  ~ProcessedTracksCleanup()
  {
    street_stats::StreetStatsDB::Instance().DeleteProcessedTracksForCountry(m_countryId);
  }

  std::string const & CountryId() const { return m_countryId; }

private:
  std::string m_countryId;
};

long HistReadProcStatusKb(char const * key)
{
  std::ifstream in("/proc/self/status");
  std::string line;
  while (std::getline(in, line))
  {
    if (line.compare(0, std::strlen(key), key) == 0)
    {
      auto const colon = line.find(':');
      if (colon == std::string::npos)
        return -1;
      return std::strtol(line.c_str() + colon + 1, nullptr, 10);
    }
  }
  return -1;
}

void HistLogRss(char const * name, size_t n, long rssBeforeKb)
{
  rusage usage{};
  getrusage(RUSAGE_SELF, &usage);
  LOG(LINFO, ("SP-085", name, "n =", n, "VmRSS_before_kb =", rssBeforeKb, "VmRSS_after_kb =",
              HistReadProcStatusKb("VmRSS:"), "VmHWM_kb =", HistReadProcStatusKb("VmHWM:"),
              "ru_maxrss_kb =", usage.ru_maxrss));
}

kml::MultiGeometry::LineT HistSpacedLine(size_t n)
{
  kml::MultiGeometry::LineT line;
  line.reserve(n);
  double lat = 48.2;
  double lon = 16.37;
  for (size_t i = 0; i < n; ++i)
  {
    line.emplace_back(mercator::FromLatLon(lat, lon));
    auto const next = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 15.0, 0.0);
    lat = next.first;
    lon = next.second;
  }
  return line;
}
}  // namespace

UNIT_TEST(HistoricalImport_FirstImportEverLiveClear)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.SetupPixels({{pixelA, false}});

  size_t const marked = fixture.Manager().ImportHistoricalTrack({ShortLineAt(lat, lon)});

  TEST_GREATER(marked, 0, ());
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
  TEST_GREATER(mercator::DistanceOnEarth(mercator::FromLatLon(latA, lonA), mercator::FromLatLon(latMid, lonMid)),
               2.0 * street_pixels_tests::kExploreRadiusMeters, ());

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
  ProcessedTracksCleanup tracks("HistoricalImport_DuplicateGeometryHashSkipsSecondPaint");
  auto const & countryId = tracks.CountryId();
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.21, 16.38));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  auto const segments = std::vector<kml::MultiGeometry::LineT>{ShortLineAt(lat, lon)};

  fixture.Manager().SetStreetPixelsOverlayForTesting(countryId, street_pixels_tests::MakePixelSet({{pixelA, false}}));

  size_t const firstMarked = fixture.Manager().ImportHistoricalTrack(segments);
  TEST_GREATER(firstMarked, 0, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  auto const geometryHash = fixture.Manager().ComputeHistoricalGeometryHashForTesting(segments);
  TEST(street_stats::StreetStatsDB::Instance().IsTrackProcessed(geometryHash, countryId), ());

  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());

  size_t const marked = fixture.Manager().ImportHistoricalTrack(segments);
  TEST_EQUAL(marked, 0, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
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

UNIT_TEST(HistoricalImport_SegmentBoundariesChangeGeometryHash)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const [latB, lonB] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 20.0, 0.0);
  auto const [latC, lonC] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 40.0, 0.0);
  auto const a = PointAt(lat, lon);
  auto const b = PointAt(latB, lonB);
  auto const c = PointAt(latC, lonC);

  auto const concatenated =
      fixture.Manager().ComputeHistoricalGeometryHashForTesting({{a, b, c}});
  auto const segmented =
      fixture.Manager().ComputeHistoricalGeometryHashForTesting({{a, b}, {c}});
  TEST_NOT_EQUAL(concatenated, segmented, ());
}

UNIT_TEST(HistoricalImport_NotReadyDoesNotWriteLedger)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  ProcessedTracksCleanup tracks("HistoricalImport_NotReadyDoesNotWriteLedger");
  auto const & countryId = tracks.CountryId();
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.22, 16.39));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  auto const segments = std::vector<kml::MultiGeometry::LineT>{ShortLineAt(lat, lon)};

  fixture.SetupPixels({{pixelA, false}});
  fixture.Manager().SetCountryIdForTesting(countryId);
  TEST_EQUAL(static_cast<int>(fixture.Manager().GetState().status),
             static_cast<int>(StreetPixelsManager::StreetPixelsStatus::NotReady), ());

  size_t const marked = fixture.Manager().ImportHistoricalTrack(segments);
  TEST_EQUAL(marked, 0, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  auto const geometryHash = fixture.Manager().ComputeHistoricalGeometryHashForTesting(segments);
  TEST(!street_stats::StreetStatsDB::Instance().IsTrackProcessed(geometryHash, countryId), ());
}

UNIT_TEST(HistoricalImport_TenThousandPointsCompletes)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  size_t constexpr kN = 10000;
  auto const line = HistSpacedLine(kN);
  long const rssBefore = HistReadProcStatusKb("VmRSS:");
  size_t const marked = fixture.Manager().ImportHistoricalTrack({line});
  TEST_GREATER_OR_EQUAL(marked, 0, (marked));
  HistLogRss("HistoricalImport_TenThousandPointsCompletes", kN, rssBefore);
}

UNIT_TEST(HistoricalImport_FiftyThousandPointsCompletes)
{
  HistoricalImportBreadcrumbCleanup cleanup;
  HistoricalImportFixture fixture;
  size_t constexpr kN = 50000;
  auto const line = HistSpacedLine(kN);
  long const rssBefore = HistReadProcStatusKb("VmRSS:");
  size_t const marked = fixture.Manager().ImportHistoricalTrack({line});
  TEST_GREATER_OR_EQUAL(marked, 0, (marked));
  HistLogRss("HistoricalImport_FiftyThousandPointsCompletes", kN, rssBefore);
}
