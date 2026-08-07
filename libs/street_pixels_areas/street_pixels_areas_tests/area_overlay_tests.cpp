#include "testing/testing.hpp"

#include "street_pixels_areas/area_overlay.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "platform/platform.hpp"

#include <optional>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

UNIT_TEST(AreaOverlay_ClassifyZoomBands)
{
  TEST_EQUAL(ClassifyAreaOverlayZoom(8), AreaOverlayZoomBand::Hidden, ());
  TEST_EQUAL(ClassifyAreaOverlayZoom(9), AreaOverlayZoomBand::City, ());
  TEST_EQUAL(ClassifyAreaOverlayZoom(12), AreaOverlayZoomBand::City, ());
  TEST_EQUAL(ClassifyAreaOverlayZoom(13), AreaOverlayZoomBand::Neighbourhood, ());
  TEST_EQUAL(ClassifyAreaOverlayZoom(15), AreaOverlayZoomBand::Neighbourhood, ());
  TEST_EQUAL(ClassifyAreaOverlayZoom(16), AreaOverlayZoomBand::Street, ());
}

UNIT_TEST(AreaOverlay_StyleStreetOutlineOnly)
{
  auto const street = StyleForCompletion(0.5, AreaOverlayZoomBand::Street);
  TEST(street.m_showOutline, ());
  TEST(!street.m_showFill, ());

  auto const city = StyleForCompletion(1.0, AreaOverlayZoomBand::City);
  TEST(city.m_showFill, ());
  TEST(city.m_showOutline, ());
  TEST_GREATER(city.m_fill.m_g, city.m_fill.m_r, ());
}

UNIT_TEST(AreaOverlay_TriangulateBox)
{
  std::vector<m2::PointD> const ring = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}};
  auto const tris = TriangulateOuterRing(ring);
  TEST(!tris.empty(), ());
  TEST_EQUAL(tris.size() % 3, 0u, ());
  TEST_GREATER_OR_EQUAL(tris.size(), 6u, ());
}

UNIT_TEST(AreaOverlay_BuildFromSidecar)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp037_overlay");
  RemoveIfExists(path);

  auto areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      policy);

  SpaWriteParams params;
  params.m_mapDataVersion = 370;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp037_overlay";
  std::vector<m2::PointD> samples = {MercatorFromLonLat(24.5, 60.5), MercatorFromLonLat(24.1, 60.1)};
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  std::vector<std::optional<double>> fractions(loaded.m_file.m_areas.size());
  for (auto const & a : loaded.m_file.m_areas)
  {
    if (a.m_compactIndex < fractions.size())
      fractions[a.m_compactIndex] = (a.m_role == AreaRole::Subdivision) ? 0.75 : 0.25;
  }

  auto geom = BuildAreaOverlayGeometry(loaded.m_file, fractions, nullptr);
  TEST_GREATER_OR_EQUAL(geom.size(), 2u, ());
  for (auto const & g : geom)
  {
    TEST(!g.m_rings.empty(), ());
    TEST_GREATER_OR_EQUAL(g.m_triangles.size(), 3u, ());
    TEST_EQUAL(g.m_triangles.size() % 3, 0u, ());
  }

  RemoveIfExists(path);
}

UNIT_TEST(AreaOverlay_NoCountryChoropleth)
{
  // Builder only emits assignable + settlement areas from a sidecar — never invents
  // country/world aggregates (spec §12.4).
  SpaFile empty;
  auto geom = BuildAreaOverlayGeometry(empty, {}, nullptr);
  TEST(geom.empty(), ());
}
}  // namespace
