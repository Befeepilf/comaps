#pragma once

#include "map/live_sample_acceptance_filter.hpp"

#include "geometry/point2d.hpp"
#include "platform/location.hpp"

#include <functional>

inline constexpr double kMaxInterpolationGapSeconds = 30.0;
inline constexpr double kPathSamplingStepMeters = 15.0;
inline constexpr double kInterpolationStepMeters = kPathSamplingStepMeters;

bool MayInterpolateSegment(location::GpsInfo const & from, location::GpsInfo const & to);

void ForEachMercatorSegmentSample(m2::PointD const & from, m2::PointD const & to, double stepMeters,
                                  std::function<void(double lat, double lon)> const & fn);

void ForEachInterpolationSample(location::GpsInfo const & from, location::GpsInfo const & to,
                                std::function<void(double lat, double lon)> const & fn);

class LiveSegmentInterpolation
{
public:
  void MarkInterpolationBarrier();
  bool HasInterpolationOrigin() const;
  void SetInterpolationOrigin(location::GpsInfo const & info);
  location::GpsInfo const & GetInterpolationOrigin() const;

  bool CanInterpolateTo(location::GpsInfo const & to) const;

private:
  bool m_hasOrigin = false;
  location::GpsInfo m_origin;
};
