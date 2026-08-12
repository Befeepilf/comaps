#include "street_pixels_areas/settlement_containment.hpp"

#include <limits>

namespace street_pixels
{
namespace
{
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

bool SettlementContainmentIndex::Entry::Contains(m2::PointD const & pt) const
{
  if (!m_bbox.IsPointInside(pt))
    return false;
  for (auto const & region : m_regions)
  {
    if (region.Contains(pt))
      return true;
  }
  return false;
}

SettlementContainmentIndex::SettlementContainmentIndex(std::vector<ExplorationArea> const & areas)
{
  m_entries.reserve(areas.size());
  m_compactToEntry.assign(areas.size(), std::numeric_limits<size_t>::max());

  for (auto const & area : areas)
  {
    if (area.m_role != AreaRole::Settlement)
      continue;

    Entry entry;
    entry.m_area = &area;
    entry.m_areaSize = area.m_area;
    entry.m_osmId = area.m_osmId;
    entry.m_compactIndex = area.m_compactIndex;
    entry.m_bbox.MakeEmpty();
    for (auto const & ring : area.m_rings)
    {
      if (ring.size() < 3)
        continue;
      m2::RegionD region(ring.begin(), ring.end());
      entry.m_bbox.Add(region.GetRect());
      entry.m_regions.push_back(std::move(region));
    }
    if (entry.m_regions.empty() || !entry.m_bbox.IsValid())
      continue;

    size_t const idx = m_entries.size();
    if (area.m_compactIndex < m_compactToEntry.size())
      m_compactToEntry[area.m_compactIndex] = idx;
    m_entries.push_back(std::move(entry));
  }
}

ExplorationArea const * SettlementContainmentIndex::Select(m2::PointD const & point) const
{
  bool found = false;
  SettlementScore best;
  ExplorationArea const * bestArea = nullptr;

  for (auto const & entry : m_entries)
  {
    if (!entry.Contains(point))
      continue;

    SettlementScore score;
    score.m_area = entry.m_areaSize;
    score.m_osmId = entry.m_osmId;
    score.m_compactIndex = entry.m_compactIndex;
    if (!found || score < best)
    {
      best = score;
      bestArea = entry.m_area;
      found = true;
    }
  }
  return bestArea;
}

bool SettlementContainmentIndex::SettlementContains(uint32_t compactIndex, m2::PointD const & point) const
{
  if (compactIndex >= m_compactToEntry.size())
    return false;
  size_t const idx = m_compactToEntry[compactIndex];
  if (idx >= m_entries.size())
    return false;
  return m_entries[idx].Contains(point);
}
}  // namespace street_pixels
