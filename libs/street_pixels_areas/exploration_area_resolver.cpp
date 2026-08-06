#include "street_pixels_areas/exploration_area_resolver.hpp"

#include <algorithm>
#include <limits>
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

struct SettlementScore
{
  double m_area = std::numeric_limits<double>::max();
  uint64_t m_osmId = std::numeric_limits<uint64_t>::max();
  uint32_t m_compactIndex = 0;

  bool operator<(SettlementScore const & other) const
  {
    if (m_area != other.m_area)
      return m_area < other.m_area;
    if (m_osmId != other.m_osmId)
      return m_osmId < other.m_osmId;
    return m_compactIndex < other.m_compactIndex;
  }
};
}  // namespace

ExplorationArea const * SelectSettlementContaining(SpaFile const & file, m2::PointD const & point)
{
  bool found = false;
  SettlementScore best;
  ExplorationArea const * bestArea = nullptr;

  for (auto const & area : file.m_areas)
  {
    if (area.m_role != AreaRole::Settlement)
      continue;
    if (!area.Contains(point))
      continue;

    SettlementScore score;
    score.m_area = area.m_area;
    score.m_osmId = area.m_osmId;
    score.m_compactIndex = area.m_compactIndex;
    if (!found || score < best)
    {
      best = score;
      bestArea = &area;
      found = true;
    }
  }
  return bestArea;
}

ExplorationArea const * LookupExplorationArea(SpaFile const & file, size_t slot,
                                              m2::PointD const & sampleCentre)
{
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
  if (!IsStrictlyAscending(universeAscendingNest))
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
{}

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
  return LookupExplorationArea(m_table.GetFile(), slot, sampleCentre);
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
