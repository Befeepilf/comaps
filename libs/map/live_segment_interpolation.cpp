#include "map/live_segment_interpolation.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <algorithm>
#include <cmath>

bool MayInterpolateSegment(location::GpsInfo const & from, location::GpsInfo const & to)
{
  double const dtSec = to.m_timestamp - from.m_timestamp;
  if (dtSec <= 0.0 || dtSec > kMaxInterpolationGapSeconds)
    return false;

  double const distanceMeters =
      ms::DistanceOnEarth(from.m_latitude, from.m_longitude, to.m_latitude, to.m_longitude);

  if (distanceMeters > kMaxJumpMeters)
    return false;

  double const impliedSpeedMps = distanceMeters / dtSec;
  if (impliedSpeedMps > kMaxImpliedSpeedMps)
    return false;

  return true;
}

void ForEachMercatorSegmentSample(m2::PointD const & from, m2::PointD const & to, double stepMeters,
                                  std::function<void(double lat, double lon)> const & fn)
{
  double const distMeters = mercator::DistanceOnEarth(from, to);
  size_t const segments = std::max<size_t>(1, static_cast<size_t>(std::ceil(distMeters / stepMeters)));
  m2::PointD const dir = (to - from).Normalize();
  double const distMerc = (to - from).Length();
  double const step = distMerc / static_cast<double>(segments);
  for (size_t s = 0; s <= segments; ++s)
  {
    m2::PointD const p = from + dir * (static_cast<double>(s) * step);
    auto const latlon = mercator::ToLatLon(p);
    fn(latlon.m_lat, latlon.m_lon);
  }
}

void ForEachInterpolationSample(location::GpsInfo const & from, location::GpsInfo const & to,
                                std::function<void(double lat, double lon)> const & fn)
{
  ForEachMercatorSegmentSample(mercator::FromLatLon(from.m_latitude, from.m_longitude),
                               mercator::FromLatLon(to.m_latitude, to.m_longitude),
                               kPathSamplingStepMeters, fn);
}

void LiveSegmentInterpolation::MarkInterpolationBarrier()
{
  m_hasOrigin = false;
  m_origin = {};
}

bool LiveSegmentInterpolation::HasInterpolationOrigin() const { return m_hasOrigin; }

void LiveSegmentInterpolation::SetInterpolationOrigin(location::GpsInfo const & info)
{
  m_origin = info;
  m_hasOrigin = true;
}

location::GpsInfo const & LiveSegmentInterpolation::GetInterpolationOrigin() const { return m_origin; }

bool LiveSegmentInterpolation::CanInterpolateTo(location::GpsInfo const & to) const
{
  return m_hasOrigin && MayInterpolateSegment(m_origin, to);
}
