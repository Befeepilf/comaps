#pragma once

#include <cstdint>
#include <limits>

namespace street_pixels
{
// Little-endian fourcc "SPA1".
uint32_t constexpr kSpaMagic = 0x31415053u;
uint32_t constexpr kSpaFormatVersion = 1;

uint32_t constexpr kNoSubdivisionUint16 = std::numeric_limits<uint16_t>::max();
uint32_t constexpr kNoSubdivisionUint32 = std::numeric_limits<uint32_t>::max();

inline uint8_t ChooseIndexWidth(uint32_t areaCount)
{
  // Reserve the max value as the no-subdivision sentinel.
  if (areaCount < kNoSubdivisionUint16)
    return 2;
  return 4;
}

inline uint32_t NoSubdivisionSentinel(uint8_t indexWidth)
{
  return indexWidth == 2 ? kNoSubdivisionUint16 : kNoSubdivisionUint32;
}
}  // namespace street_pixels
