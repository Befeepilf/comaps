#include "street_pixels_areas/areas_serdes.hpp"

#include "street_pixels_areas/areas_format.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include <limits>

namespace street_pixels
{
namespace
{
void WriteArea(Writer & writer, ExplorationArea const & area)
{
  WriteToSink(writer, area.m_osmId);
  WriteToSink(writer, static_cast<uint8_t>(area.m_osmType));
  WriteToSink(writer, static_cast<uint8_t>(area.m_role));
  WriteToSink(writer, area.m_adminLevel);
  rw::Write(writer, area.m_placeType);
  rw::Write(writer, area.m_name);
  // Host-endian IEEE754; WriteToSink is integral-only.
  writer.Write(&area.m_area, sizeof(area.m_area));

  WriteVarUint(writer, static_cast<uint32_t>(area.m_rings.size()));
  serial::GeometryCodingParams cp;
  for (auto const & ring : area.m_rings)
    serial::SaveOuterPath(ring, cp, writer);
}
}  // namespace

void WriteSpaHeader(Writer & writer, SpaHeader const & header)
{
  WriteToSink(writer, header.m_magic);
  WriteToSink(writer, header.m_formatVersion);
  WriteToSink(writer, header.m_mapDataVersion);
  WriteToSink(writer, header.m_policyVersion);
  rw::Write(writer, header.m_isoCode);
  rw::Write(writer, header.m_mwmId);
  WriteToSink(writer, header.m_areaCount);
  WriteToSink(writer, header.m_assignCount);
  WriteToSink(writer, header.m_indexWidth);
  // format_version 2 trailing fields (SPD-034). reserved[3] must be zero on write.
  WriteToSink(writer, header.m_nside);
  WriteToSink(writer, header.m_universeOrder);
  uint8_t const reserved[3] = {0, 0, 0};
  writer.Write(reserved, sizeof(reserved));
}

void WriteAreasSection(Writer & writer, std::vector<ExplorationArea> const & areas)
{
  for (auto const & area : areas)
    WriteArea(writer, area);
}

void WriteAssignSection(Writer & writer, std::vector<uint32_t> const & assignments, uint8_t indexWidth)
{
  uint32_t const sentinel = NoSubdivisionSentinel(indexWidth);
  for (uint32_t value : assignments)
  {
    if (indexWidth == 2)
    {
      if (value != sentinel && value > std::numeric_limits<uint16_t>::max())
        MYTHROW(SpaFormatException, ("Assignment value exceeds uint16", value));
      WriteToSink(writer, static_cast<uint16_t>(value));
    }
    else
      WriteToSink(writer, value);
  }
}
}  // namespace street_pixels
