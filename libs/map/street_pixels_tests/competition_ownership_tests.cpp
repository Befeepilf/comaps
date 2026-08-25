#include "testing/testing.hpp"

#include "map/identity_store.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/live_recency_store.hpp"
#include "street_pixels_areas/ownership_scoring.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"
#include "base/timer.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string CoPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void CoRemove(std::string const & path) { Platform::RemoveFileIfExists(path); }

void RemoveRecencyDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> CoLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput CoMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct CoAreaFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string spxPath;
  int64_t mapDataVersion = 42;
  uint32_t policyVersion = 1;
  std::vector<m2::PointD> samples;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
};

CoAreaFixture MakeCoAreaFixture(std::string const & leaf, bool districtEverLive)
{
  CoAreaFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = CoPath(leaf + ".pix");
  fx.spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  CoRemove(fx.spaPath);
  CoRemove(fx.pixPath);
  CoRemove(fx.spxPath);

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

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : {CoMakeAdmin(10, 10, "District", CoLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             CoMakeAdmin(8, 8, "City", CoLonLatBox(24.0, 60.0, 25.0, 61.0))})
  {
    auto result = street_pixels::FilterExplorationCandidate(input, policy);
    TEST(result.m_area.has_value(), ());
    areas.push_back(*result.m_area);
  }

  int64_t const districtId = street_pixels_tests::PixelIdForLatLon(60.5, 24.5);
  int64_t const cityOnlyId = street_pixels_tests::PixelIdForLatLon(60.1, 24.1);
  int64_t const outsideId = street_pixels_tests::PixelIdForLatLon(70.0, 30.0);

  std::vector<std::pair<int64_t, m2::PointD>> universeRows = {
      {districtId, mercator::FromLatLon(60.5, 24.5)},
      {cityOnlyId, mercator::FromLatLon(60.1, 24.1)},
      {outsideId, mercator::FromLatLon(70.0, 30.0)},
  };
  std::sort(universeRows.begin(), universeRows.end(),
            [](auto const & a, auto const & b) { return a.first < b.first; });
  std::vector<int64_t> universeIds;
  fx.samples.clear();
  for (auto const & row : universeRows)
  {
    universeIds.push_back(row.first);
    fx.samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = fx.policyVersion;
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, fx.samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{districtId, districtEverLive}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  fx.districtId = districtId;
  fx.cityOnlyId = cityOnlyId;
  fx.outsideId = outsideId;
  return fx;
}

void CleanupCoArea(CoAreaFixture const & fx)
{
  CoRemove(fx.spaPath);
  CoRemove(fx.pixPath);
  CoRemove(fx.spxPath);
}

class ConsentAndSessionCleanup
{
public:
  ConsentAndSessionCleanup()
  {
    settings::Delete("RecordingSessionActive");
    IdentityStore::RevokeCompetitionConsent();
    IdentityStore::SetCompetitionConsentGrantedHandler({});
  }

  ~ConsentAndSessionCleanup()
  {
    IdentityStore::SetCompetitionConsentGrantedHandler({});
    IdentityStore::RevokeCompetitionConsent();
    settings::Delete("RecordingSessionActive");
  }
};

class CompetitionOwnershipFixture
{
public:
  static std::int64_t constexpr kPixelA = 1000;
  static std::int64_t constexpr kPixelB = 500000000;

  CompetitionOwnershipFixture()
    : m_dbPath(CoPath("sp072_live_recency.db"))
    , m_manager(m_dataSource)
  {
    RemoveRecencyDb(m_dbPath);
    m_manager.ConfigureLiveRecencyStoreForTesting(m_dbPath);
    m_manager.SetRecordingSession(&m_session);
  }

  ~CompetitionOwnershipFixture() { RemoveRecencyDb(m_dbPath); }

  void SetupPixels(std::initializer_list<std::pair<std::int64_t, bool>> idsAndExplored)
  {
    m_manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet(idsAndExplored));
  }

  location::GpsInfo GpsAtPixel(std::int64_t pixelId, double timestampSec, double accuracyM = 5.0) const
  {
    auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(pixelId);
    return street_pixels_tests::MakeGpsInfo(lat, lon, accuracyM, timestampSec);
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }
  std::string const & DbPath() const { return m_dbPath; }

private:
  ConsentAndSessionCleanup m_cleanup;
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  std::string m_dbPath;
  StreetPixelsManager m_manager;
};

std::optional<int64_t> LastVisit(std::int64_t pixelId)
{
  return street_pixels::LiveRecencyStore::Instance().GetLastLiveVisit(pixelId);
}
}  // namespace

UNIT_TEST(CompetitionOwnership_LiveFirstVisitWritesRecency)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST(!LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));

  TEST(fixture.Manager().IsPixelEverLiveForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());
}

UNIT_TEST(CompetitionOwnership_ImportDoesNotWriteRecency)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  fixture.Manager().MarkImportedPixelsForTesting({CompetitionOwnershipFixture::kPixelA});
  TEST(fixture.Manager().IsPixelExploredForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(!fixture.Manager().IsPixelEverLiveForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(!LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());
}

UNIT_TEST(CompetitionOwnership_ImportedThenLiveWritesRecency)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  fixture.Manager().MarkImportedPixelsForTesting({CompetitionOwnershipFixture::kPixelA});
  TEST(!LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());

  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));

  TEST(fixture.Manager().IsPixelEverLiveForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());
}

UNIT_TEST(CompetitionOwnership_LiveThenImportLeavesRecencyUnchanged)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  auto const first = LastVisit(CompetitionOwnershipFixture::kPixelA);
  TEST(first.has_value(), ());

  fixture.Manager().MarkImportedPixelsForTesting({CompetitionOwnershipFixture::kPixelA});
  auto const afterImport = LastVisit(CompetitionOwnershipFixture::kPixelA);
  TEST(afterImport.has_value(), ());
  TEST_EQUAL(*afterImport, *first, ());
}

UNIT_TEST(CompetitionOwnership_RevisitUpdatesTimestamp)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelEverLiveForTesting(CompetitionOwnershipFixture::kPixelA), ());

  street_pixels::LiveRecencyStore::Instance().TouchLiveVisits({CompetitionOwnershipFixture::kPixelA}, 1000);
  TEST_EQUAL(*LastVisit(CompetitionOwnershipFixture::kPixelA), 1000, ());

  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec() + 1.0));
  auto const revisited = LastVisit(CompetitionOwnershipFixture::kPixelA);
  TEST(revisited.has_value(), ());
  TEST_GREATER(*revisited, 1000, ());
}

UNIT_TEST(CompetitionOwnership_IdleWritesNoRecency)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(!LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());
}

UNIT_TEST(CompetitionOwnership_PauseWritesNoRecency)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(!LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());
}

UNIT_TEST(CompetitionOwnership_RejectedSampleWritesNoRecency)
{
  CompetitionOwnershipFixture fixture;
  fixture.SetupPixels({{CompetitionOwnershipFixture::kPixelA, false}});
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(CompetitionOwnershipFixture::kPixelA, street_pixels_tests::CurrentTimestampSec(), 26.0));
  TEST(!fixture.Manager().IsPixelExploredForTesting(CompetitionOwnershipFixture::kPixelA), ());
  TEST(!LastVisit(CompetitionOwnershipFixture::kPixelA).has_value(), ());
}

UNIT_TEST(CompetitionOwnership_SeedOnOptInAndSecondOptInDoesNotReseed)
{
  CompetitionOwnershipFixture fixture;
  fixture.Manager().SetStreetPixelsForTesting(
      {street_pixels_tests::MakeStreetPixel(CompetitionOwnershipFixture::kPixelA, true, true)});

  IdentityStore::GrantCompetitionConsent();
  uint64_t const consent = IdentityStore::GetCompetitionConsentUnixTime();
  TEST(consent != 0, ());
  fixture.Manager().MaybeSeedLiveRecency(consent);
  TEST_EQUAL(*LastVisit(CompetitionOwnershipFixture::kPixelA), static_cast<int64_t>(consent), ());

  street_pixels::LiveRecencyStore::Instance().TouchLiveVisits({CompetitionOwnershipFixture::kPixelA}, 50);
  TEST_EQUAL(*LastVisit(CompetitionOwnershipFixture::kPixelA), 50, ());

  IdentityStore::GrantCompetitionConsent();
  uint64_t const second = IdentityStore::GetCompetitionConsentUnixTime();
  fixture.Manager().MaybeSeedLiveRecency(second);
  TEST_EQUAL(*LastVisit(CompetitionOwnershipFixture::kPixelA), 50, ());
}

UNIT_TEST(CompetitionOwnership_Score100JustVisitedFullLive)
{
  CompetitionOwnershipFixture fixture;
  auto fx = MakeCoAreaFixture("sp072_score100", true);
  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true, true),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false, false),
  });
  int64_t const now = static_cast<int64_t>(base::SecondsSinceEpoch());
  street_pixels::LiveRecencyStore::Instance().TouchLiveVisits({fx.districtId}, now);

  auto const query = fixture.Manager().QueryCompetitionOwnership(10);
  TEST_EQUAL(query.m_osmId, 10u, ());
  TEST_EQUAL(query.m_totalPixels, 1u, ());
  TEST_EQUAL(query.m_uniqueLivePixels, 1u, ());
  TEST_ALMOST_EQUAL_ABS(query.m_liveCoverageFraction, 1.0, 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(query.m_ownershipScore, 100.0, 0.01, ());
  TEST(query.m_eligible, ());
  TEST(!query.m_localContested, ());
  TEST_EQUAL(query.m_localUnclaimed, !query.m_eligible, ());
  TEST(query.m_localIsBoss, ());
  TEST_EQUAL(query.m_scoreCalcVersion, street_pixels::kScoreCalcVersion, ());

  CleanupCoArea(fx);
}

UNIT_TEST(CompetitionOwnership_ImportedOnlyScoreZeroFullPersonalCompletion)
{
  CompetitionOwnershipFixture fixture;
  auto fx = MakeCoAreaFixture("sp072_imported_only", false);
  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true, false),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false, false),
  });

  auto const personal = fixture.Manager().GetAreaCompletion(0);
  TEST(personal.has_value(), ());
  TEST_EQUAL(personal->m_explored, 1u, ());
  TEST_EQUAL(personal->m_total, 1u, ());
  TEST_EQUAL(fixture.Manager().GetAreaCompletionFraction(0), 1.0, ());

  auto const query = fixture.Manager().QueryCompetitionOwnership(10);
  TEST_EQUAL(query.m_totalPixels, 1u, ());
  TEST_EQUAL(query.m_uniqueLivePixels, 0u, ());
  TEST_ALMOST_EQUAL_ABS(query.m_ownershipScore, 0.0, 1e-12, ());
  TEST(!query.m_eligible, ());
  TEST(query.m_localUnclaimed, ());
  TEST(!query.m_localContested, ());
  TEST(!query.m_localIsBoss, ());
  TEST_EQUAL(query.m_localUnclaimed, !query.m_eligible, ());
  TEST(!LastVisit(fx.districtId).has_value(), ());

  CleanupCoArea(fx);
}

UNIT_TEST(CompetitionOwnership_ImportedOnlyDoesNotAffectEligibilityOrContested)
{
  CompetitionOwnershipFixture fixture;
  auto fx = MakeCoAreaFixture("sp072_imported_elig", false);
  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true, false),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, true, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, true, false),
  });
  fixture.Manager().MarkImportedPixelsForTesting({fx.cityOnlyId, fx.outsideId});

  auto const query = fixture.Manager().QueryCompetitionOwnership(10);
  TEST_EQUAL(query.m_uniqueLivePixels, 0u, ());
  TEST_ALMOST_EQUAL_ABS(query.m_ownershipScore, 0.0, 1e-12, ());
  TEST(!query.m_eligible, ());
  TEST(query.m_localUnclaimed, ());
  TEST(!query.m_localContested, ());

  CleanupCoArea(fx);
}

UNIT_TEST(CompetitionOwnership_UnknownOsmFailClosedZeros)
{
  CompetitionOwnershipFixture fixture;
  auto fx = MakeCoAreaFixture("sp072_unknown_osm", true);
  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto const query = fixture.Manager().QueryCompetitionOwnership(999999);
  TEST_EQUAL(query.m_totalPixels, 0u, ());
  TEST_EQUAL(query.m_uniqueLivePixels, 0u, ());
  TEST_ALMOST_EQUAL_ABS(query.m_ownershipScore, 0.0, 1e-12, ());
  TEST(!query.m_eligible, ());
  TEST(query.m_localUnclaimed, ());
  TEST(!query.m_localContested, ());
  CleanupCoArea(fx);
}

UNIT_TEST(CompetitionOwnership_PixFormatUnchangedV2)
{
  CompetitionOwnershipFixture fixture;
  auto fx = MakeCoAreaFixture("sp072_pix_format", true);
  auto const before = street_pixels_file::ProbeFile(fx.pixPath);
  TEST_EQUAL(before.header.formatVersion, street_pixels_file::kFormatVersionV2, ());

  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, false, false),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false, false),
  });
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(fx.districtId);
  fixture.Manager().OnLocationUpdate(
      street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, street_pixels_tests::CurrentTimestampSec()));

  auto const after = street_pixels_file::ProbeFile(fx.pixPath);
  TEST_EQUAL(after.header.formatVersion, street_pixels_file::kFormatVersionV2, ());
  TEST(after.kind == street_pixels_file::FileKind::HeaderedV2, ());
  CleanupCoArea(fx);
}

UNIT_TEST(CompetitionOwnership_SeedScansPixFileEverLive)
{
  CompetitionOwnershipFixture fixture;
  auto fx = MakeCoAreaFixture("sp072_seed_pix", true);
  IdentityStore::GrantCompetitionConsent();
  uint64_t const consent = IdentityStore::GetCompetitionConsentUnixTime();
  fixture.Manager().MaybeSeedLiveRecency(consent);
  TEST_EQUAL(*LastVisit(fx.districtId), static_cast<int64_t>(consent), ());
  TEST(!LastVisit(fx.cityOnlyId).has_value(), ());
  CleanupCoArea(fx);
}
