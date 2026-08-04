#include "street_pixels_areas/subdivision_assignment.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

#include <algorithm>
#include <utility>

namespace street_pixels
{
namespace
{
bool IsStrictlyAscending(std::vector<int64_t> const & ids)
{
  for (size_t i = 1; i < ids.size(); ++i)
  {
    if (ids[i] <= ids[i - 1])
      return false;
  }
  return true;
}
}  // namespace

ExplorationArea const * LookupSubdivisionBySlot(SpaFile const & file, size_t slot)
{
  if (slot >= file.m_assignments.size())
    return nullptr;
  return FindAreaByCompactIndex(file, file.m_assignments[slot]);
}

ExplorationArea const * LookupSubdivisionByHealpix(SpaFile const & file,
                                                   std::vector<int64_t> const & universeAscendingNest,
                                                   int64_t healpixNestId)
{
  if (universeAscendingNest.size() != file.m_assignments.size())
    return nullptr;
  auto const it =
      std::lower_bound(universeAscendingNest.begin(), universeAscendingNest.end(), healpixNestId);
  if (it == universeAscendingNest.end() || *it != healpixNestId)
    return nullptr;
  return LookupSubdivisionBySlot(file, static_cast<size_t>(it - universeAscendingNest.begin()));
}

bool VerifyDenseAssignments(SpaFile const & file, std::vector<m2::PointD> const & sampleCentresInSlotOrder,
                            CountryPolicy const & policy)
{
  if (sampleCentresInSlotOrder.size() != file.m_assignments.size())
    return false;

  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  auto const recomputed = BuildDenseAssignments(sampleCentresInSlotOrder, file.m_areas, policy, sentinel);
  if (recomputed != file.m_assignments)
    return false;

  for (uint32_t value : file.m_assignments)
  {
    if (value == sentinel)
      continue;
    auto const * area = FindAreaByCompactIndex(file, value);
    if (area == nullptr || !area->IsAssignable())
      return false;
  }
  return true;
}

SubdivisionAssignmentTable::SubdivisionAssignmentTable(SpaFile file, std::vector<int64_t> universe)
  : m_file(std::move(file)), m_universe(std::move(universe))
{}

std::optional<SubdivisionAssignmentTable> SubdivisionAssignmentTable::TryLoad(
    std::string const & path, std::vector<int64_t> universeAscendingNest, int64_t expectedMapDataVersion,
    uint32_t expectedPolicyVersion)
{
  auto loaded = TryLoadAndVerifyExplorationSidecar(path, expectedMapDataVersion, expectedPolicyVersion);
  if (loaded.m_status != SpaLoadStatus::Ok)
    return std::nullopt;

  if (universeAscendingNest.size() != loaded.m_file.m_assignments.size())
    return std::nullopt;
  if (!IsStrictlyAscending(universeAscendingNest))
    return std::nullopt;

  return SubdivisionAssignmentTable(std::move(loaded.m_file), std::move(universeAscendingNest));
}

ExplorationArea const * SubdivisionAssignmentTable::LookupBySlot(size_t slot) const
{
  return LookupSubdivisionBySlot(m_file, slot);
}

ExplorationArea const * SubdivisionAssignmentTable::LookupByHealpix(int64_t healpixNestId) const
{
  return LookupSubdivisionByHealpix(m_file, m_universe, healpixNestId);
}
}  // namespace street_pixels
