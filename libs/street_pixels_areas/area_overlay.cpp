#include "street_pixels_areas/area_overlay.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/bg_point.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include "std/boost_geometry.hpp"

namespace street_pixels
{
namespace
{
double Clamp01(double v)
{
  if (v < 0.0)
    return 0.0;
  if (v > 1.0)
    return 1.0;
  return v;
}

std::vector<m2::PointD> DropClosingDuplicate(std::vector<m2::PointD> ring)
{
  if (ring.size() >= 2 && ring.front().EqualDxDy(ring.back(), 1e-9))
    ring.pop_back();
  return ring;
}

bool PointInTriangle(m2::PointD const & p, m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
{
  auto cross = [](m2::PointD const & u, m2::PointD const & v) { return u.x * v.y - u.y * v.x; };
  m2::PointD const ab = b - a;
  m2::PointD const bc = c - b;
  m2::PointD const ca = a - c;
  double const c1 = cross(ab, p - a);
  double const c2 = cross(bc, p - b);
  double const c3 = cross(ca, p - c);
  bool const hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0);
  bool const hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0);
  return !(hasNeg && hasPos);
}

double SignedArea(std::vector<m2::PointD> const & poly)
{
  double a = 0.0;
  size_t const n = poly.size();
  for (size_t i = 0; i < n; ++i)
  {
    auto const & p0 = poly[i];
    auto const & p1 = poly[(i + 1) % n];
    a += p0.x * p1.y - p1.x * p0.y;
  }
  return 0.5 * a;
}

bool IsEar(std::vector<m2::PointD> const & poly, size_t i, bool ccw)
{
  size_t const n = poly.size();
  size_t const prev = (i + n - 1) % n;
  size_t const next = (i + 1) % n;
  m2::PointD const & a = poly[prev];
  m2::PointD const & b = poly[i];
  m2::PointD const & c = poly[next];
  double const cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  if (ccw ? cross <= 1e-12 : cross >= -1e-12)
    return false;
  for (size_t j = 0; j < n; ++j)
  {
    if (j == prev || j == i || j == next)
      continue;
    if (PointInTriangle(poly[j], a, b, c))
      return false;
  }
  return true;
}

using BgPoly = boost::geometry::model::polygon<m2::PointD>;
using BgMulti = boost::geometry::model::multi_polygon<BgPoly>;

BgMulti RingsToMulti(std::vector<std::vector<m2::PointD>> const & rings)
{
  BgMulti multi;
  for (auto const & ring : rings)
  {
    if (ring.size() < 3)
      continue;
    BgPoly poly;
    poly.outer().assign(ring.begin(), ring.end());
    if (poly.outer().size() >= 2 && !poly.outer().front().EqualDxDy(poly.outer().back(), 1e-12))
      poly.outer().push_back(poly.outer().front());
    boost::geometry::correct(poly);
    if (boost::geometry::num_points(poly) >= 4)
      multi.push_back(std::move(poly));
  }
  return multi;
}

std::vector<std::vector<m2::PointD>> MultiToRings(BgMulti const & multi)
{
  std::vector<std::vector<m2::PointD>> rings;
  for (auto const & poly : multi)
  {
    if (poly.outer().size() >= 4)
      rings.emplace_back(poly.outer().begin(), poly.outer().end());
    for (auto const & inner : poly.inners())
    {
      if (inner.size() >= 4)
        rings.emplace_back(inner.begin(), inner.end());
    }
  }
  return rings;
}

std::vector<m2::PointD> KeyholeRing(BgPoly const & poly)
{
  auto ring = DropClosingDuplicate({poly.outer().begin(), poly.outer().end()});
  for (auto const & inner : poly.inners())
  {
    auto hole = DropClosingDuplicate({inner.begin(), inner.end()});
    if (ring.size() < 3 || hole.size() < 3)
      continue;
    size_t bestI = 0;
    size_t bestJ = 0;
    double best = std::numeric_limits<double>::max();
    for (size_t i = 0; i < ring.size(); ++i)
    {
      for (size_t j = 0; j < hole.size(); ++j)
      {
        double const d = ring[i].SquaredLength(hole[j]);
        if (d < best)
        {
          best = d;
          bestI = i;
          bestJ = j;
        }
      }
    }
    std::vector<m2::PointD> injected;
    injected.reserve(hole.size() + 2);
    for (size_t k = 0; k < hole.size(); ++k)
      injected.push_back(hole[(bestJ + k) % hole.size()]);
    injected.push_back(hole[bestJ]);
    injected.push_back(ring[bestI]);
    ring.insert(ring.begin() + static_cast<std::ptrdiff_t>(bestI) + 1, injected.begin(), injected.end());
  }
  if (ring.size() >= 3)
    ring.push_back(ring.front());
  return ring;
}

BgMulti SubtractBetter(BgMulti const & subject, BgMulti const & clipper)
{
  if (boost::geometry::is_empty(clipper) || boost::geometry::is_empty(subject))
    return subject;
  BgMulti out;
  try
  {
    boost::geometry::difference(subject, clipper, out);
  }
  catch (...)
  {
    return subject;
  }
  return out;
}

bool WinnerBetter(ExplorationArea const & a, ExplorationArea const & b, CountryPolicy const & policy)
{
  int const rankA = SubdivisionPriorityRank(a, policy);
  int const rankB = SubdivisionPriorityRank(b, policy);
  if (rankA != rankB)
    return rankA < rankB;
  if (a.m_area != b.m_area)
    return a.m_area < b.m_area;
  if (a.m_osmId != b.m_osmId)
    return a.m_osmId < b.m_osmId;
  return a.m_compactIndex < b.m_compactIndex;
}

std::vector<m2::PointD> ClosedRing(std::vector<m2::PointD> ring)
{
  ring = DropClosingDuplicate(std::move(ring));
  if (ring.size() < 3)
    return {};
  ring.push_back(ring.front());
  return ring;
}
}  // namespace

AreaOverlayStyle StyleForCompletion(double fraction, AreaOverlayZoomBand band)
{
  AreaOverlayStyle style;
  if (band == AreaOverlayZoomBand::Hidden)
    return style;

  if (IsAreaCompleted(fraction))
  {
    // §18.6: green completion outline + restrained fill; street stays outline-only.
    style.m_completed = true;
    style.m_showOutline = true;
    style.m_outline = Rgba8{24, 150, 72, 255};
    switch (band)
    {
    case AreaOverlayZoomBand::City:
      style.m_showFill = true;
      style.m_fill = Rgba8{40, 160, 80, 48};
      style.m_outlineWidthPx = 4.5f;
      style.m_showCheck = true;
      break;
    case AreaOverlayZoomBand::Neighbourhood:
      style.m_showFill = true;
      style.m_fill = Rgba8{40, 160, 80, 36};
      style.m_outlineWidthPx = 5.5f;
      style.m_showCheck = true;
      break;
    case AreaOverlayZoomBand::Street:
      style.m_showFill = false;
      style.m_outlineWidthPx = 4.0f;
      style.m_outline.m_a = 230;
      break;
    case AreaOverlayZoomBand::Hidden:
      break;
    }
    return style;
  }

  double const t = Clamp01(fraction);
  // Interpolate unexplored-ish red → explored green for fill tint.
  uint8_t const r = static_cast<uint8_t>(std::lround(220.0 * (1.0 - t) + 40.0 * t));
  uint8_t const g = static_cast<uint8_t>(std::lround(60.0 * (1.0 - t) + 180.0 * t));
  uint8_t const b = static_cast<uint8_t>(std::lround(60.0 * (1.0 - t) + 90.0 * t));

  style.m_outline = Rgba8{static_cast<uint8_t>(r / 2), static_cast<uint8_t>(g / 2), static_cast<uint8_t>(b / 2), 220};
  style.m_showOutline = true;

  switch (band)
  {
  case AreaOverlayZoomBand::City:
    style.m_showFill = true;
    style.m_fill = Rgba8{r, g, b, 90};
    style.m_outlineWidthPx = 4.0f;
    break;
  case AreaOverlayZoomBand::Neighbourhood:
    style.m_showFill = true;
    style.m_fill = Rgba8{r, g, b, 55};
    style.m_outlineWidthPx = 4.5f;
    break;
  case AreaOverlayZoomBand::Street:
    style.m_showFill = false;
    style.m_outlineWidthPx = 3.0f;
    style.m_outline.m_a = 160;
    break;
  case AreaOverlayZoomBand::Hidden:
    break;
  }
  return style;
}

std::vector<m2::PointD> TriangulateOuterRing(std::vector<m2::PointD> const & ringIn)
{
  auto poly = DropClosingDuplicate(ringIn);
  if (poly.size() < 3)
    return {};

  bool const ccw = SignedArea(poly) > 0.0;
  std::vector<m2::PointD> tris;
  tris.reserve((poly.size() - 2) * 3);

  while (poly.size() > 3)
  {
    bool clipped = false;
    size_t const n = poly.size();
    for (size_t i = 0; i < n; ++i)
    {
      if (!IsEar(poly, i, ccw))
        continue;
      size_t const prev = (i + n - 1) % n;
      size_t const next = (i + 1) % n;
      tris.push_back(poly[prev]);
      tris.push_back(poly[i]);
      tris.push_back(poly[next]);
      poly.erase(poly.begin() + static_cast<std::ptrdiff_t>(i));
      clipped = true;
      break;
    }
    if (!clipped)
      return {};
  }
  tris.push_back(poly[0]);
  tris.push_back(poly[1]);
  tris.push_back(poly[2]);
  return tris;
}

std::vector<m2::PointD> SimplifyRingForOverlay(std::vector<m2::PointD> const & ring, AreaOverlayZoomBand band)
{
  auto poly = DropClosingDuplicate(ring);
  if (poly.size() < 3)
    return ring;

  size_t maxVerts = poly.size();
  if (band == AreaOverlayZoomBand::City)
    maxVerts = std::min(maxVerts, size_t{48});
  else if (band == AreaOverlayZoomBand::Neighbourhood)
    maxVerts = std::min(maxVerts, size_t{96});

  if (poly.size() <= maxVerts)
  {
    poly.push_back(poly.front());
    return poly;
  }

  std::vector<m2::PointD> out;
  out.reserve(maxVerts + 1);
  double const step = static_cast<double>(poly.size()) / static_cast<double>(maxVerts);
  for (size_t i = 0; i < maxVerts; ++i)
  {
    size_t const idx = std::min(poly.size() - 1, static_cast<size_t>(i * step));
    if (out.empty() || !out.back().EqualDxDy(poly[idx], 1e-9))
      out.push_back(poly[idx]);
  }
  if (out.size() < 3)
    return ring;
  out.push_back(out.front());
  return out;
}

std::vector<AreaOverlayGeometry> BuildAreaOverlayGeometry(
    SpaFile const & file, CountryPolicy const & policy,
    std::vector<std::optional<double>> const & fractionByCompactIndex,
    m2::RectD const * viewportOrNull)
{
  std::vector<uint8_t> winnerMask(file.m_areas.size(), 0);
  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  bool anySentinel = false;
  for (uint32_t idx : file.m_assignments)
  {
    if (idx == sentinel)
    {
      anySentinel = true;
      continue;
    }
    if (idx < winnerMask.size())
      winnerMask[idx] = 1;
  }

  std::vector<ExplorationArea const *> winners;
  winners.reserve(file.m_areas.size());
  for (auto const & area : file.m_areas)
  {
    if (area.m_rings.empty() || area.m_compactIndex >= winnerMask.size())
      continue;
    bool const assignedWinner = winnerMask[area.m_compactIndex] != 0;
    bool const settlementFallback =
        anySentinel && policy.m_configured && area.m_role == AreaRole::Settlement;
    if (!assignedWinner && !settlementFallback)
      continue;
    winners.push_back(&area);
  }
  std::sort(winners.begin(), winners.end(),
            [&policy](ExplorationArea const * a, ExplorationArea const * b)
            { return WinnerBetter(*a, *b, policy); });

  std::vector<BgMulti> betterPolys;
  betterPolys.reserve(winners.size());
  std::vector<m2::RectD> betterBounds;
  betterBounds.reserve(winners.size());

  std::vector<AreaOverlayGeometry> out;
  out.reserve(winners.size());

  for (auto const * area : winners)
  {
    BgMulti clipped = RingsToMulti(area->m_rings);
    m2::RectD bbox;
    for (auto const & ring : area->m_rings)
    {
      for (auto const & pt : ring)
        bbox.Add(pt);
    }
    for (size_t i = 0; i < betterPolys.size(); ++i)
    {
      if (!bbox.IsIntersect(betterBounds[i]))
        continue;
      clipped = SubtractBetter(clipped, betterPolys[i]);
    }
    betterPolys.push_back(RingsToMulti(area->m_rings));
    betterBounds.push_back(bbox);

    auto rings = MultiToRings(clipped);
    if (rings.empty())
      continue;

    AreaOverlayGeometry geom;
    geom.m_compactIndex = area->m_compactIndex;
    geom.m_role = area->m_role;
    geom.m_name = area->m_name;
    if (area->m_compactIndex < fractionByCompactIndex.size() &&
        fractionByCompactIndex[area->m_compactIndex].has_value())
      geom.m_fraction = *fractionByCompactIndex[area->m_compactIndex];
    else
      geom.m_fraction = 0.0;

    for (auto const & ring : rings)
    {
      auto closed = ClosedRing(ring);
      if (closed.size() < 4)
        continue;
      for (auto const & pt : closed)
        geom.m_bounds.Add(pt);
      geom.m_rings.push_back(std::move(closed));
    }

    for (auto const & poly : clipped)
    {
      auto tris = TriangulateOuterRing(KeyholeRing(poly));
      geom.m_triangles.insert(geom.m_triangles.end(), tris.begin(), tris.end());
    }

    if (geom.m_rings.empty())
      continue;
    if (viewportOrNull != nullptr && !viewportOrNull->IsIntersect(geom.m_bounds))
      continue;

    if (geom.m_triangles.size() >= 3)
    {
      geom.m_labelPoint =
          m2::ApplyCalculator(geom.m_triangles, m2::CalculatePointOnSurface(geom.m_bounds));
    }
    else
      geom.m_labelPoint = geom.m_bounds.Center();

    out.push_back(std::move(geom));
  }
  return out;
}
}  // namespace street_pixels
