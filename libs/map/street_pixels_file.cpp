#include "map/street_pixels_file.hpp"

#include "coding/endianness.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <vector>

namespace street_pixels_file
{
namespace
{
Header DecodeHeaderBytes(uint8_t const * data)
{
  Header header;
  std::memcpy(&header.magic, data + 0, sizeof(header.magic));
  std::memcpy(&header.formatVersion, data + 4, sizeof(header.formatVersion));
  std::memcpy(&header.flags, data + 6, sizeof(header.flags));
  std::memcpy(&header.mapDataVersion, data + 8, sizeof(header.mapDataVersion));
  std::memcpy(&header.reserved, data + 16, sizeof(header.reserved));
  header.magic = SwapIfBigEndianMacroBased(header.magic);
  header.formatVersion = SwapIfBigEndianMacroBased(header.formatVersion);
  header.flags = SwapIfBigEndianMacroBased(header.flags);
  header.mapDataVersion = SwapIfBigEndianMacroBased(header.mapDataVersion);
  header.reserved = SwapIfBigEndianMacroBased(header.reserved);
  return header;
}
}  // namespace

bool LooksLikeHeader(uint8_t const * data, uint64_t size)
{
  if (data == nullptr || size < kHeaderSize || ((size - kHeaderSize) % sizeof(int64_t)) != 0)
    return false;
  Header const header = DecodeHeaderBytes(data);
  return header.magic == kMagic && (header.flags & kFlagsHasHeaderBit) != 0;
}

Header ReadHeader(uint8_t const * data)
{
  return DecodeHeaderBytes(data);
}

ProbeResult Probe(uint8_t const * data, uint64_t size)
{
  ProbeResult result;
  if (LooksLikeHeader(data, size))
  {
    result.header = DecodeHeaderBytes(data);
    if (result.header.formatVersion == kFormatVersionV1)
      result.kind = FileKind::HeaderedV1;
    else if (result.header.formatVersion == kFormatVersionV2)
      result.kind = FileKind::HeaderedV2;
    else
      result.kind = FileKind::UnsupportedFormat;
    return result;
  }

  if (size > 0 && (size % sizeof(int64_t)) == 0)
  {
    result.kind = FileKind::Legacy;
    return result;
  }

  result.kind = FileKind::Corrupt;
  return result;
}

ProbeResult ProbeFile(std::string const & path)
{
  uint64_t size = 0;
  if (!Platform::GetFileSizeByFullPath(path, size) || size == 0)
  {
    ProbeResult result;
    result.kind = FileKind::Corrupt;
    return result;
  }

  uint8_t headerBytes[kHeaderSize] = {};
  size_t const toRead = static_cast<size_t>(std::min<uint64_t>(size, kHeaderSize));
  try
  {
    FileReader reader(path);
    reader.Read(0, headerBytes, toRead);
  }
  catch (Reader::Exception const &)
  {
    ProbeResult result;
    result.kind = FileKind::Corrupt;
    return result;
  }

  return Probe(headerBytes, size);
}

bool MayRecoverByDerive(FileKind kind)
{
  return kind == FileKind::Corrupt;
}

void WriteHeader(Writer & writer, int64_t mapDataVersion, uint16_t formatVersion, uint16_t flags)
{
  WriteToSink(writer, kMagic);
  WriteToSink(writer, formatVersion);
  WriteToSink(writer, flags);
  WriteToSink(writer, mapDataVersion);
  WriteToSink(writer, static_cast<uint64_t>(0));
}

bool SaveUnexploredIds(std::string const & path, std::set<int64_t> const & pixelIds, int64_t mapDataVersion)
{
  return base::WriteToTempAndRenameToFile(path, [&](std::string const & tmpPath)
  {
    try
    {
      FileWriter writer(tmpPath, FileWriter::OP_WRITE_TRUNCATE);
      WriteHeader(writer, mapDataVersion);
      for (int64_t const pixelId : pixelIds)
        WriteToSink(writer, pixelId);
      writer.Flush();
      return true;
    }
    catch (Writer::Exception const & ex)
    {
      LOG(LERROR, ("Failed to write street pixels file", path, ex.what()));
      return false;
    }
  });
}

void MigrateLegacyFile(std::string const & path, int64_t mapDataVersion)
{
  uint64_t size = 0;
  if (!Platform::GetFileSizeByFullPath(path, size) || size == 0 || (size % sizeof(int64_t)) != 0)
    MYTHROW(StreetPixelsMigrationException, ("Legacy street pixels file is unreadable", path));

  ProbeResult const probe = ProbeFile(path);
  if (probe.kind != FileKind::Legacy)
    MYTHROW(StreetPixelsMigrationException, ("Expected legacy street pixels file", path));

  bool const ok = base::WriteToTempAndRenameToFile(path, [&](std::string const & tmpPath)
  {
    try
    {
      FileReader reader(path);
      FileWriter writer(tmpPath, FileWriter::OP_WRITE_TRUNCATE);
      WriteHeader(writer, mapDataVersion);

      std::vector<uint8_t> buffer(static_cast<size_t>(std::min<uint64_t>(size, kMigrateChunkBytes)));
      uint64_t offset = 0;
      while (offset < size)
      {
        size_t const chunk = static_cast<size_t>(std::min<uint64_t>(size - offset, buffer.size()));
        reader.Read(offset, buffer.data(), chunk);
        writer.Write(buffer.data(), chunk);
        offset += chunk;
      }
      writer.Flush();
      return true;
    }
    catch (Reader::Exception const & ex)
    {
      LOG(LERROR, ("Failed to read legacy street pixels during migrate", path, ex.what()));
      return false;
    }
    catch (Writer::Exception const & ex)
    {
      LOG(LERROR, ("Failed to write migrated street pixels", path, ex.what()));
      return false;
    }
  });

  if (!ok)
    MYTHROW(StreetPixelsMigrationException, ("Failed to migrate legacy street pixels file", path));
}

int64_t PackPixelEntry(int64_t pixelId, bool explored, bool everLive)
{
  int64_t word = pixelId & kPixelIdMask;
  if (explored)
    word |= kExploredBit;
  if (everLive)
    word |= kEverLiveBit;
  return word;
}

std::optional<ExploredEverLiveMap> ScanExploredEverLive(std::string const & path)
{
  ExploredEverLiveMap explored;
  uint64_t size = 0;
  if (!Platform::GetFileSizeByFullPath(path, size) || size == 0)
    return explored;

  ProbeResult const probe = ProbeFile(path);
  uint64_t offset = 0;
  switch (probe.kind)
  {
  case FileKind::HeaderedV1:
  case FileKind::HeaderedV2:
    if (size < kHeaderSize || ((size - kHeaderSize) % sizeof(int64_t)) != 0)
    {
      LOG(LWARNING, ("Street pixels file size invalid for scan", path));
      return std::nullopt;
    }
    offset = kHeaderSize;
    break;
  case FileKind::Legacy:
    if ((size % sizeof(int64_t)) != 0)
    {
      LOG(LWARNING, ("Legacy street pixels file size invalid for scan", path));
      return std::nullopt;
    }
    offset = 0;
    break;
  case FileKind::UnsupportedFormat:
    LOG(LWARNING, ("Unsupported street pixels format; refusing scan", path));
    return std::nullopt;
  case FileKind::Corrupt:
    return explored;
  }

  try
  {
    FileReader reader(path);
    std::vector<uint8_t> buffer(static_cast<size_t>(std::min<uint64_t>(size - offset, kMigrateChunkBytes)));
    if (buffer.empty())
      return explored;

    size_t const wordsPerChunk = buffer.size() / sizeof(int64_t);
    buffer.resize(wordsPerChunk * sizeof(int64_t));
    if (buffer.empty())
      return explored;

    while (offset < size)
    {
      size_t const chunk = static_cast<size_t>(std::min<uint64_t>(size - offset, buffer.size()));
      if ((chunk % sizeof(int64_t)) != 0)
      {
        LOG(LWARNING, ("Street pixels scan hit misaligned trailing bytes", path));
        return std::nullopt;
      }
      reader.Read(offset, buffer.data(), chunk);
      size_t const wordCount = chunk / sizeof(int64_t);
      for (size_t i = 0; i < wordCount; ++i)
      {
        int64_t word = 0;
        std::memcpy(&word, buffer.data() + i * sizeof(int64_t), sizeof(word));
        if ((word & kExploredBit) == 0)
          continue;
        int64_t const pixelId = word & kPixelIdMask;
        bool const everLive = (word & kEverLiveBit) != 0;
        explored[pixelId] = everLive;
      }
      offset += chunk;
    }
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Failed to scan explored street pixels", path, ex.what()));
    return std::nullopt;
  }

  return explored;
}

bool SaveRematchedUniverse(std::string const & path, std::set<int64_t> const & newIds,
                           ExploredEverLiveMap const & exploredEverLive, int64_t mapDataVersion)
{
  return base::WriteToTempAndRenameToFile(path, [&](std::string const & tmpPath)
  {
    try
    {
      FileWriter writer(tmpPath, FileWriter::OP_WRITE_TRUNCATE);
      WriteHeader(writer, mapDataVersion);
      for (int64_t const pixelId : newIds)
      {
        auto const it = exploredEverLive.find(pixelId);
        bool const explored = it != exploredEverLive.end();
        bool const everLive = explored && it->second;
        WriteToSink(writer, PackPixelEntry(pixelId, explored, everLive));
      }
      writer.Flush();
      return true;
    }
    catch (Writer::Exception const & ex)
    {
      LOG(LERROR, ("Failed to write rematched street pixels file", path, ex.what()));
      return false;
    }
  });
}
}  // namespace street_pixels_file
