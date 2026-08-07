#pragma once

#include "street_pixels_areas/areas_format.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
// Mercator centre of a HEALPix NEST cell at the frozen production nside (SPD-017/034).
// Light helper — links healpix only; does not pull libs/map.
m2::PointD MercatorCentreFromNestId(int64_t nestId, uint32_t nside = kSpaNside);

// Build sample centres in ascending-NEST slot order. Returns nullopt when
// `nestIds` is not strictly ascending (fail-closed for AscendingNest contract).
std::optional<std::vector<m2::PointD>> MercatorCentresFromAscendingNest(
    std::vector<int64_t> const & nestIds, uint32_t nside = kSpaNside);

// Nest id at production nside for a lon/lat (degrees). Test / offline helper.
int64_t NestIdFromLonLat(double lonDeg, double latDeg, uint32_t nside = kSpaNside);

// Thin `.pix` ascending-universe scan matching `street_pixels_file::ScanUniverseAscending`
// contract (explored bit ignored; nullopt on corrupt / non-ascending). Avoids linking map.
std::optional<std::vector<int64_t>> ScanPixUniverseAscending(std::string const & path);
}  // namespace street_pixels
