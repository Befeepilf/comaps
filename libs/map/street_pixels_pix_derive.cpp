#include "map/street_pixels_pix_derive.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"

#include "indexer/features_vector.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/files_container.hpp"
#include "coding/reader.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <exception>
#include <string>

namespace street_pixels
{
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

bool ContainerLooksLikeMwm(std::string const & mwmPath)
{
  try
  {
    FileReader reader(mwmPath);
    if (reader.Size() < sizeof(uint64_t))
      return false;
    uint64_t const offset = ReadPrimitiveFromPos<uint64_t>(reader, 0);
    if (offset >= reader.Size())
      return false;

    FilesContainerR const container(mwmPath);
    return container.IsExist(VERSION_FILE_TAG) && container.IsExist(HEADER_FILE_TAG) &&
           container.IsExist(FEATURES_FILE_TAG);
  }
  catch (RootException const &)
  {
    return false;
  }
  catch (std::exception const &)
  {
    return false;
  }
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
  if (!ContainerLooksLikeMwm(mwmPath))
  {
    result.m_status = PixDeriveStatus::UnreadableMwm;
    return result;
  }

  try
  {
    int64_t mapDataVersion = mapDataVersionOverride;
    if (mapDataVersion == 0)
    {
      FilesContainerR const container(mwmPath);
      mapDataVersion = static_cast<int64_t>(version::MwmVersion::Read(container).GetVersion());
    }
    result.m_mapDataVersion = mapDataVersion;

    FeaturesVectorTest featuresVector(mwmPath);
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
