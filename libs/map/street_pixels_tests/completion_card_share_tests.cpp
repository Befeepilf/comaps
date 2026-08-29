#include "testing/testing.hpp"

#include "map/completion_card_analytics.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/completion_card.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
class ShAmAnalyticsGuard
{
public:
  ShAmAnalyticsGuard() { street_pixels::CompletionCardAnalytics::ResetForTesting(); }
  ~ShAmAnalyticsGuard() { street_pixels::CompletionCardAnalytics::ResetForTesting(); }
};

bool ShAmContainsForbiddenKey(std::string const & text)
{
  std::string const lower = strings::MakeLowerCase(text);
  std::string_view constexpr kForbidden[] = {
      "lat",      "lon",      "latitude", "longitude", "geometry", "polyline", "pixel", "area",
      "coord",    "mwm",      "country",  "osm",       "healpix",  "gps",      "geo:",  "ge0"};
  for (auto const token : kForbidden)
  {
    if (lower.find(token) != std::string::npos)
      return true;
  }
  return false;
}

bool ShAmContainsForbiddenText(std::string const & text)
{
  std::string const lower = strings::MakeLowerCase(text);
  for (auto const & token : street_pixels::CompletionCardDeniedKeys())
  {
    if (lower.find(strings::MakeLowerCase(token)) != std::string::npos)
      return true;
  }
  return false;
}

std::string ShAmPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void ShAmRemove(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> ShAmLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput ShAmMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct ShAmFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string dbPath;
  int64_t mapDataVersion = 42;
};

ShAmFixture MakeShAmFixture(std::string const & leaf)
{
  ShAmFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = ShAmPath(leaf + ".pix");
  fx.dbPath = ShAmPath(leaf + "_milestones.db");
  ShAmRemove(fx.spaPath);
  ShAmRemove(fx.pixPath);
  ShAmRemove(fx.dbPath);

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
  for (auto const & input : {ShAmMakeAdmin(10, 10, "District", ShAmLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             ShAmMakeAdmin(8, 8, "City", ShAmLonLatBox(24.0, 60.0, 25.0, 61.0))})
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
  street_pixels_file::ExploredEverLiveMap seed{{districtId, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  return fx;
}

void CleanupShAm(ShAmFixture const & fx)
{
  ShAmRemove(fx.spaPath);
  ShAmRemove(fx.pixPath);
  ShAmRemove(fx.dbPath);
  street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
}
}  // namespace

UNIT_TEST(CompletionCardShare_AnalyticsDefaultZero)
{
  ShAmAnalyticsGuard guard;
  auto const snapshot = street_pixels::CompletionCardAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_generated, 0, ());
  TEST_EQUAL(snapshot.m_shareInitiated, 0, ());
}

UNIT_TEST(CompletionCardShare_AnalyticsKeysHaveNoLocationOrArea)
{
  ShAmAnalyticsGuard guard;
  street_pixels::CompletionCardAnalytics::RecordGenerated();
  std::string const keys[] = {std::string(street_pixels::CompletionCardAnalytics::kCardGeneratedKey),
                              std::string(street_pixels::CompletionCardAnalytics::kShareInitiatedKey)};
  for (auto const & key : keys)
    TEST(!ShAmContainsForbiddenKey(key), (key));
  auto const serialized = street_pixels::CompletionCardAnalytics::SerializedSnapshot();
  TEST_EQUAL(serialized.size(), 2, ());
  for (auto const & entry : serialized)
  {
    std::string const name(entry.first);
    TEST_EQUAL(strings::MakeLowerCase(name), name, ());
    TEST(!ShAmContainsForbiddenKey(name), (name));
  }
  std::string const debug = DebugPrint(street_pixels::CompletionCardAnalytics::LoadSnapshot());
  TEST_EQUAL(strings::MakeLowerCase(debug), debug, ());
  TEST(!ShAmContainsForbiddenKey(debug), (debug));
}

UNIT_TEST(CompletionCardShare_GeneratedIncrementsOnDisplayGet)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_gen");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  manager.SetCompletionCardGeneratedHandler([] { street_pixels::CompletionCardAnalytics::RecordGenerated(); });
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_generated, 0, ());
  TEST(manager.GetCompletionCardForCurrentPresentation(true).has_value(), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_generated, 1, ());
  TEST(manager.GetCompletionCardForCurrentPresentation(false).has_value(), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_generated, 1, ());
  TEST(manager.GetCompletionCardForCurrentPresentation(true).has_value(), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_generated, 2, ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_shareInitiated, 0, ());
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_ShareIncrementsOnlyOnRecord)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_share");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_shareInitiated, 0, ());
  manager.RecordCompletionCardShareInitiated();
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_shareInitiated, 1, ());
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_PrepareUsesTransientPngNotTrack)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_png");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto payload = manager.PrepareCompletionCardShare();
  TEST(payload.has_value(), ());
  TEST_EQUAL(payload->m_path, street_pixels::CompletionCardTransientPath(), ());
  TEST(payload->m_path.find(GetPlatform().TmpDir()) == 0, (payload->m_path));
  TEST(payload->m_path.size() >= 4 && payload->m_path.substr(payload->m_path.size() - 4) == ".png", (payload->m_path));
  TEST(payload->m_path.find(".kml") == std::string::npos, ());
  TEST(payload->m_path.find(".gpx") == std::string::npos, ());
  TEST(payload->m_path.find(".kmz") == std::string::npos, ());
  TEST_EQUAL(payload->m_mimeType, street_pixels::kCompletionCardShareMime, ());
  TEST(Platform::IsFileExistsByFullPath(payload->m_path), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_shareInitiated, 0, ());
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_IncludesStoredDate)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_date_off");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  auto rec = manager.GetAreaMilestoneRecord(peek->m_osmId);
  TEST(rec.has_value() && rec->m_completed100At.has_value(), ());
  auto payload = manager.PrepareCompletionCardShare();
  TEST(payload.has_value(), ());
  TEST(!ShAmContainsForbiddenText(payload->m_text), (payload->m_text));
  auto card = manager.GetCompletionCardForCurrentPresentation(false);
  TEST(card.has_value(), ());
  TEST(card->m_completedDate.has_value(), ());
  TEST_EQUAL(card->m_completedDate->size(), 10u, ());
  TEST(payload->m_text.find(*card->m_completedDate) != std::string::npos, (payload->m_text));
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_ShareTextContainsIsoDate)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_date_on");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto payload = manager.PrepareCompletionCardShare();
  TEST(payload.has_value(), ());
  auto card = manager.GetCompletionCardForCurrentPresentation(false);
  TEST(card.has_value(), ());
  TEST(card->m_completedDate.has_value(), ());
  TEST_EQUAL(card->m_completedDate->size(), 10u, ());
  TEST_EQUAL(std::count(card->m_completedDate->begin(), card->m_completedDate->end(), '-'), 2, ());
  TEST(card->m_completedDate->find('T') == std::string::npos, ());
  TEST(card->m_completedDate->find(':') == std::string::npos, ());
  TEST(payload->m_text.find(*card->m_completedDate) != std::string::npos, (payload->m_text));
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_TextHasNoCoordinates)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_text");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto payload = manager.PrepareCompletionCardShare();
  TEST(payload.has_value(), ());
  TEST(!ShAmContainsForbiddenText(payload->m_text), (payload->m_text));
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_PrepareFailsWithoutHundredPercent)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp068_p50");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(manager.PrepareCompletionCardShare().has_value(), ());
  manager.AcknowledgeAreaMilestonePresentation();
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());
  TEST(!manager.PrepareCompletionCardShare().has_value(), ());
  TEST_EQUAL(street_pixels::CompletionCardAnalytics::LoadSnapshot().m_shareInitiated, 0, ());
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_AcknowledgeKeepsPngWhileShareInFlight)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp089_png_share");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto payload = manager.PrepareCompletionCardShare();
  TEST(payload.has_value(), ());
  TEST(Platform::IsFileExistsByFullPath(payload->m_path), ());
  manager.RecordCompletionCardShareInitiated();
  manager.AcknowledgeAreaMilestonePresentation();
  TEST(Platform::IsFileExistsByFullPath(payload->m_path), ());
  manager.ReleaseCompletionCardShare();
  TEST(!Platform::IsFileExistsByFullPath(payload->m_path), ());
  CleanupShAm(fx);
}

UNIT_TEST(CompletionCardShare_AcknowledgeDeletesPngWithoutShare)
{
  ShAmAnalyticsGuard guard;
  auto fx = MakeShAmFixture("sp089_png_ack");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto payload = manager.PrepareCompletionCardShare();
  TEST(payload.has_value(), ());
  TEST(Platform::IsFileExistsByFullPath(payload->m_path), ());
  manager.AcknowledgeAreaMilestonePresentation();
  TEST(!Platform::IsFileExistsByFullPath(payload->m_path), ());
  CleanupShAm(fx);
}
