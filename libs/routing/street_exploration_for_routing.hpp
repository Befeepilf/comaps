#pragma once

#include "routing_common/num_mwm_id.hpp"

namespace routing
{
class RoadGeometry;
class Segment;

class IStreetExplorationWeights
{
public:
  virtual ~IStreetExplorationWeights() = default;

  virtual double GetSegmentWeightMultiplier(NumMwmIds const & numMwmIds, NumMwmId mwmId, Segment const & segment,
                                            RoadGeometry const & road) const = 0;

  virtual bool IsAvoidExclusionActive() const = 0;
  virtual bool IsSegmentExcluded(NumMwmIds const & numMwmIds, NumMwmId mwmId, Segment const & segment,
                                 RoadGeometry const & road) const = 0;
};
}  // namespace routing
