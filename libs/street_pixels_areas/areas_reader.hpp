#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include <optional>
#include <string>

namespace street_pixels
{
SpaFile ReadExplorationSidecar(std::string const & path);
std::optional<SpaHeader> TryReadSpaHeader(std::string const & path);
bool ShouldSkipExistingSpa(std::string const & path, int64_t mapDataVersion, uint32_t policyVersion,
                           std::string const & iso, std::string const & mwmId);
}  // namespace street_pixels
