#include "testing/testing.hpp"

#include "map/area_milestone_presentation.hpp"
#include "map/exploration_haptics.hpp"
#include "map/first_goal.hpp"
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
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/mercator.hpp"

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
class ExplorationHapticCleanup
{
public:
  ExplorationHapticCleanup()
  {
    settings::Delete("RecordingSessionActive");
    settings::Delete(street_pixels::kExplorationHapticsSettingsKey);
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
  }

  ~ExplorationHapticCleanup()
  {
    settings::Delete("RecordingSessionActive");
    settings::Delete(street_pixels::kExplorationHapticsSettingsKey);
    street_pixels::FirstGoalTracker::ClearPersistedForTesting();
  }
};

class HapticCollectionFixture
{
public:
  static std::int64_t constexpr kPixelA = 1000;

  HapticCollectionFixture() : m_manager(m_dataSource) { m_manager.SetRecordingSession(&m_session); }

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

class HapticFirstGoalFixture
{
public:
  HapticFirstGoalFixture() : m_manager(m_dataSource)
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

std::string HapticAmPath(std::string const & name) { return base::JoinPath(GetPlatform().WritableDir(), name); }

void HapticAmRemove(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
  Platform::RemoveFileIfExists(path + "-wal");
  Platform::RemoveFileIfExists(path + "-shm");
}

std::vector<m2::PointD> HapticAmLonLatBox(double west, double south, double east, double north)
{
  return {{west, south}, {east, south}, {east, north}, {west, north}, {west, south}};
}

street_pixels::AreaCandidateInput HapticAmMakeAdmin(uint64_t osmId, int adminLevel, std::string const & name,
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

struct HapticAmFixture
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

HapticAmFixture MakeHapticAmFixture(std::string const & leaf)
{
  HapticAmFixture fx;
  fx.leaf = leaf;
  fx.spaPath = street_pixels::ExplorationSidecarPath(GetPlatform().WritableDir(), leaf);
  fx.pixPath = HapticAmPath(leaf + ".pix");
  fx.dbPath = HapticAmPath(leaf + "_milestones.db");
  HapticAmRemove(fx.spaPath);
  HapticAmRemove(fx.pixPath);
  HapticAmRemove(fx.dbPath);

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
  for (auto const & input : {HapticAmMakeAdmin(10, 10, "District", HapticAmLonLatBox(24.2, 60.2, 24.8, 60.8)),
                             HapticAmMakeAdmin(8, 8, "City", HapticAmLonLatBox(24.0, 60.0, 25.0, 61.0))})
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

void CleanupHapticAm(HapticAmFixture const & fx)
{
  HapticAmRemove(fx.spaPath);
  HapticAmRemove(fx.pixPath);
  HapticAmRemove(fx.dbPath);
  street_pixels::AreaMilestoneStore::Instance().Reopen(street_pixels::AreaMilestoneStore::DefaultDbPath());
}
}  // namespace

UNIT_TEST(ExplorationHaptic_Predicate_AllTrue_Allows)
{
  street_pixels::ExplorationHapticGate gate;
  gate.recording = true;
  gate.foreground = true;
  gate.toggleOn = true;
  TEST(street_pixels::ShouldPlayExplorationHaptic(gate), ());
}

UNIT_TEST(ExplorationHaptic_Predicate_NotRecording_Denies)
{
  street_pixels::ExplorationHapticGate gate;
  gate.recording = false;
  gate.foreground = true;
  gate.toggleOn = true;
  TEST(!street_pixels::ShouldPlayExplorationHaptic(gate), ());
}

UNIT_TEST(ExplorationHaptic_Predicate_Background_Denies)
{
  street_pixels::ExplorationHapticGate gate;
  gate.recording = true;
  gate.foreground = false;
  gate.toggleOn = true;
  TEST(!street_pixels::ShouldPlayExplorationHaptic(gate), ());
}

UNIT_TEST(ExplorationHaptic_Predicate_ToggleOff_Denies)
{
  street_pixels::ExplorationHapticGate gate;
  gate.recording = true;
  gate.foreground = true;
  gate.toggleOn = false;
  TEST(!street_pixels::ShouldPlayExplorationHaptic(gate), ());
}

UNIT_TEST(ExplorationHaptic_CollectionPulse_ZeroPixels_Denies)
{
  street_pixels::ExplorationHapticGate gate;
  gate.recording = true;
  gate.foreground = true;
  gate.toggleOn = true;
  TEST(!street_pixels::ShouldPlayCollectionPulse(gate, 0), ());
}

UNIT_TEST(ExplorationHaptic_CollectionPulse_OneOrMany_Same)
{
  street_pixels::ExplorationHapticGate gate;
  gate.recording = true;
  gate.foreground = true;
  gate.toggleOn = true;
  TEST(street_pixels::ShouldPlayCollectionPulse(gate, 1), ());
  TEST(street_pixels::ShouldPlayCollectionPulse(gate, 3), ());
}

UNIT_TEST(ExplorationHaptic_Toggle_DefaultOnWhenKeyMissing)
{
  ExplorationHapticCleanup cleanup;
  settings::Delete(street_pixels::kExplorationHapticsSettingsKey);
  TEST(street_pixels::ExplorationHapticsToggleEnabled(), ());
}

UNIT_TEST(ExplorationHaptic_Toggle_OffWhenSetFalse)
{
  ExplorationHapticCleanup cleanup;
  settings::Set(street_pixels::kExplorationHapticsSettingsKey, false);
  TEST(!street_pixels::ExplorationHapticsToggleEnabled(), ());
  settings::Delete(street_pixels::kExplorationHapticsSettingsKey);
}

UNIT_TEST(ExplorationHaptic_Manager_ZeroNewPixels_NoPulse)
{
  ExplorationHapticCleanup cleanup;
  HapticCollectionFixture fixture;
  fixture.SetupPixels({{HapticCollectionFixture::kPixelA, true}});
  size_t calls = 0;
  fixture.Manager().SetVibrationHandler([&calls](street_pixels::ExplorationHapticKind) { ++calls; });
  fixture.Manager().SetApplicationForeground(true);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(HapticCollectionFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST_EQUAL(calls, 0, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(HapticCollectionFixture::kPixelA), ());
}

UNIT_TEST(ExplorationHaptic_Manager_OneNewPixel_OneCollectionPulse)
{
  ExplorationHapticCleanup cleanup;
  HapticCollectionFixture fixture;
  fixture.SetupPixels({{HapticCollectionFixture::kPixelA, false}});
  size_t calls = 0;
  street_pixels::ExplorationHapticKind last = street_pixels::ExplorationHapticKind::FiftyPercent;
  fixture.Manager().SetVibrationHandler([&](street_pixels::ExplorationHapticKind kind) {
    ++calls;
    last = kind;
  });
  fixture.Manager().SetApplicationForeground(true);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(HapticCollectionFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST_EQUAL(calls, 1, ());
  TEST_EQUAL(last, street_pixels::ExplorationHapticKind::Collection, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(HapticCollectionFixture::kPixelA), ());
}

UNIT_TEST(ExplorationHaptic_Manager_ManyNewPixels_OneCollectionPulse)
{
  ExplorationHapticCleanup cleanup;
  HapticCollectionFixture fixture;
  double const lat = 50.0;
  double const lon = 10.0;
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  auto const [latB, lonB] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 10.0, 0.0);
  auto const pixelB = street_pixels_tests::PixelIdForLatLon(latB, lonB);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}, {pixelB, false}}));
  size_t calls = 0;
  street_pixels::ExplorationHapticKind last = street_pixels::ExplorationHapticKind::FiftyPercent;
  fixture.Manager().SetVibrationHandler([&](street_pixels::ExplorationHapticKind kind) {
    ++calls;
    last = kind;
  });
  fixture.Manager().SetApplicationForeground(true);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      street_pixels_tests::MakeGpsInfo(lat, lon, 5.0, street_pixels_tests::CurrentTimestampSec()));
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelB), ());
  TEST_EQUAL(calls, 1, ());
  TEST_EQUAL(last, street_pixels::ExplorationHapticKind::Collection, ());
}

UNIT_TEST(ExplorationHaptic_Manager_Background_NoCollectionPulse)
{
  ExplorationHapticCleanup cleanup;
  HapticCollectionFixture fixture;
  fixture.SetupPixels({{HapticCollectionFixture::kPixelA, false}});
  size_t calls = 0;
  fixture.Manager().SetVibrationHandler([&calls](street_pixels::ExplorationHapticKind) { ++calls; });
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(HapticCollectionFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST_EQUAL(calls, 0, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(HapticCollectionFixture::kPixelA), ());
}

UNIT_TEST(ExplorationHaptic_Manager_ToggleOff_NoCollectionPulse)
{
  ExplorationHapticCleanup cleanup;
  HapticCollectionFixture fixture;
  settings::Set(street_pixels::kExplorationHapticsSettingsKey, false);
  fixture.SetupPixels({{HapticCollectionFixture::kPixelA, false}});
  size_t calls = 0;
  fixture.Manager().SetVibrationHandler([&calls](street_pixels::ExplorationHapticKind) { ++calls; });
  fixture.Manager().SetApplicationForeground(true);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(HapticCollectionFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST_EQUAL(calls, 0, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(HapticCollectionFixture::kPixelA), ());
}

UNIT_TEST(ExplorationHaptic_Manager_Paused_NoCollectionPulse)
{
  ExplorationHapticCleanup cleanup;
  HapticCollectionFixture fixture;
  fixture.SetupPixels({{HapticCollectionFixture::kPixelA, false}});
  size_t calls = 0;
  fixture.Manager().SetVibrationHandler([&calls](street_pixels::ExplorationHapticKind) { ++calls; });
  fixture.Manager().SetApplicationForeground(true);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  TEST_EQUAL(fixture.Session().Pause(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().OnLocationUpdate(
      fixture.GpsAtPixel(HapticCollectionFixture::kPixelA, street_pixels_tests::CurrentTimestampSec()));
  TEST_EQUAL(calls, 0, ());
  TEST(!fixture.Manager().IsPixelExploredForTesting(HapticCollectionFixture::kPixelA), ());
}

UNIT_TEST(ExplorationHaptic_Manager_P100ThenP50_PlayOnceEachWhenAllowed)
{
  ExplorationHapticCleanup cleanup;
  auto fx = MakeHapticAmFixture("sp066_haptic_p100p50");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  RecordingSession session;
  manager.SetRecordingSession(&session);
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  manager.SetApplicationForeground(true);

  std::vector<street_pixels::ExplorationHapticKind> kinds;
  manager.SetVibrationHandler([&kinds](street_pixels::ExplorationHapticKind kind) { kinds.push_back(kind); });
  std::vector<street_pixels::AreaMilestoneHapticEvent> events;
  manager.SetAreaMilestoneHapticHandler([&events](street_pixels::AreaMilestoneHapticEvent event)
                                        { events.push_back(event); });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(kinds.size(), 1u, ());
  TEST_EQUAL(kinds[0], street_pixels::ExplorationHapticKind::HundredPercent, ());
  TEST_EQUAL(events.size(), 1u, ());
  TEST_EQUAL(events[0], street_pixels::AreaMilestoneHapticEvent::HundredPercent, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(kinds.size(), 2u, ());
  TEST_EQUAL(kinds[1], street_pixels::ExplorationHapticKind::FiftyPercent, ());
  TEST_EQUAL(events.size(), 2u, ());
  TEST_EQUAL(events[1], street_pixels::AreaMilestoneHapticEvent::FiftyPercent, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(kinds.size(), 2u, ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P25, ());

  session.Finish();
  session.Reset();
  CleanupHapticAm(fx);
}

UNIT_TEST(ExplorationHaptic_Manager_P25_NoPattern)
{
  ExplorationHapticCleanup cleanup;
  auto fx = MakeHapticAmFixture("sp066_haptic_p25");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  RecordingSession session;
  manager.SetRecordingSession(&session);
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  manager.SetApplicationForeground(true);

  std::vector<street_pixels::ExplorationHapticKind> kinds;
  manager.SetVibrationHandler([&kinds](street_pixels::ExplorationHapticKind kind) { kinds.push_back(kind); });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  manager.AcknowledgeAreaMilestonePresentation();
  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(kinds.size(), 2u, ());
  TEST_EQUAL(kinds[0], street_pixels::ExplorationHapticKind::HundredPercent, ());
  TEST_EQUAL(kinds[1], street_pixels::ExplorationHapticKind::FiftyPercent, ());
  auto peek = manager.GetCurrentAreaMilestonePresentation();
  TEST(peek.has_value(), ());
  TEST_EQUAL(peek->m_threshold, street_pixels::AreaMilestoneThreshold::P25, ());

  session.Finish();
  session.Reset();
  CleanupHapticAm(fx);
}

UNIT_TEST(ExplorationHaptic_Manager_Milestone_ToggleOff_NoPlay)
{
  ExplorationHapticCleanup cleanup;
  auto fx = MakeHapticAmFixture("sp066_haptic_toggle");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  RecordingSession session;
  manager.SetRecordingSession(&session);
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());
  manager.SetApplicationForeground(true);
  settings::Set(street_pixels::kExplorationHapticsSettingsKey, false);

  size_t plays = 0;
  manager.SetVibrationHandler([&plays](street_pixels::ExplorationHapticKind) { ++plays; });
  std::vector<street_pixels::AreaMilestoneHapticEvent> events;
  manager.SetAreaMilestoneHapticHandler([&events](street_pixels::AreaMilestoneHapticEvent event)
                                        { events.push_back(event); });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(events.size(), 1u, ());
  TEST_EQUAL(events[0], street_pixels::AreaMilestoneHapticEvent::HundredPercent, ());
  TEST_EQUAL(plays, 0u, ());

  session.Finish();
  session.Reset();
  CleanupHapticAm(fx);
}

UNIT_TEST(ExplorationHaptic_Manager_Milestone_Background_NoPlay)
{
  ExplorationHapticCleanup cleanup;
  auto fx = MakeHapticAmFixture("sp066_haptic_bg");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  RecordingSession session;
  manager.SetRecordingSession(&session);
  TEST_EQUAL(session.Start(), RecordingSession::TransitionResult::Ok, ());

  size_t plays = 0;
  manager.SetVibrationHandler([&plays](street_pixels::ExplorationHapticKind) { ++plays; });
  std::vector<street_pixels::AreaMilestoneHapticEvent> events;
  manager.SetAreaMilestoneHapticHandler([&events](street_pixels::AreaMilestoneHapticEvent event)
                                        { events.push_back(event); });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(events.size(), 1u, ());
  TEST_EQUAL(events[0], street_pixels::AreaMilestoneHapticEvent::HundredPercent, ());
  TEST_EQUAL(plays, 0u, ());

  session.Finish();
  session.Reset();
  CleanupHapticAm(fx);
}

UNIT_TEST(ExplorationHaptic_Manager_Milestone_NotRecording_NoPlay)
{
  ExplorationHapticCleanup cleanup;
  auto fx = MakeHapticAmFixture("sp066_haptic_norec");
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  manager.ConfigureAreaMilestoneStoreForTesting(fx.dbPath);
  manager.SetApplicationForeground(true);

  size_t plays = 0;
  manager.SetVibrationHandler([&plays](street_pixels::ExplorationHapticKind) { ++plays; });
  std::vector<street_pixels::AreaMilestoneHapticEvent> events;
  manager.SetAreaMilestoneHapticHandler([&events](street_pixels::AreaMilestoneHapticEvent event)
                                        { events.push_back(event); });

  TEST(manager.RebuildAreaCompletionCache(fx.leaf, fx.spaPath, fx.mapDataVersion), ());
  TEST_EQUAL(events.size(), 1u, ());
  TEST_EQUAL(events[0], street_pixels::AreaMilestoneHapticEvent::HundredPercent, ());
  TEST_EQUAL(plays, 0u, ());

  manager.AcknowledgeAreaMilestonePresentation();
  TEST_EQUAL(events.size(), 2u, ());
  TEST_EQUAL(events[1], street_pixels::AreaMilestoneHapticEvent::FiftyPercent, ());
  TEST_EQUAL(plays, 0u, ());

  CleanupHapticAm(fx);
}

UNIT_TEST(ExplorationHaptic_Manager_FirstGoalComplete_StrongerOnce)
{
  ExplorationHapticCleanup cleanup;
  HapticFirstGoalFixture fixture;
  fixture.SetupUnexplored(12);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());
  fixture.Manager().SetApplicationForeground(true);

  std::vector<street_pixels::ExplorationHapticKind> kinds;
  fixture.Manager().SetVibrationHandler(
      [&kinds](street_pixels::ExplorationHapticKind kind) { kinds.push_back(kind); });
  size_t completeCalls = 0;
  fixture.Manager().SetFirstGoalCompleteHandler([&completeCalls]() { ++completeCalls; });

  for (int i = 0; i < 9; ++i)
    fixture.Collect(i);
  TEST_EQUAL(kinds.size(), 9u, ());
  for (auto const kind : kinds)
    TEST_EQUAL(kind, street_pixels::ExplorationHapticKind::Collection, ());
  TEST_EQUAL(completeCalls, 0u, ());

  fixture.Collect(9);
  TEST_EQUAL(kinds.size(), 10u, ());
  TEST_EQUAL(kinds[9], street_pixels::ExplorationHapticKind::FirstGoalComplete, ());
  TEST_EQUAL(completeCalls, 1u, ());
}

UNIT_TEST(ExplorationHaptic_Manager_FirstGoalComplete_Background_NoPlay)
{
  ExplorationHapticCleanup cleanup;
  HapticFirstGoalFixture fixture;
  fixture.SetupUnexplored(12);
  TEST_EQUAL(fixture.Session().Start(), RecordingSession::TransitionResult::Ok, ());

  size_t plays = 0;
  fixture.Manager().SetVibrationHandler([&plays](street_pixels::ExplorationHapticKind) { ++plays; });
  size_t completeCalls = 0;
  fixture.Manager().SetFirstGoalCompleteHandler([&completeCalls]() { ++completeCalls; });

  for (int i = 0; i < 10; ++i)
    fixture.Collect(i);
  TEST_EQUAL(completeCalls, 1u, ());
  TEST_EQUAL(plays, 0u, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(fixture.PixelAt(9)), ());
}
