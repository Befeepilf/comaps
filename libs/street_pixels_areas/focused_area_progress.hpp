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
}  // namespace street_pixels
