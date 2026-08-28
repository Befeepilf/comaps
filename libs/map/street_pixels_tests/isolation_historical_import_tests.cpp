#include "testing/testing.hpp"

#include "map/competition_hint.hpp"
#include "map/competition_upload_payload.hpp"
#include "map/explorer_pro.hpp"
#include "map/first_goal.hpp"
#include "map/identity_store.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/live_recency_store.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"
#include "street_pixels_areas/weekly_city_live_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "kml/types.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"
#include "base/timer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
int64_t constexpr kIsoCityAOsm = 8;
int64_t constexpr kIsoDistrictOsm = 10;

std::string IsoPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void IsoRemove(std::string const & path) { Platform::RemoveFileIfExists(path); }

void IsoRemoveSqliteDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> IsoLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput IsoMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct IsoAreaFixture
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

IsoAreaFixture MakeIsoAreaFixture(std::string const & leaf, bool twoCities)
{
  IsoAreaFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = IsoPath(leaf + ".pix");
  fx.spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  IsoRemove(fx.spaPath);
  IsoRemove(fx.pixPath);
  IsoRemove(fx.spxPath);

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
      IsoMakeAdmin(kIsoDistrictOsm, 10, "District", IsoLonLatBox(24.2, 60.2, 24.8, 60.8)),
      IsoMakeAdmin(kIsoCityAOsm, 8, "CityA", IsoLonLatBox(24.0, 60.0, 25.0, 61.0)),
  };
  if (twoCities)
    inputs.push_back(IsoMakeAdmin(18, 8, "CityB", IsoLonLatBox(10.0, 50.0, 11.0, 51.0)));

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

void CleanupIsoArea(IsoAreaFixture const & fx)
{
  IsoRemove(fx.spaPath);
  IsoRemove(fx.pixPath);
  IsoRemove(fx.spxPath);
}

kml::MultiGeometry::LineT IsoShortLineAt(double lat, double lon)
{
  auto const [lat2, lon2] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 0.0, 10.0);
  return {geometry::PointWithAltitude(mercator::FromLatLon(lat, lon)),
          geometry::PointWithAltitude(mercator::FromLatLon(lat2, lon2))};
}

class IsoFakeEntitlementSource : public explorer_pro::EntitlementSource
{
public:
  explicit IsoFakeEntitlementSource(bool entitled) : m_entitled(entitled) {}

  bool IsEntitled() const override { return m_entitled; }

private:
  bool m_entitled;
};

class IsoEntitlementSourceScope
{
public:
  explicit IsoEntitlementSourceScope(explorer_pro::EntitlementSource * source)
  {
    explorer_pro::SetEntitlementSource(source);
  }

  ~IsoEntitlementSourceScope() { explorer_pro::SetEntitlementSource(nullptr); }
};

class IsoCapabilityAvailabilityScope
{
public:
  IsoCapabilityAvailabilityScope(explorer_pro::Capability capability, bool available)
    : m_capability(capability)
    , m_previous(explorer_pro::IsCapabilityAvailable(capability))
  {
    explorer_pro::SetCapabilityAvailable(capability, available);
  }

  ~IsoCapabilityAvailabilityScope() { explorer_pro::SetCapabilityAvailable(m_capability, m_previous); }

private:
  explorer_pro::Capability m_capability;
  bool m_previous;
};

class IsoCleanup
{
public:
  IsoCleanup()
  {
    settings::Delete("RecordingSessionActive");
    settings::Delete("Explore.CompetitionUploadPending");
  }

  ~IsoCleanup()
  {
    settings::Delete("RecordingSessionActive");
    settings::Delete("Explore.CompetitionUploadPending");
  }
};

class IsoFixture
{
public:
  explicit IsoFixture(std::string const & leaf)
    : m_weeklyPath(IsoPath(leaf + "_weekly_city_live.db"))
    , m_recencyPath(IsoPath(leaf + "_live_recency.db"))
    , m_milestonePath(IsoPath(leaf + "_area_milestones.db"))
    , m_manager(m_dataSource)
  {
    IsoRemoveSqliteDb(m_weeklyPath);
    IsoRemoveSqliteDb(m_recencyPath);
    IsoRemoveSqliteDb(m_milestonePath);
    m_manager.ConfigureWeeklyCityLiveStoreForTesting(m_weeklyPath);
    m_manager.ConfigureLiveRecencyStoreForTesting(m_recencyPath);
    m_manager.ConfigureAreaMilestoneStoreForTesting(m_milestonePath);
    m_manager.SetRecordingSession(&m_session);
    m_fx = MakeIsoAreaFixture(leaf, false);
    TEST(m_manager.RebuildAreaCompletionCache(m_fx.leaf, m_fx.spaPath, m_fx.mapDataVersion), ());
    TEST(m_manager.IsAreaCompletionCacheValid(), ());
    SetupPixels({{m_fx.districtId, false}, {m_fx.cityOnlyId, false}, {m_fx.outsideId, false}});
    auto const district = m_manager.GetAreaCompletion(0);
    TEST(district.has_value(), ());
    TEST_EQUAL(district->m_osmId, static_cast<uint64_t>(kIsoDistrictOsm), ());
    TEST_EQUAL(district->m_total, 1u, ());
    TEST_EQUAL(district->m_explored, 0u, ());
    auto const city = m_manager.GetAreaCompletion(1);
    TEST(city.has_value(), ());
    TEST_EQUAL(city->m_osmId, static_cast<uint64_t>(kIsoCityAOsm), ());
    TEST_EQUAL(city->m_total, 1u, ());
    TEST_EQUAL(city->m_explored, 0u, ());
  }

  ~IsoFixture()
  {
    CleanupIsoArea(m_fx);
    IsoRemoveSqliteDb(m_weeklyPath);
    IsoRemoveSqliteDb(m_recencyPath);
    IsoRemoveSqliteDb(m_milestonePath);
    street_pixels::WeeklyCityLiveStore::Instance().Reopen(street_pixels::WeeklyCityLiveStore::DefaultDbPath());
    street_pixels::LiveRecencyStore::Instance().Reopen(street_pixels::LiveRecencyStore::DefaultDbPath());
    street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
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

  size_t ImportDistrict()
  {
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(m_fx.districtId);
    return m_manager.ImportHistoricalTrack({IsoShortLineAt(lat, lon)});
  }

  size_t ImportDistrictAndCityOnly()
  {
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(m_fx.districtId);
    auto const [cityLat, cityLon] = street_pixels_tests::LatLonForPixelId(m_fx.cityOnlyId);
    return m_manager.ImportHistoricalTrack({IsoShortLineAt(lat, lon), IsoShortLineAt(cityLat, cityLon)});
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }
  int64_t DistrictId() const { return m_fx.districtId; }
  int64_t CityOnlyId() const { return m_fx.cityOnlyId; }
  int64_t MapDataVersion() const { return m_fx.mapDataVersion; }

private:
  IsoCleanup m_cleanup;
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  std::string m_weeklyPath;
  std::string m_recencyPath;
  std::string m_milestonePath;
  StreetPixelsManager m_manager;
  IsoAreaFixture m_fx;
};

void IsoAssertImportedOnlyIsolation(StreetPixelsManager & manager, int64_t districtId, int64_t mapDataVersion)
{
  TEST(manager.IsAreaCompletionCacheValid(), ());
  auto const personal = manager.GetAreaCompletion(0);
  TEST(personal.has_value(), ());
  TEST_EQUAL(personal->m_osmId, static_cast<uint64_t>(kIsoDistrictOsm), ());
  TEST_EQUAL(personal->m_total, 1u, ());
  TEST_EQUAL(personal->m_explored, 1u, ());
  TEST(manager.IsPixelExploredForTesting(districtId), ());
  TEST(!manager.IsPixelEverLiveForTesting(districtId), ());
  TEST(!street_pixels::LiveRecencyStore::Instance().GetLastLiveVisit(districtId).has_value(), ());
  TEST_EQUAL(manager.QueryWeeklyCityLive(kIsoCityAOsm).m_newLiveCount, 0, ());
  auto const query = manager.QueryCompetitionOwnership(kIsoDistrictOsm);
  TEST_EQUAL(query.m_uniqueLivePixels, 0u, ());
  TEST_ALMOST_EQUAL_ABS(query.m_ownershipScore, 0.0, 1e-12, ());
  TEST(!query.m_eligible, ());
  bool pending = false;
  TEST(!settings::Get("Explore.CompetitionUploadPending", pending), ());
  int64_t const now = static_cast<int64_t>(base::SecondsSinceEpoch());
  auto const snapshot = manager.BuildCompetitionUploadSnapshot(now);
  TEST_EQUAL(snapshot.m_mapDataVersion, mapDataVersion, ());
  TEST(CompetitionUploadPayloadIsEmpty(snapshot), ());
}

void IsoAssertImportThenLive(IsoFixture & fixture)
{
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.DistrictId()), ());
  TEST_EQUAL(fixture.Manager().QueryWeeklyCityLive(kIsoCityAOsm).m_newLiveCount, 0, ());
  auto const personalBefore = fixture.Manager().GetAreaCompletion(0);
  TEST(personalBefore.has_value(), ());
  TEST_EQUAL(personalBefore->m_osmId, static_cast<uint64_t>(kIsoDistrictOsm), ());
  TEST_EQUAL(personalBefore->m_explored, 1u, ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  double const ts = street_pixels_tests::CurrentTimestampSec();
  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fixture.DistrictId(), ts));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fixture.DistrictId()), ());
  TEST(street_pixels::LiveRecencyStore::Instance().GetLastLiveVisit(fixture.DistrictId()).has_value(), ());
  TEST_EQUAL(fixture.Manager().QueryWeeklyCityLive(kIsoCityAOsm).m_newLiveCount, 1, ());

  fixture.Manager().OnLocationUpdate(fixture.GpsAtPixel(fixture.DistrictId(), ts + 1.0));
  TEST_EQUAL(fixture.Manager().QueryWeeklyCityLive(kIsoCityAOsm).m_newLiveCount, 1, ());

  auto const personalAfter = fixture.Manager().GetAreaCompletion(0);
  TEST(personalAfter.has_value(), ());
  TEST_EQUAL(personalAfter->m_explored, personalBefore->m_explored, ());
}

void IsoAssertLiveThenImport(IsoFixture & fixture)
{
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(fixture.DistrictId(), street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fixture.DistrictId()), ());
  auto const recencyBefore = street_pixels::LiveRecencyStore::Instance().GetLastLiveVisit(fixture.DistrictId());
  TEST(recencyBefore.has_value(), ());
  TEST_EQUAL(fixture.Manager().QueryWeeklyCityLive(kIsoCityAOsm).m_newLiveCount, 1, ());

  size_t const marked = fixture.ImportDistrictAndCityOnly();
  TEST_EQUAL(marked, 1, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.CityOnlyId()), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.CityOnlyId()), ());
  TEST(!street_pixels::LiveRecencyStore::Instance().GetLastLiveVisit(fixture.CityOnlyId()).has_value(), ());

  auto const recencyAfter = street_pixels::LiveRecencyStore::Instance().GetLastLiveVisit(fixture.DistrictId());
  TEST(recencyAfter.has_value(), ());
  TEST_EQUAL(*recencyAfter, *recencyBefore, ());
  TEST_EQUAL(fixture.Manager().QueryWeeklyCityLive(kIsoCityAOsm).m_newLiveCount, 1, ());
  TEST(fixture.Manager().IsPixelEverLiveForTesting(fixture.DistrictId()), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.DistrictId()), ());
}

class IsoFirstGoalCleanup
{
public:
  IsoFirstGoalCleanup()
  {
    settings::Delete("RecordingSessionActive");
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
  }

  ~IsoFirstGoalCleanup()
  {
    settings::Delete("RecordingSessionActive");
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
  }
};

class IsoFirstGoalFixture
{
public:
  IsoFirstGoalFixture() : m_manager(m_dataSource)
  {
    m_manager.ResetFirstGoalForTesting();
    m_manager.SetRecordingSession(&m_session);
  }

  std::int64_t PixelAt(int index) const
  {
    return street_pixels_tests::PixelIdForLatLon(50.0 + static_cast<double>(index), 10.0);
  }

  void SetupUnexplored(size_t count)
  {
    std::vector<df::StreetPixel> pixels;
    pixels.reserve(count);
    for (size_t i = 0; i < count; ++i)
      pixels.push_back(street_pixels_tests::MakeStreetPixel(PixelAt(static_cast<int>(i)), false));
    m_manager.SetStreetPixelsForTesting(std::move(pixels));
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};

class IsoCompetitionHintCleanup
{
public:
  IsoCompetitionHintCleanup()
  {
    settings::Delete("RecordingSessionActive");
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
    street_pixels::CompetitionHintTracker::ClearPersistedForTesting();
    IdentityStore::RevokeCompetitionConsent();
  }

  ~IsoCompetitionHintCleanup()
  {
    IdentityStore::RevokeCompetitionConsent();
    street_pixels::CompetitionHintTracker::ClearPersistedForTesting();
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
    settings::Delete("RecordingSessionActive");
  }
};

class IsoCompetitionHintFixture
{
public:
  IsoCompetitionHintFixture() : m_manager(m_dataSource)
  {
    m_manager.ResetFirstGoalForTesting();
    m_manager.ResetCompetitionHintForTesting();
    m_manager.SetRecordingSession(&m_session);
  }

  std::int64_t PixelAt(int index) const
  {
    return street_pixels_tests::PixelIdForLatLon(50.0 + static_cast<double>(index), 10.0);
  }

  void SetupUnexplored(size_t count)
  {
    std::vector<df::StreetPixel> pixels;
    pixels.reserve(count);
    for (size_t i = 0; i < count; ++i)
      pixels.push_back(street_pixels_tests::MakeStreetPixel(PixelAt(static_cast<int>(i)), false));
    m_manager.SetStreetPixelsForTesting(std::move(pixels));
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};

size_t IsoImportPixelsZeroAndOne(StreetPixelsManager & manager, std::int64_t pixel0, std::int64_t pixel1)
{
  auto const [lat0, lon0] = street_pixels_tests::LatLonForPixelId(pixel0);
  auto const [lat1, lon1] = street_pixels_tests::LatLonForPixelId(pixel1);
  return manager.ImportHistoricalTrack({IsoShortLineAt(lat0, lon0), IsoShortLineAt(lat1, lon1)});
}
}  // namespace

UNIT_TEST(IsolationHistoricalImport_MarksExploredNeverLive)
{
  IsoFixture fixture("sp082_marks_explored");
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.DistrictId()), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.DistrictId()), ());
}

UNIT_TEST(IsolationHistoricalImport_PersonalCompletionIncrements)
{
  IsoFixture fixture("sp082_personal_completion");
  auto const before = fixture.Manager().GetAreaCompletion(0);
  TEST(before.has_value(), ());
  TEST_EQUAL(before->m_osmId, static_cast<uint64_t>(kIsoDistrictOsm), ());
  TEST_EQUAL(before->m_explored, 0u, ());
  TEST_EQUAL(before->m_total, 1u, ());

  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());

  auto const after = fixture.Manager().GetAreaCompletion(0);
  TEST(after.has_value(), ());
  TEST_EQUAL(after->m_osmId, static_cast<uint64_t>(kIsoDistrictOsm), ());
  TEST_EQUAL(after->m_explored, 1u, ());
  TEST_EQUAL(after->m_total, 1u, ());
}

UNIT_TEST(IsolationHistoricalImport_NoRecencyWeeklyOwnershipOrPending)
{
  IsoFixture fixture("sp082_no_competitive");
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  IsoAssertImportedOnlyIsolation(fixture.Manager(), fixture.DistrictId(), fixture.MapDataVersion());
}

UNIT_TEST(IsolationHistoricalImport_OwnershipZeroAtFullPersonal)
{
  IsoFixture fixture("sp082_ownership_zero");
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());

  auto const personal = fixture.Manager().GetAreaCompletion(0);
  TEST(personal.has_value(), ());
  TEST_EQUAL(personal->m_osmId, static_cast<uint64_t>(kIsoDistrictOsm), ());
  TEST_EQUAL(personal->m_explored, 1u, ());
  TEST_EQUAL(personal->m_total, 1u, ());
  TEST_EQUAL(fixture.Manager().GetAreaCompletionFraction(0), 1.0, ());

  auto const query = fixture.Manager().QueryCompetitionOwnership(kIsoDistrictOsm);
  TEST_EQUAL(query.m_uniqueLivePixels, 0u, ());
  TEST_ALMOST_EQUAL_ABS(query.m_ownershipScore, 0.0, 1e-12, ());
  TEST(!query.m_eligible, ());
}

UNIT_TEST(IsolationHistoricalImport_ThenLiveCountsOnce)
{
  IsoFixture fixture("sp082_then_live");
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  IsoAssertImportThenLive(fixture);
}

UNIT_TEST(IsolationHistoricalImport_LiveThenImportLeavesRecencyUnchanged)
{
  IsoFixture fixture("sp082_live_then_import");
  IsoAssertLiveThenImport(fixture);
}

UNIT_TEST(IsolationHistoricalImport_FirstGoalDoesNotAdvance)
{
  IsoFirstGoalCleanup cleanup;
  IsoFirstGoalFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  size_t const marked = IsoImportPixelsZeroAndOne(fixture.Manager(), fixture.PixelAt(0), fixture.PixelAt(1));
  TEST_EQUAL(marked, 2, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(0)), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(1)), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.PixelAt(0)), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.PixelAt(1)), ());
  auto p = fixture.Manager().GetFirstGoalProgress();
  TEST_EQUAL(p.m_state, street_pixels::FirstGoalState::InProgress, ());
  TEST_EQUAL(p.m_collected, 0u, ());
}

UNIT_TEST(IsolationHistoricalImport_CompetitionHintDoesNotAdvance)
{
  IsoCompetitionHintCleanup cleanup;
  IsoCompetitionHintFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  size_t const marked = IsoImportPixelsZeroAndOne(fixture.Manager(), fixture.PixelAt(0), fixture.PixelAt(1));
  TEST_EQUAL(marked, 2, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(0)), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(1)), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.PixelAt(0)), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.PixelAt(1)), ());
  auto const hint = fixture.Manager().GetCompetitionHintProgress();
  TEST_EQUAL(hint.m_collected, 0u, ());
  TEST(!hint.m_complete, ());
}

UNIT_TEST(IsolationHistoricalImport_NotRecordingZeroHaptic)
{
  IsoFixture fixture("sp082_zero_haptic");
  TEST(!fixture.Session().IsRecording(), ());
  size_t calls = 0;
  fixture.Manager().SetVibrationHandler([&calls](street_pixels::ExplorationHapticKind) { ++calls; });
  fixture.Manager().SetApplicationForeground(true);
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  auto const record = fixture.Manager().GetAreaMilestoneRecord(static_cast<uint64_t>(kIsoDistrictOsm));
  TEST(record.has_value(), ());
  TEST((record->m_firedMask & street_pixels::kAreaMilestoneMask100) != 0, ());
  TEST_EQUAL(calls, 0, ());
}

UNIT_TEST(IsolationHistoricalImport_GateUnavailableNotEntitled)
{
  IsoFixture fixture("sp082_gate_unavail_not_ent");
  IsoCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, false);
  IsoEntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  IsoAssertImportedOnlyIsolation(fixture.Manager(), fixture.DistrictId(), fixture.MapDataVersion());
  IsoAssertImportThenLive(fixture);
}

UNIT_TEST(IsolationHistoricalImport_GateUnavailableEntitled)
{
  IsoFixture fixture("sp082_gate_unavail_ent");
  IsoCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, false);
  IsoFakeEntitlementSource entitled(true);
  IsoEntitlementSourceScope scope(&entitled);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  IsoAssertImportedOnlyIsolation(fixture.Manager(), fixture.DistrictId(), fixture.MapDataVersion());
  IsoAssertImportThenLive(fixture);
}

UNIT_TEST(IsolationHistoricalImport_GateAvailableNotEntitled)
{
  IsoFixture fixture("sp082_gate_avail_not_ent");
  IsoCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  IsoEntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  IsoAssertImportedOnlyIsolation(fixture.Manager(), fixture.DistrictId(), fixture.MapDataVersion());
  IsoAssertImportThenLive(fixture);
}

UNIT_TEST(IsolationHistoricalImport_GateAvailableEntitled)
{
  IsoFixture fixture("sp082_gate_avail_ent");
  IsoCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  IsoFakeEntitlementSource entitled(true);
  IsoEntitlementSourceScope scope(&entitled);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  size_t const marked = fixture.ImportDistrict();
  TEST_EQUAL(marked, 1, ());
  IsoAssertImportedOnlyIsolation(fixture.Manager(), fixture.DistrictId(), fixture.MapDataVersion());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(fixture.DistrictId()), ());
  IsoAssertImportThenLive(fixture);
}

UNIT_TEST(IsolationHistoricalImport_LiveThenImportWhenAvailableEntitled)
{
  IsoFixture fixture("sp082_live_then_import_pro");
  IsoCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  IsoFakeEntitlementSource entitled(true);
  IsoEntitlementSourceScope scope(&entitled);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  IsoAssertLiveThenImport(fixture);
}
