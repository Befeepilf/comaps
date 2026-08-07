#include "street_pixels_areas/area_overlay.hpp"

#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

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
      style.m_outlineWidthPx = 2.5f;
      style.m_showCheck = true;
      break;
    case AreaOverlayZoomBand::Neighbourhood:
      style.m_showFill = true;
      style.m_fill = Rgba8{40, 160, 80, 36};
      style.m_outlineWidthPx = 3.0f;
      style.m_showCheck = true;
      break;
    case AreaOverlayZoomBand::Street:
      style.m_showFill = false;
      style.m_outlineWidthPx = 2.25f;
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
    style.m_outlineWidthPx = 1.5f;
    break;
  case AreaOverlayZoomBand::Neighbourhood:
    style.m_showFill = true;
    style.m_fill = Rgba8{r, g, b, 55};
    style.m_outlineWidthPx = 2.0f;
    break;
  case AreaOverlayZoomBand::Street:
    style.m_showFill = false;
    style.m_outlineWidthPx = 1.25f;
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
    SpaFile const & file, std::vector<std::optional<double>> const & fractionByCompactIndex,
    m2::RectD const * viewportOrNull)
{
  std::vector<AreaOverlayGeometry> out;
  out.reserve(file.m_areas.size());

  for (auto const & area : file.m_areas)
  {
    if (!(area.IsAssignable() || area.m_role == AreaRole::Settlement))
      continue;
    if (area.m_rings.empty())
      continue;

    AreaOverlayGeometry geom;
    geom.m_compactIndex = area.m_compactIndex;
    geom.m_role = area.m_role;
    if (area.m_compactIndex < fractionByCompactIndex.size() && fractionByCompactIndex[area.m_compactIndex].has_value())
      geom.m_fraction = *fractionByCompactIndex[area.m_compactIndex];
    else
      geom.m_fraction = 0.0;

    for (auto const & ring : area.m_rings)
    {
      if (ring.size() < 3)
        continue;
      for (auto const & pt : ring)
        geom.m_bounds.Add(pt);
      auto simplified = SimplifyRingForOverlay(ring, AreaOverlayZoomBand::Neighbourhood);
      auto tris = TriangulateOuterRing(simplified);
      if (!tris.empty())
      {
        geom.m_rings.push_back(std::move(simplified));
        geom.m_triangles.insert(geom.m_triangles.end(), tris.begin(), tris.end());
      }
    }

    if (geom.m_rings.empty())
      continue;
    if (viewportOrNull != nullptr && !viewportOrNull->IsIntersect(geom.m_bounds))
      continue;
    out.push_back(std::move(geom));
  }
  return out;
}
}  // namespace street_pixels
