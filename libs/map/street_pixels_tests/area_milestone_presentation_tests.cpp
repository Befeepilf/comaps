#include "testing/testing.hpp"

#include "map/area_milestone_presentation.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "street_pixels_config/country_config.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

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
std::string PresAmPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void PresAmRemove(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> PresAmLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput PresAmMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct PresAmFixture
{
  std::string leaf;
  std::string spaPath;
  std::string pixPath;
  std::string dbPath;
  int64_t mapDataVersion = 42;
  std::vector<m2::PointD> samples;
  int64_t districtId = 0;
  int64_t cityOnlyId = 0;
  int64_t outsideId = 0;
};

PresAmFixture MakePresAmFixture(std::string const & leaf)
{
  PresAmFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = PresAmPath(leaf + ".pix");
  fx.dbPath = PresAmPath(leaf + "_milestones.db");
  PresAmRemove(fx.spaPath);
  PresAmRemove(fx.pixPath);
  PresAmRemove(fx.dbPath);

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
  for (auto const & input : {PresAmMakeAdmin(10, 10, "District", PresAmLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             PresAmMakeAdmin(8, 8, "City", PresAmLonLatBox(24.0, 60.0, 25.0, 61.0))})
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
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = leaf;
  street_pixels::WriteExplorationSidecar(fx.spaPath, areas, fx.samples, policy, params);

  street_pixels_file::ExploredEverLiveMap seed{{districtId, true}};
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>(universeIds.begin(), universeIds.end()), seed, fx.mapDataVersion),
       ());
  fx.districtId = districtId;
  fx.cityOnlyId = cityOnlyId;
  fx.outsideId = outsideId;
  return fx;
}

void CleanupPresAm(PresAmFixture const & fx)
{
  PresAmRemove(fx.spaPath);
  PresAmRemove(fx.pixPath);
  PresAmRemove(fx.dbPath);
  street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
}

street_pixels::AreaMilestoneCrossing PresMakeCrossing(uint64_t osmId, uint32_t compactIndex,
                                                 street_pixels::AreaMilestoneThreshold threshold)
{
  street_pixels::AreaMilestoneCrossing crossing;
  crossing.m_osmId = osmId;
  crossing.m_compactIndex = compactIndex;
  crossing.m_threshold = threshold;
  return crossing;
}

std::string PresDistrictName(uint32_t, uint64_t) { return "District"; }

void PresAckAll(StreetPixelsManager & manager)
{
  while (manager.GetCurrentAreaMilestonePresentation().has_value())
    manager.AcknowledgeAreaMilestonePresentation();
}
}  // namespace

UNIT_TEST(AreaMilestonePresentation_MapsThresholds)
{
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P25)}, PresDistrictName);
  auto peek25 = presenter.Peek();
  TEST(peek25.has_value(), ());
  TEST_EQUAL(peek25->m_threshold, street_pixels::AreaMilestoneThreshold::P25, ());
  TEST_EQUAL(peek25->m_displayName, "District", ());
  TEST(peek25->m_competitionLine.empty(), ());
  presenter.ResetForTesting();

  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P50)}, PresDistrictName);
  auto peek50 = presenter.Peek();
  TEST(peek50.has_value(), ());
  TEST_EQUAL(peek50->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());
  TEST_EQUAL(peek50->m_displayName, "District", ());
  TEST(peek50->m_competitionLine.empty(), ());
  presenter.ResetForTesting();

  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P100)}, PresDistrictName);
  auto peek100 = presenter.Peek();
  TEST(peek100.has_value(), ());
  TEST_EQUAL(peek100->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  TEST_EQUAL(peek100->m_displayName, "District", ());
  TEST(peek100->m_competitionLine.empty(), ());
}

UNIT_TEST(AreaMilestonePresentation_QueueOrder100Then50Then25)
{
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P25),
                     PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P100),
                     PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P50)},
                    PresDistrictName);
  auto peek = presenter.Peek();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  presenter.Acknowledge();
  peek = presenter.Peek();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());
  presenter.Acknowledge();
  peek = presenter.Peek();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P25, ());
  presenter.Acknowledge();
  TEST(!presenter.Peek().has_value(), ());
}

UNIT_TEST(AreaMilestonePresentation_OneAtATime)
{
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P25),
                     PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P100),
                     PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P50)},
                    PresDistrictName);
  auto first = presenter.Peek();
  TEST(first.has_value(), ());
  TEST_EQUAL(first->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  auto still = presenter.Peek();
  TEST(still.has_value(), ());
  TEST_EQUAL(still->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  TEST_EQUAL(still->m_osmId, first->m_osmId, ());
  presenter.Acknowledge();
  auto next = presenter.Peek();
  TEST(next.has_value(), ());
  TEST_EQUAL(next->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());
}

UNIT_TEST(AreaMilestonePresentation_SkipAlreadyShownThisCrossing)
{
  auto fx = MakePresAmFixture("sp065_skip_shown");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  PresAckAll(manager);
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_SkipDuplicateInQueue)
{
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P50),
                     PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P50)},
                    PresDistrictName);
  auto peek = presenter.Peek();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());
  presenter.Acknowledge();
  TEST(!presenter.Peek().has_value(), ());
}

UNIT_TEST(AreaMilestonePresentation_DisplayNameNeverMwmId)
{
  std::string const leaf = "sp065_mwm_id_leaf";
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P100)}, PresDistrictName);
  auto peek = presenter.Peek();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_displayName, "District", ());
  TEST(peek->m_displayName != leaf, ());
  TEST(peek->m_displayName != std::to_string(peek->m_osmId), ());
}

UNIT_TEST(AreaMilestonePresentation_BlankDisplayNameDropped)
{
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P100)},
                    [](uint32_t, uint64_t) { return std::string(); });
  TEST(!presenter.Peek().has_value(), ());
}

UNIT_TEST(AreaMilestonePresentation_CitySummaryDoesNotEnqueue)
{
  auto fx = MakePresAmFixture("sp065_city_sum");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  bool sawDistrict = false;
  while (auto const peek = manager.GetCurrentAreaMilestonePresentation())
  {
    TEST_NOT_EQUAL(peek->m_osmId, 8u, ());
    if (peek->m_osmId == 10u)
      sawDistrict = true;
    manager.AcknowledgeAreaMilestonePresentation();
  }
  TEST(sawDistrict, ());
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  TEST(manager.SetFocusedArea(1, fx.spaPath, true), ());
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_Haptic50And100Not25)
{
  auto fx = MakePresAmFixture("sp065_haptic");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  std::vector<street_pixels::AreaMilestoneHapticEvent> events;
  manager.SetAreaMilestoneHapticHandler([&events](street_pixels::AreaMilestoneHapticEvent event)
                                        { events.push_back(event); });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(events.size(), 1u, ());
  TEST_EQUAL(events[0], street_pixels::AreaMilestoneHapticEvent::HundredPercent, ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(events.size(), 2u, ());
  TEST_EQUAL(events[1], street_pixels::AreaMilestoneHapticEvent::FiftyPercent, ());
  peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(events.size(), 2u, ());
  peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P25, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(events.size(), 2u, ());
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_DoesNotCallCollectionVibration)
{
  auto fx = MakePresAmFixture("sp065_novib");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  size_t vibrationCalls = 0;
  manager.SetVibrationHandler([&vibrationCalls](street_pixels::ExplorationHapticKind) { ++vibrationCalls; });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(vibrationCalls, 0u, ());
  TEST(manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_FollowingDoesNotStopRoute)
{
  auto fx = MakePresAmFixture("sp065_follow");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  RecordingSession session;
  manager.SetRecordingSession(&session);
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());

  size_t vibrationCalls = 0;
  manager.SetVibrationHandler([&vibrationCalls](street_pixels::ExplorationHapticKind) { ++vibrationCalls; });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  TEST_EQUAL(vibrationCalls, 0u, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(vibrationCalls, 0u, ());
  TEST_EQUAL(session.GetState(), RecordingSession::State::Recording, ());

  session.Finish();
  session.Reset();
  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_HundredPercentDoesNotShare)
{
  auto fx = MakePresAmFixture("sp065_noshare");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());

  manager.AcknowledgeAreaMilestonePresentation();
  auto next = manager.GetCurrentAreaMilestonePresentation();
  TEST(next.has_value(), ());
  TEST_EQUAL(next->m_threshold, street_pixels::AreaMilestoneThreshold::P50, ());

  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_CompetitionLineStubEmpty)
{
  street_pixels::AreaMilestonePresenter presenter;
  presenter.Enqueue({PresMakeCrossing(10, 0, street_pixels::AreaMilestoneThreshold::P100)}, PresDistrictName);
  auto peek = presenter.Peek();
  TEST(peek.has_value(), ());
  TEST(peek->m_competitionLine.empty(), ());
}

UNIT_TEST(AreaMilestonePresentation_PreviouslyCompletedOnFocus)
{
  auto fx = MakePresAmFixture("sp065_prev_focus");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  PresAckAll(manager);
  TEST(!manager.WasAreaPreviouslyCompletedBelow100(0), ());

  std::set<int64_t> const universe = {fx.districtId, fx.cityOnlyId, fx.outsideId};
  street_pixels_file::ExploredEverLiveMap explored{{fx.cityOnlyId, false}};
  TEST(street_pixels_file::SaveRematchedUniverse(fx.pixPath, universe, explored, fx.mapDataVersion), ());

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(manager.GetAreaCompletionFraction(0), 0.0, ());
  TEST(manager.WasAreaPreviouslyCompletedBelow100(0), ());

  TEST(manager.SetFocusedArea(0, fx.spaPath, false), ());
  auto progress = manager.GetFocusedAreaProgress();
  TEST(progress.m_previouslyCompleted, ());
  TEST(!progress.m_citySummary, ());

  TEST(manager.SetFocusedArea(1, fx.spaPath, true), ());
  auto city = manager.GetFocusedAreaProgress();
  TEST(city.m_citySummary, ());
  TEST(!city.m_previouslyCompleted, ());

  CleanupPresAm(fx);
}

UNIT_TEST(AreaMilestonePresentation_DebugPreviewWithoutHundredPercent)
{
  auto fx = MakePresAmFixture("sp_dbg_card");
  TEST(street_pixels_file::SaveRematchedUniverse(
           fx.pixPath, std::set<int64_t>{fx.districtId, fx.cityOnlyId, fx.outsideId},
           street_pixels_file::ExploredEverLiveMap{}, fx.mapDataVersion),
       ());
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());

  size_t hapticEvents = 0;
  size_t plays = 0;
  manager.SetAreaMilestoneHapticHandler([&hapticEvents](street_pixels::AreaMilestoneHapticEvent) { ++hapticEvents; });
  manager.SetVibrationHandler([&plays](street_pixels::ExplorationHapticKind) { ++plays; });
  manager.SetApplicationForeground(true);

  TEST(manager.DebugPreviewCompletionCard(), ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P100, ());
  TEST(peek->m_debugPreview, ());
  TEST_EQUAL(hapticEvents, 0, ());
  TEST_EQUAL(plays, 0, ());

  auto card = manager.GetCompletionCardForCurrentPresentation(false, false);
  TEST(card.has_value(), ());
  TEST(!card->m_outlineRings.empty(), ());
  TEST_EQUAL(card->m_headline, street_pixels::kCompletionCardHeadline, ());

  auto share = manager.PrepareCompletionCardShare(false);
  TEST(share.has_value(), ());
  TEST_EQUAL(share->m_mimeType, street_pixels::kCompletionCardShareMime, ());

  auto rec = street_pixels::AreaMilestoneStore::Instance().Get(10);
  TEST(!rec.has_value() || (rec->m_firedMask & street_pixels::kAreaMilestoneMask100) == 0, ());

  TEST(manager.ClearDebugCompletionCard(), ());
  TEST(!manager.GetCurrentAreaMilestonePresentation().has_value(), ());
  TEST(!manager.ClearDebugCompletionCard(), ());

  CleanupPresAm(fx);
}
