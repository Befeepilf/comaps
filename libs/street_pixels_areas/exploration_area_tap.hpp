#pragma once

#include "geometry/point2d.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
enum class MapTapKind : uint8_t
{
  DiscreteObject = 0,
  AreaSurface = 1,
  AreaLabel = 2,
};

struct MapTapClassification
{
  bool m_isBookmark = false;
  bool m_isTrack = false;
  bool m_isMyPosition = false;
  bool m_isRoutePoint = false;
  bool m_isPointFeature = false;
  bool m_isAreaLabel = false;
};

inline MapTapKind ClassifyMapTap(MapTapClassification const & tap)
{
  if (tap.m_isBookmark || tap.m_isTrack || tap.m_isMyPosition || tap.m_isRoutePoint || tap.m_isPointFeature)
    return MapTapKind::DiscreteObject;
  if (tap.m_isAreaLabel)
    return MapTapKind::AreaLabel;
  return MapTapKind::AreaSurface;
}

inline bool ShouldOpenExplorationDetail(MapTapKind kind, bool areaHit)
{
  return kind == MapTapKind::AreaLabel && areaHit;
}

struct AreaLabelHitTarget
{
  uint32_t m_compactIndex = 0;
  m2::PointD m_labelPx;
  m2::PointD m_halfSizePx;
};

inline std::optional<uint32_t> HitExplorationAreaLabel(std::vector<AreaLabelHitTarget> const & labels,
                                                       m2::PointD const & tapPx)
{
  std::optional<uint32_t> best;
  double bestD2 = std::numeric_limits<double>::max();
  for (auto const & label : labels)
  {
    double const dx = std::abs(tapPx.x - label.m_labelPx.x);
    double const dy = std::abs(tapPx.y - label.m_labelPx.y);
    if (dx > label.m_halfSizePx.x || dy > label.m_halfSizePx.y)
      continue;
    double const d2 = dx * dx + dy * dy;
    if (d2 < bestD2)
    {
      bestD2 = d2;
      best = label.m_compactIndex;
    }
  }
  return best;
}

inline std::string DebugPrint(MapTapKind kind)
{
  switch (kind)
  {
  case MapTapKind::DiscreteObject: return "DiscreteObject";
  case MapTapKind::AreaSurface: return "AreaSurface";
  case MapTapKind::AreaLabel: return "AreaLabel";
  }
  return "UnknownMapTapKind";
}
}  // namespace street_pixels
