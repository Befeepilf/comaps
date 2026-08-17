#pragma once

namespace routing
{
class AvoidFollowStabilityGate
{
  AvoidFollowStabilityGate() = delete;

public:
  static void SetApplyAvoidExclusion(bool apply);
  static bool IsAvoidExclusionApplied();
};
}  // namespace routing
