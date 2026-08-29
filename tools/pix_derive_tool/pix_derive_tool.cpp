#include "map/street_pixels_pix_derive.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <gflags/gflags.h>

DEFINE_string(mwm_dir, "", "Directory of leaf .mwm files");
DEFINE_string(mwm, "", "Single .mwm path (can combine with --mwm_dir and leftover argv files)");
DEFINE_string(out_dir, "", "Output directory for {leaf}.pix files");
DEFINE_int64(map_data_version, 0, "map_data_version stamped into .pix headers; 0 = read from MWM");

namespace
{
bool IsWorldOrCoasts(std::string const & leafId)
{
  return leafId == WORLD_FILE_NAME || leafId == WORLD_COASTS_FILE_NAME;
}

void AppendMwmPath(std::vector<std::string> & paths, std::string const & path)
{
  if (path.empty())
    return;
  if (std::find(paths.begin(), paths.end(), path) == paths.end())
    paths.push_back(path);
}

bool CollectFromDirectory(std::string const & dir, std::vector<std::string> & paths)
{
  if (dir.empty())
    return true;
  if (!Platform::IsDirectory(dir))
  {
    LOG(LERROR, ("--mwm_dir is not a directory", dir));
    return false;
  }

  Platform::FilesList files;
  Platform::GetFilesByExt(dir, DATA_FILE_EXTENSION, files);
  std::sort(files.begin(), files.end());
  for (auto const & name : files)
  {
    std::string const leafId = base::FilenameWithoutExt(name);
    if (IsWorldOrCoasts(leafId))
      continue;
    AppendMwmPath(paths, base::JoinPath(dir, name));
  }
  return true;
}
}  // namespace

int main(int argc, char ** argv)
{
  gflags::SetUsageMessage(
      "Street Pixels offline MWM → .pix derive (SP-099).\n"
      "Derives ascending NEST universe U with the same eligibility and 15 m sampling as\n"
      "client first-open (DeriveStreetPixelsUniverse / IsExplorable / kPathSamplingStepMeters).\n"
      "Writes {leaf}.pix with empty explored / ever-live via SaveUnexploredIds.\n"
      "Fail-closed: missing, unreadable, or empty U → non-zero exit, no silent empty .pix.\n"
      "This .pix is the only production U source for spa_emit_tool --mode=production --pix_dir.\n");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::vector<std::string> mwmPaths;
  if (!CollectFromDirectory(FLAGS_mwm_dir, mwmPaths))
    return 1;
  AppendMwmPath(mwmPaths, FLAGS_mwm);
  for (int i = 1; i < argc; ++i)
    AppendMwmPath(mwmPaths, argv[i]);

  if (mwmPaths.empty() || FLAGS_out_dir.empty())
  {
    gflags::ShowUsageWithFlags(argv[0]);
    return 1;
  }

  if (!Platform::MkDirChecked(FLAGS_out_dir))
  {
    LOG(LERROR, ("Cannot create out_dir", FLAGS_out_dir));
    return street_pixels::PixDeriveStatusExitCode(street_pixels::PixDeriveStatus::BadOutput);
  }

  classificator::Load();

  base::Timer timer;
  int exitCode = 0;
  for (auto const & mwmPath : mwmPaths)
  {
    auto const result =
        street_pixels::DeriveAndWritePixFile(mwmPath, FLAGS_out_dir, FLAGS_map_data_version);
    if (result.m_status == street_pixels::PixDeriveStatus::Ok)
    {
      std::cout << "leaf=" << result.m_leafId << " |U|=" << result.m_universeSize
                << " out=" << result.m_outPath << " map_data_version=" << result.m_mapDataVersion << "\n";
      continue;
    }

    LOG(LERROR, ("pix_derive failed", mwmPath, DebugPrint(result.m_status)));
    int const code = street_pixels::PixDeriveStatusExitCode(result.m_status);
    if (exitCode == 0)
      exitCode = code;
  }

  std::cout << "elapsed_s=" << timer.ElapsedSeconds() << "\n";
  return exitCode;
}
