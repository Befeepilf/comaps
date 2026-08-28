#include "testing/testing.hpp"

#include "map/explorer_pro.hpp"
#include "map/explorer_pro_analytics.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "indexer/data_source.hpp"

#include "kml/types.hpp"

#include "platform/settings.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <string_view>
#include <vector>

using namespace street_pixels;

namespace
{
class EpaFakeEntitlementSource : public explorer_pro::EntitlementSource
{
public:
  explicit EpaFakeEntitlementSource(bool entitled) : m_entitled(entitled) {}

  bool IsEntitled() const override { return m_entitled; }

private:
  bool m_entitled;
};

class EpaEntitlementSourceScope
{
public:
  explicit EpaEntitlementSourceScope(explorer_pro::EntitlementSource * source)
  {
    explorer_pro::SetEntitlementSource(source);
  }

  ~EpaEntitlementSourceScope() { explorer_pro::SetEntitlementSource(nullptr); }
};

class EpaCapabilityAvailabilityScope
{
public:
  EpaCapabilityAvailabilityScope(explorer_pro::Capability capability, bool available)
    : m_capability(capability)
    , m_previous(explorer_pro::IsCapabilityAvailable(capability))
  {
    explorer_pro::SetCapabilityAvailable(capability, available);
  }

  ~EpaCapabilityAvailabilityScope() { explorer_pro::SetCapabilityAvailable(m_capability, m_previous); }

private:
  explorer_pro::Capability m_capability;
  bool m_previous;
};

class EpaAnalyticsGuard
{
public:
  EpaAnalyticsGuard()
  {
    explorer_pro::UnfreezeConfigurationForTesting();
    explorer_pro::SetEntitlementSource(nullptr);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
    m_hadInfoPageViewed = settings::Get(ExplorerProAnalytics::kInfoPageViewedKey, m_infoPageViewed);
    m_hadGpxImportUsage = settings::Get(ExplorerProAnalytics::kGpxImportUsageKey, m_gpxImportUsage);
    m_hadGpxExportUsage = settings::Get(ExplorerProAnalytics::kGpxExportUsageKey, m_gpxExportUsage);
    settings::Delete(ExplorerProAnalytics::kInfoPageViewedKey);
    settings::Delete(ExplorerProAnalytics::kGpxImportUsageKey);
    settings::Delete(ExplorerProAnalytics::kGpxExportUsageKey);
    ExplorerProAnalytics::ResetForTesting();
  }

  ~EpaAnalyticsGuard()
  {
    explorer_pro::UnfreezeConfigurationForTesting();
    explorer_pro::SetEntitlementSource(nullptr);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
    explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
    settings::Delete(ExplorerProAnalytics::kInfoPageViewedKey);
    settings::Delete(ExplorerProAnalytics::kGpxImportUsageKey);
    settings::Delete(ExplorerProAnalytics::kGpxExportUsageKey);
    ExplorerProAnalytics::ResetForTesting();
    if (m_hadInfoPageViewed)
      settings::Set(ExplorerProAnalytics::kInfoPageViewedKey, m_infoPageViewed);
    if (m_hadGpxImportUsage)
      settings::Set(ExplorerProAnalytics::kGpxImportUsageKey, m_gpxImportUsage);
    if (m_hadGpxExportUsage)
      settings::Set(ExplorerProAnalytics::kGpxExportUsageKey, m_gpxExportUsage);
  }

private:
  bool m_hadInfoPageViewed = false;
  bool m_hadGpxImportUsage = false;
  bool m_hadGpxExportUsage = false;
  uint64_t m_infoPageViewed = 0;
  uint64_t m_gpxImportUsage = 0;
  uint64_t m_gpxExportUsage = 0;
};

class EpaRecordingBreadcrumbCleanup
{
public:
  EpaRecordingBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~EpaRecordingBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

bool EpaContainsForbiddenLocationToken(std::string const & text)
{
  std::string const lower = strings::MakeLowerCase(text);
  std::string_view constexpr kForbidden[] = {
      "lat",      "lon",      "latitude", "longitude", "geometry", "polyline", "pixel",    "area",
      "coord",    "mwm",      "country",  "path",      "track",    "filename", "file",     "osm",
      "healpix",  "gps",      "geo:"};
  for (auto const token : kForbidden)
  {
    if (lower.find(token) != std::string::npos)
      return true;
  }
  return false;
}

kml::MultiGeometry::LineT EpaShortLineAt(double lat, double lon)
{
  auto const [lat2, lon2] = street_pixels_tests::OffsetLatLonByMeters(lat, lon, 0.0, 10.0);
  return {geometry::PointWithAltitude(mercator::FromLatLon(lat, lon)),
          geometry::PointWithAltitude(mercator::FromLatLon(lat2, lon2))};
}

void EpaFrameworkHistoricalImportHandler(StreetPixelsManager & manager,
                                         std::vector<kml::MultiGeometry::LineT> const & segments)
{
  RunHistoricalImportIfEnabled(manager, segments);
}

void EpaRunDirectImport(StreetPixelsManager & manager, double lat, double lon)
{
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));
  manager.ImportHistoricalTrack({EpaShortLineAt(lat, lon)});
}
}  // namespace

UNIT_TEST(ExplorerProAnalytics_DefaultZero)
{
  EpaAnalyticsGuard guard;
  ExplorerProAnalyticsSnapshot const snapshot = ExplorerProAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_infoPageViewed, 0, ());
  TEST_EQUAL(snapshot.m_gpxImportUsage, 0, ());
  TEST_EQUAL(snapshot.m_gpxExportUsage, 0, ());
  uint64_t value = 0;
  TEST(!settings::Get(ExplorerProAnalytics::kInfoPageViewedKey, value), ());
  TEST(!settings::Get(ExplorerProAnalytics::kGpxImportUsageKey, value), ());
  TEST(!settings::Get(ExplorerProAnalytics::kGpxExportUsageKey, value), ());
  TEST_EQUAL(DebugPrint(snapshot), std::string("pro-info-viewed=0 gpx-import-usage=0 gpx-export-usage=0"), ());
}

UNIT_TEST(ExplorerProAnalytics_RecordInfoWhenAnyAvailable)
{
  EpaAnalyticsGuard guard;
  {
    EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
    ExplorerProAnalytics::RecordInfoPageViewed();
    ExplorerProAnalytics::RecordInfoPageViewed();
    TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_infoPageViewed, 2, ());
  }
  ExplorerProAnalytics::ResetForTesting();
  {
    EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxExport, true);
    ExplorerProAnalytics::RecordInfoPageViewed();
    TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_infoPageViewed, 1, ());
  }
  ExplorerProAnalytics::ResetForTesting();
  {
    EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::AdvancedTrackManagement, true);
    ExplorerProAnalytics::RecordInfoPageViewed();
    TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_infoPageViewed, 1, ());
  }
}

UNIT_TEST(ExplorerProAnalytics_RecordInfoFailClosedWhenAllUnavailable)
{
  EpaAnalyticsGuard guard;
  ExplorerProAnalytics::RecordInfoPageViewed();
  uint64_t value = 0;
  TEST(!settings::Get(ExplorerProAnalytics::kInfoPageViewedKey, value), ());
  ExplorerProAnalyticsSnapshot const snapshot = ExplorerProAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_infoPageViewed, 0, ());
  TEST_EQUAL(snapshot.m_gpxImportUsage, 0, ());
  TEST_EQUAL(snapshot.m_gpxExportUsage, 0, ());
}

UNIT_TEST(ExplorerProAnalytics_RecordImportWhenAvailable)
{
  EpaAnalyticsGuard guard;
  {
    EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
    ExplorerProAnalytics::RecordGpxImportUsage();
    TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage, 1, ());
  }
  ExplorerProAnalytics::RecordGpxImportUsage();
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage, 1, ());
}

UNIT_TEST(ExplorerProAnalytics_RecordImportWhenAvailableNotEntitled)
{
  EpaAnalyticsGuard guard;
  EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  EpaFakeEntitlementSource notEntitled(false);
  EpaEntitlementSourceScope scope(&notEntitled);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  ExplorerProAnalytics::RecordGpxImportUsage();
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage, 1, ());
}

UNIT_TEST(ExplorerProAnalytics_RecordExportWhenAvailable)
{
  EpaAnalyticsGuard guard;
  {
    EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxExport, true);
    ExplorerProAnalytics::RecordGpxExportUsage();
    TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxExportUsage, 1, ());
  }
  ExplorerProAnalytics::RecordGpxExportUsage();
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxExportUsage, 1, ());
}

UNIT_TEST(ExplorerProAnalytics_RecordExportNoOpWhenUnavailable)
{
  EpaAnalyticsGuard guard;
  ExplorerProAnalytics::RecordGpxExportUsage();
  uint64_t value = 0;
  TEST(!settings::Get(ExplorerProAnalytics::kGpxExportUsageKey, value), ());
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxExportUsage, 0, ());
}

UNIT_TEST(ExplorerProAnalytics_UnavailableLeavesStoredZero)
{
  EpaAnalyticsGuard guard;
  ExplorerProAnalytics::RecordInfoPageViewed();
  ExplorerProAnalytics::RecordGpxImportUsage();
  ExplorerProAnalytics::RecordGpxExportUsage();
  uint64_t infoPageViewed = 0;
  uint64_t gpxImportUsage = 0;
  uint64_t gpxExportUsage = 0;
  TEST(!settings::Get(ExplorerProAnalytics::kInfoPageViewedKey, infoPageViewed) || infoPageViewed == 0, ());
  TEST(!settings::Get(ExplorerProAnalytics::kGpxImportUsageKey, gpxImportUsage) || gpxImportUsage == 0, ());
  TEST(!settings::Get(ExplorerProAnalytics::kGpxExportUsageKey, gpxExportUsage) || gpxExportUsage == 0, ());
  ExplorerProAnalyticsSnapshot const snapshot = ExplorerProAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_infoPageViewed, 0, ());
  TEST_EQUAL(snapshot.m_gpxImportUsage, 0, ());
  TEST_EQUAL(snapshot.m_gpxExportUsage, 0, ());
}

UNIT_TEST(ExplorerProAnalytics_PersistRoundTrip)
{
  EpaAnalyticsGuard guard;
  EpaCapabilityAvailabilityScope importAvailability(explorer_pro::Capability::GpxImport, true);
  EpaCapabilityAvailabilityScope exportAvailability(explorer_pro::Capability::GpxExport, true);
  ExplorerProAnalytics::RecordInfoPageViewed();
  ExplorerProAnalytics::RecordGpxImportUsage();
  ExplorerProAnalytics::RecordGpxExportUsage();
  uint64_t infoPageViewed = 0;
  uint64_t gpxImportUsage = 0;
  uint64_t gpxExportUsage = 0;
  TEST(settings::Get(ExplorerProAnalytics::kInfoPageViewedKey, infoPageViewed), ());
  TEST(settings::Get(ExplorerProAnalytics::kGpxImportUsageKey, gpxImportUsage), ());
  TEST(settings::Get(ExplorerProAnalytics::kGpxExportUsageKey, gpxExportUsage), ());
  TEST_EQUAL(infoPageViewed, 1, ());
  TEST_EQUAL(gpxImportUsage, 1, ());
  TEST_EQUAL(gpxExportUsage, 1, ());
  ExplorerProAnalyticsSnapshot const snapshot = ExplorerProAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_infoPageViewed, infoPageViewed, ());
  TEST_EQUAL(snapshot.m_gpxImportUsage, gpxImportUsage, ());
  TEST_EQUAL(snapshot.m_gpxExportUsage, gpxExportUsage, ());
}

UNIT_TEST(ExplorerProAnalytics_ResetIsolatesTests)
{
  EpaAnalyticsGuard guard;
  EpaCapabilityAvailabilityScope importAvailability(explorer_pro::Capability::GpxImport, true);
  EpaCapabilityAvailabilityScope exportAvailability(explorer_pro::Capability::GpxExport, true);
  ExplorerProAnalytics::RecordInfoPageViewed();
  ExplorerProAnalytics::RecordGpxImportUsage();
  ExplorerProAnalytics::RecordGpxExportUsage();
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_infoPageViewed, 1, ());
  ExplorerProAnalytics::ResetForTesting();
  ExplorerProAnalyticsSnapshot const snapshot = ExplorerProAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_infoPageViewed, 0, ());
  TEST_EQUAL(snapshot.m_gpxImportUsage, 0, ());
  TEST_EQUAL(snapshot.m_gpxExportUsage, 0, ());
  uint64_t value = 0;
  TEST(!settings::Get(ExplorerProAnalytics::kInfoPageViewedKey, value), ());
  TEST(!settings::Get(ExplorerProAnalytics::kGpxImportUsageKey, value), ());
  TEST(!settings::Get(ExplorerProAnalytics::kGpxExportUsageKey, value), ());
}

UNIT_TEST(ExplorerProAnalytics_SnapshotHasNoLocationKeys)
{
  EpaAnalyticsGuard guard;
  EpaCapabilityAvailabilityScope importAvailability(explorer_pro::Capability::GpxImport, true);
  EpaCapabilityAvailabilityScope exportAvailability(explorer_pro::Capability::GpxExport, true);
  ExplorerProAnalytics::RecordInfoPageViewed();
  ExplorerProAnalytics::RecordGpxImportUsage();
  ExplorerProAnalytics::RecordGpxExportUsage();
  std::string const keys[] = {std::string(ExplorerProAnalytics::kInfoPageViewedKey),
                              std::string(ExplorerProAnalytics::kGpxImportUsageKey),
                              std::string(ExplorerProAnalytics::kGpxExportUsageKey)};
  for (auto const & key : keys)
    TEST(!EpaContainsForbiddenLocationToken(key), (key));
  auto const serialized = ExplorerProAnalytics::SerializedSnapshot();
  TEST_EQUAL(serialized.size(), 3, ());
  TEST_EQUAL(std::string(serialized[0].first), std::string(ExplorerProAnalytics::kInfoPageViewedName), ());
  TEST_EQUAL(std::string(serialized[1].first), std::string(ExplorerProAnalytics::kGpxImportUsageName), ());
  TEST_EQUAL(std::string(serialized[2].first), std::string(ExplorerProAnalytics::kGpxExportUsageName), ());
  for (auto const & entry : serialized)
  {
    std::string const name(entry.first);
    TEST_EQUAL(strings::MakeLowerCase(name), name, ());
    TEST(!EpaContainsForbiddenLocationToken(name), (name));
  }
  std::string const debug = DebugPrint(ExplorerProAnalytics::LoadSnapshot());
  TEST_EQUAL(strings::MakeLowerCase(debug), debug, ());
  TEST(!EpaContainsForbiddenLocationToken(debug), (debug));
}

UNIT_TEST(ExplorerProAnalytics_HandlerDoesNotWrapManager)
{
  EpaAnalyticsGuard guard;
  EpaRecordingBreadcrumbCleanup breadcrumb;
  EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  FrozenDataSource dataSource;
  RecordingSession session;
  StreetPixelsManager manager(dataSource);
  manager.SetRecordingSession(&session);
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  EpaRunDirectImport(manager, lat, lon);
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage, 0, ());
}

UNIT_TEST(ExplorerProAnalytics_HandlerIncrementsAfterImport)
{
  EpaAnalyticsGuard guard;
  EpaRecordingBreadcrumbCleanup breadcrumb;
  EpaCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  FrozenDataSource dataSource;
  RecordingSession session;
  StreetPixelsManager manager(dataSource);
  manager.SetRecordingSession(&session);
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  manager.SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));
  {
    EpaEntitlementSourceScope closed(nullptr);
    TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
    EpaFrameworkHistoricalImportHandler(manager, {EpaShortLineAt(lat, lon)});
    TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage, 0, ());
  }
  EpaFakeEntitlementSource entitled(true);
  EpaEntitlementSourceScope open(&entitled);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());
  EpaFrameworkHistoricalImportHandler(manager, {EpaShortLineAt(lat, lon)});
  TEST_EQUAL(ExplorerProAnalytics::LoadSnapshot().m_gpxImportUsage, 1, ());
}
