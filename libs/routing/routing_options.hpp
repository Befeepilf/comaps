#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "3party/skarupke/flat_hash_map.hpp"
#include "boost/container_hash/hash.hpp"
#include "routing/vehicle_mask.hpp"

namespace routing
{
class RoutingOptions
{
public:
  enum Option : uint8_t
  {
    Usual = 1u << 0,
    Toll = 1u << 1,
    Motorway = 1u << 2,
    Ferry = 1u << 3,
    Dirty = 1u << 4,
    Steps = 1u << 5,
    Paved = 1u << 6,

    Max = (1u << 6) + 1
  };
  
  using OptionType = std::underlying_type_t<Option>;

  static constexpr OptionType kPedestrianOptionsMask = Ferry + Dirty + Steps + Paved;
  static constexpr OptionType kBicycleOptionsMask = Ferry + Dirty + Steps + Paved;
  static constexpr OptionType kVehicleOptionsMask = Toll + Motorway + Ferry + Dirty + Paved;

  RoutingOptions() = default;
  explicit RoutingOptions(OptionType mask, routing::VehicleType type) : m_options(mask), m_vehicle(type) {}

  static RoutingOptions LoadOptionsFromSettings(VehicleType type);
  static void SaveOptionsToSettings(RoutingOptions options);

  void Add(Option type);
  void Remove(Option type);
  bool Has(Option type) const;

  void setVehicleType(VehicleType vt) { m_vehicle = vt; }

  OptionType GetOptions() const { return m_options; }

private:
  OptionType m_options = 0;
  VehicleType m_vehicle = VehicleType::Car;
};

/// \brief Options specific to pedestrian and bicycle routing with trail preferences
struct TrailRoutingOptions
{
  /// \brief Trail preference from 0.0 (fastest route) to 100.0 (trails only)
  double m_trailPreference = 50.0;

  bool m_preferTrails = false;

  static TrailRoutingOptions LoadFromSettings();
  static void SaveToSettings(TrailRoutingOptions const & settings);

  static constexpr double kMinTrailPreference = 0.0;
  static constexpr double kMaxTrailPreference = 100.0;
  static constexpr double kDefaultTrailPreference = 50.0;
};

struct StreetExplorationRoutingOptions
{
  bool m_enabled = false;

  /// \brief Strength from 0.0 (no effect) to 100.0 (maximum preference for unexplored streets)
  double m_strength = 50.0;

  static StreetExplorationRoutingOptions LoadFromSettings();
  static void SaveToSettings(StreetExplorationRoutingOptions const & settings);

  static constexpr double kMinStrength = 0.0;
  static constexpr double kMaxStrength = 100.0;
  static constexpr double kDefaultStrength = 50.0;
};

class RoutingOptionsClassifier
{
public:
  RoutingOptionsClassifier();

  std::optional<RoutingOptions::Option> Get(uint32_t type) const;
  static RoutingOptionsClassifier const & Instance();

private:
  ska::flat_hash_map<uint32_t, RoutingOptions::Option, boost::hash<uint32_t>> m_data;
};

RoutingOptions::Option ChooseMainRoutingOption(RoutingOptions options, bool isCarRouter);

std::string DebugPrint(RoutingOptions const & routingOptions);
std::string DebugPrint(RoutingOptions::Option type);

/// Options guard for debugging/tests.
class RoutingOptionSetter
{
public:
  explicit RoutingOptionSetter(RoutingOptions::OptionType optionssMask, VehicleType type);
  ~RoutingOptionSetter();

private:
  RoutingOptions m_saved;
};
}  // namespace routing
