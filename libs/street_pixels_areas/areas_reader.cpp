#include "street_pixels_areas/areas_reader.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_serdes.hpp"

#include "coding/files_container.hpp"
#include "coding/reader.hpp"

#include "defines.hpp"

namespace street_pixels
{
SpaFile ReadExplorationSidecar(std::string const & path)
{
  FilesContainerR container(path);
  SpaFile file;

  {
    FilesContainerR::TReader reader = container.GetReader(SPA_HEADER_FILE_TAG);
    ReaderSource src(reader);
    file.m_header = ReadSpaHeader(src);
  }
  {
    FilesContainerR::TReader reader = container.GetReader(SPA_AREAS_FILE_TAG);
    ReaderSource src(reader);
    file.m_areas = ReadAreasSection(src, file.m_header.m_areaCount);
  }
  {
    FilesContainerR::TReader reader = container.GetReader(SPA_ASSIGN_FILE_TAG);
    ReaderSource src(reader);
    file.m_assignments = ReadAssignSection(src, file.m_header.m_assignCount, file.m_header.m_indexWidth);
  }

  if (file.m_areas.size() != file.m_header.m_areaCount)
    MYTHROW(SpaFormatException, ("Area count mismatch"));
  if (file.m_assignments.size() != file.m_header.m_assignCount)
    MYTHROW(SpaFormatException, ("Assign count mismatch"));

  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  for (uint32_t value : file.m_assignments)
  {
    if (value == sentinel)
      continue;
    if (value >= file.m_areas.size())
      MYTHROW(SpaFormatException, ("Assignment compact index out of range", value));
    if (!file.m_areas[value].IsAssignable())
      MYTHROW(SpaFormatException, ("Assignment points at non-assignable area", value));
  }
  return file;
}
}  // namespace street_pixels
