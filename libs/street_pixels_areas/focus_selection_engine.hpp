#pragma once

#include <cstdint>
#include <optional>

namespace street_pixels
{
// Spec §12.5 focus behaviour. Pure decision function — no I/O.
// City-scale band is provisional until SP-039 polish; see IsCityScaleDrawScale.

inline constexpr int kCityScaleMaxDrawScale = 12;

inline bool IsCityScaleDrawScale(int drawScale)
{
  return drawScale > 0 && drawScale <= kCityScaleMaxDrawScale;
}

enum class FocusEvent : uint8_t
{
  // Rule 1: active recording or user-location driven update.
  RecordingOrUserLocation = 0,
  // Rule 2: user panned; map centre may take focus when not recording.
  MapPan = 1,
  // Rule 3: explicit area selection (tap wiring lands in SP-038).
  ExplicitSelect = 2,
  // Rule 4: recentre / follow-my-position returns to the user area.
  Recentre = 3,
  // Rule 5: zoom changed; city-scale summary when atCityScale.
  ZoomChanged = 4,
};

enum class FocusTargetKind : uint8_t
{
  None = 0,
  ExplorationArea = 1,
  CitySummary = 2,
};

struct FocusSelectionRequest
{
  FocusEvent m_event = FocusEvent::MapPan;
  bool m_recordingActive = false;
  bool m_atCityScale = false;
  std::optional<uint32_t> m_userAreaCompactIndex;
  std::optional<uint32_t> m_mapCentreAreaCompactIndex;
  std::optional<uint32_t> m_explicitAreaCompactIndex;
  std::optional<uint32_t> m_cityCompactIndex;
};

struct FocusSelectionDecision
{
  FocusTargetKind m_kind = FocusTargetKind::None;
  std::optional<uint32_t> m_compactIndex;
};

// Resolves the next focus target from a single §12.5 event.
// Recording vs pan (rules 1 vs 2): during active recording, user area wins.
FocusSelectionDecision SelectFocusedArea(FocusSelectionRequest const & request);

inline std::string DebugPrint(FocusTargetKind kind)
{
  switch (kind)
  {
  case FocusTargetKind::None: return "None";
  case FocusTargetKind::ExplorationArea: return "ExplorationArea";
  case FocusTargetKind::CitySummary: return "CitySummary";
  }
  return "UnknownFocusTargetKind";
}

inline std::string DebugPrint(FocusEvent event)
{
  switch (event)
  {
  case FocusEvent::RecordingOrUserLocation: return "RecordingOrUserLocation";
  case FocusEvent::MapPan: return "MapPan";
  case FocusEvent::ExplicitSelect: return "ExplicitSelect";
  case FocusEvent::Recentre: return "Recentre";
  case FocusEvent::ZoomChanged: return "ZoomChanged";
  }
  return "UnknownFocusEvent";
}
}  // namespace street_pixels
