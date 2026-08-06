#pragma once

#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/subdivision_assignment.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
// SPD-007 / SPD-025 settlement fallback on true municipal rings from the
// sidecar. Not CitiesBoundariesTable. Dense assign stays subdivision-only.

// Among Settlement-role areas whose true rings contain `point`, pick smallest
// m_area then lower OSM id (then compact index). Empty / none → nullptr.
ExplorationArea const * SelectSettlementContaining(SpaFile const & file, m2::PointD const & point);

// Layering: SP-028 assignable (subdivision / place) → keep; else settlement
// PIP on sample centre; else no-area (nullptr). Never invents grids.
ExplorationArea const * LookupExplorationArea(SpaFile const & file, size_t slot,
                                              m2::PointD const & sampleCentre);

// Fail closed on size mismatch, non-ascending / duplicate U, or unknown id
// (no settlement PIP for undecidable slots). Prefer ExplorationAreaResolver.
ExplorationArea const * LookupExplorationArea(SpaFile const & file,
                                              std::vector<int64_t> const & universeAscendingNest,
                                              int64_t healpixNestId, m2::PointD const & sampleCentre);

// Version-gated consumption: same TryLoad gates as SubdivisionAssignmentTable,
// then layered lookup (subdiv/place → settlement → none).
class ExplorationAreaResolver
{
public:
  static std::optional<ExplorationAreaResolver> TryLoad(std::string const & path,
                                                        std::vector<int64_t> universeAscendingNest,
                                                        int64_t expectedMapDataVersion,
                                                        uint32_t expectedPolicyVersion);

  SpaFile const & GetFile() const { return m_table.GetFile(); }
  std::vector<int64_t> const & Universe() const { return m_table.Universe(); }
  SubdivisionAssignmentTable const & SubdivisionTable() const { return m_table; }

  ExplorationArea const * LookupBySlot(size_t slot, m2::PointD const & sampleCentre) const;
  ExplorationArea const * LookupByHealpix(int64_t healpixNestId, m2::PointD const & sampleCentre) const;

private:
  explicit ExplorationAreaResolver(SubdivisionAssignmentTable table);

  SubdivisionAssignmentTable m_table;
};
}  // namespace street_pixels
