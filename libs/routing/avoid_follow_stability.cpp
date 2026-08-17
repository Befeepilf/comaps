#include "routing/avoid_follow_stability.hpp"

#include <atomic>

namespace routing
{
static std::atomic<bool> g_applyAvoidExclusion{true};

void AvoidFollowStabilityGate::SetApplyAvoidExclusion(bool apply)
{
  g_applyAvoidExclusion.store(apply, std::memory_order_relaxed);
}

bool AvoidFollowStabilityGate::IsAvoidExclusionApplied()
{
  return g_applyAvoidExclusion.load(std::memory_order_relaxed);
}
}  // namespace routing
