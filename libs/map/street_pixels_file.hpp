#pragma once

#include "base/exception.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>

class Writer;

namespace street_pixels_file
{
uint32_t constexpr kMagic = 0x58495053u;
uint16_t constexpr kFormatVersionV1 = 1;
uint16_t constexpr kFormatVersionV2 = 2;
uint16_t constexpr kFlagsHasHeaderBit = 1;
size_t constexpr kHeaderSize = 24;
size_t constexpr kMigrateChunkBytes = 1 << 20;

int64_t constexpr kPixelIdMask = 0x3FFFFFFFFFFFFFFFLL;
int64_t constexpr kExploredBit = static_cast<int64_t>(0x8000000000000000ULL);
int64_t constexpr kEverLiveBit = static_cast<int64_t>(0x4000000000000000ULL);

using ExploredEverLiveMap = std::unordered_map<int64_t, bool>;

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
  HeaderedV2,
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

void WriteHeader(Writer & writer, int64_t mapDataVersion, uint16_t formatVersion = kFormatVersionV2,
                 uint16_t flags = kFlagsHasHeaderBit);

bool SaveUnexploredIds(std::string const & path, std::set<int64_t> const & pixelIds, int64_t mapDataVersion);
void MigrateLegacyFile(std::string const & path, int64_t mapDataVersion);

int64_t PackPixelEntry(int64_t pixelId, bool explored, bool everLive);
std::optional<ExploredEverLiveMap> ScanExploredEverLive(std::string const & path);
bool SaveRematchedUniverse(std::string const & path, std::set<int64_t> const & newIds,
                           ExploredEverLiveMap const & exploredEverLive, int64_t mapDataVersion);
}  // namespace street_pixels_file
