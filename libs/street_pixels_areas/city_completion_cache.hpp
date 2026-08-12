#pragma once

#include "street_pixels_areas/area_completion_cache.hpp"
#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/settlement_containment.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace street_pixels
{
// City / settlement personal completion (SP-039): sum explored/total over the
// settlement row plus every assignable area whose geometry lies in that
// settlement. Counts are not averaged. Pixels remain exclusive (SPD-026).

AreaCompletionCounts AggregateCityCompletion(SpaFile const & file, AreaCompletionCache const & cache,
                                             uint32_t settlementCompactIndex);

AreaCompletionCounts AggregateCityCompletion(SpaFile const & file, AreaCompletionCache const & cache,
                                             SettlementContainmentIndex const & settlements,
                                             uint32_t settlementCompactIndex);

class CityCompletionCache
{
public:
  bool IsValid() const { return m_valid; }
  void Invalidate();

  // One row per Settlement-role area in the sidecar.
  static CityCompletionCache Build(SpaFile const & file, AreaCompletionCache const & areaCache);

  std::optional<AreaCompletionCounts> Get(uint32_t settlementCompactIndex) const;
  double GetFraction(uint32_t settlementCompactIndex) const;
  std::vector<AreaCompletionCounts> const & Rows() const { return m_rows; }

private:
  bool m_valid = false;
  std::vector<AreaCompletionCounts> m_rows;
};
}  // namespace street_pixels
