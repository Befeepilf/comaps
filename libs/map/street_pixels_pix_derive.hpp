#pragma once

#include <cstdint>
#include <set>
#include <string>

namespace street_pixels
{
enum class PixDeriveStatus
{
  Ok,
  MissingMwm,
  UnreadableMwm,
  EmptyUniverse,
  WriteFailed,
  BadOutput,
  NotALeaf
};

struct PixDeriveResult
{
  PixDeriveStatus m_status = PixDeriveStatus::MissingMwm;
  std::string m_leafId;
  std::string m_outPath;
  size_t m_universeSize = 0;
  int64_t m_mapDataVersion = 0;
};

std::string DebugPrint(PixDeriveStatus status);

int PixDeriveStatusExitCode(PixDeriveStatus status);

PixDeriveStatus WriteUnexploredUniversePix(std::string const & outPath, std::set<std::int64_t> const & universe,
                                           int64_t mapDataVersion);

PixDeriveResult DeriveAndWritePixFile(std::string const & mwmPath, std::string const & outDir,
                                      int64_t mapDataVersionOverride);
}  // namespace street_pixels
