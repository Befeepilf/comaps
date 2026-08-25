#include "testing/testing.hpp"

#include "map/recording_session.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"
#include "street_pixels_areas/weekly_city_live_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"
#include "base/timer.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
int64_t constexpr kWeeklyCityLiveSecondsPerWeek = 7 * 86400;
int64_t constexpr kCityAOsm = 8;
int64_t constexpr kDistrictOsm = 10;
int64_t constexpr kCityBOsm = 18;

std::string WkPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void WkRemove(std::string const & path) { Platform::RemoveFileIfExists(path); }

void RemoveSqliteDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> WkLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput WkMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
                                              std::vector<m2::PointD> const & ring)
{
  street_pixels::AreaCandidateInput input;
  input.m_osmId = osmId;
  input.m_osmType = street_pixels::OsmObjectType::Relation;
  input.m_geometrySource = street_pixels::GeometrySource::TrueClosedRing;
  input.m_name = name;
  input.m_kind = "admin";
  input.m_adminLevel = adminLevel;
  input.m_lonLatRings = {ring};
  return input;
}

struct WkAreaFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string spxPath;
  int64_t mapDataVersion = 42;
  uint32_t policyVersion = 1;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t cityBId = 0;
  int64_t outsideId = 0;
};

WkAreaFixture MakeWkAreaFixture(std::string const & leaf, bool twoCities)
{
  WkAreaFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = WkPath(leaf + ".pix");
  fx.spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  WkRemove(fx.spaPath);
  WkRemove(fx.pixPath);
  WkRemove(fx.spxPath);

  auto const config = street_pixels::CountryConfig::LoadFromString(R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": { "enabled": true, "place_types": ["neighbourhood"] }
    }
  }
})");
  auto const policy = config.GetByIso("FI");
  fx.policyVersion = config.GetPolicyVersion();

  std::vector<street_pixels::AreaCandidateInput> inputs = {
      WkMakeAdmin(kDistrictOsm, 10, "District", WkLonLatBox(24.2, 60.2, 24.8, 60.8)),
      WkMakeAdmin(kCityAOsm, 8, "CityA", WkLonLatBox(24.0, 60.0, 25.0, 61.0)),
  };
  if (twoCities)
    inputs.push_back(WkMakeAdmin(kCityBOsm, 8, "CityB", WkLonLatBox(10.0, 50.0, 11.0, 51.0)));

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : inputs)
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  fx.districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  fx.cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  fx.cityBId = street_pixels_tests::PixelIdForLatLon(50.5, 10.5);
  fx.outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);

  std::vector<std::pair<int64_t, m2::PointD>> universeRows = {
      {fx.districtId, mercator::FromLatLon(60.5, 24.5)},
      {fx.cityOnlyId, mercator::FromLatLon(60.1, 24.1)},
      {fx.outsideId, mercator::FromLatLon(70.0, 30.0)},
  };
  if (twoCities)
    universeRows.push_back({fx.cityBId, mercator::FromLatLon(50.5, 10.5)});
  std::sort(universeRows.begin(), universeRows.end(),
            [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  std::vector<m2::PointD> samples;
  for (auto const & row : universeRows)
  {
    universeIds.push_back(row.first);
    samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = fx.policyVersion;
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);

  TEST(street_pixels_file::SaveRematchedUniverse(fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()),
                                                 {}, fx.mapDataVersion),
       ());
  return fx;
}

void CleanupWkArea(WkAreaFixture const & fx)
{
  WkRemove(fx.spaPath);
  WkRemove(fx.pixPath);
  WkRemove(fx.spxPath);
}

class WeeklyCityLiveSessionCleanup
{
public:
  WeeklyCityLiveSessionCleanup() { settings::Delete("RecordingSessionActive"); }
  ~WeeklyCityLiveSessionCleanup() { settings::Delete("RecordingSessionActive"); }
};

class WeeklyCityLiveFixture
{
public:
  WeeklyCityLiveFixture()
    : m_weeklyPath(WkPath("sp073_weekly_city_live.db"))
    , m_recencyPath(WkPath("sp073_live_recency.db"))
    , m_manager(m_dataSource)
  {
    RemoveSqliteDb(m_weeklyPath);
    RemoveSqliteDb(m_recencyPath);
    m_manager.ConfigureWeeklyCityLiveStoreForTesting(m_weeklyPath);
    m_manager.ConfigureLiveRecencyStoreForTesting(m_recencyPath);
    m_manager.SetRecordingSession(&m_session);
  }

  ~WeeklyCityLiveFixture()
  {
    RemoveSqliteDb(m_weeklyPath);
    RemoveSqliteDb(m_recencyPath);
  }

  void SetupPixels(std::initializer_list<std::pair<std::int64_t, bool>> idsAndExplored)
  {
    m_manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet(idsAndExplored));
  }

  void LoadAreas(WkAreaFixture const & fx)
  {
    TEST(m_manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  }

  location::GpsInfo GpsAtPixel(std::int64_t pixelId, double timestampSec, double accuracyM = 5.0) const
  {
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(pixelId);
    return street_pixels_tests::MakeGpsInfo(lat, lon, accuracyM, timestampSec);
  }

  int64_t Count(int64_t cityOsmId) { return m_manager.QueryWeeklyCityLive(cityOsmId).m_newLiveCount; }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  WeeklyCityLiveSessionCleanup m_cleanup;
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  std::string m_weeklyPath;
  std::string m_recencyPath;
  StreetPixelsManager m_manager;
};
}  // namespace

UNIT_TEST(WeeklyCityLive_FirstLiveVisitCounts)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_first_live", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(fx.districtId, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_SecondVisitSameCellDoesNot)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_second_visit", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts));
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts + 1.0));
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_ImportOnlyDoesNot)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_import_only", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  fixture.Manager().MarkImportedPixelsForTesting({fx.districtId});
  TEST(fixture.Manager().IsPixelExploredForTesting(fx.districtId), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_TrackReplayDoesNot)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_track_replay", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  fixture.Manager().MarkTrackPixelsForTesting({fx.districtId});
  TEST(fixture.Manager().IsPixelExploredForTesting(fx.districtId), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_AlreadyEverLiveDoesNot)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_already_live", false);
  fixture.LoadAreas(fx);
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true, true),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false, false),
  });
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(fx.districtId, street_pixels_tests::CurrentTimestampSec()));
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_ImportedThenLiveCountsOnce)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_import_then_live", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  fixture.Manager().MarkImportedPixelsForTesting({fx.districtId});
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts + 1.0));
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_IdlePauseRejectedDoNot)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_idle_pause_reject", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  double const ts = street_pixels_tests::CurrentTimestampSec();

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts));
  TEST(!fixture.Manager().IsPixelExploredForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts + 1.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  TEST_EQUAL(fixture.Session().Resume(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts + 2.0, 26.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(fx.districtId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_TwoCitiesIndependent)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_two_cities_mgr", true);
  fixture.LoadAreas(fx);
  fixture.SetupPixels(
      {{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.cityBId, false}, {fx.outsideId, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  double const ts = street_pixels_tests::CurrentTimestampSec();

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.districtId, ts));
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());
  TEST_EQUAL(fixture.Count(kCityBOsm), 0, ());

  fixture.Manager().MarkInterpolationBarrier();
  fixture.Manager().ResetSampleAcceptanceReference();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fx.cityBId, ts + 1.0));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fx.cityBId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());
  TEST_EQUAL(fixture.Count(kCityBOsm), 1, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_QueryBySettlementNotSubdivision)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_settlement_key", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(fx.districtId, street_pixels_tests::CurrentTimestampSec()));

  TEST_EQUAL(fixture.Count(kCityAOsm), 1, ());
  auto const subdiv = fixture.Manager().QueryWeeklyCityLive(kDistrictOsm);
  TEST_EQUAL(subdiv.m_cityOsmId, kDistrictOsm, ());
  TEST_EQUAL(subdiv.m_newLiveCount, 0, ());
  TEST(subdiv.m_usedUtcFallback, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_NoAreaPixelDoesNotInventCity)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_no_area", false);
  fixture.LoadAreas(fx);
  fixture.SetupPixels({{fx.districtId, false}, {fx.cityOnlyId, false}, {fx.outsideId, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(fx.outsideId, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fx.outsideId), ());
  TEST_EQUAL(fixture.Count(kCityAOsm), 0, ());
  TEST_EQUAL(fixture.Count(kDistrictOsm), 0, ());
  TEST_EQUAL(fixture.Manager().QueryWeeklyCityLive(0).m_newLiveCount, 0, ());

  CleanupWkArea(fx);
}

UNIT_TEST(WeeklyCityLive_QueryWeekRemaining)
{
  WeeklyCityLiveFixture fixture;
  int64_t const now = static_cast<int64_t>(base::SecondsSinceEpoch());
  auto const q = fixture.Manager().QueryWeeklyCityLive(kCityAOsm, now);
  TEST_EQUAL(q.m_remainingSeconds, q.m_weekEndUnix - now, ());
  TEST_GREATER(q.m_remainingSeconds, 0, ());
  TEST_LESS_OR_EQUAL(q.m_remainingSeconds, kWeeklyCityLiveSecondsPerWeek, ());
  TEST(q.m_usedUtcFallback, ());
  TEST_EQUAL(q.m_newLiveCount, 0, ());
}

UNIT_TEST(WeeklyCityLive_InterpolationCountsOnce)
{
  WeeklyCityLiveFixture fixture;
  auto fx = MakeWkAreaFixture("sp073_interpolation", false);
  fixture.LoadAreas(fx);

  double const startLat = 60.5;
  double const startLon = 24.5;
  auto const mid = street_pixels_tests::OffsetLatLonByMeters(startLat, startLon, 50.0, 0.0);
  auto const end = street_pixels_tests::OffsetLatLonByMeters(startLat, startLon, 100.0, 0.0);
  int64_t const pixelStart = street_pixels_tests::PixelIdForLatLon(startLat, startLon);
  int64_t const pixelMid = street_pixels_tests::PixelIdForLatLon(mid.first, mid.second);
  int64_t const pixelEnd = street_pixels_tests::PixelIdForLatLon(end.first, end.second);
  TEST_EQUAL(pixelStart, fx.districtId, ());
  TEST_NOT_EQUAL(pixelStart, pixelMid, ());
  TEST_NOT_EQUAL(pixelMid, pixelEnd, ());

  fixture.SetupPixels({{pixelStart, false},
                       {pixelMid, false},
                       {pixelEnd, false},
                       {fx.cityOnlyId, false},
                       {fx.outsideId, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(startLat, startLon, 5.0, ts));
  int64_t const afterStart = fixture.Count(kCityAOsm);
  TEST_GREATER(afterStart, 0, ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(pixelStart), ());

  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(end.first, end.second, 5.0, ts + 20.0));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(pixelMid), ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(pixelEnd), ());
  int64_t const afterInterp = fixture.Count(kCityAOsm);
  TEST_GREATER(afterInterp, afterStart, ());

  fixture.Manager().OnLocationUpdate(street_pixels_tests::MakeGpsInfo(end.first, end.second, 5.0, ts + 21.0));
  TEST_EQUAL(fixture.Count(kCityAOsm), afterInterp, ());

  CleanupWkArea(fx);
}
