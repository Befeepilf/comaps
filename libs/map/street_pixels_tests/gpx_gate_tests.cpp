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

class FakeEntitlementSource : public explorer_pro::EntitlementSource
{
public:
  explicit FakeEntitlementSource(bool entitled) : m_entitled(entitled) {}

  bool IsEntitled() const override { return m_entitled; }

private:
  bool m_entitled;
};

class EntitlementSourceScope
{
public:
  explicit EntitlementSourceScope(explorer_pro::EntitlementSource * source)
  {
    explorer_pro::SetEntitlementSource(source);
  }

  ~EntitlementSourceScope() { explorer_pro::SetEntitlementSource(nullptr); }
};

class CapabilityAvailabilityScope
{
public:
  CapabilityAvailabilityScope(explorer_pro::Capability capability, bool available)
    : m_capability(capability)
    , m_previous(explorer_pro::IsCapabilityAvailable(capability))
  {
    explorer_pro::SetCapabilityAvailable(capability, available);
  }

  ~CapabilityAvailabilityScope() { explorer_pro::SetCapabilityAvailable(m_capability, m_previous); }

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

void ResetCapabilities()
{
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
}  // namespace

UNIT_TEST(GpxGate_HandlerClosedDoesNotPaint)
{
  GpxGateBreadcrumbCleanup cleanup;
  ResetCapabilities();
  EntitlementSourceScope scope(nullptr);
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
  ResetCapabilities();
  CapabilityAvailabilityScope availability(explorer_pro::Capability::GpxImport, true);
  FakeEntitlementSource entitled(true);
  EntitlementSourceScope scope(&entitled);
  TEST(explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  HandleHistoricalTrackImport(fixture.Manager(), {ShortLineAt(lat, lon)});

  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}

UNIT_TEST(GpxGate_DirectImportClosedStillPaints)
{
  GpxGateBreadcrumbCleanup cleanup;
  ResetCapabilities();
  EntitlementSourceScope scope(nullptr);
  TEST(!explorer_pro::IsCapabilityEnabled(explorer_pro::Capability::GpxImport), ());

  GpxGateFixture fixture;
  auto const [lat, lon] = street_pixels_tests::LatLonForPixelId(street_pixels_tests::PixelIdForLatLon(48.2, 16.37));
  auto const pixelA = street_pixels_tests::PixelIdForLatLon(lat, lon);
  fixture.Manager().SetStreetPixelsForTesting(street_pixels_tests::MakePixelSet({{pixelA, false}}));

  size_t const marked = fixture.Manager().ImportHistoricalTrack({ShortLineAt(lat, lon)});

  TEST_GREATER(marked, 0, ());
  TEST(fixture.Manager().IsPixelExploredForTesting(pixelA), ());
}
