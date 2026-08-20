#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace street_pixels
{
struct CompletionCardAnalyticsSnapshot
{
  uint64_t m_generated = 0;
  uint64_t m_shareInitiated = 0;
};

class CompletionCardAnalytics
{
public:
  static std::string_view constexpr kCardGeneratedKey = "Explore.CardGenerated";
  static std::string_view constexpr kShareInitiatedKey = "Explore.ShareInitiated";
  static std::string_view constexpr kCardGeneratedName = "card-generated";
  static std::string_view constexpr kShareInitiatedName = "share-initiated";

  static void RecordGenerated();
  static void RecordShareInitiated();
  static CompletionCardAnalyticsSnapshot LoadSnapshot();
  static std::array<std::pair<std::string_view, uint64_t>, 2> SerializedSnapshot();
  static void ResetForTesting();
};

std::string DebugPrint(CompletionCardAnalyticsSnapshot const & snapshot);
}  // namespace street_pixels
