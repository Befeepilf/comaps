#pragma once

#include <cstdint>
#include <string>

namespace street_pixels
{
// Snapshot for the primary progress badge (SP-035 / SP-036).
struct FocusedAreaProgress
{
  bool m_hasFocus = false;
  bool m_fractionValid = false;
  // True when §12.5 rule 5 selected city-summary mode (rollup polish → SP-039).
  bool m_citySummary = false;
  uint32_t m_compactIndex = 0;
  uint64_t m_osmId = 0;
  std::string m_displayName;
  double m_fraction = 0.0;
};
}  // namespace street_pixels
