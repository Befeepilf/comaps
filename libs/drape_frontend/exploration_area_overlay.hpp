#pragma once

#include "drape/color.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <algorithm>
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
  dp::Color m_ringColor;
  float m_outlineWidthPx = 1.5f;
  bool m_showCheck = false;
  std::vector<m2::PointD> m_checkPolyline;
  bool m_showName = true;
  bool m_showPct = false;
  float m_fontSize = 28.0f;
  std::string m_percentText;
  m2::PointF m_ringOffsetPx;
};

inline constexpr int kExplorationAreaOverlayMinZoom = 9;
inline constexpr int kExplorationAreaOverlayCityMaxZoom = 12;
inline constexpr int kExplorationAreaOverlayNeighbourhoodMaxZoom = 15;
inline constexpr int kExplorationAreaOverlayMaxZoom = 20;
inline constexpr int kExplorationAreaOverlayLabelMinZoom = 13;
inline constexpr float kExplorationAreaOverlayRingRadiusPx = 16.0f;

struct ExplorationAreaOverlayZoomRange
{
  int m_labelMinZoom = kExplorationAreaOverlayLabelMinZoom;
  int m_labelMaxZoom = kExplorationAreaOverlayMaxZoom;
  int m_fillMinZoom = kExplorationAreaOverlayMinZoom;
  int m_fillMaxZoom = kExplorationAreaOverlayNeighbourhoodMaxZoom;
};

inline bool OverlayChromeVisible(int zoom, int minZoom, int maxZoom)
{
  return zoom >= minZoom && zoom <= maxZoom;
}

inline float OverlayFillOpacityFactor(double zoom, int minZoom, int maxZoom)
{
  if (minZoom > maxZoom)
    return 0.0f;
  double const fadeIn = std::clamp(zoom - static_cast<double>(minZoom - 1), 0.0, 1.0);
  double const fadeOut = std::clamp(static_cast<double>(maxZoom + 1) - zoom, 0.0, 1.0);
  return static_cast<float>(fadeIn * fadeOut);
}
}  // namespace df
