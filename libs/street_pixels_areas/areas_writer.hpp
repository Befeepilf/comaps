#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "street_pixels_config/country_config.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace street_pixels
{
struct SpaWriteParams
{
  int64_t m_mapDataVersion = 0;
  uint32_t m_policyVersion = 0;
  std::string m_isoCode;
  std::string m_mwmId;
};

// Assigns compact indices, builds dense assignments for `samplePoints`, and
// writes `{mwmId}.spa` via FilesContainer sections hdr/areas/assign.
void WriteExplorationSidecar(std::string const & path, std::vector<ExplorationArea> areas,
                             std::vector<m2::PointD> const & samplePoints, CountryPolicy const & policy,
                             SpaWriteParams const & params);
}  // namespace street_pixels
