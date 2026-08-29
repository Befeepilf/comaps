#include "map/street_pixels_pix_derive.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "indexer/data_header.hpp"
#include "indexer/features_vector.hpp"

#include "platform/constants.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <exception>
#include <string>

namespace street_pixels
{
namespace
{
bool IsWorldOrCoastsLeafId(std::string const & leafId)
{
  return leafId == WORLD_FILE_NAME || leafId == WORLD_COASTS_FILE_NAME;
}

bool ContainerHasRequiredMwmTags(FilesContainerR const & container)
{
  return container.IsExist(VERSION_FILE_TAG) && container.IsExist(HEADER_FILE_TAG) &&
         container.IsExist(FEATURES_FILE_TAG);
}

bool IsCountryMwmHeader(feature::DataHeader const & header)
{
  return header.GetType() == feature::DataHeader::MapType::Country;
}
}  // namespace

std::string DebugPrint(PixDeriveStatus status)
{
  switch (status)
  {
  case PixDeriveStatus::Ok: return "Ok";
  case PixDeriveStatus::MissingMwm: return "MissingMwm";
  case PixDeriveStatus::UnreadableMwm: return "UnreadableMwm";
  case PixDeriveStatus::EmptyUniverse: return "EmptyUniverse";
  case PixDeriveStatus::WriteFailed: return "WriteFailed";
  case PixDeriveStatus::BadOutput: return "BadOutput";
  case PixDeriveStatus::NotALeaf: return "NotALeaf";
  }
  return "Unknown";
}

int PixDeriveStatusExitCode(PixDeriveStatus status)
{
  switch (status)
  {
  case PixDeriveStatus::Ok: return 0;
  case PixDeriveStatus::MissingMwm: return 1;
  case PixDeriveStatus::UnreadableMwm: return 2;
  case PixDeriveStatus::EmptyUniverse: return 3;
  case PixDeriveStatus::WriteFailed: return 4;
  case PixDeriveStatus::BadOutput: return 5;
  case PixDeriveStatus::NotALeaf: return 1;
  }
  return 1;
}

PixDeriveStatus WriteUnexploredUniversePix(std::string const & outPath, std::set<std::int64_t> const & universe,
                                           int64_t mapDataVersion)
{
  if (outPath.empty())
    return PixDeriveStatus::BadOutput;
  if (universe.empty())
    return PixDeriveStatus::EmptyUniverse;
  if (!street_pixels_file::SaveUnexploredIds(outPath, universe, mapDataVersion))
    return PixDeriveStatus::WriteFailed;
  return PixDeriveStatus::Ok;
}

PixDeriveResult DeriveAndWritePixFile(std::string const & mwmPath, std::string const & outDir,
                                      int64_t mapDataVersionOverride)
{
  PixDeriveResult result;
  result.m_leafId = base::GetNameFromFullPathWithoutExt(mwmPath);
  if (result.m_leafId.empty() || outDir.empty())
  {
    result.m_status = PixDeriveStatus::BadOutput;
    return result;
  }
  if (!Platform::MkDirChecked(outDir))
  {
    result.m_status = PixDeriveStatus::BadOutput;
    return result;
  }

  result.m_outPath = base::JoinPath(outDir, result.m_leafId + PIX_FILE_EXTENSION);

  if (mwmPath.empty() || !Platform::IsFileExistsByFullPath(mwmPath) || Platform::IsDirectory(mwmPath))
  {
    result.m_status = PixDeriveStatus::MissingMwm;
    return result;
  }
  if (IsWorldOrCoastsLeafId(result.m_leafId))
  {
    result.m_status = PixDeriveStatus::NotALeaf;
    return result;
  }

  try
  {
    FilesContainerR const container(mwmPath, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
    if (!ContainerHasRequiredMwmTags(container))
    {
      result.m_status = PixDeriveStatus::UnreadableMwm;
      return result;
    }

    feature::DataHeader const header(container);
    if (!IsCountryMwmHeader(header))
    {
      result.m_status = PixDeriveStatus::NotALeaf;
      return result;
    }

    int64_t mapDataVersion = mapDataVersionOverride;
    if (mapDataVersion == 0)
      mapDataVersion = static_cast<int64_t>(version::MwmVersion::Read(container).GetVersion());
    result.m_mapDataVersion = mapDataVersion;

    FeaturesVectorTest featuresVector(container);
    std::set<std::int64_t> const universe = DeriveStreetPixelsUniverse(featuresVector);
    result.m_universeSize = universe.size();
    result.m_status = WriteUnexploredUniversePix(result.m_outPath, universe, mapDataVersion);
    return result;
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Unreadable MWM", mwmPath, ex.what()));
    result.m_status = PixDeriveStatus::UnreadableMwm;
    return result;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Unreadable MWM", mwmPath, ex.what()));
    result.m_status = PixDeriveStatus::UnreadableMwm;
    return result;
  }
}
}  // namespace street_pixels
