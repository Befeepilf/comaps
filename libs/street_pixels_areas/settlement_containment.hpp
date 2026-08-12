#pragma once

#include "street_pixels_areas/areas_types.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace street_pixels
{
// Cached settlement PIP geometry for batch / repeated SelectSettlementContaining.
// Tie-break matches SelectSettlementContaining: smallest m_area, then OSM id,
// then compact index. Outside all settlements → nullptr.
class SettlementContainmentIndex
{
public:
  SettlementContainmentIndex() = default;
  explicit SettlementContainmentIndex(std::vector<ExplorationArea> const & areas);

  ExplorationArea const * Select(m2::PointD const & point) const;

  // Point-in-settlement for a specific compact index (city rollup). False if the
  // index is unknown or not a cached settlement.
  bool SettlementContains(uint32_t compactIndex, m2::PointD const & point) const;

  size_t Size() const { return m_entries.size(); }

private:
  struct Entry
  {
    ExplorationArea const * m_area = nullptr;
    std::vector<m2::RegionD> m_regions;
    m2::RectD m_bbox;
    double m_areaSize = std::numeric_limits<double>::max();
    uint64_t m_osmId = std::numeric_limits<uint64_t>::max();
    uint32_t m_compactIndex = 0;

    bool Contains(m2::PointD const & pt) const;
  };

  std::vector<Entry> m_entries;
  // Parallel to SpaFile::m_areas compact indices; size 0 when unused.
  std::vector<size_t> m_compactToEntry;
};
}  // namespace street_pixels
