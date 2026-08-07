#include "street_pixels_areas/focus_selection_engine.hpp"

namespace street_pixels
{
namespace
{
FocusSelectionDecision AreaOrNone(std::optional<uint32_t> const & compactIndex)
{
  if (!compactIndex.has_value())
    return {};
  FocusSelectionDecision d;
  d.m_kind = FocusTargetKind::ExplorationArea;
  d.m_compactIndex = compactIndex;
  return d;
}

FocusSelectionDecision CityOrNone(std::optional<uint32_t> const & compactIndex)
{
  if (!compactIndex.has_value())
    return {};
  FocusSelectionDecision d;
  d.m_kind = FocusTargetKind::CitySummary;
  d.m_compactIndex = compactIndex;
  return d;
}
}  // namespace

FocusSelectionDecision SelectFocusedArea(FocusSelectionRequest const & request)
{
  // Rule 3 + §12.3: explicit selection focuses the tapped area even at city scale
  // (exact percentage after selection). City summary otherwise.
  if (request.m_event == FocusEvent::ExplicitSelect)
    return AreaOrNone(request.m_explicitAreaCompactIndex);

  // Rule 5: zooming to city scale changes the summary badge to city completion.
  if (request.m_atCityScale)
    return CityOrNone(request.m_cityCompactIndex);

  if (request.m_event == FocusEvent::RecordingOrUserLocation || request.m_event == FocusEvent::Recentre)
  {
    // Rules 1 and 4: focus follows the user's current area.
    return AreaOrNone(request.m_userAreaCompactIndex);
  }

  if (request.m_event == FocusEvent::MapPan)
  {
    // Rule 2: panning may focus the area under the map centre.
    // When recording is active, rule 1 wins (user area).
    if (request.m_recordingActive)
      return AreaOrNone(request.m_userAreaCompactIndex);
    return AreaOrNone(request.m_mapCentreAreaCompactIndex);
  }

  // ZoomChanged after leaving city scale: prefer map centre.
  return AreaOrNone(request.m_mapCentreAreaCompactIndex);
}
}  // namespace street_pixels
