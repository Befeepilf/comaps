#include "street_pixels_areas/subdivision_assigner.hpp"

#include "street_pixels_areas/bg_point.hpp"

#include "geometry/rect2d.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include <boost/geometry/index/rtree.hpp>
#include "std/boost_geometry.hpp"

namespace street_pixels
{
int SubdivisionPriorityRank(ExplorationArea const & area, CountryPolicy const & policy)
{
  if (area.m_role == AreaRole::PlaceBoundary)
    return static_cast<int>(policy.m_subdivisionAdminLevels.size());
  if (area.m_role != AreaRole::Subdivision)
    return std::numeric_limits<int>::max();

  auto const & levels = policy.m_subdivisionAdminLevels;
  auto const it = std::find(levels.begin(), levels.end(), static_cast<int>(area.m_adminLevel));
  if (it == levels.end())
    return std::numeric_limits<int>::max();
  return static_cast<int>(std::distance(levels.begin(), it));
}

namespace
{
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

struct CandidateScore
{
  int m_priorityRank = std::numeric_limits<int>::max();
  double m_area = std::numeric_limits<double>::max();
  uint64_t m_osmId = std::numeric_limits<uint64_t>::max();
  uint32_t m_compactIndex = 0;

  bool operator<(CandidateScore const & other) const
  {
    if (m_priorityRank != other.m_priorityRank)
      return m_priorityRank < other.m_priorityRank;
    if (m_area != other.m_area)
      return m_area < other.m_area;
    if (m_osmId != other.m_osmId)
      return m_osmId < other.m_osmId;
    return m_compactIndex < other.m_compactIndex;
  }
};

// Cached PIP geometry for one assignable area (avoids rebuilding RegionD per sample).
struct CachedAssignable
{
  ExplorationArea const * m_area = nullptr;
  std::vector<m2::RegionD> m_regions;
  m2::RectD m_bbox;
  int m_priorityRank = std::numeric_limits<int>::max();

  bool Contains(m2::PointD const & pt) const
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
};

using Box = bg::model::box<m2::PointD>;
using RtreeValue = std::pair<Box, size_t>;
using AssignRtree = bgi::rtree<RtreeValue, bgi::quadratic<16>>;

Box RectToBox(m2::RectD const & rect)
{
  return Box(m2::PointD(rect.minX(), rect.minY()), m2::PointD(rect.maxX(), rect.maxY()));
}

bool BuildCachedAssignables(std::vector<ExplorationArea> const & areas, CountryPolicy const & policy,
                            std::vector<CachedAssignable> & out, AssignRtree & tree)
{
  out.clear();
  out.reserve(areas.size());
  std::vector<RtreeValue> values;
  values.reserve(areas.size());

  for (auto const & area : areas)
  {
    if (!area.IsAssignable())
      continue;
    int const rank = SubdivisionPriorityRank(area, policy);
    if (rank == std::numeric_limits<int>::max())
      continue;

    CachedAssignable cached;
    cached.m_area = &area;
    cached.m_priorityRank = rank;
    cached.m_bbox.MakeEmpty();
    for (auto const & ring : area.m_rings)
    {
      if (ring.size() < 3)
        continue;
      m2::RegionD region(ring.begin(), ring.end());
      cached.m_bbox.Add(region.GetRect());
      cached.m_regions.push_back(std::move(region));
    }
    if (cached.m_regions.empty() || !cached.m_bbox.IsValid())
      continue;

    size_t const index = out.size();
    values.emplace_back(RectToBox(cached.m_bbox), index);
    out.push_back(std::move(cached));
  }

  tree = AssignRtree(values.begin(), values.end());
  return true;
}

uint32_t PickBestAmongCached(m2::PointD const & point, std::vector<CachedAssignable> const & cached,
                             std::vector<size_t> const & candidateIndices, uint32_t sentinel)
{
  bool found = false;
  CandidateScore best;
  for (size_t idx : candidateIndices)
  {
    auto const & entry = cached[idx];
    if (!entry.Contains(point))
      continue;

    CandidateScore score;
    score.m_priorityRank = entry.m_priorityRank;
    score.m_area = entry.m_area->m_area;
    score.m_osmId = entry.m_area->m_osmId;
    score.m_compactIndex = entry.m_area->m_compactIndex;

    if (!found || score < best)
    {
      best = score;
      found = true;
    }
  }
  return found ? best.m_compactIndex : sentinel;
}
}  // namespace

uint32_t AssignSubdivision(m2::PointD const & point, std::vector<ExplorationArea> const & areas,
                           CountryPolicy const & policy, uint32_t sentinel)
{
  if (!policy.m_configured)
    return sentinel;

  bool found = false;
  CandidateScore best;
  for (auto const & area : areas)
  {
    if (!area.IsAssignable())
      continue;
    if (!area.Contains(point))
      continue;

    CandidateScore score;
    score.m_priorityRank = SubdivisionPriorityRank(area, policy);
    if (score.m_priorityRank == std::numeric_limits<int>::max())
      continue;
    score.m_area = area.m_area;
    score.m_osmId = area.m_osmId;
    score.m_compactIndex = area.m_compactIndex;

    if (!found || score < best)
    {
      best = score;
      found = true;
    }
  }
  return found ? best.m_compactIndex : sentinel;
}

std::vector<uint32_t> BuildDenseAssignments(std::vector<m2::PointD> const & points,
                                            std::vector<ExplorationArea> const & areas,
                                            CountryPolicy const & policy, uint32_t sentinel)
{
  std::vector<uint32_t> out;
  out.reserve(points.size());
  if (!policy.m_configured)
  {
    out.assign(points.size(), sentinel);
    return out;
  }

  std::vector<CachedAssignable> cached;
  AssignRtree tree;
  BuildCachedAssignables(areas, policy, cached, tree);

  if (cached.empty())
  {
    out.assign(points.size(), sentinel);
    return out;
  }

  std::vector<RtreeValue> hits;
  std::vector<size_t> candidateIndices;
  hits.reserve(32);
  candidateIndices.reserve(32);

  for (auto const & pt : points)
  {
    hits.clear();
    candidateIndices.clear();
    tree.query(bgi::intersects(pt), std::back_inserter(hits));
    for (auto const & hit : hits)
      candidateIndices.push_back(hit.second);
    out.push_back(PickBestAmongCached(pt, cached, candidateIndices, sentinel));
  }
  return out;
}
}  // namespace street_pixels
