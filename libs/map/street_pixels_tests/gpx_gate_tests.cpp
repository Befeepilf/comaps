#include "testing/testing.hpp"

#include "map/explorer_pro.hpp"
#include "map/recording_session.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "indexer/data_source.hpp"

#include "kml/types.hpp"

#include "platform/settings.hpp"

#include <vector>

namespace
{
class GpxGateBreadcrumbCleanup
{
public:
  GpxGateBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }

  ~GpxGateBreadcrumbCleanup() { settings::Delete("RecordingSessionActive"); }
};

class GpxGateFakeEntitlementSource : public explorer_pro::EntitlementSource
{
public:
  explicit GpxGateFakeEntitlementSource(bool entitled) : m_entitled(entitled) {}

  bool IsEntitled() const override { return m_entitled; }

private:
  bool m_entitled;
};

class GpxGateEntitlementSourceScope
{
public:
  explicit GpxGateEntitlementSourceScope(explorer_pro::EntitlementSource * source)
  {
    explorer_pro::SetEntitlementSource(source);
  }

  ~GpxGateEntitlementSourceScope() { explorer_pro::SetEntitlementSource(nullptr); }
};

class GpxGateCapabilityAvailabilityScope
{
public:
  GpxGateCapabilityAvailabilityScope(explorer_pro::Capability capability, bool available)
    : m_capability(capability)
    , m_previous(explorer_pro::IsCapabilityAvailable(capability))
  {
    explorer_pro::SetCapabilityAvailable(capability, available);
  }

  ~GpxGateCapabilityAvailabilityScope() { explorer_pro::SetCapabilityAvailable(m_capability, m_previous); }

private:
  explorer_pro::Capability m_capability;
  bool m_previous;
};

class GpxGateFixture
{
public:
  GpxGateFixture()
    : m_manager(m_dataSource)
  {
    m_manager.SetRecordingSession(&m_session);
  }

  StreetPixelsManager & Manager() { return m_manager; }

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

void GpxGateResetCapabilities()
{
  explorer_pro::UnfreezeConfigurationForTesting();
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxImport, false);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::GpxExport, false);
  explorer_pro::SetCapabilityAvailable(explorer_pro::Capability::AdvancedTrackManagement, false);
}

void HandleHistoricalTrackImport(StreetPixelsManager & manager,
                                   std::vector<kml::MultiGeometry::LineT> const & segments)
{
  if (!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport))
    return;
  manager.ImportHistoricalTrack(segments);
}

bool GpxGateShouldWriteExport(bool isGpx)
{
  return !isGpx || explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxExport);
}

bool GpxGateAllowImportBatch(size_t gpxCount)
{
  if (!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport))
    return false;
  return gpxCount <= 1 || explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::AdvancedTrackManagement);
}
}  // namespace

UNIT_TEST(GpxGate_HandlerClosedDoesNotPaint)
{
  GpxGateBreadcrumbCleanup cleanup;
  GpxGateResetCapabilities();
  GpxGateEntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  HandleHistoricalTrackImport(fixture.Manager(), {ShortLineAt(lat, lon)});

  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}

UNIT_TEST(GpxGate_HandlerUnavailableEntitledDoesNotPaint)
{
  GpxGateBreadcrumbCleanup cleanup;
  GpxGateResetCapabilities();
  GpxGateFakeEntitlementSource entitled(true);
  GpxGateEntitlementSourceScope scope(&entitled);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  HandleHistoricalTrackImport(fixture.Manager(), {ShortLineAt(lat, lon)});

  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}

UNIT_TEST(GpxGate_HandlerAvailableNotEntitledDoesNotPaint)
{
  GpxGateBreadcrumbCleanup cleanup;
  GpxGateResetCapabilities();
  GpxGateCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  GpxGateEntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  HandleHistoricalTrackImport(fixture.Manager(), {ShortLineAt(lat, lon)});

  TEST(!fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}

UNIT_TEST(GpxGate_HandlerOpenPaints)
{
  GpxGateBreadcrumbCleanup cleanup;
  GpxGateResetCapabilities();
  GpxGateCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  GpxGateFakeEntitlementSource entitled(true);
  GpxGateEntitlementSourceScope scope(&entitled);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  HandleHistoricalTrackImport(fixture.Manager(), {ShortLineAt(lat, lon)});

  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}

UNIT_TEST(GpxGate_ExportFourCell)
{
  GpxGateResetCapabilities();
  {
    GpxGateEntitlementSourceScope scope(nullptr);
    TEST(!GpxGateShouldWriteExport(true), ());
    TEST(GpxGateShouldWriteExport(false), ());
  }
  {
    GpxGateFakeEntitlementSource entitled(true);
    GpxGateEntitlementSourceScope scope(&entitled);
    TEST(!GpxGateShouldWriteExport(true), ());
    TEST(GpxGateShouldWriteExport(false), ());
  }
  {
    GpxGateCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxExport, true);
    GpxGateEntitlementSourceScope scope(nullptr);
    TEST(!GpxGateShouldWriteExport(true), ());
    TEST(GpxGateShouldWriteExport(false), ());
  }
  {
    GpxGateCapabilityAvailabilityScope availability(explorer_pro::Capability::GpxExport, true);
    GpxGateFakeEntitlementSource entitled(true);
    GpxGateEntitlementSourceScope scope(&entitled);
    TEST(GpxGateShouldWriteExport(true), ());
    TEST(GpxGateShouldWriteExport(false), ());
  }
}

UNIT_TEST(GpxGate_BatchFourCell)
{
  GpxGateResetCapabilities();
  {
    GpxGateEntitlementSourceScope scope(nullptr);
    TEST(!GpxGateAllowImportBatch(1), ());
    TEST(!GpxGateAllowImportBatch(2), ());
  }
  {
    GpxGateFakeEntitlementSource entitled(true);
    GpxGateEntitlementSourceScope scope(&entitled);
    TEST(!GpxGateAllowImportBatch(1), ());
    TEST(!GpxGateAllowImportBatch(2), ());
  }
  {
    GpxGateCapabilityAvailabilityScope importAvail(explorer_pro::Capability::GpxImport, true);
    GpxGateEntitlementSourceScope scope(nullptr);
    TEST(!GpxGateAllowImportBatch(1), ());
    TEST(!GpxGateAllowImportBatch(2), ());
  }
  {
    GpxGateCapabilityAvailabilityScope importAvail(explorer_pro::Capability::GpxImport, true);
    GpxGateFakeEntitlementSource entitled(true);
    GpxGateEntitlementSourceScope scope(&entitled);
    TEST(GpxGateAllowImportBatch(1), ());
    TEST(!GpxGateAllowImportBatch(2), ());
    GpxGateCapabilityAvailabilityScope atmAvail(explorer_pro::Capability::AdvancedTrackManagement, true);
    TEST(GpxGateAllowImportBatch(2), ());
  }
}

UNIT_TEST(GpxGate_DirectImportClosedStillPaints)
{
  GpxGateBreadcrumbCleanup cleanup;
  GpxGateResetCapabilities();
  GpxGateEntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  size_t const marked = fixture.Manager().ImportHistoricalTrack({ShortLineAt(lat, lon)});

  TEST_GREATER(marked, 0, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}
