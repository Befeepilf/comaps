#pragma once

#include <cstdint>
#include <string>

namespace street_pixels
{
enum class MapTapKind : uint8_t
{
  DiscreteObject = 0,
  AreaSurface = 1,
};

struct MapTapClassification
{
  bool m_isBookmark = false;
  bool m_isTrack = false;
  bool m_isMyPosition = false;
  bool m_isRoutePoint = false;
  bool m_isPointFeature = false;
};

inline MapTapKind ClassifyMapTap(MapTapClassification const & tap)
{
  if (tap.m_isBookmark || tap.m_isTrack || tap.m_isMyPosition || tap.m_isRoutePoint || tap.m_isPointFeature)
    return MapTapKind::DiscreteObject;
  return MapTapKind::AreaSurface;
}

inline bool ShouldOpenExplorationDetail(MapTapKind kind, bool areaHit, bool hasMapFeature)
{
  if (kind != MapTapKind::AreaSurface)
    return false;
  return areaHit || !hasMapFeature;
}

inline std::string DebugPrint(MapTapKind kind)
{
  switch (kind)
  {
  case MapTapKind::DiscreteObject: return "DiscreteObject";
  case MapTapKind::AreaSurface: return "AreaSurface";
  }
  return "UnknownMapTapKind";
}
}  // namespace street_pixels
