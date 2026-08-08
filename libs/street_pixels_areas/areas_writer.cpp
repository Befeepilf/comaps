#include "street_pixels_areas/areas_writer.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_serdes.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

#include "coding/files_container.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "defines.hpp"

#include <utility>
#include <vector>

namespace street_pixels
{
namespace
{
std::vector<ExplorationArea> RoundTripAreasForAssign(std::vector<ExplorationArea> const & areas)
{
  std::vector<uint8_t> buffer;
  {
    MemWriter<std::vector<uint8_t>> writer(buffer);
    WriteAreasSection(writer, areas);
  }
  MemReader reader(buffer.data(), buffer.size());
  ReaderSource src(reader);
  return ReadAreasSection(src, static_cast<uint32_t>(areas.size()));
}
}  // namespace

void WriteExplorationSidecar(std::string const & path, std::vector<ExplorationArea> areas,
                             std::vector<m2::PointD> const & samplePoints, CountryPolicy const & policy,
                             SpaWriteParams const & params)
{
  for (uint32_t i = 0; i < areas.size(); ++i)
    areas[i].m_compactIndex = i;

  areas = RoundTripAreasForAssign(areas);

  SpaHeader header;
  header.m_magic = kSpaMagic;
  header.m_formatVersion = kSpaFormatVersion;
  header.m_mapDataVersion = params.m_mapDataVersion;
  header.m_policyVersion = params.m_policyVersion;
  header.m_isoCode = params.m_isoCode;
  header.m_mwmId = params.m_mwmId;
  header.m_areaCount = static_cast<uint32_t>(areas.size());
  header.m_indexWidth = ChooseIndexWidth(header.m_areaCount);
  header.m_nside = kSpaNside;
  header.m_universeOrder = kSpaUniverseOrderAscendingNest;
  uint32_t const sentinel = NoSubdivisionSentinel(header.m_indexWidth);

  auto assignments = BuildDenseAssignments(samplePoints, areas, policy, sentinel);
  header.m_assignCount = static_cast<uint32_t>(assignments.size());

  FilesContainerW container(path, FileWriter::OP_WRITE_TRUNCATE);
  {
    auto w = container.GetWriter(SPA_HEADER_FILE_TAG);
    WriteSpaHeader(*w, header);
  }
  {
    auto w = container.GetWriter(SPA_AREAS_FILE_TAG);
    WriteAreasSection(*w, areas);
  }
  {
    auto w = container.GetWriter(SPA_ASSIGN_FILE_TAG);
    WriteAssignSection(*w, assignments, header.m_indexWidth);
  }
  container.Finish();
}
}  // namespace street_pixels
