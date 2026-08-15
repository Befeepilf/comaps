#include "testing/testing.hpp"

#include "routing/routing_options.hpp"

#include "platform/settings.hpp"

#include <string>
#include <string_view>

using namespace routing;

namespace
{
std::string_view constexpr kModeKey = "street_exploration_routing_mode";
std::string_view constexpr kEnabledKey = "street_exploration_routing_enabled";
std::string_view constexpr kStrengthKey = "street_exploration_routing_strength";

class StreetExplorationRoutingOptionsGuard
{
public:
  StreetExplorationRoutingOptionsGuard()
  {
    m_hadMode = settings::Get(kModeKey, m_mode);
    m_hadEnabled = settings::Get(kEnabledKey, m_enabled);
    m_hadStrength = settings::Get(kStrengthKey, m_strength);
    settings::Delete(kModeKey);
    settings::Delete(kEnabledKey);
    settings::Delete(kStrengthKey);
  }

  ~StreetExplorationRoutingOptionsGuard()
  {
    settings::Delete(kModeKey);
    settings::Delete(kEnabledKey);
    settings::Delete(kStrengthKey);
    if (m_hadMode)
      settings::Set(kModeKey, m_mode);
    if (m_hadEnabled)
      settings::Set(kEnabledKey, m_enabled);
    if (m_hadStrength)
      settings::Set(kStrengthKey, m_strength);
  }

private:
  bool m_hadMode = false;
  bool m_hadEnabled = false;
  bool m_hadStrength = false;
  std::string m_mode;
  std::string m_enabled;
  std::string m_strength;
};

std::string ReadEnabledKey()
{
  std::string enabled;
  TEST(settings::Get(kEnabledKey, enabled), ());
  return enabled;
}
}  // namespace

UNIT_TEST(StreetExplorationRoutingOptions_DefaultNeither)
{
  StreetExplorationRoutingOptionsGuard guard;
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Neither, ());
  TEST_EQUAL(loaded.m_strength, StreetExplorationRoutingOptions::kDefaultStrength, ());
  TEST(!loaded.IsPreferEnabled(), ());
  TEST(!loaded.IsAvoidEnabled(), ());
}

UNIT_TEST(StreetExplorationRoutingOptions_MigrateEnabledTrueToPrefer)
{
  StreetExplorationRoutingOptionsGuard guard;
  settings::Set(kEnabledKey, std::string("true"));
  settings::Set(kStrengthKey, std::string("75"));
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Prefer, ());
  TEST_EQUAL(loaded.m_strength, 75.0, ());
  TEST(loaded.IsPreferEnabled(), ());
}

UNIT_TEST(StreetExplorationRoutingOptions_MigrateEnabledFalseToNeither)
{
  StreetExplorationRoutingOptionsGuard guard;
  settings::Set(kEnabledKey, std::string("false"));
  settings::Set(kStrengthKey, std::string("40"));
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Neither, ());
  TEST_EQUAL(loaded.m_strength, 40.0, ());
}

UNIT_TEST(StreetExplorationRoutingOptions_MigrateEnabledMissingToNeither)
{
  StreetExplorationRoutingOptionsGuard guard;
  settings::Set(kStrengthKey, std::string("33"));
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Neither, ());
  TEST_EQUAL(loaded.m_strength, 33.0, ());
}

UNIT_TEST(StreetExplorationRoutingOptions_PersistPreferRoundTrip)
{
  StreetExplorationRoutingOptionsGuard guard;
  StreetExplorationRoutingOptions saved;
  saved.m_mode = StreetExplorationRoutingMode::Prefer;
  saved.m_strength = 42.0;
  StreetExplorationRoutingOptions::SaveToSettings(saved);
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Prefer, ());
  TEST_EQUAL(loaded.m_strength, 42.0, ());
  TEST_EQUAL(ReadEnabledKey(), std::string("true"), ());
}

UNIT_TEST(StreetExplorationRoutingOptions_PersistAvoidRoundTrip)
{
  StreetExplorationRoutingOptionsGuard guard;
  StreetExplorationRoutingOptions saved;
  saved.m_mode = StreetExplorationRoutingMode::Avoid;
  saved.m_strength = 10.0;
  StreetExplorationRoutingOptions::SaveToSettings(saved);
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Avoid, ());
  TEST_EQUAL(loaded.m_strength, 10.0, ());
  TEST(loaded.IsAvoidEnabled(), ());
  TEST(!loaded.IsPreferEnabled(), ());
  TEST_EQUAL(ReadEnabledKey(), std::string("false"), ());
}

UNIT_TEST(StreetExplorationRoutingOptions_PersistNeitherRoundTrip)
{
  StreetExplorationRoutingOptionsGuard guard;
  StreetExplorationRoutingOptions saved;
  saved.m_mode = StreetExplorationRoutingMode::Neither;
  StreetExplorationRoutingOptions::SaveToSettings(saved);
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Neither, ());
  TEST_EQUAL(ReadEnabledKey(), std::string("false"), ());
}

UNIT_TEST(StreetExplorationRoutingOptions_ModeKeyWinsOverEnabled)
{
  StreetExplorationRoutingOptionsGuard guard;
  settings::Set(kModeKey, std::string("avoid"));
  settings::Set(kEnabledKey, std::string("true"));
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Avoid, ());
}

UNIT_TEST(StreetExplorationRoutingOptions_InvalidModeIsNeither)
{
  StreetExplorationRoutingOptionsGuard guard;
  settings::Set(kModeKey, std::string("nope"));
  StreetExplorationRoutingOptions const loaded = StreetExplorationRoutingOptions::LoadFromSettings();
  TEST_EQUAL(loaded.m_mode, StreetExplorationRoutingMode::Neither, ());
}

UNIT_TEST(StreetExplorationRoutingOptions_StrengthClamp)
{
  StreetExplorationRoutingOptionsGuard guard;
  StreetExplorationRoutingOptions high;
  high.m_mode = StreetExplorationRoutingMode::Prefer;
  high.m_strength = 999.0;
  StreetExplorationRoutingOptions::SaveToSettings(high);
  TEST_EQUAL(StreetExplorationRoutingOptions::LoadFromSettings().m_strength, 100.0, ());

  StreetExplorationRoutingOptions low;
  low.m_mode = StreetExplorationRoutingMode::Prefer;
  low.m_strength = -1.0;
  StreetExplorationRoutingOptions::SaveToSettings(low);
  TEST_EQUAL(StreetExplorationRoutingOptions::LoadFromSettings().m_strength, 0.0, ());
}
