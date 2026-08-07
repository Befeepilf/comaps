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
  TEST(!street.m_completed, ());

  auto const city = StyleForCompletion(0.99, AreaOverlayZoomBand::City);
  TEST(city.m_showFill, ());
  TEST(city.m_showOutline, ());
  TEST(!city.m_completed, ());
  TEST_GREATER(city.m_fill.m_g, city.m_fill.m_r, ());
}

UNIT_TEST(AreaOverlay_StyleCompletedDistinctFromInProgress)
{
  auto const inProgress = StyleForCompletion(0.99, AreaOverlayZoomBand::Neighbourhood);
  auto const completed = StyleForCompletion(1.0, AreaOverlayZoomBand::Neighbourhood);
  TEST(!inProgress.m_completed, ());
  TEST(completed.m_completed, ());
  TEST(completed.m_showOutline, ());
  TEST(completed.m_showFill, ());
  TEST(completed.m_showCheck, ());
  TEST_GREATER(completed.m_outlineWidthPx, inProgress.m_outlineWidthPx, ());
  TEST(!(completed.m_outline == inProgress.m_outline), ());

  auto const streetDone = StyleForCompletion(1.0, AreaOverlayZoomBand::Street);
  TEST(streetDone.m_completed, ());
  TEST(streetDone.m_showOutline, ());
  TEST(!streetDone.m_showFill, ());
  TEST(!streetDone.m_showCheck, ());

  auto const cityDone = StyleForCompletion(1.0, AreaOverlayZoomBand::City);
  TEST(cityDone.m_completed, ());
  TEST(cityDone.m_showFill, ());
  TEST(cityDone.m_showCheck, ());

  TEST(IsAreaCompleted(1.0), ());
  TEST(!IsAreaCompleted(0.999), ());
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
