#include "street_pixels_areas/exploration_sidecar.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_reader.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

namespace street_pixels
{
std::string ExplorationSidecarPath(std::string const & directory, std::string const & mwmLeafId)
{
  return base::JoinPath(directory, mwmLeafId + SPA_FILE_EXTENSION);
}

std::string ExplorationSidecarPathBesideMwm(std::string const & mwmPath)
{
  return ExplorationSidecarPath(base::GetDirectory(mwmPath), base::GetNameFromFullPathWithoutExt(mwmPath));
}

SpaLoadResult TryLoadExplorationSidecar(std::string const & path)
{
  SpaLoadResult result;
  if (!Platform::IsFileExistsByFullPath(path))
  {
    result.m_status = SpaLoadStatus::Missing;
    return result;
  }
  // Directories also "exist"; never invent areas from a non-file path.
  if (Platform::IsDirectory(path))
  {
    LOG(LWARNING, ("Exploration sidecar path is a directory", path));
    result.m_status = SpaLoadStatus::Corrupt;
    return result;
  }

  try
  {
    result.m_file = ReadExplorationSidecar(path);
    result.m_status = SpaLoadStatus::Ok;
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Corrupt exploration sidecar", path, ex.Msg()));
    result.m_file = SpaFile{};
    result.m_status = SpaLoadStatus::Corrupt;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Corrupt exploration sidecar", path, ex.what()));
    result.m_file = SpaFile{};
    result.m_status = SpaLoadStatus::Corrupt;
  }
  catch (...)
  {
    LOG(LWARNING, ("Corrupt exploration sidecar", path, "unknown exception"));
    result.m_file = SpaFile{};
    result.m_status = SpaLoadStatus::Corrupt;
  }
  return result;
}

SpaLoadResult TryLoadAndVerifyExplorationSidecar(std::string const & path, int64_t expectedMapDataVersion,
                                                 uint32_t expectedPolicyVersion)
{
  SpaLoadResult result = TryLoadExplorationSidecar(path);
  if (result.m_status != SpaLoadStatus::Ok)
    return result;

  if (result.m_file.m_header.m_mapDataVersion != expectedMapDataVersion ||
      result.m_file.m_header.m_policyVersion != expectedPolicyVersion)
  {
    LOG(LWARNING, ("Exploration sidecar version mismatch", path, "map", result.m_file.m_header.m_mapDataVersion,
                   expectedMapDataVersion, "policy", result.m_file.m_header.m_policyVersion, expectedPolicyVersion));
    result.m_file = SpaFile{};
    result.m_status = SpaLoadStatus::VersionMismatch;
  }
  return result;
}

uint64_t StableOsmId(ExplorationArea const & area)
{
  return area.m_osmId;
}

std::string const & DisplayName(ExplorationArea const & area)
{
  return area.m_name;
}

std::vector<ExplorationArea const *> AreasByRole(SpaFile const & file, AreaRole role)
{
  std::vector<ExplorationArea const *> out;
  for (auto const & area : file.m_areas)
  {
    if (area.m_role == role)
      out.push_back(&area);
  }
  return out;
}

std::vector<uint32_t> const & DenseAssignments(SpaFile const & file)
{
  return file.m_assignments;
}

std::vector<ExplorationArea const *> SettlementAreas(SpaFile const & file)
{
  return AreasByRole(file, AreaRole::Settlement);
}

ExplorationArea const * FindAreaByCompactIndex(SpaFile const & file, uint32_t compactIndex)
{
  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  if (compactIndex == sentinel || compactIndex >= file.m_areas.size())
    return nullptr;
  return &file.m_areas[compactIndex];
}

std::string DebugPrint(SpaLoadStatus status)
{
  switch (status)
  {
  case SpaLoadStatus::Ok: return "Ok";
  case SpaLoadStatus::Missing: return "Missing";
  case SpaLoadStatus::Corrupt: return "Corrupt";
  case SpaLoadStatus::VersionMismatch: return "VersionMismatch";
  }
  return "UnknownSpaLoadStatus";
}
}  // namespace street_pixels
