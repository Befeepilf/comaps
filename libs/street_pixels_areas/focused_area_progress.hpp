#pragma once

#include <cstdint>
#include <string>

namespace street_pixels
{
// Snapshot for the primary progress badge (SP-035). Focus selection rules are
// SP-036; until then a temporary map-centre stub may set focus.
struct FocusedAreaProgress
{
  bool m_hasFocus = false;
  bool m_fractionValid = false;
  uint32_t m_compactIndex = 0;
  uint64_t m_osmId = 0;
  std::string m_displayName;
  double m_fraction = 0.0;
};
}  // namespace street_pixels
