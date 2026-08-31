#pragma once

#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
// Personal area completion (SPD-026 / Phase 5 working formula):
// explored valid street pixels in the area / total valid street pixels in the area.
// Live + imported both count. Zero-total → fraction 0. Keyed by compact index, never MWM id.

struct AreaCompletionCounts
{
  uint32_t m_compactIndex = 0;
  uint64_t m_osmId = 0;
  uint64_t m_explored = 0;
  uint64_t m_total = 0;
};

inline double AreaCompletionFraction(AreaCompletionCounts const & counts)
{
  if (counts.m_total == 0)
    return 0.0;
  return static_cast<double>(counts.m_explored) / static_cast<double>(counts.m_total);
}

struct AccLoadResult;

class AreaCompletionCache
{
public:
  AreaCompletionCache() = default;

  bool IsValid() const { return m_valid; }
  int64_t MapDataVersion() const { return m_mapDataVersion; }
  uint32_t PolicyVersion() const { return m_policyVersion; }
  uint64_t UniverseSize() const { return m_universeSize; }
  uint64_t ExploredCount() const { return m_exploredCount; }
  std::vector<AreaCompletionCounts> const & Rows() const { return m_rows; }

  void Invalidate();

  // universeAscendingNest must match the resolver universe / dense assign column.
  // universeCentres may be empty (centres computed on demand for sentinel slots) or
  // parallel to the universe. exploredAscendingNest must be strictly ascending; ids
  // not in the universe are ignored. No-area pixels contribute to no row. Rows exist
  // for every sidecar area (zero-total allowed).
  // includeSentinelSlots=false skips settlement PIP (neighbourhood rows still exact).
  static AreaCompletionCache Build(ExplorationAreaResolver const & resolver,
                                   std::vector<int64_t> const & universeAscendingNest,
                                   std::vector<m2::PointD> const & universeCentres,
                                   std::vector<int64_t> const & exploredAscendingNest,
                                   bool includeSentinelSlots = true);

  bool MatchesFingerprint(int64_t mapDataVersion, uint32_t policyVersion, uint64_t universeSize,
                          uint64_t exploredCount) const;

  bool RecountExplored(ExplorationAreaResolver const & resolver,
                       std::vector<int64_t> const & universeAscendingNest,
                       std::vector<m2::PointD> const & universeCentres,
                       std::vector<int64_t> const & exploredAscendingNest,
                       bool includeSentinelSlots = true);

  bool Save(std::string const & path) const;

  std::optional<AreaCompletionCounts> Get(uint32_t compactIndex) const;
  std::optional<AreaCompletionCounts> GetByOsmId(uint64_t osmId) const;
  double GetFraction(uint32_t compactIndex) const;

  // Live collect / import: bump explored for one universe pixel. Same slot rules as Build.
  // No-op and false when invalid, unknown, no-area, or already at total.
  bool AddExploredHealpix(ExplorationAreaResolver const & resolver, int64_t healpixNestId);

  friend AccLoadResult TryLoadAreaCompletionCache(std::string const & path);

private:
  void ApplyCounts(SpaFile const & file, std::vector<uint64_t> const & totals,
                   std::vector<uint64_t> const & explored);

  bool m_valid = false;
  int64_t m_mapDataVersion = 0;
  uint32_t m_policyVersion = 0;
  uint64_t m_universeSize = 0;
  uint64_t m_exploredCount = 0;
  std::vector<AreaCompletionCounts> m_rows;
};

enum class AccLoadStatus : uint8_t
{
  Ok = 0,
  Missing = 1,
  Corrupt = 2,
};

struct AccLoadResult
{
  AccLoadStatus m_status = AccLoadStatus::Missing;
  AreaCompletionCache m_cache;
};

std::string AreaCompletionCachePath(std::string const & directory, std::string const & countryId);
AccLoadResult TryLoadAreaCompletionCache(std::string const & path);
}  // namespace street_pixels
