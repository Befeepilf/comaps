#include "routing/street_exploration_routing_analytics.hpp"

#include "platform/settings.hpp"

#include <sstream>

namespace routing
{
namespace
{
void IncrementCounter(std::string_view key)
{
  uint64_t value = 0;
  settings::TryGet(key, value);
  settings::Set(key, value + 1);
}
}  // namespace

void StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode mode)
{
  switch (mode)
  {
  case StreetExplorationRoutingMode::Prefer:
    IncrementCounter(kPreferUsedKey);
    break;
  case StreetExplorationRoutingMode::Avoid:
    IncrementCounter(kAvoidUsedKey);
    break;
  case StreetExplorationRoutingMode::Neither: break;
  }
}

void StreetExplorationRoutingAnalytics::RecordAvoidFallbackPrefer()
{
  IncrementCounter(kAvoidFallbackPreferKey);
}

StreetExplorationRoutingAnalyticsSnapshot StreetExplorationRoutingAnalytics::LoadSnapshot()
{
  StreetExplorationRoutingAnalyticsSnapshot snapshot;
  settings::TryGet(kPreferUsedKey, snapshot.m_preferUsed);
  settings::TryGet(kAvoidUsedKey, snapshot.m_avoidUsed);
  settings::TryGet(kAvoidFallbackPreferKey, snapshot.m_avoidFallbackPrefer);
  return snapshot;
}

std::array<std::pair<std::string_view, uint64_t>, 3> StreetExplorationRoutingAnalytics::SerializedSnapshot()
{
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = LoadSnapshot();
  return {{
      {kPreferUsedName, snapshot.m_preferUsed},
      {kAvoidUsedName, snapshot.m_avoidUsed},
      {kAvoidFallbackPreferName, snapshot.m_avoidFallbackPrefer},
  }};
}

void StreetExplorationRoutingAnalytics::ResetForTesting()
{
  settings::Delete(kPreferUsedKey);
  settings::Delete(kAvoidUsedKey);
  settings::Delete(kAvoidFallbackPreferKey);
}

std::string DebugPrint(StreetExplorationRoutingAnalyticsSnapshot const & snapshot)
{
  std::ostringstream oss;
  oss << "prefer-used=" << snapshot.m_preferUsed << " avoid-used=" << snapshot.m_avoidUsed
      << " avoid-fallback-prefer=" << snapshot.m_avoidFallbackPrefer;
  return oss.str();
}
}  // namespace routing
