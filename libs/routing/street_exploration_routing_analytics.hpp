#pragma once

#include "routing/routing_options.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace routing
{
enum class StreetExplorationRoutingAnalyticsCounter : uint8_t
{
  PreferUsed = 0,
  AvoidUsed = 1,
  AvoidFallbackPrefer = 2,
};

inline uint8_t constexpr kStreetExplorationRoutingAnalyticsCounterCount = 3;

struct StreetExplorationRoutingAnalyticsSnapshot
{
  uint64_t m_preferUsed = 0;
  uint64_t m_avoidUsed = 0;
  uint64_t m_avoidFallbackPrefer = 0;
};

class StreetExplorationRoutingAnalytics
{
public:
  static std::string_view constexpr kPreferUsedName = "prefer-used";
  static std::string_view constexpr kAvoidUsedName = "avoid-used";
  static std::string_view constexpr kAvoidFallbackPreferName = "avoid-fallback-prefer";

  static std::string_view constexpr kPreferUsedKey =
      "street_exploration_routing_analytics_prefer_used";
  static std::string_view constexpr kAvoidUsedKey =
      "street_exploration_routing_analytics_avoid_used";
  static std::string_view constexpr kAvoidFallbackPreferKey =
      "street_exploration_routing_analytics_avoid_fallback_prefer";

  static void RecordSuccessfulBuild(StreetExplorationRoutingMode mode);
  static void RecordAvoidFallbackPrefer();
  static StreetExplorationRoutingAnalyticsSnapshot LoadSnapshot();
  static std::array<std::pair<std::string_view, uint64_t>, 3> SerializedSnapshot();
  static void ResetForTesting();
};

std::string DebugPrint(StreetExplorationRoutingAnalyticsSnapshot const & snapshot);
}  // namespace routing
