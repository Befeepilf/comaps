#pragma once

#include "kml/types.hpp"

#include "platform/location.hpp"

#include <cstddef>
#include <vector>

kml::MultiGeometry MakeTrackRecordingGeometry(std::vector<location::GpsInfo> const & points,
                                              std::vector<size_t> const & segmentBoundaryIndices);
