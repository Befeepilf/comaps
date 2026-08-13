#pragma once

#include "drape/color.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace df
{
// Snapshot pushed from StreetPixelsManager (SP-037). Colors are baked by the
// shared street_pixels::area_overlay module so drape stays free of that lib.
// Street-pixel circles remain in StreetPixelRenderer (SP-033 keep-current).
struct ExplorationAreaOverlayItem
{
  uint32_t m_compactIndex = 0;
  double m_fraction = 0.0;
  bool m_completed = false;
  std::string m_name;
  m2::PointD m_labelPoint;
  std::vector<std::vector<m2::PointD>> m_rings;
  std::vector<m2::PointD> m_triangles;
  m2::RectD m_bounds;
  dp::Color m_fillColor;
  dp::Color m_outlineColor;
  float m_outlineWidthPx = 1.5f;
};

inline constexpr int kExplorationAreaOverlayMinZoom = 9;
inline constexpr int kExplorationAreaOverlayCityMaxZoom = 12;
inline constexpr int kExplorationAreaOverlayNeighbourhoodMaxZoom = 15;
}  // namespace df
