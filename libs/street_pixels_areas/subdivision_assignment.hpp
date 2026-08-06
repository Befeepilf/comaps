#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "street_pixels_config/country_config.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace street_pixels
{
// Universe-order contract (SPD-021 / SP-028):
//   slot i ↔ ascending HEALPix NEST id U[i] ↔ assign[i]
// See docs/implementation/notes/SP-028-universe-order.md.
// Lookups never invent areas; sentinel / unknown / OOB / non-assignable → nullptr.

ExplorationArea const * LookupSubdivisionBySlot(SpaFile const & file, size_t slot);

// Free-function path: fails closed on size mismatch, non-ascending / duplicate U,
// or missing id → nullptr. Prefer SubdivisionAssignmentTable for production
// (validates U once at TryLoad; hot-path lookups skip the ascending re-scan).
ExplorationArea const * LookupSubdivisionByHealpix(SpaFile const & file,
                                                   std::vector<int64_t> const & universeAscendingNest,
                                                   int64_t healpixNestId);

// Fixture helper: recompute §8.8 dense assignments from area rings + sample
// centres (slot order) and compare to the blob column. Also rejects settlement
// (non-assignable) targets in the stored column.
bool VerifyDenseAssignments(SpaFile const & file, std::vector<m2::PointD> const & sampleCentresInSlotOrder,
                            CountryPolicy const & policy);

// Version-gated consumption of a precomputed subdivision assignment blob.
// Fail closed: missing/corrupt/version/universe mismatch → nullopt.
class SubdivisionAssignmentTable
{
public:
  static std::optional<SubdivisionAssignmentTable> TryLoad(std::string const & path,
                                                           std::vector<int64_t> universeAscendingNest,
                                                           int64_t expectedMapDataVersion,
                                                           uint32_t expectedPolicyVersion);

  SpaFile const & GetFile() const { return m_file; }
  std::vector<int64_t> const & Universe() const { return m_universe; }

  ExplorationArea const * LookupBySlot(size_t slot) const;
  ExplorationArea const * LookupByHealpix(int64_t healpixNestId) const;

private:
  SubdivisionAssignmentTable(SpaFile file, std::vector<int64_t> universe);

  SpaFile m_file;
  std::vector<int64_t> m_universe;
};
}  // namespace street_pixels
