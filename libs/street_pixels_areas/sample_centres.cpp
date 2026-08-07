#include "street_pixels_areas/sample_centres.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <cstring>

extern "C"
{
#include "chealpix.h"
}

namespace street_pixels
{
namespace
{
// Mirror street_pixels_file constants (keep in sync with libs/map/street_pixels_file.hpp).
uint32_t constexpr kPixMagic = 0x58495053u;
uint16_t constexpr kPixFormatV1 = 1;
uint16_t constexpr kPixFormatV2 = 2;
size_t constexpr kPixHeaderSize = 24;
size_t constexpr kPixChunkBytes = 1 << 20;
int64_t constexpr kPixIdMask = 0x3FFFFFFFFFFFFFFFLL;

enum class PixKind
{
  HeaderedV1,
  HeaderedV2,
  Legacy,
  UnsupportedFormat,
  Corrupt
};

struct PixHeader
{
  uint32_t magic = 0;
  uint16_t formatVersion = 0;
  uint16_t flags = 0;
  int64_t mapDataVersion = 0;
  uint64_t reserved = 0;
};

static_assert(sizeof(PixHeader) == 24);

bool LooksLikePixHeader(uint8_t const * data, uint64_t size)
{
  if (data == nullptr || size < kPixHeaderSize)
    return false;
  PixHeader header;
  std::memcpy(&header, data, sizeof(header));
  return header.magic == kPixMagic &&
         (header.formatVersion == kPixFormatV1 || header.formatVersion == kPixFormatV2);
}

PixKind ProbePix(uint8_t const * data, uint64_t size)
{
  if (size == 0)
    return PixKind::Corrupt;
  if (LooksLikePixHeader(data, size))
  {
    PixHeader header;
    std::memcpy(&header, data, sizeof(header));
    if (header.formatVersion == kPixFormatV1)
      return PixKind::HeaderedV1;
    if (header.formatVersion == kPixFormatV2)
      return PixKind::HeaderedV2;
    return PixKind::UnsupportedFormat;
  }
  if ((size % sizeof(int64_t)) == 0)
    return PixKind::Legacy;
  return PixKind::Corrupt;
}
}  // namespace

m2::PointD MercatorCentreFromNestId(int64_t nestId, uint32_t nside)
{
  double theta = 0.0;
  double phi = 0.0;
  pix2ang_nest64(static_cast<int64_t>(nside), nestId, &theta, &phi);
  double const lat = math::RadToDeg(M_PI_2 - theta);
  double const lon = math::RadToDeg(phi);
  return mercator::FromLatLon(lat, lon);
}

std::optional<std::vector<m2::PointD>> MercatorCentresFromAscendingNest(std::vector<int64_t> const & nestIds,
                                                                        uint32_t nside)
{
  for (size_t i = 1; i < nestIds.size(); ++i)
  {
    if (nestIds[i] <= nestIds[i - 1])
    {
      LOG(LWARNING, ("Nest id list is not strictly ascending at index", i));
      return std::nullopt;
    }
  }

  std::vector<m2::PointD> centres;
  centres.reserve(nestIds.size());
  for (int64_t id : nestIds)
    centres.push_back(MercatorCentreFromNestId(id, nside));
  return centres;
}

int64_t NestIdFromLonLat(double lonDeg, double latDeg, uint32_t nside)
{
  double const latRad = math::DegToRad(latDeg);
  double const lonRad = math::DegToRad(lonDeg);
  double const theta = M_PI_2 - latRad;
  double const phi = lonRad;
  int64_t ipix = 0;
  ang2pix_nest64(static_cast<int64_t>(nside), theta, phi, &ipix);
  return ipix;
}

std::optional<std::vector<int64_t>> ScanPixUniverseAscending(std::string const & path)
{
  std::vector<int64_t> universe;
  uint64_t size = 0;
  if (!Platform::GetFileSizeByFullPath(path, size) || size == 0)
    return universe;

  uint8_t headerBytes[kPixHeaderSize] = {};
  try
  {
    FileReader probeReader(path);
    size_t const toRead = static_cast<size_t>(std::min<uint64_t>(size, kPixHeaderSize));
    probeReader.Read(0, headerBytes, toRead);
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Failed to probe street pixels file", path, ex.what()));
    return std::nullopt;
  }

  PixKind const kind = ProbePix(headerBytes, size);
  uint64_t offset = 0;
  switch (kind)
  {
  case PixKind::HeaderedV1:
  case PixKind::HeaderedV2:
    if (size < kPixHeaderSize || ((size - kPixHeaderSize) % sizeof(int64_t)) != 0)
    {
      LOG(LWARNING, ("Street pixels file size invalid for universe scan", path));
      return std::nullopt;
    }
    offset = kPixHeaderSize;
    break;
  case PixKind::Legacy:
    if ((size % sizeof(int64_t)) != 0)
    {
      LOG(LWARNING, ("Legacy street pixels file size invalid for universe scan", path));
      return std::nullopt;
    }
    offset = 0;
    break;
  case PixKind::UnsupportedFormat:
    LOG(LWARNING, ("Unsupported street pixels format; refusing universe scan", path));
    return std::nullopt;
  case PixKind::Corrupt:
    LOG(LWARNING, ("Corrupt street pixels file; refusing universe scan", path));
    return std::nullopt;
  }

  try
  {
    FileReader reader(path);
    std::vector<uint8_t> buffer(static_cast<size_t>(std::min<uint64_t>(size - offset, kPixChunkBytes)));
    if (buffer.empty())
      return universe;

    size_t const wordsPerChunk = buffer.size() / sizeof(int64_t);
    buffer.resize(wordsPerChunk * sizeof(int64_t));
    if (buffer.empty())
      return universe;

    while (offset < size)
    {
      size_t const chunk = static_cast<size_t>(std::min<uint64_t>(size - offset, buffer.size()));
      if ((chunk % sizeof(int64_t)) != 0)
      {
        LOG(LWARNING, ("Street pixels universe scan hit misaligned trailing bytes", path));
        return std::nullopt;
      }
      reader.Read(offset, buffer.data(), chunk);
      size_t const wordCount = chunk / sizeof(int64_t);
      for (size_t i = 0; i < wordCount; ++i)
      {
        int64_t word = 0;
        std::memcpy(&word, buffer.data() + i * sizeof(int64_t), sizeof(word));
        int64_t const pixelId = word & kPixIdMask;
        if (!universe.empty() && pixelId <= universe.back())
        {
          LOG(LWARNING, ("Street pixels universe not strictly ascending", path));
          return std::nullopt;
        }
        universe.push_back(pixelId);
      }
      offset += chunk;
    }
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Failed to scan street pixels universe", path, ex.what()));
    return std::nullopt;
  }

  return universe;
}
}  // namespace street_pixels
