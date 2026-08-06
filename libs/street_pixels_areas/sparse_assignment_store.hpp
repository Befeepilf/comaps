#pragma once

#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"

#include "geometry/point2d.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
DECLARE_EXCEPTION(SpxFormatException, RootException);

struct SpxHeader
{
  uint32_t m_magic = 0;
  uint32_t m_formatVersion = 0;
  int64_t m_mapDataVersion = 0;
  uint32_t m_policyVersion = 0;
  uint32_t m_entryCount = 0;
  uint8_t m_indexWidth = 2;
};

struct SparseAssignmentEntry
{
  int64_t m_healpixNestId = 0;
  // Compact area index, or no-area sentinel for the store's index_width.
  uint32_t m_compactIndex = 0;
};

enum class SpxLoadStatus : uint8_t
{
  Ok = 0,
  Missing = 1,
  Corrupt = 2,
  VersionMismatch = 3,
};

// `{directory}/{countryId}.spx` — durable sparse store beside `.pix` (SPD-022).
std::string SparseAssignmentPath(std::string const & directory, std::string const & countryId);
std::string SparseAssignmentPathBesidePix(std::string const & pixPath);

// Sparse explored HEALPix → compact area index. No uint64 OSM column.
// Rematerialize from ExplorationAreaResolver + explored set (SPD-021/022).
class SparseAssignmentStore
{
public:
  SparseAssignmentStore() = default;
  SparseAssignmentStore(SpxHeader header, std::vector<SparseAssignmentEntry> entries);

  SpxHeader const & GetHeader() const { return m_header; }
  std::vector<SparseAssignmentEntry> const & Entries() const { return m_entries; }

  bool MatchesVersions(int64_t mapDataVersion, uint32_t policyVersion) const;

  // True when entries are exactly the explored set (both strictly ascending).
  bool CoversExplored(std::vector<int64_t> const & exploredAscendingNest) const;

  // Binary search; nullopt if healpix is not in the sparse map.
  std::optional<uint32_t> FindCompactIndex(int64_t healpixNestId) const;

  // Sentinel / missing area row → nullptr. Never invents grids.
  ExplorationArea const * LookupArea(SpaFile const & file, int64_t healpixNestId) const;

  // exploredAscendingNest must be strictly ascending; centres parallel by index.
  // Missing / non-assignable / no-area → sentinel. index_width follows sidecar area count.
  static SparseAssignmentStore Build(ExplorationAreaResolver const & resolver,
                                     std::vector<int64_t> const & exploredAscendingNest,
                                     std::vector<m2::PointD> const & sampleCentres);

  // Alias of Build — rematerialize sparse state from dense sidecar + settlement layering.
  static SparseAssignmentStore Rematerialize(ExplorationAreaResolver const & resolver,
                                             std::vector<int64_t> const & exploredAscendingNest,
                                             std::vector<m2::PointD> const & sampleCentres);

  // Temp+rename. Returns false on write failure (dest left intact).
  bool Save(std::string const & path) const;

private:
  SpxHeader m_header;
  std::vector<SparseAssignmentEntry> m_entries;
};

struct SpxLoadResult
{
  SpxLoadStatus m_status = SpxLoadStatus::Missing;
  SparseAssignmentStore m_store;
};

// Missing / corrupt → status; does not throw.
SpxLoadResult TryLoadSparseAssignmentStore(std::string const & path);

// Like TryLoad, then requires map_data + policy versions.
SpxLoadResult TryLoadAndVerifySparseAssignmentStore(std::string const & path, int64_t expectedMapDataVersion,
                                                    uint32_t expectedPolicyVersion);

// Load if versions match and sparse rows cover explored; else rematerialize + save.
// Corrupt / incomplete / version mismatch → rebuild. Never touches `.pix`.
// Returns the store when durable state is current; nullopt on failure.
std::optional<SparseAssignmentStore> EnsureSparseAssignmentStore(
    std::string const & spxPath, ExplorationAreaResolver const & resolver,
    std::vector<int64_t> const & exploredAscendingNest, std::vector<m2::PointD> const & sampleCentres);

std::string DebugPrint(SpxLoadStatus status);
}  // namespace street_pixels
