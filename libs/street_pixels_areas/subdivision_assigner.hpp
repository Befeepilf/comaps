#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "street_pixels_config/country_config.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

namespace street_pixels
{
// §8.8: highest-priority configured level, then smallest polygon, then OSM id.
// Settlements are never returned — only subdivision / place-boundary indices
// or the no-subdivision sentinel (SPD-022 / SPD-025).
uint32_t AssignSubdivision(m2::PointD const & point, std::vector<ExplorationArea> const & areas,
                           CountryPolicy const & policy, uint32_t sentinel);

// Precompute dense assignments for an ordered list of sample points (e.g. HEALPix
// cell centres). Does not invent answers outside the sample list.
std::vector<uint32_t> BuildDenseAssignments(std::vector<m2::PointD> const & points,
                                            std::vector<ExplorationArea> const & areas,
                                            CountryPolicy const & policy, uint32_t sentinel);
}  // namespace street_pixels
