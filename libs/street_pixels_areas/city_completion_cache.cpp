#include "street_pixels_areas/city_completion_cache.hpp"

#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/settlement_containment.hpp"

namespace street_pixels
{
namespace
{
m2::PointD RepresentativePoint(ExplorationArea const & area)
{
  for (auto const & ring : area.m_rings)
  {
    for (auto const & pt : ring)
      return pt;
  }
  return {};
}
}  // namespace

AreaCompletionCounts AggregateCityCompletion(SpaFile const & file, AreaCompletionCache const & cache,
                                             uint32_t settlementCompactIndex)
{
  SettlementContainmentIndex settlements(file.m_areas);
  return AggregateCityCompletion(file, cache, settlements, settlementCompactIndex);
}

AreaCompletionCounts AggregateCityCompletion(SpaFile const & file, AreaCompletionCache const & cache,
                                             SettlementContainmentIndex const & settlements,
                                             uint32_t settlementCompactIndex)
{
  AreaCompletionCounts out;
  out.m_compactIndex = settlementCompactIndex;

  auto const * settlement = FindAreaByCompactIndex(file, settlementCompactIndex);
  if (settlement == nullptr || settlement->m_role != AreaRole::Settlement)
    return out;

  out.m_osmId = StableOsmId(*settlement);

  auto add = [&](uint32_t compactIndex)
  {
    auto const counts = cache.Get(compactIndex);
    if (!counts)
      return;
    out.m_explored += counts->m_explored;
    out.m_total += counts->m_total;
  };

  add(settlementCompactIndex);

  for (auto const & area : file.m_areas)
  {
    if (!area.IsAssignable())
      continue;
    if (area.m_rings.empty())
      continue;
    m2::PointD const pt = RepresentativePoint(area);
    if (!settlements.SettlementContains(settlementCompactIndex, pt))
      continue;
    add(area.m_compactIndex);
  }
  return out;
}

void CityCompletionCache::Invalidate()
{
  m_valid = false;
  m_rows.clear();
}

CityCompletionCache CityCompletionCache::Build(SpaFile const & file, AreaCompletionCache const & areaCache)
{
  CityCompletionCache out;
  if (!areaCache.IsValid())
    return out;

  SettlementContainmentIndex settlements(file.m_areas);
  for (auto const & area : file.m_areas)
  {
    if (area.m_role != AreaRole::Settlement)
      continue;
    out.m_rows.push_back(
        AggregateCityCompletion(file, areaCache, settlements, area.m_compactIndex));
  }
  out.m_valid = true;
  return out;
}

std::optional<AreaCompletionCounts> CityCompletionCache::Get(uint32_t settlementCompactIndex) const
{
  if (!m_valid)
    return std::nullopt;
  for (auto const & row : m_rows)
  {
    if (row.m_compactIndex == settlementCompactIndex)
      return row;
  }
  return std::nullopt;
}

double CityCompletionCache::GetFraction(uint32_t settlementCompactIndex) const
{
  auto const counts = Get(settlementCompactIndex);
  if (!counts)
    return 0.0;
  return AreaCompletionFraction(*counts);
}
}  // namespace street_pixels
