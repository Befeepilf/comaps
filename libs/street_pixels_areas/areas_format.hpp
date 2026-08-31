#pragma once

#include <cstdint>
#include <limits>

namespace street_pixels
{
// Little-endian fourcc "SPA1".
uint32_t constexpr kSpaMagic = 0x31415053u;
// Production format (SPD-034 / SP-043). Dual-read still accepts geometry-only v1.
uint32_t constexpr kSpaFormatVersionV1 = 1;
uint32_t constexpr kSpaFormatVersion = 2;

// Frozen production HEALPix universe contract (SPD-017 / SPD-034).
uint32_t constexpr kSpaNside = 1048576;
uint8_t constexpr kSpaUniverseOrderAscendingNest = 1;

// Little-endian fourcc "SPX1" — sparse explored assignment store (SPD-022).
uint32_t constexpr kSpxMagic = 0x31585053u;
uint32_t constexpr kSpxFormatVersion = 1;

// Little-endian fourcc "ACC1" — durable area completion rows beside `.pix`.
uint32_t constexpr kAccMagic = 0x31434341u;
uint32_t constexpr kAccFormatVersion = 1;

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
