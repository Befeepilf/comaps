#include "map/track_recording_geometry.hpp"

#include "geometry/mercator.hpp"

kml::MultiGeometry MakeTrackRecordingGeometry(std::vector<location::GpsInfo> const & points,
                                              std::vector<size_t> const & segmentBoundaryIndices)
{
  kml::MultiGeometry geometry;
  if (points.empty())
    return geometry;

  geometry.m_lines.emplace_back();
  geometry.m_timestamps.emplace_back();

  size_t boundaryIdx = 0;
  for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
  {
    while (boundaryIdx < segmentBoundaryIndices.size() && pointIndex == segmentBoundaryIndices[boundaryIdx])
    {
      geometry.m_lines.emplace_back();
      geometry.m_timestamps.emplace_back();
      ++boundaryIdx;
    }

    auto const & pt = points[pointIndex];
    geometry.m_lines.back().emplace_back(mercator::FromLatLon(pt.m_latitude, pt.m_longitude), pt.m_altitude);
    geometry.m_timestamps.back().emplace_back(pt.m_timestamp);
  }

  size_t writeIdx = 0;
  for (size_t readIdx = 0; readIdx < geometry.m_lines.size(); ++readIdx)
  {
    if (geometry.m_lines[readIdx].empty())
      continue;
    if (writeIdx != readIdx)
    {
      geometry.m_lines[writeIdx] = std::move(geometry.m_lines[readIdx]);
      geometry.m_timestamps[writeIdx] = std::move(geometry.m_timestamps[readIdx]);
    }
    ++writeIdx;
  }
  geometry.m_lines.resize(writeIdx);
  geometry.m_timestamps.resize(writeIdx);
  return geometry;
}
