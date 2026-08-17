#include "testing/testing.hpp"

#include "routing/street_exploration_routing_analytics.hpp"

#include "platform/settings.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <string_view>

using namespace routing;

namespace
{
class StreetExplorationRoutingAnalyticsGuard
{
public:
  StreetExplorationRoutingAnalyticsGuard()
  {
    m_hadPreferUsed = settings::Get(StreetExplorationRoutingAnalytics::kPreferUsedKey, m_preferUsed);
    m_hadAvoidUsed = settings::Get(StreetExplorationRoutingAnalytics::kAvoidUsedKey, m_avoidUsed);
    m_hadAvoidFallbackPrefer =
        settings::Get(StreetExplorationRoutingAnalytics::kAvoidFallbackPreferKey, m_avoidFallbackPrefer);
    settings::Delete(StreetExplorationRoutingAnalytics::kPreferUsedKey);
    settings::Delete(StreetExplorationRoutingAnalytics::kAvoidUsedKey);
    settings::Delete(StreetExplorationRoutingAnalytics::kAvoidFallbackPreferKey);
    StreetExplorationRoutingAnalytics::ResetForTesting();
  }

  ~StreetExplorationRoutingAnalyticsGuard()
  {
    settings::Delete(StreetExplorationRoutingAnalytics::kPreferUsedKey);
    settings::Delete(StreetExplorationRoutingAnalytics::kAvoidUsedKey);
    settings::Delete(StreetExplorationRoutingAnalytics::kAvoidFallbackPreferKey);
    if (m_hadPreferUsed)
      settings::Set(StreetExplorationRoutingAnalytics::kPreferUsedKey, m_preferUsed);
    if (m_hadAvoidUsed)
      settings::Set(StreetExplorationRoutingAnalytics::kAvoidUsedKey, m_avoidUsed);
    if (m_hadAvoidFallbackPrefer)
      settings::Set(StreetExplorationRoutingAnalytics::kAvoidFallbackPreferKey, m_avoidFallbackPrefer);
  }

private:
  bool m_hadPreferUsed = false;
  bool m_hadAvoidUsed = false;
  bool m_hadAvoidFallbackPrefer = false;
  uint64_t m_preferUsed = 0;
  uint64_t m_avoidUsed = 0;
  uint64_t m_avoidFallbackPrefer = 0;
};

bool ContainsForbiddenLocationToken(std::string const & text)
{
  std::string const lower = strings::MakeLowerCase(text);
  std::string_view constexpr kForbidden[] = {"lat",      "lon",   "latitude", "longitude", "geometry", "polyline",
                                             "pixel",    "area",  "coord",    "mwm",       "country"};
  for (auto const token : kForbidden)
  {
    if (lower.find(token) != std::string::npos)
      return true;
  }
  return false;
}
}  // namespace

UNIT_TEST(StreetExplorationRoutingAnalytics_DefaultZero)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 0, ());
  TEST_EQUAL(DebugPrint(snapshot), std::string("prefer-used=0 avoid-used=0 avoid-fallback-prefer=0"), ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_RecordPreferUsed)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Prefer);
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Prefer);
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 2, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 0, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_RecordAvoidUsed)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Avoid);
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 1, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 0, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_NeitherDoesNotIncrement)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Neither);
  uint64_t preferUsed = 0;
  TEST(!settings::Get(StreetExplorationRoutingAnalytics::kPreferUsedKey, preferUsed), ());
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 0, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_RecordAvoidFallbackPrefer)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordAvoidFallbackPrefer();
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 1, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_FallbackIsNotPreferUsed)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordAvoidFallbackPrefer();
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Prefer);
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 1, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 1, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_PersistRoundTrip)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Prefer);
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Avoid);
  StreetExplorationRoutingAnalytics::RecordAvoidFallbackPrefer();
  uint64_t preferUsed = 0;
  uint64_t avoidUsed = 0;
  uint64_t avoidFallbackPrefer = 0;
  TEST(settings::Get(StreetExplorationRoutingAnalytics::kPreferUsedKey, preferUsed), ());
  TEST(settings::Get(StreetExplorationRoutingAnalytics::kAvoidUsedKey, avoidUsed), ());
  TEST(settings::Get(StreetExplorationRoutingAnalytics::kAvoidFallbackPreferKey, avoidFallbackPrefer), ());
  TEST_EQUAL(preferUsed, 1, ());
  TEST_EQUAL(avoidUsed, 1, ());
  TEST_EQUAL(avoidFallbackPrefer, 1, ());
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, preferUsed, ());
  TEST_EQUAL(snapshot.m_avoidUsed, avoidUsed, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, avoidFallbackPrefer, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_ResetIsolatesTests)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Prefer);
  TEST_EQUAL(StreetExplorationRoutingAnalytics::LoadSnapshot().m_preferUsed, 1, ());
  StreetExplorationRoutingAnalytics::ResetForTesting();
  StreetExplorationRoutingAnalyticsSnapshot const snapshot = StreetExplorationRoutingAnalytics::LoadSnapshot();
  TEST_EQUAL(snapshot.m_preferUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidUsed, 0, ());
  TEST_EQUAL(snapshot.m_avoidFallbackPrefer, 0, ());
}

UNIT_TEST(StreetExplorationRoutingAnalytics_SnapshotHasNoLocationKeys)
{
  StreetExplorationRoutingAnalyticsGuard guard;
  StreetExplorationRoutingAnalytics::RecordSuccessfulBuild(StreetExplorationRoutingMode::Prefer);
  auto const serialized = StreetExplorationRoutingAnalytics::SerializedSnapshot();
  TEST_EQUAL(serialized.size(), 3, ());
  TEST_EQUAL(std::string(serialized[0].first), std::string(StreetExplorationRoutingAnalytics::kPreferUsedName), ());
  TEST_EQUAL(std::string(serialized[1].first), std::string(StreetExplorationRoutingAnalytics::kAvoidUsedName), ());
  TEST_EQUAL(std::string(serialized[2].first), std::string(StreetExplorationRoutingAnalytics::kAvoidFallbackPreferName),
             ());
  for (auto const & entry : serialized)
  {
    std::string const name(entry.first);
    TEST_EQUAL(strings::MakeLowerCase(name), name, ());
    TEST(!ContainsForbiddenLocationToken(name), (name));
  }
  std::string const debug = DebugPrint(StreetExplorationRoutingAnalytics::LoadSnapshot());
  TEST_EQUAL(strings::MakeLowerCase(debug), debug, ());
  TEST(!ContainsForbiddenLocationToken(debug), (debug));
}
