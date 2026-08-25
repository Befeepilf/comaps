#include "testing/testing.hpp"

#include "map/backend_config.hpp"
#include "map/competition_upload_payload.hpp"
#include "map/competition_upload_service.hpp"
#include "map/identity_store.hpp"
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
#include "platform/secure_storage.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
bool JsonHasQuotedKey(std::string const & json, std::string_view key)
{
  std::string const needle = "\"" + std::string(key) + "\"";
  return json.find(needle) != std::string::npos;
}

std::vector<std::string> ExtractQuotedJsonKeys(std::string const & json)
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

CompetitionUploadPayload SamplePayload()
{
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
  return payload;
}

std::set<std::string> const kAllowKeys = {
    "profile_id",         "nickname",         "map_data_version", "score_calc_version",
    "last_update_unix",   "areas",            "area_osm_id",      "ownership_score",
    "live_coverage_pct",  "eligible",         "weekly_cities",    "city_osm_id",
    "new_live_count"};

std::string_view constexpr kDenyKeys[] = {
    "lat",           "lon",         "latitude",          "longitude", "gps",        "track",
    "tracks",        "healpix",     "pixel_id",          "pixelId",   "deviceId",   "advertising",
    "gaid",          "idfa",        "aaid",              "friend",    "friends",    "user_id",
    "unique_live",   "uniqueLivePixels", "explored_pixels", "regionId", "weekStart", "speed",
    "bearing",       "accuracy",    "mercator",          "coord",     "live_movement", "session",
    "week_id"};

void ClearCompetitionUploadSettings()
{
  settings::Delete("Explore.CompetitionUploadPending");
  settings::Delete("Explore.CompetitionLastSuccessUnix");
  settings::Delete("Explore.CompetitionNextAllowedUnix");
  backend::SetApiBaseUrl("");
  settings::Delete("Explore.ConsentGiven");
  settings::Delete("Explore.CompetitionEnabled");
  settings::Delete("Explore.AggregateSharingEnabled");
  settings::Delete("Explore.ConsentPolicyVersion");
  settings::Delete("Explore.ConsentUnixTime");
  settings::Delete("Explore.Username");
  settings::Delete("Explore.NicknameDraft");
  settings::Delete("Explore.NicknameLastChangedUnix");
  IdentityStore::SetNicknameClaimHandlerForTesting({});
  IdentityStore::SetCompetitionConsentGrantedHandler({});
  platform::SecureStorage storage;
  storage.Remove("Explore.DeviceId");
}

class ScopedCompetitionUpload
{
public:
  ScopedCompetitionUpload() { ClearCompetitionUploadSettings(); }
  ~ScopedCompetitionUpload() { ClearCompetitionUploadSettings(); }
};

CompetitionUploadPayload LiveSnapshot()
{
  CompetitionUploadPayload payload;
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
  return payload;
}

bool HasForbiddenHeader(CompetitionUploadService::Headers const & headers)
{
  for (auto const & header : headers)
  {
    std::string name = header.first;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (name == "x-device-id" || name == "x-username")
      return true;
    if (name.find("friend") != std::string::npos)
      return true;
  }
  return false;
}

bool DeviceIdExists()
{
  std::string id;
  platform::SecureStorage storage;
  return storage.Load("Explore.DeviceId", id) && !id.empty();
}

struct UploadHarness
{
  ScopedCompetitionUpload scoped;
  int64_t now = 0;
  int64_t jitter = 0;
  bool connected = true;
  CompetitionUploadPayload snapshot = LiveSnapshot();
  int posts = 0;
  std::string lastUrl;
  std::string lastBody;
  CompetitionUploadService::Headers lastHeaders;
  int status = 200;

  CompetitionUploadService MakeService()
  {
    return CompetitionUploadService(
        [this]() { return now; }, [this]() { return jitter; }, [this]() { return connected; },
        [this](std::string const & url, std::string const & body,
               CompetitionUploadService::Headers const & headers)
        {
          ++posts;
          lastUrl = url;
          lastBody = body;
          lastHeaders = headers;
          return status;
        },
        [this](int64_t) { return snapshot; });
  }

  void GrantConsentAndUsername()
  {
    IdentityStore::GrantCompetitionConsent();
    IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
    TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  }

  void ConfigureApi() { backend::SetApiBaseUrl("https://example.com/api"); }

  bool Pending() const
  {
    bool pending = false;
    settings::Get(std::string_view("Explore.CompetitionUploadPending"), pending);
    return pending;
  }

  uint64_t NextAllowed() const
  {
    uint64_t value = 0;
    settings::Get(std::string_view("Explore.CompetitionNextAllowedUnix"), value);
    return value;
  }
};

std::string Sp074Path(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void Sp074Remove(std::string const & path) { Platform::RemoveFileIfExists(path); }

void Sp074RemoveDb(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> Sp074LonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput Sp074MakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct Sp074AreaFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string spxPath;
  int64_t mapDataVersion = 42;
  uint32_t policyVersion = 1;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
};

Sp074AreaFixture MakeSp074AreaFixture(std::string const & leaf)
{
  Sp074AreaFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = Sp074Path(leaf + ".pix");
  fx.spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), leaf);
  Sp074Remove(fx.spaPath);
  Sp074Remove(fx.pixPath);
  Sp074Remove(fx.spxPath);

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
  for (auto const & input : {Sp074MakeAdmin(10, 10, "District", Sp074LonLatBox(24.2, 60.2, 24.8, 60.8)),
                             Sp074MakeAdmin(8, 8, "City", Sp074LonLatBox(24.0, 60.0, 25.0, 61.0))})
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
  params.m_policyVersion = fx.policyVersion;
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, samples, policy, params);
  street_pixels_file::ExploredEverLiveMap seed{{districtId, false}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  fx.districtId = districtId;
  fx.cityOnlyId = cityOnlyId;
  fx.outsideId = outsideId;
  return fx;
}

void CleanupSp074Area(Sp074AreaFixture const & fx)
{
  Sp074Remove(fx.spaPath);
  Sp074Remove(fx.pixPath);
  Sp074Remove(fx.spxPath);
}

class SnapshotManagerFixture
{
public:
  SnapshotManagerFixture()
    : m_weeklyPath(Sp074Path("sp074_weekly_city_live.db"))
    , m_recencyPath(Sp074Path("sp074_live_recency.db"))
    , m_manager(m_dataSource)
  {
    Sp074RemoveDb(m_weeklyPath);
    Sp074RemoveDb(m_recencyPath);
    m_manager.ConfigureWeeklyCityLiveStoreForTesting(m_weeklyPath);
    m_manager.ConfigureLiveRecencyStoreForTesting(m_recencyPath);
    m_manager.SetRecordingSession(&m_session);
  }

  ~SnapshotManagerFixture()
  {
    Sp074RemoveDb(m_weeklyPath);
    Sp074RemoveDb(m_recencyPath);
  }

  StreetPixelsManager & Manager() { return m_manager; }

private:
  FrozenDataSource m_dataSource;
  RecordingSession m_session;
  std::string m_weeklyPath;
  std::string m_recencyPath;
  StreetPixelsManager m_manager;
};
}  // namespace

UNIT_TEST(CompetitionUpload_PayloadAllowList)
{
  std::string const json = SerializeCompetitionUploadPayload(SamplePayload());
  auto const keys = ExtractQuotedJsonKeys(json);
  TEST(!keys.empty(), ());
  std::set<std::string> seen;
  for (auto const & key : keys)
  {
    TEST(kAllowKeys.count(key) == 1, (key, json));
    seen.insert(key);
  }
  for (auto const & allowed : kAllowKeys)
    TEST(seen.count(allowed) == 1, (allowed, json));
  TEST_EQUAL(json.find("uniqueLivePixels"), std::string::npos, (json));
  TEST_EQUAL(json.find("unique_live"), std::string::npos, (json));
}

UNIT_TEST(CompetitionUpload_PayloadDenyList)
{
  std::string const json = SerializeCompetitionUploadPayload(SamplePayload());
  TEST(JsonHasQuotedKey(json, "last_update_unix"), (json));
  TEST(!JsonHasQuotedKey(json, "lat"), (json));
  TEST(!JsonHasQuotedKey(json, "lon"), (json));
  for (auto const key : kDenyKeys)
    TEST(!JsonHasQuotedKey(json, key), (key, json));
}

UNIT_TEST(CompetitionUpload_MarkPendingDoesNotPost)
{
  UploadHarness h;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  service.MarkPending();
  TEST_EQUAL(h.posts, 0, ());
  TEST(h.Pending(), ());
  TEST_EQUAL(h.NextAllowed(), 900u, ());
  TEST(!DeviceIdExists(), ());
}

UNIT_TEST(CompetitionUpload_CadenceJitterZero)
{
  UploadHarness h;
  h.jitter = 0;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  TEST_EQUAL(h.posts, 0, ());
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(!DeviceIdExists(), ());
  h.now = 899;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
  TEST_EQUAL(h.lastUrl, backend::GetCompetitionAggregatesUrl(), ());
  TEST(h.lastUrl.find("/stats/upload") == std::string::npos, (h.lastUrl));
  TEST(!h.Pending(), ());
  TEST(DeviceIdExists(), ());
  TEST(!HasForbiddenHeader(h.lastHeaders), ());
  for (auto const key : kDenyKeys)
    TEST(!JsonHasQuotedKey(h.lastBody, key), (key, h.lastBody));
  h.now = 901;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
  service.MarkPending();
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
  h.now = 1800;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 2, ());
}

UNIT_TEST(CompetitionUpload_CadenceJitter900)
{
  UploadHarness h;
  h.jitter = 900;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  TEST_EQUAL(h.NextAllowed(), 1800u, ());
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  h.now = 1799;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  h.now = 1800;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
}

UNIT_TEST(CompetitionUpload_OptOutZeroHttp)
{
  UploadHarness h;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  IdentityStore::RevokeCompetitionConsent();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(!h.Pending(), ());
}

UNIT_TEST(CompetitionUpload_EmptyApiBaseZeroHttp)
{
  UploadHarness h;
  h.GrantConsentAndUsername();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(h.Pending(), ());
  TEST(!DeviceIdExists(), ());
}

UNIT_TEST(CompetitionUpload_OfflineThenFlush)
{
  UploadHarness h;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  h.connected = false;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(h.Pending(), ());
  TEST(!DeviceIdExists(), ());
  h.connected = true;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
  TEST(!h.Pending(), ());
}

UNIT_TEST(CompetitionUpload_ConsentGivenBooleanOnlyZeroHttp)
{
  UploadHarness h;
  h.ConfigureApi();
  settings::Set("Explore.ConsentGiven", true);
  IdentityStore::SetNicknameClaimHandlerForTesting([](std::string_view) { return 200; });
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Ok, ());
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(!IdentityStore::HasCompetitionConsent(), ());
}

UNIT_TEST(CompetitionUpload_NicknameDraftOnlyZeroHttp)
{
  UploadHarness h;
  IdentityStore::GrantCompetitionConsent();
  TEST(IdentityStore::TryClaimNickname("Alice_1") == IdentityStore::NicknameClaimResult::Unavailable, ());
  TEST(!IdentityStore::HasUsername(), ());
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(h.Pending(), ());
  TEST(!DeviceIdExists(), ());
}

UNIT_TEST(CompetitionUpload_NoFriendsHeaders)
{
  UploadHarness h;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
  TEST(!HasForbiddenHeader(h.lastHeaders), ());
  TEST(h.lastHeaders.empty(), ());
  TEST(!JsonHasQuotedKey(h.lastBody, "friend"), (h.lastBody));
  TEST(!JsonHasQuotedKey(h.lastBody, "friends"), (h.lastBody));
  TEST(!JsonHasQuotedKey(h.lastBody, "user_id"), (h.lastBody));
  TEST(!JsonHasQuotedKey(h.lastBody, "deviceId"), (h.lastBody));
}

UNIT_TEST(CompetitionUpload_StatsUploadUrlNeverUsed)
{
  UploadHarness h;
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 1, ());
  TEST_EQUAL(h.lastUrl, "https://example.com/api/v1/competition/aggregates", ());
  TEST(h.lastUrl.find("/stats/upload") == std::string::npos, (h.lastUrl));
  TEST_EQUAL(h.lastUrl.find(backend::GetStatsUploadUrl()), std::string::npos, (h.lastUrl));
  TEST(h.lastUrl != backend::GetStatsUploadUrl(), ());
}

UNIT_TEST(CompetitionUpload_EmptySnapshotSkipsHttpKeepsPending)
{
  UploadHarness h;
  h.snapshot = CompetitionUploadPayload{};
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(h.Pending(), ());
  TEST(!DeviceIdExists(), ());
}

UNIT_TEST(CompetitionUpload_SnapshotLiveOnlyOmitsUniqueCountsAndWeekId)
{
  SnapshotManagerFixture fixture;
  auto fx = MakeSp074AreaFixture("sp074_live_snap");
  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true, true),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, true, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, true, false),
  });
  int64_t const nowUnix = 1770000000;
  street_pixels::WeeklyCityLiveStore::Instance().RecordFirstLive({20, 8}, nowUnix);
  street_pixels::WeeklyCityLiveStore::Instance().RecordFirstLive({8}, nowUnix);

  auto const payload = fixture.Manager().BuildCompetitionUploadSnapshot(nowUnix);
  TEST(!payload.m_areas.empty(), ());
  TEST_EQUAL(payload.m_mapDataVersion, fx.mapDataVersion, ());
  TEST_EQUAL(payload.m_scoreCalcVersion, street_pixels::kScoreCalcVersion, ());
  for (size_t i = 1; i < payload.m_areas.size(); ++i)
    TEST(payload.m_areas[i - 1].m_areaOsmId < payload.m_areas[i].m_areaOsmId, ());
  TEST(!payload.m_weeklyCities.empty(), ());
  for (size_t i = 1; i < payload.m_weeklyCities.size(); ++i)
    TEST(payload.m_weeklyCities[i - 1].m_cityOsmId < payload.m_weeklyCities[i].m_cityOsmId, ());
  for (auto const & city : payload.m_weeklyCities)
    TEST(city.m_newLiveCount > 0, ());

  std::string const json = SerializeCompetitionUploadPayload(payload);
  TEST(!JsonHasQuotedKey(json, "uniqueLivePixels"), (json));
  TEST(!JsonHasQuotedKey(json, "unique_live"), (json));
  TEST(!JsonHasQuotedKey(json, "week_id"), (json));
  TEST(!JsonHasQuotedKey(json, "weekStart"), (json));
  TEST(!JsonHasQuotedKey(json, "healpix"), (json));
  TEST(JsonHasQuotedKey(json, "area_osm_id"), (json));
  TEST(JsonHasQuotedKey(json, "city_osm_id"), (json));
  TEST(JsonHasQuotedKey(json, "new_live_count"), (json));
  CleanupSp074Area(fx);
}

UNIT_TEST(CompetitionUpload_ImportedOnlyZeroCompetitiveHttp)
{
  SnapshotManagerFixture fixture;
  auto fx = MakeSp074AreaFixture("sp074_imported_only");
  TEST(fixture.Manager().RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  fixture.Manager().SetStreetPixelsForTesting({
      street_pixels_tests::MakeStreetPixel(fx.districtId, true, false),
      street_pixels_tests::MakeStreetPixel(fx.cityOnlyId, true, false),
      street_pixels_tests::MakeStreetPixel(fx.outsideId, true, false),
  });
  fixture.Manager().MarkImportedPixelsForTesting({fx.cityOnlyId, fx.outsideId});

  UploadHarness h;
  h.snapshot = fixture.Manager().BuildCompetitionUploadSnapshot(1770000000);
  TEST(CompetitionUploadPayloadIsEmpty(h.snapshot), ());
  h.GrantConsentAndUsername();
  h.ConfigureApi();
  auto service = h.MakeService();
  h.now = 0;
  service.MarkPending();
  h.now = 900;
  service.MaybeUpload();
  TEST_EQUAL(h.posts, 0, ());
  TEST(h.Pending(), ());
  TEST(!DeviceIdExists(), ());
  CleanupSp074Area(fx);
}


