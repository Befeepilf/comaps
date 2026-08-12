#pragma once

#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/settlement_containment.hpp"
#include "street_pixels_areas/subdivision_assignment.hpp"

#include "street_pixels_config/country_config.hpp"

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
// Builds a one-shot SettlementContainmentIndex (cached RegionD). Prefer
// ExplorationAreaResolver::Settlements() for batch / hot paths.
ExplorationArea const * SelectSettlementContaining(SpaFile const & file, m2::PointD const & point);

// Point-based layering for focus stub / SP-036: AssignSubdivision (policy) →
// settlement PIP → nullptr. Never invents grids. Never uses MWM id as a name.
ExplorationArea const * LookupExplorationAreaAtPoint(SpaFile const & file, CountryPolicy const & policy,
                                                     m2::PointD const & point);

// Layering: SP-028 assignable (subdivision / place) → keep; else settlement
// PIP on sample centre; else no-area (nullptr). Never invents grids.
// OOB slot → nullptr (fail closed; no settlement invent from centre alone).
ExplorationArea const * LookupExplorationArea(SpaFile const & file, size_t slot,
                                              m2::PointD const & sampleCentre);

// Fail closed on size mismatch, non-ascending / duplicate U, or unknown id
// (no settlement PIP for undecidable slots). Prefer ExplorationAreaResolver.
ExplorationArea const * LookupExplorationArea(SpaFile const & file,
                                              std::vector<int64_t> const & universeAscendingNest,
                                              int64_t healpixNestId, m2::PointD const & sampleCentre);

// Version-gated consumption: same TryLoad gates as SubdivisionAssignmentTable,
// then layered lookup (subdiv/place → settlement → none).
// Not copyable: settlement index aliases SpaFile areas owned by m_table.
class ExplorationAreaResolver
{
public:
  static std::optional<ExplorationAreaResolver> TryLoad(std::string const & path,
                                                        std::vector<int64_t> universeAscendingNest,
                                                        int64_t expectedMapDataVersion,
                                                        uint32_t expectedPolicyVersion);

  ExplorationAreaResolver(ExplorationAreaResolver const &) = delete;
  ExplorationAreaResolver & operator=(ExplorationAreaResolver const &) = delete;
  ExplorationAreaResolver(ExplorationAreaResolver && other) noexcept;
  ExplorationAreaResolver & operator=(ExplorationAreaResolver && other) noexcept;

  SpaFile const & GetFile() const { return m_table.GetFile(); }
  std::vector<int64_t> const & Universe() const { return m_table.Universe(); }
  SubdivisionAssignmentTable const & SubdivisionTable() const { return m_table; }
  SettlementContainmentIndex const & Settlements() const { return m_settlements; }

  ExplorationArea const * LookupBySlot(size_t slot, m2::PointD const & sampleCentre) const;
  ExplorationArea const * LookupByHealpix(int64_t healpixNestId, m2::PointD const & sampleCentre) const;

private:
  explicit ExplorationAreaResolver(SubdivisionAssignmentTable table);

  SubdivisionAssignmentTable m_table;
  SettlementContainmentIndex m_settlements;
};
}  // namespace street_pixels
