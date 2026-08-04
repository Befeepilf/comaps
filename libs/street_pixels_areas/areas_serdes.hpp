#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "base/exception.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace street_pixels
{
DECLARE_EXCEPTION(SpaFormatException, RootException);

void WriteSpaHeader(Writer & writer, SpaHeader const & header);
void WriteAreasSection(Writer & writer, std::vector<ExplorationArea> const & areas);
void WriteAssignSection(Writer & writer, std::vector<uint32_t> const & assignments, uint8_t indexWidth);

template <typename Source>
SpaHeader ReadSpaHeader(Source & src);

template <typename Source>
std::vector<ExplorationArea> ReadAreasSection(Source & src, uint32_t expectedCount);

template <typename Source>
std::vector<uint32_t> ReadAssignSection(Source & src, uint32_t expectedCount, uint8_t indexWidth);
}  // namespace street_pixels

#include "street_pixels_areas/areas_serdes_impl.hpp"
