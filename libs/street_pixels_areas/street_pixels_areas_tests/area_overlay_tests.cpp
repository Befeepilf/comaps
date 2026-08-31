#include "testing/testing.hpp"

#include "street_pixels_areas/area_overlay.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_tap.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "geometry/region2d.hpp"
#include "platform/platform.hpp"

#include <algorithm>
#include <cmath>
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

UNIT_TEST(AreaOverlay_CompletedCheckDrawPathInvokedWhenShowCheck)
{
  auto const cityDone = StyleForCompletion(1.0, AreaOverlayZoomBand::City);
  TEST(cityDone.m_showCheck, ());
  m2::RectD bounds(0.0, 0.0, 10.0, 8.0);
  m2::PointD const label = bounds.Center();
  auto const path = OverlayCheckDrawPath(cityDone, label, bounds);
  TEST_EQUAL(path.size(), 3u, ());
  TEST(!path[0].EqualDxDy(path[1], 1e-12), ());
  TEST(!path[1].EqualDxDy(path[2], 1e-12), ());
  auto const expected = CompletedCheckPolyline(label, std::max(bounds.SizeX(), bounds.SizeY()) * 0.12);
  TEST_EQUAL(path.size(), expected.size(), ());
  for (size_t i = 0; i < path.size(); ++i)
    TEST(path[i].EqualDxDy(expected[i], 1e-12), ());

  auto const neighbourhoodDone = StyleForCompletion(1.0, AreaOverlayZoomBand::Neighbourhood);
  TEST(neighbourhoodDone.m_showCheck, ());
  TEST_EQUAL(OverlayCheckDrawPath(neighbourhoodDone, label, bounds).size(), 3u, ());

  auto const streetDone = StyleForCompletion(1.0, AreaOverlayZoomBand::Street);
  TEST(!streetDone.m_showCheck, ());
  TEST(OverlayCheckDrawPath(streetDone, label, bounds).empty(), ());

  auto const inProgress = StyleForCompletion(0.99, AreaOverlayZoomBand::City);
  TEST(!inProgress.m_showCheck, ());
  TEST(OverlayCheckDrawPath(inProgress, label, bounds).empty(), ());

  TEST(CompletedCheckPolyline(label, 0.0).empty(), ());
  TEST(OverlayCheckDrawPath(cityDone, label, m2::RectD()).empty(), ());
  TEST(OverlayCheckDrawPath(cityDone, label, m2::RectD(1.0, 1.0, 1.0, 1.0)).empty(), ());
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
          MakeAdminCandidate(9, 9, "Borough", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(23.9, 59.9, 25.1, 61.1)),
      },
      policy);

  SpaWriteParams params;
  params.m_mapDataVersion = 370;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp037_overlay";
  std::vector<m2::PointD> samples = {MercatorFromLonLat(24.5, 60.5)};
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  std::vector<std::optional<double>> fractions(loaded.m_file.m_areas.size());
  for (auto const & a : loaded.m_file.m_areas)
  {
    if (a.m_compactIndex < fractions.size())
      fractions[a.m_compactIndex] = (a.m_role == AreaRole::Subdivision) ? 0.75 : 0.25;
  }

  auto geom = BuildAreaOverlayGeometry(loaded.m_file, policy, fractions, nullptr);
  TEST_EQUAL(geom.size(), 1u, ());
  TEST_EQUAL(geom[0].m_role, AreaRole::Subdivision, ());
  uint32_t districtIndex = 0;
  bool foundDistrict = false;
  for (auto const & a : loaded.m_file.m_areas)
  {
    if (a.m_name == "District")
    {
      districtIndex = a.m_compactIndex;
      foundDistrict = true;
    }
  }
  TEST(foundDistrict, ());
  TEST_EQUAL(geom[0].m_compactIndex, districtIndex, ());
  TEST_EQUAL(geom[0].m_name, "District", ());
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
  SpaFile empty;
  auto geom = BuildAreaOverlayGeometry(empty, CountryConfig::UnconfiguredPolicy(), {}, nullptr);
  TEST(geom.empty(), ());
}

UNIT_TEST(AreaOverlay_GeometryOnlyHasNoWinners)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp037_overlay_geom");
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
  params.m_mwmId = "sp037_overlay_geom";
  WriteExplorationSidecar(path, areas, /*samplePoints=*/{}, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  TEST_EQUAL(loaded.m_file.m_assignments.size(), 0u, ());

  auto geom = BuildAreaOverlayGeometry(loaded.m_file, policy, {}, nullptr);
  TEST(geom.empty(), ());

  RemoveIfExists(path);
}

UNIT_TEST(AreaOverlay_ClipNestedWinners)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp037_overlay_clip");
  RemoveIfExists(path);

  auto areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(9, 9, "Borough", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      policy);

  SpaWriteParams params;
  params.m_mapDataVersion = 370;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp037_overlay_clip";
  std::vector<m2::PointD> samples = {MercatorFromLonLat(24.5, 60.5), MercatorFromLonLat(24.1, 60.1)};
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  auto geom = BuildAreaOverlayGeometry(loaded.m_file, policy, {}, nullptr);
  TEST_EQUAL(geom.size(), 2u, ());

  AreaOverlayGeometry const * district = nullptr;
  AreaOverlayGeometry const * borough = nullptr;
  for (auto const & g : geom)
  {
    if (g.m_name == "District")
      district = &g;
    if (g.m_name == "Borough")
      borough = &g;
  }
  TEST(district != nullptr, ());
  TEST(borough != nullptr, ());
  TEST_EQUAL(district->m_rings.size(), 1u, ());
  TEST_GREATER_OR_EQUAL(borough->m_rings.size(), 2u, ());

  RemoveIfExists(path);
}

bool OverlayRingContains(AreaOverlayGeometry const & geom, m2::PointD const & mercator)
{
  int hits = 0;
  for (auto const & ring : geom.m_rings)
  {
    if (ring.size() < 3)
      continue;
    m2::RegionD region(ring.begin(), ring.end());
    if (region.Contains(mercator))
      ++hits;
  }
  return (hits % 2) == 1;
}

UNIT_TEST(AreaOverlay_ClipPrefersConfiguredPriorityOverArea)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp037_overlay_rank");
  RemoveIfExists(path);

  auto areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(11, 11, "Fine", LonLatBox(24.8, 60.4, 25.2, 60.6)),
      },
      policy);

  SpaWriteParams params;
  params.m_mapDataVersion = 370;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp037_overlay_rank";
  std::vector<m2::PointD> samples = {MercatorFromLonLat(24.5, 60.5), MercatorFromLonLat(25.1, 60.5)};
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  auto geom = BuildAreaOverlayGeometry(loaded.m_file, policy, {}, nullptr);
  TEST_EQUAL(geom.size(), 2u, ());

  AreaOverlayGeometry const * district = nullptr;
  AreaOverlayGeometry const * fine = nullptr;
  for (auto const & g : geom)
  {
    if (g.m_name == "District")
      district = &g;
    if (g.m_name == "Fine")
      fine = &g;
  }
  TEST(district != nullptr, ());
  TEST(fine != nullptr, ());
  TEST_EQUAL(district->m_rings.size(), 1u, ());
  TEST(OverlayRingContains(*district, MercatorFromLonLat(24.5, 60.5)), ());
  TEST(!OverlayRingContains(*fine, MercatorFromLonLat(24.5, 60.5)), ());
  TEST(OverlayRingContains(*fine, MercatorFromLonLat(25.1, 60.5)), ());

  RemoveIfExists(path);
}

UNIT_TEST(AreaOverlay_SettlementFallbackForSentinel)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp037_overlay_settle");
  RemoveIfExists(path);

  auto areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(23.9, 59.9, 25.1, 61.1)),
      },
      policy);

  SpaWriteParams params;
  params.m_mapDataVersion = 370;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp037_overlay_settle";
  std::vector<m2::PointD> samples = {MercatorFromLonLat(24.5, 60.5), MercatorFromLonLat(24.0, 60.0)};
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  auto geom = BuildAreaOverlayGeometry(loaded.m_file, policy, {}, nullptr);
  TEST_EQUAL(geom.size(), 2u, ());

  AreaOverlayGeometry const * district = nullptr;
  AreaOverlayGeometry const * city = nullptr;
  for (auto const & g : geom)
  {
    if (g.m_name == "District")
      district = &g;
    if (g.m_name == "City")
      city = &g;
  }
  TEST(district != nullptr, ());
  TEST(city != nullptr, ());
  TEST_EQUAL(city->m_role, AreaRole::Settlement, ());
  TEST_GREATER_OR_EQUAL(city->m_rings.size(), 2u, ());
  TEST(OverlayRingContains(*city, MercatorFromLonLat(24.0, 60.0)), ());
  TEST(!OverlayRingContains(*city, MercatorFromLonLat(24.5, 60.5)), ());

  RemoveIfExists(path);
}

UNIT_TEST(AreaOverlay_FillAlphaFromOpacityPct)
{
  TEST_EQUAL(FillAlphaFromOpacityPct(0), 0, ());
  TEST_EQUAL(FillAlphaFromOpacityPct(22), 56, ());
  TEST_EQUAL(FillAlphaFromOpacityPct(100), 255, ());
  TEST_EQUAL(FillAlphaFromOpacityPct(-10), 0, ());
  TEST_EQUAL(FillAlphaFromOpacityPct(200), 255, ());

  auto const street = StyleForCompletion(0.5, AreaOverlayZoomBand::Street);
  TEST(!street.m_showFill, ());
  auto const neighbourhood = StyleForCompletion(0.5, AreaOverlayZoomBand::Neighbourhood);
  TEST(neighbourhood.m_showFill, ());
  TEST_EQUAL(neighbourhood.m_fill.m_a, 55, ());
  TEST_EQUAL(street.m_fill.m_r, neighbourhood.m_fill.m_r, ());
  TEST_EQUAL(street.m_fill.m_g, neighbourhood.m_fill.m_g, ());
  TEST_EQUAL(street.m_fill.m_b, neighbourhood.m_fill.m_b, ());
  TEST_EQUAL(street.m_fill.m_a, 0, ());
  auto const streetDone = StyleForCompletion(1.0, AreaOverlayZoomBand::Street);
  TEST(!streetDone.m_showFill, ());
  TEST_EQUAL(streetDone.m_fill.m_r, 40, ());
  TEST_EQUAL(streetDone.m_fill.m_g, 160, ());
  TEST_EQUAL(streetDone.m_fill.m_b, 80, ());
}

UNIT_TEST(AreaOverlay_FormatPercent)
{
  TEST_EQUAL(FormatAreaOverlayPercent(0.0), "0%", ());
  TEST_EQUAL(FormatAreaOverlayPercent(0.42), "42%", ());
  TEST_EQUAL(FormatAreaOverlayPercent(1.0), "100%", ());
  TEST_EQUAL(FormatAreaOverlayPercent(1.5), "100%", ());
}

UNIT_TEST(AreaOverlay_ChromeVisibility)
{
  auto const neither = MakeAreaOverlayChrome(false, false, 28.0f, "District");
  TEST(!neither.m_showName, ());
  TEST(!neither.m_showPct, ());
  TEST_EQUAL(neither.m_halfSizePx.x, 0.0, ());
  TEST_EQUAL(neither.m_halfSizePx.y, 0.0, ());

  auto const nameOnly = MakeAreaOverlayChrome(true, false, 28.0f, "District");
  TEST(nameOnly.m_showName, ());
  TEST(!nameOnly.m_showPct, ());
  TEST_GREATER(nameOnly.m_halfSizePx.x, 0.0, ());
  TEST_GREATER(nameOnly.m_halfSizePx.y, 0.0, ());

  auto const pctOnly = MakeAreaOverlayChrome(false, true, 28.0f, "District");
  TEST(!pctOnly.m_showName, ());
  TEST(pctOnly.m_showPct, ());
  TEST_GREATER(pctOnly.m_halfSizePx.x, kAreaOverlayRingRadiusPx, ());
  TEST_EQUAL(pctOnly.m_ringOffsetPx.x, 0.0f, ());
  TEST_EQUAL(pctOnly.m_ringOffsetPx.y, 0.0f, ());

  auto const both = MakeAreaOverlayChrome(true, true, 28.0f, "District");
  TEST(both.m_showName, ());
  TEST(both.m_showPct, ());
  TEST_GREATER(both.m_halfSizePx.y, nameOnly.m_halfSizePx.y, ());
  TEST_LESS(both.m_ringOffsetPx.y, 0.0f, ());

  auto const emptyName = MakeAreaOverlayChrome(true, false, 28.0f, "");
  TEST(!emptyName.m_showName, ());
}

UNIT_TEST(AreaOverlay_ChromeHitAabbCoversRing)
{
  auto const both = MakeAreaOverlayChrome(true, true, 28.0f, "District");
  double const ringTop = std::abs(static_cast<double>(both.m_ringOffsetPx.y)) + kAreaOverlayRingRadiusPx;
  TEST_GREATER_OR_EQUAL(both.m_halfSizePx.y, ringTop - 0.01, ());

  std::vector<street_pixels::AreaLabelHitTarget> labels;
  labels.push_back({1u, {100.0, 100.0}, both.m_halfSizePx});
  m2::PointD const ringPx{100.0 + both.m_ringOffsetPx.x, 100.0 + both.m_ringOffsetPx.y};
  TEST(street_pixels::HitExplorationAreaLabel(labels, ringPx).has_value(), ());

  auto const nameOnly = MakeAreaOverlayChrome(true, false, 28.0f, "District");
  std::vector<street_pixels::AreaLabelHitTarget> cityLabels;
  cityLabels.push_back({1u, {100.0, 100.0}, nameOnly.m_halfSizePx});
  TEST(!street_pixels::HitExplorationAreaLabel(cityLabels, ringPx).has_value(), ());
}
}  // namespace
