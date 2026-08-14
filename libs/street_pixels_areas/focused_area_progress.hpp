#pragma once

#include <cstdint>
#include <string>

namespace street_pixels
{
// Snapshot for the primary progress badge (SP-035 / SP-036 / SP-040).
struct FocusedAreaProgress
{
  bool m_hasFocus = false;
  bool m_fractionValid = false;
  // True when §12.5 rule 5 selected city-summary mode (rollup → SP-039).
  bool m_citySummary = false;
  // True when focused area personal completion is 100% (§18.6).
  bool m_areaCompleted = false;
  // True after focus resolved to no exploration area (§31). Never invent a name.
  bool m_noExplorationArea = false;
  uint32_t m_compactIndex = 0;
  uint64_t m_osmId = 0;
  std::string m_displayName;
  double m_fraction = 0.0;
};

inline bool operator==(FocusedAreaProgress const & lhs, FocusedAreaProgress const & rhs)
{
  return lhs.m_hasFocus == rhs.m_hasFocus && lhs.m_fractionValid == rhs.m_fractionValid &&
         lhs.m_citySummary == rhs.m_citySummary && lhs.m_areaCompleted == rhs.m_areaCompleted &&
         lhs.m_noExplorationArea == rhs.m_noExplorationArea && lhs.m_compactIndex == rhs.m_compactIndex &&
         lhs.m_osmId == rhs.m_osmId && lhs.m_displayName == rhs.m_displayName && lhs.m_fraction == rhs.m_fraction;
}

inline bool operator!=(FocusedAreaProgress const & lhs, FocusedAreaProgress const & rhs) { return !(lhs == rhs); }
}  // namespace street_pixels
