#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include <string>

namespace street_pixels
{
SpaFile ReadExplorationSidecar(std::string const & path);
}  // namespace street_pixels
