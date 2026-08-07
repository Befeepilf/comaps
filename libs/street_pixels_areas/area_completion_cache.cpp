#include "street_pixels_areas/area_completion_cache.hpp"

#include "street_pixels_areas/exploration_sidecar.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace street_pixels
{
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
  CHECK_EQUAL(universeAscendingNest.size(), universeCentres.size(), ());
  CHECK_EQUAL(universeAscendingNest.size(), resolver.Universe().size(), ());

  size_t const areaCount = file.m_areas.size();
  std::vector<uint64_t> totals(areaCount, 0);
  std::vector<uint64_t> explored(areaCount, 0);

  for (size_t slot = 0; slot < universeAscendingNest.size(); ++slot)
  {
    ExplorationArea const * area = resolver.LookupBySlot(slot, universeCentres[slot]);
    if (area == nullptr)
      continue;
    uint32_t const idx = area->m_compactIndex;
    if (idx >= areaCount)
      continue;
    ++totals[idx];
  }

  for (int64_t healpix : exploredAscendingNest)
  {
    auto const it = std::lower_bound(universeAscendingNest.begin(), universeAscendingNest.end(), healpix);
    if (it == universeAscendingNest.end() || *it != healpix)
      continue;
    size_t const slot = static_cast<size_t>(std::distance(universeAscendingNest.begin(), it));
    ExplorationArea const * area = resolver.LookupBySlot(slot, universeCentres[slot]);
    if (area == nullptr)
      continue;
    uint32_t const idx = area->m_compactIndex;
    if (idx >= areaCount)
      continue;
    ++explored[idx];
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
}  // namespace street_pixels
