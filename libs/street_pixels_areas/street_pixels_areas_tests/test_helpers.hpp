#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "street_pixels_config/country_config.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <string>
#include <vector>

namespace street_pixels::test_helpers
{
inline CountryPolicy FinlandPolicy()
{
  auto const config = CountryConfig::LoadFromString(R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": {
        "enabled": true,
        "place_types": ["neighbourhood", "quarter", "suburb"]
      }
    }
  }
})");
  return config.GetByIso("FI");
}

inline CountryConfig FinlandConfig()
{
  return CountryConfig::LoadFromString(R"({
  "policy_version": 1,
  "schema_version": 1,
  "countries": {
    "FI": {
      "mwm_root_ids": ["Finland"],
      "subdivision_admin_levels": [10, 9, 11],
      "settlement_admin_levels": [8],
      "place_boundaries": {
        "enabled": true,
        "place_types": ["neighbourhood", "quarter", "suburb"]
      }
    }
  }
})");
}

// Axis-aligned lon/lat box as a closed ring (lon, lat) points.
inline std::vector<m2::PointD> LonLatBox(double west, double south, double east, double north)
{
  return {
      {west, south},
      {east, south},
      {east, north},
      {west, north},
      {west, south},
  };
}

inline AreaCandidateInput MakeAdminCandidate(uint64_t osmId, int adminLevel, std::string const & name,
                                             std::vector<m2::PointD> const & lonLatRing,
                                             GeometrySource source = GeometrySource::TrueClosedRing)
{
  AreaCandidateInput input;
  input.m_osmId = osmId;
  input.m_osmType = OsmObjectType::Relation;
  input.m_geometrySource = source;
  input.m_name = name;
  input.m_kind = "admin";
  input.m_adminLevel = adminLevel;
  input.m_lonLatRings = {lonLatRing};
  return input;
}

inline AreaCandidateInput MakePlaceCandidate(uint64_t osmId, std::string const & placeType, std::string const & name,
                                             std::vector<m2::PointD> const & lonLatRing)
{
  AreaCandidateInput input;
  input.m_osmId = osmId;
  input.m_osmType = OsmObjectType::Relation;
  input.m_geometrySource = GeometrySource::TrueClosedRing;
  input.m_name = name;
  input.m_kind = "place";
  input.m_placeType = placeType;
  input.m_lonLatRings = {lonLatRing};
  return input;
}

inline m2::PointD MercatorFromLonLat(double lon, double lat)
{
  return mercator::FromLatLon(lat, lon);
}
}  // namespace street_pixels::test_helpers
