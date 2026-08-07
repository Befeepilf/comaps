#pragma once

#include "street_pixels_areas/areas_format.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

namespace street_pixels
{
namespace spa_detail
{
template <typename Source>
ExplorationArea ReadArea(Source & src, uint32_t compactIndex)
{
  ExplorationArea area;
  area.m_compactIndex = compactIndex;
  area.m_osmId = ReadPrimitiveFromSource<uint64_t>(src);
  area.m_osmType = static_cast<OsmObjectType>(ReadPrimitiveFromSource<uint8_t>(src));
  area.m_role = static_cast<AreaRole>(ReadPrimitiveFromSource<uint8_t>(src));
  area.m_adminLevel = ReadPrimitiveFromSource<int8_t>(src);
  rw::Read(src, area.m_placeType);
  rw::Read(src, area.m_name);
  // Host-endian IEEE754; matches WriteArea (WriteToSink is integral-only).
  src.Read(&area.m_area, sizeof(area.m_area));

  uint32_t const ringCount = ReadVarUint<uint32_t>(src);
  area.m_rings.resize(ringCount);
  serial::GeometryCodingParams cp;
  for (uint32_t i = 0; i < ringCount; ++i)
    serial::LoadOuterPath(src, cp, area.m_rings[i]);
  return area;
}
}  // namespace spa_detail

template <typename Source>
SpaHeader ReadSpaHeader(Source & src)
{
  SpaHeader header;
  header.m_magic = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_formatVersion = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_mapDataVersion = ReadPrimitiveFromSource<int64_t>(src);
  header.m_policyVersion = ReadPrimitiveFromSource<uint32_t>(src);
  rw::Read(src, header.m_isoCode);
  rw::Read(src, header.m_mwmId);
  header.m_areaCount = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_assignCount = ReadPrimitiveFromSource<uint32_t>(src);
  header.m_indexWidth = ReadPrimitiveFromSource<uint8_t>(src);

  if (header.m_magic != kSpaMagic)
    MYTHROW(SpaFormatException, ("Bad .spa magic", header.m_magic));
  if (header.m_indexWidth != 2 && header.m_indexWidth != 4)
    MYTHROW(SpaFormatException, ("Unsupported .spa index_width", header.m_indexWidth));

  if (header.m_formatVersion == kSpaFormatVersion)
  {
    header.m_nside = ReadPrimitiveFromSource<uint32_t>(src);
    header.m_universeOrder = ReadPrimitiveFromSource<uint8_t>(src);
    uint8_t reserved[3] = {};
    src.Read(reserved, sizeof(reserved));
    if (header.m_nside != kSpaNside)
      MYTHROW(SpaFormatException, ("Unsupported .spa nside", header.m_nside));
    if (header.m_universeOrder != kSpaUniverseOrderAscendingNest)
      MYTHROW(SpaFormatException, ("Unsupported .spa universe_order", header.m_universeOrder));
    if (reserved[0] != 0 || reserved[1] != 0 || reserved[2] != 0)
      MYTHROW(SpaFormatException, ("Non-zero .spa header reserved bytes"));
  }
  else if (header.m_formatVersion == kSpaFormatVersionV1)
  {
    // Geometry-only dual-read: v1 headers end at index_width. Assigning blobs
    // without a frozen nside / universe_order tag are rejected fail-closed.
    if (header.m_assignCount != 0)
      MYTHROW(SpaFormatException, ("Rejected .spa format_version 1 with assign_count > 0",
                                   header.m_assignCount));
  }
  else
  {
    MYTHROW(SpaFormatException, ("Unsupported .spa format_version", header.m_formatVersion));
  }
  return header;
}

template <typename Source>
std::vector<ExplorationArea> ReadAreasSection(Source & src, uint32_t expectedCount)
{
  std::vector<ExplorationArea> areas;
  areas.reserve(expectedCount);
  for (uint32_t i = 0; i < expectedCount; ++i)
    areas.push_back(spa_detail::ReadArea(src, i));
  return areas;
}

template <typename Source>
std::vector<uint32_t> ReadAssignSection(Source & src, uint32_t expectedCount, uint8_t indexWidth)
{
  std::vector<uint32_t> assignments;
  assignments.reserve(expectedCount);
  for (uint32_t i = 0; i < expectedCount; ++i)
  {
    if (indexWidth == 2)
      assignments.push_back(ReadPrimitiveFromSource<uint16_t>(src));
    else
      assignments.push_back(ReadPrimitiveFromSource<uint32_t>(src));
  }
  return assignments;
}
}  // namespace street_pixels
