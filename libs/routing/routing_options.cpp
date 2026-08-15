#include "routing/routing_options.hpp"

#include "platform/settings.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/string_utils.hpp"

#include <sstream>
#include <string_view>

namespace routing
{
using namespace std;

// RoutingOptions -------------------------------------------------------------------------------------

std::string_view constexpr kAvoidRoutingOptionSettingsForCar = "avoid_routing_options_car";
std::string_view constexpr kAvoidRoutingOptionSettingsForBicycle = "avoid_routing_options_bicycle";
std::string_view constexpr kAvoidRoutingOptionSettingsForPedestrian = "avoid_routing_options_pedestrian";

RoutingOptions RoutingOptions::LoadOptionsFromSettings(VehicleType type)
{
  uint32_t mode = 0;
  std::string_view settingsName;
  OptionType mask = -1;
  switch (type)
  {
  case VehicleType::Car:
    settingsName = kAvoidRoutingOptionSettingsForCar;
    mask = kVehicleOptionsMask;
    break;
  case VehicleType::Bicycle:
    settingsName = kAvoidRoutingOptionSettingsForBicycle;
    mask = kBicycleOptionsMask;
    break;
  case VehicleType::Pedestrian:
  case VehicleType::Transit:
    settingsName = kAvoidRoutingOptionSettingsForPedestrian;
    mask = kPedestrianOptionsMask;
    break;
  default: UNREACHABLE();
  }

  if (!settings::Get(settingsName, mode))
    if (!settings::Get(kAvoidRoutingOptionSettingsForCar,
                  mode))  // use car to init other modes when the new settings get rolled out
      mode = 0;

  return RoutingOptions(base::checked_cast<OptionType>(mode) & mask, type);
}

// static
void RoutingOptions::SaveOptionsToSettings(RoutingOptions options)
{
  std::string_view settingsName;
  switch (options.m_vehicle)
  {
  case VehicleType::Car: settingsName = kAvoidRoutingOptionSettingsForCar; break;
  case VehicleType::Bicycle: settingsName = kAvoidRoutingOptionSettingsForBicycle; break;
  case VehicleType::Transit:
  case VehicleType::Pedestrian: settingsName = kAvoidRoutingOptionSettingsForPedestrian; break;
  default: UNREACHABLE();
  }

  settings::Set(settingsName, strings::to_string(static_cast<int32_t>(options.GetOptions())));
}

void RoutingOptions::Add(RoutingOptions::Option type)
{
  if (type == RoutingOptions::Option::Paved)
    Remove(RoutingOptions::Option::Dirty);
  else if (type == RoutingOptions::Option::Dirty)
    Remove(RoutingOptions::Option::Paved);
  m_options |= static_cast<OptionType>(type);
}

void RoutingOptions::Remove(RoutingOptions::Option type)
{
  m_options &= ~static_cast<OptionType>(type);
}

bool RoutingOptions::Has(RoutingOptions::Option type) const
{
  return (m_options & static_cast<OptionType>(type)) != 0;
}

// TrailRoutingOptions -------------------------------------------------------------------------------------

TrailRoutingOptions TrailRoutingOptions::LoadFromSettings()
{
  TrailRoutingOptions settings;

  std::string preferTrailsStr;
  if (!settings::Get("trail_routing_enabled", preferTrailsStr))
    settings.m_preferTrails = false;
  else
    settings.m_preferTrails = (preferTrailsStr == "true");

  std::string preferenceStr;
  if (!settings::Get("trail_preference", preferenceStr))
    settings.m_trailPreference = kDefaultTrailPreference;
  else
  {
    double preference = kDefaultTrailPreference;
    if (!strings::to_double(preferenceStr, preference))
      preference = kDefaultTrailPreference;
    settings.m_trailPreference = std::max(kMinTrailPreference, std::min(kMaxTrailPreference, preference));
  }

  return settings;
}

void TrailRoutingOptions::SaveToSettings(TrailRoutingOptions const & settings)
{
  settings::Set("trail_routing_enabled", settings.m_preferTrails);
  settings::Set("trail_preference", settings.m_trailPreference);
}

std::string_view constexpr kStreetExplorationRoutingMode = "street_exploration_routing_mode";
std::string_view constexpr kStreetExplorationRoutingEnabled = "street_exploration_routing_enabled";
std::string_view constexpr kStreetExplorationRoutingStrength = "street_exploration_routing_strength";

std::string_view constexpr kStreetExplorationModeNeither = "neither";
std::string_view constexpr kStreetExplorationModePrefer = "prefer";
std::string_view constexpr kStreetExplorationModeAvoid = "avoid";

StreetExplorationRoutingOptions StreetExplorationRoutingOptions::LoadFromSettings()
{
  StreetExplorationRoutingOptions settings;

  std::string strengthStr;
  if (!settings::Get(kStreetExplorationRoutingStrength, strengthStr))
    settings.m_strength = kDefaultStrength;
  else
  {
    double strength = kDefaultStrength;
    if (!strings::to_double(strengthStr, strength))
      strength = kDefaultStrength;
    settings.m_strength = std::max(kMinStrength, std::min(kMaxStrength, strength));
  }

  std::string modeStr;
  if (settings::Get(kStreetExplorationRoutingMode, modeStr))
  {
    if (modeStr == kStreetExplorationModePrefer)
      settings.m_mode = StreetExplorationRoutingMode::Prefer;
    else if (modeStr == kStreetExplorationModeAvoid)
      settings.m_mode = StreetExplorationRoutingMode::Avoid;
    else
      settings.m_mode = StreetExplorationRoutingMode::Neither;
  }
  else
  {
    std::string enabledStr;
    if (settings::Get(kStreetExplorationRoutingEnabled, enabledStr) && enabledStr == "true")
      settings.m_mode = StreetExplorationRoutingMode::Prefer;
    else
      settings.m_mode = StreetExplorationRoutingMode::Neither;
  }

  return settings;
}

void StreetExplorationRoutingOptions::SaveToSettings(StreetExplorationRoutingOptions const & settings)
{
  std::string_view modeStr = kStreetExplorationModeNeither;
  if (settings.m_mode == StreetExplorationRoutingMode::Prefer)
    modeStr = kStreetExplorationModePrefer;
  else if (settings.m_mode == StreetExplorationRoutingMode::Avoid)
    modeStr = kStreetExplorationModeAvoid;

  settings::Set(kStreetExplorationRoutingMode, std::string(modeStr));
  settings::Set(kStreetExplorationRoutingEnabled, settings.m_mode == StreetExplorationRoutingMode::Prefer);
  settings::Set(kStreetExplorationRoutingStrength, settings.m_strength);
}

// RoutingOptionsClassifier ---------------------------------------------------------------------------

RoutingOptionsClassifier::RoutingOptionsClassifier()
{
  Classificator const & c = classif();

  pair<vector<string>, RoutingOptions::Option> const types[] = {
      {{"highway", "motorway"}, RoutingOptions::Option::Motorway},

      {{"hwtag", "toll"}, RoutingOptions::Option::Toll},

      {{"route", "ferry"}, RoutingOptions::Option::Ferry},

      {{"highway", "track"}, RoutingOptions::Option::Dirty},
      {{"highway", "road"}, RoutingOptions::Option::Dirty},
      {{"psurface", "unpaved_bad"}, RoutingOptions::Option::Dirty},
      {{"psurface", "unpaved_good"}, RoutingOptions::Option::Dirty},
      {{"highway", "steps"}, RoutingOptions::Option::Steps},
      {{"highway", "ladder"}, RoutingOptions::Option::Steps},
      {{"psurface", "paved_good"}, RoutingOptions::Option::Paved},
      {{"psurface", "paved_bad"}, RoutingOptions::Option::Paved}};

  m_data.reserve(std::size(types));
  for (auto const & data : types)
    m_data.insert({c.GetTypeByPath(data.first), data.second});
}

optional<RoutingOptions::Option> RoutingOptionsClassifier::Get(uint32_t type) const
{
  ftype::TruncValue(type, 2);  // in case of highway-motorway-bridge

  auto const it = m_data.find(type);
  if (it != m_data.cend())
    return it->second;
  return {};
}

RoutingOptionsClassifier const & RoutingOptionsClassifier::Instance()
{
  static RoutingOptionsClassifier instance;
  return instance;
}

RoutingOptions::Option ChooseMainRoutingOption(RoutingOptions options, bool isCarRouter)
{
  if (isCarRouter && options.Has(RoutingOptions::Option::Toll))
    return RoutingOptions::Option::Toll;

  if (options.Has(RoutingOptions::Option::Ferry))
    return RoutingOptions::Option::Ferry;

  if (options.Has(RoutingOptions::Option::Dirty))
    return RoutingOptions::Option::Dirty;

  if (options.Has(RoutingOptions::Option::Motorway))
    return RoutingOptions::Option::Motorway;

  if (options.Has(RoutingOptions::Option::Steps))
    return RoutingOptions::Option::Steps;

  if (options.Has(RoutingOptions::Option::Paved))
    return RoutingOptions::Option::Paved;

  return RoutingOptions::Option::Usual;
}

string DebugPrint(RoutingOptions const & routingOptions)
{
  ostringstream ss;
  ss << "RoutingOptions: {";

  bool wasAppended = false;
  auto const append = [&](RoutingOptions::Option road)
  {
    if (routingOptions.Has(road))
    {
      wasAppended = true;
      ss << " | " << DebugPrint(road);
    }
  };

  append(RoutingOptions::Option::Usual);
  append(RoutingOptions::Option::Toll);
  append(RoutingOptions::Option::Motorway);
  append(RoutingOptions::Option::Ferry);
  append(RoutingOptions::Option::Dirty);
  append(RoutingOptions::Option::Steps);
  append(RoutingOptions::Option::Paved);

  if (wasAppended)
    ss << " | ";

  ss << "}";

  return ss.str();
}

string DebugPrint(RoutingOptions::Option type)
{
  switch (type)
  {
  case RoutingOptions::Option::Toll: return "toll";
  case RoutingOptions::Option::Motorway: return "motorway";
  case RoutingOptions::Option::Ferry: return "ferry";
  case RoutingOptions::Option::Dirty: return "dirty";
  case RoutingOptions::Option::Steps: return "steps";
  case RoutingOptions::Option::Paved: return "paved";
  case RoutingOptions::Option::Usual: return "usual";
  case RoutingOptions::Option::Max: return "max";
  }

  UNREACHABLE();
}

string DebugPrint(StreetExplorationRoutingMode mode)
{
  switch (mode)
  {
  case StreetExplorationRoutingMode::Neither: return "neither";
  case StreetExplorationRoutingMode::Prefer: return "prefer";
  case StreetExplorationRoutingMode::Avoid: return "avoid";
  }

  UNREACHABLE();
}

RoutingOptionSetter::RoutingOptionSetter(RoutingOptions::OptionType roadsMask, VehicleType type)
{
  m_saved = RoutingOptions::LoadOptionsFromSettings(type);
  RoutingOptions::SaveOptionsToSettings(RoutingOptions(roadsMask, type));
}

RoutingOptionSetter::~RoutingOptionSetter()
{
  RoutingOptions::SaveOptionsToSettings(m_saved);
}

}  // namespace routing
