#include "street_pixels_areas/exploration_filter.hpp"

#include "geometry/mercator.hpp"
#include "geometry/region2d.hpp"

#include <algorithm>
#include <cctype>

namespace street_pixels
{
namespace
{
bool IsBlank(std::string const & s)
{
  return std::all_of(s.begin(), s.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
}

std::vector<m2::PointD> LonLatRingToMercator(std::vector<m2::PointD> const & lonLat)
{
  std::vector<m2::PointD> mercator;
  mercator.reserve(lonLat.size());
  for (auto const & pt : lonLat)
  {
    // Input points are (lon, lat) as in the SP-023 JSONL fixture.
    mercator.push_back(mercator::FromLatLon(pt.y, pt.x));
  }
  if (mercator.size() >= 2 && mercator.front().EqualDxDy(mercator.back(), 1e-12))
    mercator.pop_back();
  return mercator;
}

bool LevelIn(std::vector<int> const & levels, int level)
{
  return std::find(levels.begin(), levels.end(), level) != levels.end();
}

bool PlaceTypeAllowed(PlaceBoundaryPolicy const & placePolicy, std::string const & placeType)
{
  if (!placePolicy.m_enabled)
    return false;
  return std::find(placePolicy.m_placeTypes.begin(), placePolicy.m_placeTypes.end(), placeType) !=
         placePolicy.m_placeTypes.end();
}

double RingsArea(std::vector<std::vector<m2::PointD>> const & rings)
{
  double area = 0.0;
  for (auto const & ring : rings)
  {
    if (ring.size() < 3)
      continue;
    m2::RegionD region(ring.begin(), ring.end());
    area += region.CalculateArea();
  }
  return area;
}
}  // namespace

FilterResult FilterExplorationCandidate(AreaCandidateInput const & input, CountryPolicy const & policy)
{
  FilterResult result;
  if (!policy.m_configured)
  {
    result.m_reason = RejectReason::UnconfiguredCountry;
    return result;
  }
  if (input.m_geometrySource == GeometrySource::ThreeBoxApprox)
  {
    result.m_reason = RejectReason::ThreeBox;
    return result;
  }
  if (input.m_geometrySource == GeometrySource::PlaceNodeInvented)
  {
    result.m_reason = RejectReason::PlaceNodeInvented;
    return result;
  }
  if (IsBlank(input.m_name))
  {
    result.m_reason = RejectReason::Unnamed;
    return result;
  }
  if (input.m_lonLatRings.empty())
  {
    result.m_reason = RejectReason::EmptyRings;
    return result;
  }

  AreaRole role = AreaRole::Subdivision;
  if (input.m_kind == "admin")
  {
    if (LevelIn(policy.m_subdivisionAdminLevels, input.m_adminLevel))
      role = AreaRole::Subdivision;
    else if (LevelIn(policy.m_settlementAdminLevels, input.m_adminLevel))
      role = AreaRole::Settlement;
    else
    {
      result.m_reason = RejectReason::PolicyMismatch;
      return result;
    }
  }
  else if (input.m_kind == "place")
  {
    if (!PlaceTypeAllowed(policy.m_placeBoundaries, input.m_placeType))
    {
      result.m_reason = RejectReason::PolicyMismatch;
      return result;
    }
    role = AreaRole::PlaceBoundary;
  }
  else
  {
    result.m_reason = RejectReason::PolicyMismatch;
    return result;
  }

  ExplorationArea area;
  area.m_osmId = input.m_osmId;
  area.m_osmType = input.m_osmType;
  area.m_role = role;
  area.m_adminLevel = static_cast<int8_t>(input.m_adminLevel);
  area.m_placeType = input.m_placeType;
  area.m_name = input.m_name;
  area.m_rings.reserve(input.m_lonLatRings.size());
  for (auto const & lonLatRing : input.m_lonLatRings)
  {
    auto mercator = LonLatRingToMercator(lonLatRing);
    if (mercator.size() < 3)
    {
      result.m_reason = RejectReason::InvalidRing;
      return result;
    }
    m2::RegionD region(mercator.begin(), mercator.end());
    if (!region.IsValid())
    {
      result.m_reason = RejectReason::InvalidRing;
      return result;
    }
    area.m_rings.push_back(std::move(mercator));
  }
  area.m_area = RingsArea(area.m_rings);
  result.m_area = std::move(area);
  result.m_reason = RejectReason::Accepted;
  return result;
}

char const * DebugPrint(RejectReason reason)
{
  switch (reason)
  {
  case RejectReason::Accepted: return "Accepted";
  case RejectReason::ThreeBox: return "ThreeBox";
  case RejectReason::PlaceNodeInvented: return "PlaceNodeInvented";
  case RejectReason::Unnamed: return "Unnamed";
  case RejectReason::EmptyRings: return "EmptyRings";
  case RejectReason::InvalidRing: return "InvalidRing";
  case RejectReason::PolicyMismatch: return "PolicyMismatch";
  case RejectReason::UnconfiguredCountry: return "UnconfiguredCountry";
  }
  return "Unknown";
}
}  // namespace street_pixels
