#include "street_pixels_areas/exploration_area_resolver.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

#include <algorithm>
#include <utility>

namespace street_pixels
{
namespace
{
// Named distinctly from subdivision_assignment.cpp for unity builds.
bool ExplorationUniverseIsStrictlyAscending(std::vector<int64_t> const & ids)
{
  for (size_t i = 1; i < ids.size(); ++i)
  {
    if (ids[i] <= ids[i - 1])
      return false;
  }
  return true;
}
}  // namespace

ExplorationArea const * SelectSettlementContaining(SpaFile const & file, m2::PointD const & point)
{
  SettlementContainmentIndex index(file.m_areas);
  return index.Select(point);
}

ExplorationArea const * LookupExplorationAreaAtPoint(SpaFile const & file, CountryPolicy const & policy,
                                                     m2::PointD const & point)
{
  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  uint32_t const compact = AssignSubdivision(point, file.m_areas, policy, sentinel);
  if (compact != sentinel)
  {
    if (auto const * area = FindAreaByCompactIndex(file, compact))
      return area;
  }
  return SelectSettlementContaining(file, point);
}

ExplorationArea const * LookupExplorationArea(SpaFile const & file, size_t slot,
                                              m2::PointD const & sampleCentre)
{
  // Fail closed on OOB: do not invent a settlement from a sample centre alone
  // when the slot is outside the dense assign universe (same class as unknown
  // HEALPix id). Valid sentinel slots still fall through to settlement PIP.
  if (slot >= file.m_assignments.size())
    return nullptr;
  if (auto const * assignable = LookupSubdivisionBySlot(file, slot))
    return assignable;
  return SelectSettlementContaining(file, sampleCentre);
}

ExplorationArea const * LookupExplorationArea(SpaFile const & file,
                                              std::vector<int64_t> const & universeAscendingNest,
                                              int64_t healpixNestId, m2::PointD const & sampleCentre)
{
  if (universeAscendingNest.size() != file.m_assignments.size())
    return nullptr;
  if (!ExplorationUniverseIsStrictlyAscending(universeAscendingNest))
    return nullptr;

  auto const it =
      std::lower_bound(universeAscendingNest.begin(), universeAscendingNest.end(), healpixNestId);
  if (it == universeAscendingNest.end() || *it != healpixNestId)
    return nullptr;

  return LookupExplorationArea(file, static_cast<size_t>(it - universeAscendingNest.begin()),
                               sampleCentre);
}

ExplorationAreaResolver::ExplorationAreaResolver(SubdivisionAssignmentTable table)
  : m_table(std::move(table))
  , m_settlements(m_table.GetFile().m_areas)
{}

ExplorationAreaResolver::ExplorationAreaResolver(ExplorationAreaResolver && other) noexcept
  : m_table(std::move(other.m_table))
  , m_settlements(m_table.GetFile().m_areas)
{
  other.m_settlements = SettlementContainmentIndex();
}

ExplorationAreaResolver & ExplorationAreaResolver::operator=(ExplorationAreaResolver && other) noexcept
{
  if (this == &other)
    return *this;
  m_table = std::move(other.m_table);
  m_settlements = SettlementContainmentIndex(m_table.GetFile().m_areas);
  other.m_settlements = SettlementContainmentIndex();
  return *this;
}

std::optional<ExplorationAreaResolver> ExplorationAreaResolver::TryLoad(
    std::string const & path, std::vector<int64_t> universeAscendingNest, int64_t expectedMapDataVersion,
    uint32_t expectedPolicyVersion)
{
  auto loaded = SubdivisionAssignmentTable::TryLoad(path, std::move(universeAscendingNest),
                                                    expectedMapDataVersion, expectedPolicyVersion);
  if (!loaded.has_value())
    return std::nullopt;
  return ExplorationAreaResolver(std::move(*loaded));
}

ExplorationArea const * ExplorationAreaResolver::LookupBySlot(size_t slot,
                                                              m2::PointD const & sampleCentre) const
{
  SpaFile const & file = m_table.GetFile();
  if (slot >= file.m_assignments.size())
    return nullptr;
  if (auto const * assignable = LookupSubdivisionBySlot(file, slot))
    return assignable;
  return m_settlements.Select(sampleCentre);
}

ExplorationArea const * ExplorationAreaResolver::LookupByHealpix(int64_t healpixNestId,
                                                                 m2::PointD const & sampleCentre) const
{
  auto const & universe = m_table.Universe();
  auto const it = std::lower_bound(universe.begin(), universe.end(), healpixNestId);
  if (it == universe.end() || *it != healpixNestId)
    return nullptr;
  return LookupBySlot(static_cast<size_t>(it - universe.begin()), sampleCentre);
}
}  // namespace street_pixels
