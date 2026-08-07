#pragma once

#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"

#include "street_pixels_config/country_config.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace street_pixels
{
// SP-023 JSONL record after parse (lon/lat rings + optional centroid).
struct JsonlRingRecord
{
  AreaCandidateInput m_input;
  // Lon/lat centroid when present in the JSONL (used for MWM border attribution).
  std::optional<m2::PointD> m_centroidLonLat;
};

struct JsonlFilterStats
{
  uint32_t m_records = 0;
  uint32_t m_admitted = 0;
  std::map<std::string, uint32_t> m_rejects;
};

struct SpaSectionSizes
{
  uint64_t m_fileBytes = 0;
  uint64_t m_hdrBytes = 0;
  uint64_t m_areasBytes = 0;
  uint64_t m_assignBytes = 0;
};

struct KnownIdSpotCheck
{
  uint64_t m_osmId = 0;
  std::string m_expectedNameHint;
  bool m_found = false;
  std::string m_actualName;
  AreaRole m_role = AreaRole::Subdivision;
  int8_t m_adminLevel = -1;
};

// Parse one SP-023 finland_admin_place_rings.jsonl line into a candidate.
// Returns nullopt on blank/invalid JSON object structure.
std::optional<JsonlRingRecord> ParseJsonlRingLine(std::string const & line);

// Filter every JSONL record with FilterExplorationCandidate. When
// `centroidFilter` is set, only records whose lon/lat centroid lies inside one
// of the mercator polygons are considered (Helsinki leaf attribution).
JsonlFilterStats FilterJsonlRings(std::string const & jsonlPath, CountryPolicy const & policy,
                                  std::vector<m2::RegionD> const * centroidFilterMercator,
                                  std::vector<ExplorationArea> & outAreas);

// Geometry-only emit (assign_count == 0 is valid).
void WriteGeometryOnlyExplorationSidecar(std::string const & path, std::vector<ExplorationArea> areas,
                                         CountryPolicy const & policy, SpaWriteParams const & params);

SpaSectionSizes MeasureSpaSectionSizes(std::string const & path);

// Load OSM .poly border file into mercator regions (outers only).
bool LoadPolyFileAsMercatorRegions(std::string const & polyPath, std::vector<m2::RegionD> & outRegions);

bool CentroidInsideAny(m2::PointD const & lonLat, std::vector<m2::RegionD> const & mercatorRegions);

// Helsinki known-id spot-check (not an allowlist — SPD-004).
std::vector<std::pair<uint64_t, std::string>> HelsinkiKnownOsmIds();

std::vector<KnownIdSpotCheck> SpotCheckKnownIds(std::vector<ExplorationArea> const & areas,
                                                std::vector<std::pair<uint64_t, std::string>> const & known);
}  // namespace street_pixels
