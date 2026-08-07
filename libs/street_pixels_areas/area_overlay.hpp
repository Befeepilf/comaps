#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace street_pixels
{
// Zoom bands for §12.1–§12.3. City band matches SP-036 kCityScaleMaxDrawScale.
inline constexpr int kAreaOverlayMinZoom = 9;
inline constexpr int kAreaOverlayCityMaxZoom = 12;
inline constexpr int kAreaOverlayNeighbourhoodMaxZoom = 15;

enum class AreaOverlayZoomBand : uint8_t
{
  Hidden = 0,
  City = 1,
  Neighbourhood = 2,
  Street = 3,
};

inline AreaOverlayZoomBand ClassifyAreaOverlayZoom(int drawScale)
{
  if (drawScale < kAreaOverlayMinZoom)
    return AreaOverlayZoomBand::Hidden;
  if (drawScale <= kAreaOverlayCityMaxZoom)
    return AreaOverlayZoomBand::City;
  if (drawScale <= kAreaOverlayNeighbourhoodMaxZoom)
    return AreaOverlayZoomBand::Neighbourhood;
  return AreaOverlayZoomBand::Street;
}

struct Rgba8
{
  uint8_t m_r = 0;
  uint8_t m_g = 0;
  uint8_t m_b = 0;
  uint8_t m_a = 0;
};

inline bool operator==(Rgba8 const & a, Rgba8 const & b)
{
  return a.m_r == b.m_r && a.m_g == b.m_g && a.m_b == b.m_b && a.m_a == b.m_a;
}

struct AreaOverlayStyle
{
  Rgba8 m_fill;
  Rgba8 m_outline;
  float m_outlineWidthPx = 1.0f;
  bool m_showFill = false;
  bool m_showOutline = false;
};

// Completion shading: low → higher alpha red-ish; high → green. Street band is outline-only.
AreaOverlayStyle StyleForCompletion(double fraction, AreaOverlayZoomBand band);

// Fan triangulation for simple outer rings (no holes). Returns empty on failure.
std::vector<m2::PointD> TriangulateOuterRing(std::vector<m2::PointD> const & ring);

// Decimate ring vertices for city-scale LOD (SP-033: keep pixel circles; thin rings instead).
std::vector<m2::PointD> SimplifyRingForOverlay(std::vector<m2::PointD> const & ring, AreaOverlayZoomBand band);

struct AreaOverlayGeometry
{
  uint32_t m_compactIndex = 0;
  AreaRole m_role = AreaRole::Subdivision;
  double m_fraction = 0.0;
  std::vector<std::vector<m2::PointD>> m_rings;
  std::vector<m2::PointD> m_triangles;
  m2::RectD m_bounds;
};

// Build overlay geometry from sidecar areas + per-area personal completion.
// Includes assignable areas; settlements included for City band consumers.
// fractionOrNull: nullopt → treat as 0. viewport may cull by bounds.
std::vector<AreaOverlayGeometry> BuildAreaOverlayGeometry(
    SpaFile const & file, std::vector<std::optional<double>> const & fractionByCompactIndex,
    m2::RectD const * viewportOrNull = nullptr);

inline std::string DebugPrint(AreaOverlayZoomBand band)
{
  switch (band)
  {
  case AreaOverlayZoomBand::Hidden: return "Hidden";
  case AreaOverlayZoomBand::City: return "City";
  case AreaOverlayZoomBand::Neighbourhood: return "Neighbourhood";
  case AreaOverlayZoomBand::Street: return "Street";
  }
  return "UnknownAreaOverlayZoomBand";
}
}  // namespace street_pixels
