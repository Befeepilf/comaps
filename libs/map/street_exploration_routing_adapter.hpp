#pragma once

#include "routing/street_exploration_for_routing.hpp"

class StreetPixelsManager;

class StreetExplorationRoutingAdapter final : public routing::IStreetExplorationWeights
{
public:
  explicit StreetExplorationRoutingAdapter(StreetPixelsManager & streetPixelsManager);

  double GetSegmentWeightMultiplier(routing::NumMwmIds const & numMwmIds, routing::NumMwmId mwmId,
                                    routing::Segment const & segment,
                                    routing::RoadGeometry const & road) const override;

private:
  StreetPixelsManager & m_streetPixelsManager;
};
