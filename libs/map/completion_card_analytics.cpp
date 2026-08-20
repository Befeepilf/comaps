#include "map/completion_card_analytics.hpp"

#include "platform/settings.hpp"

#include <sstream>

namespace street_pixels
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

void CompletionCardAnalytics::RecordGenerated()
{
  IncrementCounter(kCardGeneratedKey);
}

void CompletionCardAnalytics::RecordShareInitiated()
{
  IncrementCounter(kShareInitiatedKey);
}

CompletionCardAnalyticsSnapshot CompletionCardAnalytics::LoadSnapshot()
{
  CompletionCardAnalyticsSnapshot snapshot;
  settings::TryGet(kCardGeneratedKey, snapshot.m_generated);
  settings::TryGet(kShareInitiatedKey, snapshot.m_shareInitiated);
  return snapshot;
}

std::array<std::pair<std::string_view, uint64_t>, 2> CompletionCardAnalytics::SerializedSnapshot()
{
  CompletionCardAnalyticsSnapshot const snapshot = LoadSnapshot();
  return {{
      {kCardGeneratedName, snapshot.m_generated},
      {kShareInitiatedName, snapshot.m_shareInitiated},
  }};
}

void CompletionCardAnalytics::ResetForTesting()
{
  settings::Delete(kCardGeneratedKey);
  settings::Delete(kShareInitiatedKey);
}

std::string DebugPrint(CompletionCardAnalyticsSnapshot const & snapshot)
{
  std::ostringstream oss;
  oss << "card-generated=" << snapshot.m_generated << " share-initiated=" << snapshot.m_shareInitiated;
  return oss.str();
}
}  // namespace street_pixels
