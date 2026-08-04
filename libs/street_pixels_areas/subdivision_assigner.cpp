#include "street_pixels_areas/subdivision_assigner.hpp"

#include <algorithm>
#include <limits>

namespace street_pixels
{
namespace
{
int SubdivisionPriorityRank(ExplorationArea const & area, CountryPolicy const & policy)
{
  if (area.m_role == AreaRole::PlaceBoundary)
  {
    // Place boundaries are a sparse supplement after all admin subdivision levels.
    return static_cast<int>(policy.m_subdivisionAdminLevels.size());
  }
  if (area.m_role != AreaRole::Subdivision)
    return std::numeric_limits<int>::max();

  auto const & levels = policy.m_subdivisionAdminLevels;
  auto const it = std::find(levels.begin(), levels.end(), static_cast<int>(area.m_adminLevel));
  if (it == levels.end())
    return std::numeric_limits<int>::max();
  return static_cast<int>(std::distance(levels.begin(), it));
}

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
  for (auto const & pt : points)
    out.push_back(AssignSubdivision(pt, areas, policy, sentinel));
  return out;
}
}  // namespace street_pixels
