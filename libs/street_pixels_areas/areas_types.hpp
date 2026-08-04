#pragma once

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace street_pixels
{
enum class AreaRole : uint8_t
{
  Subdivision = 0,
  Settlement = 1,
  PlaceBoundary = 2,
};

enum class OsmObjectType : uint8_t
{
  Relation = 0,
  Way = 1,
};

enum class GeometrySource : uint8_t
{
  TrueClosedRing = 0,
  ThreeBoxApprox = 1,
  PlaceNodeInvented = 2,
};

struct ExplorationArea
{
  uint32_t m_compactIndex = 0;
  uint64_t m_osmId = 0;
  OsmObjectType m_osmType = OsmObjectType::Relation;
  AreaRole m_role = AreaRole::Subdivision;
  int8_t m_adminLevel = -1;
  std::string m_placeType;
  std::string m_name;
  // Mercator outer rings (multipolygon outers; holes are not stored).
  std::vector<std::vector<m2::PointD>> m_rings;
  double m_area = 0.0;

  bool IsAssignable() const
  {
    return m_role == AreaRole::Subdivision || m_role == AreaRole::PlaceBoundary;
  }

  bool Contains(m2::PointD const & pt) const
  {
    for (auto const & ring : m_rings)
    {
      if (ring.size() < 3)
        continue;
      m2::RegionD region(ring.begin(), ring.end());
      if (region.Contains(pt))
        return true;
    }
    return false;
  }
};

struct SpaHeader
{
  uint32_t m_magic = 0;
  uint32_t m_formatVersion = 0;
  int64_t m_mapDataVersion = 0;
  uint32_t m_policyVersion = 0;
  std::string m_isoCode;
  std::string m_mwmId;
  uint32_t m_areaCount = 0;
  uint32_t m_assignCount = 0;
  uint8_t m_indexWidth = 2;
};

struct SpaFile
{
  SpaHeader m_header;
  std::vector<ExplorationArea> m_areas;
  // Dense compact area indices; sentinel means no subdivision (SPD-022).
  std::vector<uint32_t> m_assignments;
};

struct AreaCandidateInput
{
  uint64_t m_osmId = 0;
  OsmObjectType m_osmType = OsmObjectType::Relation;
  GeometrySource m_geometrySource = GeometrySource::TrueClosedRing;
  std::string m_name;
  // "admin" or "place"
  std::string m_kind;
  int m_adminLevel = -1;
  std::string m_placeType;
  // Lon/lat closed rings (first point may equal last).
  std::vector<std::vector<m2::PointD>> m_lonLatRings;
};

inline std::string DebugPrint(AreaRole role)
{
  switch (role)
  {
  case AreaRole::Subdivision: return "Subdivision";
  case AreaRole::Settlement: return "Settlement";
  case AreaRole::PlaceBoundary: return "PlaceBoundary";
  }
  return "UnknownAreaRole";
}
}  // namespace street_pixels
