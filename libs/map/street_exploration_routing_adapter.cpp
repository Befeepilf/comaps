#include "map/street_exploration_routing_adapter.hpp"

#include "routing/avoid_follow_stability.hpp"
#include "map/street_pixels_manager.hpp"

#include "routing/routing_options.hpp"
#include "routing/segment.hpp"

StreetExplorationRoutingAdapter::StreetExplorationRoutingAdapter(StreetPixelsManager & streetPixelsManager)
  : m_streetPixelsManager(streetPixelsManager)
{}

double StreetExplorationRoutingAdapter::GetSegmentWeightMultiplier(routing::NumMwmIds const & numMwmIds,
                                                                   routing::NumMwmId mwmId,
                                                                   routing::Segment const & segment,
                                                                   routing::RoadGeometry const & road) const
{
  if (mwmId == routing::kFakeNumMwmId || !numMwmIds.ContainsFileForMwm(mwmId))
    return 1.0;

  std::string const & country = numMwmIds.GetFile(mwmId).GetName();
  return m_streetPixelsManager.GetSegmentExplorationWeightMultiplier(country, segment, road);
}

bool StreetExplorationRoutingAdapter::IsAvoidExclusionActive() const
{
  if (!routing::StreetExplorationRoutingOptions::LoadFromSettings().IsAvoidEnabled())
    return false;
  return routing::AvoidFollowStabilityGate::IsAvoidExclusionApplied();
}

bool StreetExplorationRoutingAdapter::IsSegmentExcluded(routing::NumMwmIds const & numMwmIds,
                                                        routing::NumMwmId mwmId, routing::Segment const & segment,
                                                        routing::RoadGeometry const & road) const
{
  if (mwmId == routing::kFakeNumMwmId || !numMwmIds.ContainsFileForMwm(mwmId))
    return false;

  std::string const & country = numMwmIds.GetFile(mwmId).GetName();
  return m_streetPixelsManager.IsSegmentExcludedForAvoid(country, segment, road);
}
