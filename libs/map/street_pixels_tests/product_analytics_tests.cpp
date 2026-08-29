#include "testing/testing.hpp"

#include "map/competition_upload_payload.hpp"
#include "map/completion_card_analytics.hpp"
#include "map/explorer_pro.hpp"
#include "map/explorer_pro_analytics.hpp"
#include "map/first_goal.hpp"
#include "map/identity_store.hpp"
#include "map/product_analytics.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "routing/street_exploration_routing_analytics.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/weekly_city_week.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace street_pixels;

namespace
{
class ProductAnalyticsGuard
{
public:
  ProductAnalyticsGuard()
  {
    settings::Delete("RecordingSessionActive");
    FirstGoalTracker::ClearPersistedForTesting();
    ProductAnalytics::ResetForTesting();
    CompletionCardAnalytics::ResetForTesting();
    ExplorerProAnalytics::ResetForTesting();
    routing::StreetExplorationRoutingAnalytics::ResetForTesting();
    explorer_pro::UnfreezeConfigurationForTesting();
    explorer_pro::SetEntitlementSource(nullptr);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
    ClearConsent();
  }

  ~ProductAnalyticsGuard()
  {
    settings::Delete("RecordingSessionActive");
    FirstGoalTracker::ClearPersistedForTesting();
    ProductAnalytics::ResetForTesting();
    CompletionCardAnalytics::ResetForTesting();
    ExplorerProAnalytics::ResetForTesting();
    routing::StreetExplorationRoutingAnalytics::ResetForTesting();
    explorer_pro::UnfreezeConfigurationForTesting();
    explorer_pro::SetEntitlementSource(nullptr);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
    ClearConsent();
  }

private:
  static void ClearConsent()
  {
    settings::Delete("Explore.CompetitionEnabled");
    settings::Delete("Explore.AggregateSharingEnabled");
    settings::Delete("Explore.ConsentPolicyVersion");
    settings::Delete("Explore.ConsentUnixTime");
    settings::Delete("Explore.ConsentGiven");
    IdentityStore::SetCompetitionConsentGrantedHandler({});
  }
};

bool ContainsForbiddenLocationToken(std::string const & text)
{
  std::string const lower = strings::MakeLowerCase(text);
  std::string_view constexpr kForbidden[] = {
      "lat",      "lon",      "latitude", "longitude", "geometry", "polyline", "pixel",    "area",
      "coord",    "mwm",      "country",  "path",      "track",    "filename", "file",     "osm",
      "healpix",  "gps",      "geo:",     "home",      "route"};
  for (auto const token : kForbidden)
  {
    if (lower.find(token) != std::string::npos)
      return true;
  }
  return false;
}

bool PaJsonHasQuotedKey(std::string const & json, std::string_view key)
{
  std::string const needle = "\"" + std::string(key) + "\"";
  return json.find(needle) != std::string::npos;
}

std::vector<std::string> PaExtractQuotedJsonKeys(std::string const & json)
{
  std::vector<std::string> keys;
  for (size_t i = 0; i + 1 < json.size(); ++i)
  {
    if (json[i] != '"')
      continue;
    size_t j = i + 1;
    while (j < json.size() && json[j] != '"')
    {
      if (json[j] == '\\' && j + 1 < json.size())
        j += 2;
      else
        ++j;
    }
    if (j >= json.size())
      break;
    size_t k = j + 1;
    while (k < json.size() && (json[k] == ' ' || json[k] == '\n' || json[k] == '\t'))
      ++k;
    if (k < json.size() && json[k] == ':')
      keys.emplace_back(json.substr(i + 1, j - i - 1));
    i = j;
  }
  return keys;
}

bool StoredValueIsUint64(std::string_view key)
{
  uint64_t value = 0;
  if (!settings::Get(key, value))
    return false;
  std::string raw;
  if (!settings::StringStorage::Instance().GetValue(key, raw))
    return false;
  if (raw.empty())
    return false;
  for (char const c : raw)
  {
    if (!std::isdigit(static_cast<unsigned char>(c)))
      return false;
  }
  return true;
}

class LiveCollectFixture
{
public:
  LiveCollectFixture() : m_manager(m_dataSource)
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

  void Collect(int index)
  {
    m_manager.ResetSampleAcceptanceReference();
    m_manager.MarkInterpolationBarrier();
    m_manager.OnLocationUpdate(street_pixels_tests::MakeGpsInfo(
        50.0 + static_cast<double>(index), 10.0, 5.0, street_pixels_tests::CurrentTimestampSec() + index));
  }

  StreetPixelsManager & Manager() { return m_manager; }
  RecordingSession & Session() { return m_session; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  StreetPixelsManager m_manager;
};

std::string PaPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void PaRemove(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> PaLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput PaMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct PaAreaFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string dbPath;
  int64_t mapDataVersion = 42;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
};

PaAreaFixture MakePaAreaFixture(std::string const & leaf)
{
  PaAreaFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = PaPath(leaf + ".pix");
  fx.dbPath = PaPath(leaf + "_milestones.db");
  PaRemove(fx.spaPath);
  PaRemove(fx.pixPath);
  PaRemove(fx.dbPath);

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

  std::vector<street_pixels::ExplorationArea> areas;
  for (auto const & input : {PaMakeAdmin(10, 10, "District", PaLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             PaMakeAdmin(8, 8, "City", PaLonLatBox(24.0, 60.0, 25.0, 61.0))})
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
  std::vector<m2::PointD> samples;
  for (auto const & row : universeRows)
  {
    universeIds.push_back(row.first);
    samples.push_back(row.second);
  }

  street_pixels::SpaWriteParams params;
  params.m_mapDataVersion = fx.mapDataVersion;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);
  street_pixels_file::ExploredEverLiveMap seed{};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  fx.districtId = districtId;
  fx.cityOnlyId = cityOnlyId;
  fx.outsideId = outsideId;
  return fx;
}

void CleanupPaArea(PaAreaFixture const & fx)
{
  PaRemove(fx.spaPath);
  PaRemove(fx.pixPath);
  PaRemove(fx.dbPath);
  street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
}
}  // namespace

UNIT_TEST(ProductAnalytics_DefaultZero)
{
  ProductAnalyticsGuard guard;
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_positionPermissionGranted, 0, ());
  TEST_EQUAL(snapshot.m_notifyPermissionGranted, 0, ());
  TEST_EQUAL(snapshot.m_firstRecordingStarted, 0, ());
  TEST_EQUAL(snapshot.m_firstCollected, 0, ());
  TEST_EQUAL(snapshot.m_firstTenCollected, 0, ());
  TEST_EQUAL(snapshot.m_firstGoalComplete, 0, ());
  TEST_EQUAL(snapshot.m_firstRecordingCompleted, 0, ());
  TEST_EQUAL(snapshot.m_recordingSessions, 0, ());
  TEST_EQUAL(snapshot.m_newCollectedThisWeek, 0, ());
  TEST_EQUAL(snapshot.m_placesWithProgress, 0, ());
  TEST_EQUAL(snapshot.m_firstMilestone25, 0, ());
  TEST_EQUAL(snapshot.m_firstMilestone50, 0, ());
  TEST_EQUAL(snapshot.m_firstComplete, 0, ());
  TEST_EQUAL(snapshot.m_competitionPromptViewed, 0, ());
  TEST_EQUAL(snapshot.m_competitionOptIn, 0, ());
  TEST_EQUAL(snapshot.m_leadershipQualified, 0, ());
  TEST_EQUAL(snapshot.m_becameBoss, 0, ());
  TEST_EQUAL(snapshot.m_becameContested, 0, ());
  TEST_EQUAL(snapshot.m_becameUnclaimed, 0, ());
  TEST_EQUAL(snapshot.m_weeklyBoardUsed, 0, ());
}

UNIT_TEST(ProductAnalytics_PermissionAndPromptOnceOrCount)
{
  ProductAnalyticsGuard guard;
  ProductAnalytics::RecordPositionPermissionGranted();
  ProductAnalytics::RecordPositionPermissionGranted();
  ProductAnalytics::RecordNotifyPermissionGranted();
  ProductAnalytics::RecordNotifyPermissionGranted();
  ProductAnalytics::RecordCompetitionPromptViewed();
  ProductAnalytics::RecordCompetitionPromptViewed();
  ProductAnalytics::RecordWeeklyBoardUsed();
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_positionPermissionGranted, 1, ());
  TEST_EQUAL(snapshot.m_notifyPermissionGranted, 1, ());
  TEST_EQUAL(snapshot.m_competitionPromptViewed, 2, ());
  TEST_EQUAL(snapshot.m_weeklyBoardUsed, 1, ());
}

UNIT_TEST(ProductAnalytics_RecordingStartAndFinish)
{
  ProductAnalyticsGuard guard;
  RecordingSession session;
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstRecordingStarted, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_recordingSessions, 1, ());
  TEST_EQUAL(session.Finish(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstRecordingCompleted, 1, ());
  TEST_EQUAL(session.Reset(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstRecordingStarted, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_recordingSessions, 2, ());
  TEST_EQUAL(session.Discard(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstRecordingCompleted, 1, ());
}

UNIT_TEST(ProductAnalytics_LiveCollectFirstTenAndGoal)
{
  ProductAnalyticsGuard guard;
  LiveCollectFixture fixture;
  fixture.SetupUnexplored(12);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Collect(0);
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstCollected, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstTenCollected, 0, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstGoalComplete, 0, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_newCollectedThisWeek, 1, ());
  for (int i = 1; i < 9; ++i)
    fixture.Collect(i);
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstTenCollected, 0, ());
  fixture.Collect(9);
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstCollected, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstTenCollected, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstGoalComplete, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_newCollectedThisWeek, 10, ());
  fixture.Collect(10);
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstTenCollected, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstGoalComplete, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_newCollectedThisWeek, 11, ());
}

UNIT_TEST(ProductAnalytics_ImportDoesNotIncrementLiveEvents)
{
  ProductAnalyticsGuard guard;
  LiveCollectFixture fixture;
  fixture.SetupUnexplored(3);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().MarkImportedPixelsForTesting({fixture.PixelAt(0), fixture.PixelAt(1)});
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_firstCollected, 0, ());
  TEST_EQUAL(snapshot.m_firstTenCollected, 0, ());
  TEST_EQUAL(snapshot.m_firstGoalComplete, 0, ());
  TEST_EQUAL(snapshot.m_newCollectedThisWeek, 0, ());
  TEST_EQUAL(snapshot.m_placesWithProgress, 0, ());
  TEST_EQUAL(snapshot.m_firstMilestone25, 0, ());
  TEST_EQUAL(snapshot.m_firstComplete, 0, ());
  TEST_EQUAL(snapshot.m_leadershipQualified, 0, ());
  TEST_EQUAL(snapshot.m_becameBoss, 0, ());
}

UNIT_TEST(ProductAnalytics_WeekBucketResetsOnNewWeek)
{
  ProductAnalyticsGuard guard;
  ProductAnalytics::RecordLiveCollectedAt(3, 0);
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_newCollectedThisWeek, 3, ());
  ProductAnalytics::RecordLiveCollectedAt(2, kWeeklyCitySecondsPerWeek);
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_newCollectedThisWeek, 2, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstCollected, 1, ());
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstTenCollected, 0, ());
}

UNIT_TEST(ProductAnalytics_CompetitionOptInAndOnceFlags)
{
  ProductAnalyticsGuard guard;
  IdentityStore::GrantCompetitionConsent();
  IdentityStore::GrantCompetitionConsent();
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_competitionOptIn, 2, ());
  ProductAnalytics::RecordLeadershipQualified();
  ProductAnalytics::RecordLeadershipQualified();
  ProductAnalytics::RecordBecameBoss();
  ProductAnalytics::RecordBecameContested();
  ProductAnalytics::RecordBecameUnclaimed();
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_leadershipQualified, 1, ());
  TEST_EQUAL(snapshot.m_becameBoss, 1, ());
  TEST_EQUAL(snapshot.m_becameContested, 1, ());
  TEST_EQUAL(snapshot.m_becameUnclaimed, 1, ());
}

UNIT_TEST(ProductAnalytics_ImportDoesNotFireMilestones)
{
  ProductAnalyticsGuard guard;
  auto fx = MakePaAreaFixture("sp091_import_ms");
  FrozenDataSource dataSource;
  RecordingSession session;
  StreetPixelsManager manager(dataSource);
  manager.SetRecordingSession(&session);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  manager.SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, false),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false),
  });
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  manager.MarkImportedPixelsForTesting({fx.cityOnlyId});
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_firstCollected, 0, ());
  TEST_EQUAL(snapshot.m_placesWithProgress, 0, ());
  TEST_EQUAL(snapshot.m_firstMilestone25, 0, ());
  TEST_EQUAL(snapshot.m_firstMilestone50, 0, ());
  TEST_EQUAL(snapshot.m_firstComplete, 0, ());
  TEST_EQUAL(snapshot.m_newCollectedThisWeek, 0, ());
  CleanupPaArea(fx);
}

UNIT_TEST(ProductAnalytics_LiveCollectionFiresMilestones)
{
  ProductAnalyticsGuard guard;
  auto fx = MakePaAreaFixture("sp091_live_ms");
  FrozenDataSource dataSource;
  RecordingSession session;
  StreetPixelsManager manager(dataSource);
  manager.SetRecordingSession(&session);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  manager.SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, false),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, false),
  });
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(fx.cityOnlyId);
  manager.ResetSampleAcceptanceReference();
  manager.MarkInterpolationBarrier();
  manager.OnLocationUpdate(street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, street_pixels_tests::CurrentTimestampSec()));
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_firstCollected, 1, ());
  TEST_EQUAL(snapshot.m_placesWithProgress, 1, ());
  TEST_EQUAL(snapshot.m_firstMilestone25, 1, ());
  TEST_EQUAL(snapshot.m_firstMilestone50, 1, ());
  TEST_EQUAL(snapshot.m_firstComplete, 1, ());
  CleanupPaArea(fx);
}

UNIT_TEST(ProductAnalytics_StoresUint64Only)
{
  ProductAnalyticsGuard guard;
  ProductAnalytics::RecordPositionPermissionGranted();
  ProductAnalytics::RecordLiveCollectedAt(4, 100);
  ProductAnalytics::RecordPlacesWithProgress();
  ProductAnalytics::RecordCompetitionPromptViewed();
  std::string_view const keys[] = {
      ProductAnalytics::kPositionPermissionGrantedKey, ProductAnalytics::kFirstCollectedKey,
      ProductAnalytics::kNewCollectedThisWeekKey,      ProductAnalytics::kLiveCollectedTotalKey,
      ProductAnalytics::kPlacesWithProgressKey,        ProductAnalytics::kCompetitionPromptViewedKey,
      ProductAnalytics::kNewCollectedWeekIdKey};
  for (auto const key : keys)
  {
    TEST(StoredValueIsUint64(key), (std::string(key)));
    TEST(!ContainsForbiddenLocationToken(std::string(key)), (std::string(key)));
  }
}

UNIT_TEST(ProductAnalytics_SnapshotHasNoLocationKeys)
{
  ProductAnalyticsGuard guard;
  ProductAnalytics::RecordPositionPermissionGranted();
  ProductAnalytics::RecordNotifyPermissionGranted();
  ProductAnalytics::RecordLiveCollectedAt(10, 0);
  ProductAnalytics::RecordFirstGoalComplete();
  ProductAnalytics::RecordPlacesWithProgress();
  ProductAnalytics::RecordFirstMilestone25();
  ProductAnalytics::RecordFirstComplete();
  ProductAnalytics::RecordCompetitionPromptViewed();
  ProductAnalytics::RecordCompetitionOptIn();
  ProductAnalytics::RecordLeadershipQualified();
  ProductAnalytics::RecordBecameBoss();
  ProductAnalytics::RecordWeeklyBoardUsed();
  auto const serialized = ProductAnalytics::SerializedSnapshot();
  TEST_EQUAL(serialized.size(), kProductAnalyticsCounterCount, ());
  for (auto const & entry : serialized)
  {
    std::string const name(entry.first);
    TEST_EQUAL(strings::MakeLowerCase(name), name, ());
    TEST(!ContainsForbiddenLocationToken(name), (name));
  }
  std::string const debug = DebugPrint(ProductAnalytics::LoadSnapshot());
  TEST_EQUAL(strings::MakeLowerCase(debug), debug, ());
  TEST(!ContainsForbiddenLocationToken(debug), (debug));
}

UNIT_TEST(ProductAnalytics_ExistingGrowthHasNoAreaId)
{
  ProductAnalyticsGuard guard;
  CompletionCardAnalytics::RecordGenerated();
  CompletionCardAnalytics::RecordShareInitiated();
  auto const serialized = CompletionCardAnalytics::SerializedSnapshot();
  for (auto const & entry : serialized)
  {
    TEST(!ContainsForbiddenLocationToken(std::string(entry.first)), (std::string(entry.first)));
    TEST(!ContainsForbiddenLocationToken(std::string(CompletionCardAnalytics::kCardGeneratedKey)), ());
    TEST(!ContainsForbiddenLocationToken(std::string(CompletionCardAnalytics::kShareInitiatedKey)), ());
  }
}

UNIT_TEST(ProductAnalytics_ProCountersAbsentWhenUnavailable)
{
  ProductAnalyticsGuard guard;
  explorer_pro::UnfreezeConfigurationForTesting();
  explorer_pro::SetEntitlementSource(nullptr);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
  ExplorerProAnalytics::RecordInfoPageViewed();
  ExplorerProAnalytics::RecordGpxImportUsage();
  ExplorerProAnalytics::RecordGpxExportUsage();
  ExplorerProAnalyticsSnapshot const snapshot = ExplorerProAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_infoPageViewed, 0, ());
  TEST_EQUAL(snapshot.m_gpxImportUsage, 0, ());
  TEST_EQUAL(snapshot.m_gpxExportUsage, 0, ());
}

UNIT_TEST(ProductAnalytics_ReleaseUploadPayloadsHaveNoLocation)
{
  ProductAnalyticsGuard guard;
  CompetitionUploadPayload payload;
  payload.m_profileId = "profile-sample";
  payload.m_nickname = "Alice_1";
  payload.m_mapDataVersion = 340;
  payload.m_scoreCalcVersion = 1;
  payload.m_lastUpdateUnix = 1770000000;
  CompetitionUploadArea area;
  area.m_areaOsmId = 10;
  area.m_ownershipScore = 100.0;
  area.m_liveCoveragePct = 100.0;
  area.m_eligible = true;
  payload.m_areas.push_back(area);
  CompetitionUploadWeeklyCity city;
  city.m_cityOsmId = 20;
  city.m_newLiveCount = 3;
  payload.m_weeklyCities.push_back(city);
  std::string const json = SerializeCompetitionUploadPayload(payload);
  std::set<std::string> const allowKeys = {
      "profile_id",        "nickname",        "map_data_version", "score_calc_version", "last_update_unix",
      "areas",             "area_osm_id",     "ownership_score",  "live_coverage_pct",  "eligible",
      "weekly_cities",     "city_osm_id",     "new_live_count"};
  auto const keys = PaExtractQuotedJsonKeys(json);
  TEST(!keys.empty(), ());
  for (auto const & key : keys)
    TEST(allowKeys.count(key) == 1, (key, json));
  std::string_view constexpr kDeny[] = {"lat",      "lon",      "latitude", "longitude", "gps",
                                        "track",    "home",     "polyline", "route",     "geometry",
                                        "healpix",  "pixel_id", "session"};
  for (auto const key : kDeny)
    TEST(!PaJsonHasQuotedKey(json, key), (key, json));
  TEST_EQUAL(json.find("Explore."), std::string::npos, (json));
  TEST_EQUAL(json.find("CardGenerated"), std::string::npos, (json));
  TEST_EQUAL(json.find("prefer-used"), std::string::npos, (json));

  ProductAnalytics::RecordLiveCollectedAt(1, 0);
  CompletionCardAnalytics::RecordGenerated();
  routing::StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(
      routing::StreetExplorationRoutingMode::Prefer);
  for (auto const & entry : ProductAnalytics::SerializedSnapshot())
    TEST(!ContainsForbiddenLocationToken(std::string(entry.first)), (std::string(entry.first)));
  for (auto const & entry : CompletionCardAnalytics::SerializedSnapshot())
    TEST(!ContainsForbiddenLocationToken(std::string(entry.first)), (std::string(entry.first)));
  for (auto const & entry : routing::StreetExplorationRoutingAnalytics::SerializedSnapshot())
    TEST(!ContainsForbiddenLocationToken(std::string(entry.first)), (std::string(entry.first)));
}

UNIT_TEST(ProductAnalytics_ResetIsolatesTests)
{
  ProductAnalyticsGuard guard;
  ProductAnalytics::RecordLiveCollectedAt(10, 0);
  ProductAnalytics::RecordPlacesWithProgress();
  TEST_EQUAL(ProductAnalytics::LoadSnapshot().m_firstCollected, 1, ());
  ProductAnalytics::ResetForTesting();
  ProductAnalyticsSnapshot const snapshot = ProductAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_firstCollected, 0, ());
  TEST_EQUAL(snapshot.m_newCollectedThisWeek, 0, ());
  TEST_EQUAL(snapshot.m_placesWithProgress, 0, ());
  uint64_t value = 0;
  TEST(!settings::Get(ProductAnalytics::kLiveCollectedTotalKey, value), ());
  TEST(!settings::Get(ProductAnalytics::kNewCollectedWeekIdKey, value), ());
}
