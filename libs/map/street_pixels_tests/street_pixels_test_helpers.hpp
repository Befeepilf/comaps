#pragma once

#include "drape_frontend/street_pixel.hpp"

#include "map/street_pixels_manager.hpp"

#include "platform/location.hpp"

#include "base/math.hpp"
#include "base/timer.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <vector>

namespace street_pixels_tests
{
double constexpr kEarthRadiusMeters = 6371000.0;
double constexpr kExploreRadiusMeters = 25.0;
double constexpr kExploreRadiusRads = kExploreRadiusMeters / kEarthRadiusMeters;

inline double CurrentTimestampSec() { return static_cast<double>(base::SecondsSinceEpoch()); }

inline std::pair<double, double> OffsetLatLonByMeters(double lat, double lon, double northM, double eastM)
{
  double const latRad = math::DegToRad(lat);
  double const dLat = northM / kEarthRadiusMeters;
  double const dLon = eastM / (kEarthRadiusMeters * std::cos(latRad));
  return {lat + math::RadToDeg(dLat), lon + math::RadToDeg(dLon)};
}

inline std::pair<double, double> LatLonForPixelId(std::int64_t pixelId)
{
  pointing const ang = hp::GetHealpixBase().pix2ang(pixelId);
  double const lat = math::RadToDeg(M_PI_2 - ang.theta);
  double const lon = math::RadToDeg(ang.phi);
  return {lat, lon};
}

inline location::GpsInfo MakeGpsInfo(double lat, double lon, double horizontalAccuracyM, double timestampSec,
                                     location::TLocationSource source = location::EAndroidNative)
{
  location::GpsInfo info;
  info.m_source = source;
  info.m_latitude = lat;
  info.m_longitude = lon;
  info.m_horizontalAccuracy = horizontalAccuracyM;
  info.m_timestamp = timestampSec;
  return info;
}

inline std::vector<location::GpsInfo> MakeGpsSequence(double startLat, double startLon, double dLat, double dLon,
                                                      size_t count, double accuracyM, double startTimestamp,
                                                      double dtSec)
{
  std::vector<location::GpsInfo> sequence;
  sequence.reserve(count);
  for (size_t i = 0; i < count; ++i)
  {
    sequence.push_back(MakeGpsInfo(startLat + dLat * static_cast<double>(i), startLon + dLon * static_cast<double>(i),
                                   accuracyM, startTimestamp + dtSec * static_cast<double>(i)));
  }
  return sequence;
}

inline df::StreetPixel MakeStreetPixel(std::int64_t pixelId, bool explored = false)
{
  std::int64_t raw = pixelId & 0x7FFFFFFFFFFFFFFF;
  if (explored)
    raw |= static_cast<std::int64_t>(0x8000000000000000ULL);
  df::StreetPixel pixel;
  static_assert(sizeof(df::StreetPixel) == sizeof(std::int64_t));
  std::memcpy(&pixel, &raw, sizeof(raw));
  return pixel;
}

inline std::vector<df::StreetPixel> MakePixelSet(
    std::initializer_list<std::pair<std::int64_t, bool>> idsAndExplored)
{
  std::vector<df::StreetPixel> pixels;
  pixels.reserve(idsAndExplored.size());
  for (auto const & entry : idsAndExplored)
    pixels.push_back(MakeStreetPixel(entry.first, entry.second));
  return pixels;
}
}  // namespace street_pixels_tests
