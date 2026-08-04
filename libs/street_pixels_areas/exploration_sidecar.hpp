#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace street_pixels
{
enum class SpaLoadStatus : uint8_t
{
  Ok = 0,
  Missing = 1,
  Corrupt = 2,
  VersionMismatch = 3,
};

struct SpaLoadResult
{
  SpaLoadStatus m_status = SpaLoadStatus::Missing;
  SpaFile m_file;
};

// `{directory}/{mwmLeafId}.spa` — sidecar sits beside the MWM leaf.
std::string ExplorationSidecarPath(std::string const & directory, std::string const & mwmLeafId);

// `{dir}/{leaf}.mwm` → `{dir}/{leaf}.spa`.
std::string ExplorationSidecarPathBesideMwm(std::string const & mwmPath);

// Missing / corrupt → empty SpaFile, no throw. Offline-only; no network.
SpaLoadResult TryLoadExplorationSidecar(std::string const & path);

// Like TryLoad, then requires header map_data_version and policy_version.
// Mismatch → VersionMismatch and empty SpaFile (safe for rematch deferral).
SpaLoadResult TryLoadAndVerifyExplorationSidecar(std::string const & path, int64_t expectedMapDataVersion,
                                                 uint32_t expectedPolicyVersion);

uint64_t StableOsmId(ExplorationArea const & area);

// Stored OSM name only. Never falls back to MWM leaf / country id.
std::string const & DisplayName(ExplorationArea const & area);

std::vector<ExplorationArea const *> AreasByRole(SpaFile const & file, AreaRole role);

std::vector<uint32_t> const & DenseAssignments(SpaFile const & file);

// Settlement rows with true rings (SPD-025). Not CitiesBoundariesTable.
std::vector<ExplorationArea const *> SettlementAreas(SpaFile const & file);

// Sentinel / out-of-range → nullptr. Does not invent areas.
ExplorationArea const * FindAreaByCompactIndex(SpaFile const & file, uint32_t compactIndex);

std::string DebugPrint(SpaLoadStatus status);
}  // namespace street_pixels
