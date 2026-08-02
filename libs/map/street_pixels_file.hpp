#pragma once

#include "base/exception.hpp"

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>

class Writer;

namespace street_pixels_file
{
uint32_t constexpr kMagic = 0x58495053u;
uint16_t constexpr kFormatVersionV1 = 1;
uint16_t constexpr kFlagsHasHeaderBit = 1;
size_t constexpr kHeaderSize = 24;
size_t constexpr kMigrateChunkBytes = 1 << 20;

DECLARE_EXCEPTION(StreetPixelsFileException, RootException);
DECLARE_EXCEPTION(UnsupportedStreetPixelsFormat, StreetPixelsFileException);
DECLARE_EXCEPTION(StreetPixelsMigrationException, StreetPixelsFileException);
DECLARE_EXCEPTION(CorruptStreetPixelsFile, StreetPixelsFileException);

struct Header
{
  uint32_t magic = 0;
  uint16_t formatVersion = 0;
  uint16_t flags = 0;
  int64_t mapDataVersion = 0;
  uint64_t reserved = 0;
};

static_assert(kHeaderSize == 24);
static_assert(sizeof(Header) == 24);
static_assert(offsetof(Header, magic) == 0);
static_assert(offsetof(Header, formatVersion) == 4);
static_assert(offsetof(Header, flags) == 6);
static_assert(offsetof(Header, mapDataVersion) == 8);
static_assert(offsetof(Header, reserved) == 16);

enum class FileKind
{
  HeaderedV1,
  Legacy,
  UnsupportedFormat,
  Corrupt
};

struct ProbeResult
{
  FileKind kind = FileKind::Corrupt;
  Header header;
};

bool LooksLikeHeader(uint8_t const * data, uint64_t size);
Header ReadHeader(uint8_t const * data);
ProbeResult Probe(uint8_t const * data, uint64_t size);
ProbeResult ProbeFile(std::string const & path);
bool MayRecoverByDerive(FileKind kind);

void WriteHeader(Writer & writer, int64_t mapDataVersion, uint16_t formatVersion = kFormatVersionV1,
                 uint16_t flags = kFlagsHasHeaderBit);

bool SaveUnexploredIds(std::string const & path, std::set<int64_t> const & pixelIds, int64_t mapDataVersion);
void MigrateLegacyFile(std::string const & path, int64_t mapDataVersion);
}  // namespace street_pixels_file
