#include "base/assert.hpp"
#include "base/math.hpp"

#include "drape_frontend/street_pixel.hpp"

#include "drape/color.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "map/street_pixels_manager.hpp"

#include <healpix_base.h>
#include <cmath>
#include <cstdint>

namespace df
{
namespace
{
std::int64_t constexpr kPixelIdMask = 0x3FFFFFFFFFFFFFFF;
std::int64_t constexpr kExploredBit = static_cast<std::int64_t>(0x8000000000000000ULL);
std::int64_t constexpr kEverLiveBit = static_cast<std::int64_t>(0x4000000000000000ULL);
}  // namespace

std::int64_t StreetPixel::GetPixelId() const { return m_pixelId & kPixelIdMask; }

bool StreetPixel::IsExplored() const { return (m_pixelId & kExploredBit) != 0; }

void StreetPixel::SetExplored(bool explored)
{
  if (explored)
    m_pixelId |= kExploredBit;
}

bool StreetPixel::IsEverLive() const { return (m_pixelId & kEverLiveBit) != 0; }

void StreetPixel::SetEverLive(bool everLive)
{
  if (everLive)
    m_pixelId |= kEverLiveBit;
}

dp::Color const StreetPixel::GetColor() const
{
  if (IsExplored())
    return dp::Color::Green();
  return dp::Color::Red();
}

m2::PointD const StreetPixel::GetPoint() const
{
  pointing const & ang = hp::GetHealpixBase().pix2ang(GetPixelId());
  double const latDeg = math::RadToDeg(M_PI_2 - ang.theta);
  double const lonDeg = math::RadToDeg(ang.phi);
  return mercator::FromLatLon(latDeg, lonDeg);
}
}  // namespace df
