#include "street_pixels_areas/area_completion_cache.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/sample_centres.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>
#include <optional>

namespace street_pixels
{
namespace
{
m2::PointD CentreForSlot(std::vector<int64_t> const & universeAscendingNest,
                         std::vector<m2::PointD> const & universeCentres, size_t slot)
{
  if (universeCentres.size() == universeAscendingNest.size())
    return universeCentres[slot];
  return MercatorCentreFromNestId(universeAscendingNest[slot]);
}

std::optional<uint32_t> CompactIndexForSlot(SpaFile const & file,
                                            SettlementContainmentIndex const & settlements,
                                            uint32_t sentinel, size_t areaCount, size_t slot,
                                            std::vector<int64_t> const & universeAscendingNest,
                                            std::vector<m2::PointD> const & universeCentres)
{
  uint32_t const assign = file.m_assignments[slot];
  if (assign != sentinel)
  {
    if (assign < areaCount && file.m_areas[assign].IsAssignable())
      return assign;
    return std::nullopt;
  }

  ExplorationArea const * area =
      settlements.Select(CentreForSlot(universeAscendingNest, universeCentres, slot));
  if (area == nullptr)
    return std::nullopt;
  uint32_t const idx = area->m_compactIndex;
  if (idx >= areaCount)
    return std::nullopt;
  return idx;
}

void AccumulateSlot(SpaFile const & file, SettlementContainmentIndex const & settlements,
                    uint32_t sentinel, size_t areaCount, size_t slot,
                    std::vector<int64_t> const & universeAscendingNest,
                    std::vector<m2::PointD> const & universeCentres, std::vector<uint64_t> & counts)
{
  if (auto const idx = CompactIndexForSlot(file, settlements, sentinel, areaCount, slot,
                                           universeAscendingNest, universeCentres))
    ++counts[*idx];
}
}  // namespace

void AreaCompletionCache::Invalidate()
{
  m_valid = false;
  m_mapDataVersion = 0;
  m_policyVersion = 0;
  m_rows.clear();
}

AreaCompletionCache AreaCompletionCache::Build(ExplorationAreaResolver const & resolver,
                                               std::vector<int64_t> const & universeAscendingNest,
                                               std::vector<m2::PointD> const & universeCentres,
                                               std::vector<int64_t> const & exploredAscendingNest)
{
  AreaCompletionCache cache;
  SpaFile const & file = resolver.GetFile();
  CHECK_EQUAL(universeAscendingNest.size(), resolver.Universe().size(), ());
  CHECK(universeCentres.empty() || universeCentres.size() == universeAscendingNest.size(), ());
  CHECK_EQUAL(universeAscendingNest.size(), file.m_assignments.size(), ());

  size_t const areaCount = file.m_areas.size();
  std::vector<uint64_t> totals(areaCount, 0);
  std::vector<uint64_t> explored(areaCount, 0);

  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  SettlementContainmentIndex const & settlements = resolver.Settlements();

  for (size_t slot = 0; slot < universeAscendingNest.size(); ++slot)
  {
    AccumulateSlot(file, settlements, sentinel, areaCount, slot, universeAscendingNest, universeCentres,
                   totals);
  }

  size_t universeSlot = 0;
  int64_t prevExplored = std::numeric_limits<int64_t>::min();
  bool sawExplored = false;
  for (int64_t healpix : exploredAscendingNest)
  {
    if (sawExplored)
      CHECK_GREATER(healpix, prevExplored, ());
    sawExplored = true;
    prevExplored = healpix;

    while (universeSlot < universeAscendingNest.size() &&
           universeAscendingNest[universeSlot] < healpix)
    {
      ++universeSlot;
    }
    if (universeSlot >= universeAscendingNest.size() ||
        universeAscendingNest[universeSlot] != healpix)
    {
      continue;
    }
    AccumulateSlot(file, settlements, sentinel, areaCount, universeSlot, universeAscendingNest,
                   universeCentres, explored);
  }

  cache.m_rows.reserve(areaCount);
  for (size_t i = 0; i < areaCount; ++i)
  {
    AreaCompletionCounts row;
    row.m_compactIndex = file.m_areas[i].m_compactIndex;
    row.m_osmId = StableOsmId(file.m_areas[i]);
    row.m_total = totals[i];
    row.m_explored = explored[i];
    if (row.m_explored > row.m_total)
      row.m_explored = row.m_total;
    cache.m_rows.push_back(row);
  }

  cache.m_mapDataVersion = file.m_header.m_mapDataVersion;
  cache.m_policyVersion = file.m_header.m_policyVersion;
  cache.m_valid = true;
  return cache;
}

std::optional<AreaCompletionCounts> AreaCompletionCache::Get(uint32_t compactIndex) const
{
  if (!m_valid)
    return std::nullopt;
  for (auto const & row : m_rows)
  {
    if (row.m_compactIndex == compactIndex)
      return row;
  }
  return std::nullopt;
}

double AreaCompletionCache::GetFraction(uint32_t compactIndex) const
{
  auto const counts = Get(compactIndex);
  if (!counts)
    return 0.0;
  return AreaCompletionFraction(*counts);
}

bool AreaCompletionCache::AddExploredHealpix(ExplorationAreaResolver const & resolver,
                                             int64_t healpixNestId)
{
  if (!m_valid)
    return false;

  auto const & universe = resolver.Universe();
  auto const it = std::lower_bound(universe.begin(), universe.end(), healpixNestId);
  if (it == universe.end() || *it != healpixNestId)
    return false;

  SpaFile const & file = resolver.GetFile();
  size_t const slot = static_cast<size_t>(it - universe.begin());
  uint32_t const sentinel = NoSubdivisionSentinel(file.m_header.m_indexWidth);
  auto const compact = CompactIndexForSlot(file, resolver.Settlements(), sentinel, file.m_areas.size(),
                                           slot, universe, {});
  if (!compact)
    return false;

  for (auto & row : m_rows)
  {
    if (row.m_compactIndex != *compact)
      continue;
    if (row.m_explored >= row.m_total)
      return false;
    ++row.m_explored;
    return true;
  }
  return false;
}
}  // namespace street_pixels
