#include "testing/testing.hpp"

#include "map/street_pixels_manager.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "base/stl_helpers.hpp"

#include <initializer_list>

namespace
{
bool Explorable(std::initializer_list<base::StringIL> const & paths,
                feature::GeomType geomType = feature::GeomType::Line)
{
  auto const & cl = classif();
  feature::TypesHolder types(geomType);
  for (auto const & path : paths)
    types.Add(cl.GetTypeByPath(path));
  return IsExplorableFeature(geomType, types);
}
}  // namespace

UNIT_TEST(Eligibility_IncludesCommonHighways)
{
  classificator::Load();

  TEST(Explorable({{"highway", "residential"}}), ());
  TEST(Explorable({{"highway", "living_street"}}), ());
  TEST(Explorable({{"highway", "primary"}}), ());
  TEST(Explorable({{"highway", "primary_link"}}), ());
  TEST(Explorable({{"highway", "cycleway"}}), ());
  TEST(Explorable({{"highway", "footway"}}), ());
  TEST(Explorable({{"highway", "steps"}}), ());
  TEST(Explorable({{"highway", "track"}}), ());
  TEST(Explorable({{"highway", "path"}}), ());
  TEST(Explorable({{"highway", "pedestrian"}}), ());
  TEST(Explorable({{"highway", "service"}}), ());
  TEST(Explorable({{"highway", "bridleway"}}), ());
  TEST(Explorable({{"highway", "unclassified"}}), ());
  TEST(Explorable({{"highway", "trunk"}}), ());
}

UNIT_TEST(Eligibility_IncludesBridges)
{
  classificator::Load();

  TEST(Explorable({{"highway", "footway", "bridge"}}), ());
  TEST(Explorable({{"highway", "primary", "bridge"}}), ());
  TEST(Explorable({{"highway", "cycleway", "bridge"}}), ());
  TEST(Explorable({{"highway", "residential", "bridge"}}), ());
  TEST(Explorable({{"highway", "motorway", "bridge"}, {"hwtag", "yesbicycle"}}), ());
  TEST(Explorable({{"highway", "motorway_link", "bridge"}, {"hwtag", "yesbicycle"}}), ());
}

UNIT_TEST(Eligibility_ExcludesDrivewayTunnelPrivateNoAccess)
{
  classificator::Load();

  TEST(!Explorable({{"highway", "service", "driveway"}}), ());
  TEST(!Explorable({{"highway", "service"}, {"highway", "service", "driveway"}}), ());
  TEST(!Explorable({{"highway", "footway", "tunnel"}}), ());
  TEST(!Explorable({{"highway", "primary", "tunnel"}}), ());
  TEST(!Explorable({{"highway", "footway"}, {"highway", "footway", "tunnel"}}), ());
  TEST(!Explorable({{"highway", "residential"}, {"hwtag", "private"}}), ());
  TEST(!Explorable({{"highway", "track", "no-access"}}), ());
  TEST(!Explorable({{"highway", "track"}, {"highway", "track", "no-access"}}), ());
}

UNIT_TEST(Eligibility_ExcludesConstructionElevatorRaceway)
{
  classificator::Load();

  TEST(!Explorable({{"highway", "construction"}}), ());
  TEST(!Explorable({{"highway", "construction", "primary"}}), ());
  TEST(!Explorable({{"highway", "construction", "primary_link"}}), ());
  TEST(!Explorable({{"highway", "construction", "motorway"}}), ());
  TEST(!Explorable({{"highway", "construction", "motorway_link"}}), ());
  TEST(!Explorable({{"highway", "construction", "service"}}), ());
  TEST(!Explorable({{"highway", "construction", "residential"}}), ());
  TEST(!Explorable({{"highway", "construction", "track"}}), ());
  TEST(!Explorable({{"highway", "elevator"}}), ());
  TEST(!Explorable({{"highway", "raceway"}}), ());
}

UNIT_TEST(Eligibility_MotorwayRequiresYesBicycle)
{
  classificator::Load();

  TEST(!Explorable({{"highway", "motorway"}}), ());
  TEST(!Explorable({{"highway", "motorway_link"}}), ());
  TEST(!Explorable({{"highway", "motorway", "bridge"}}), ());
  TEST(!Explorable({{"highway", "motorway_link", "bridge"}}), ());
  TEST(!Explorable({{"highway", "motorway", "bridge"}, {"hwtag", "yesfoot"}}), ());
  TEST(!Explorable({{"highway", "motorway_link"}, {"hwtag", "yesfoot"}}), ());
  TEST(Explorable({{"highway", "motorway"}, {"hwtag", "yesbicycle"}}), ());
  TEST(Explorable({{"highway", "motorway_link"}, {"hwtag", "yesbicycle"}}), ());
  TEST(Explorable({{"highway", "motorway", "bridge"}, {"hwtag", "yesbicycle"}}), ());
  TEST(Explorable({{"highway", "motorway_link", "bridge"}, {"hwtag", "yesbicycle"}}), ());
  TEST(!Explorable({{"highway", "motorway", "tunnel"}, {"hwtag", "yesbicycle"}}), ());
  TEST(!Explorable({{"highway", "motorway_link", "tunnel"}, {"hwtag", "yesbicycle"}}), ());
}

UNIT_TEST(Eligibility_RequiresBikeOrFootAccess)
{
  classificator::Load();

  TEST(Explorable({{"highway", "residential"}, {"hwtag", "nobicycle"}}), ());
  TEST(Explorable({{"highway", "residential"}, {"hwtag", "nofoot"}}), ());
  TEST(!Explorable({{"highway", "residential"}, {"hwtag", "nobicycle"}, {"hwtag", "nofoot"}}), ());
  TEST(Explorable({{"highway", "residential"}, {"hwtag", "yesbicycle"}, {"hwtag", "nofoot"}}), ());
  TEST(Explorable({{"highway", "residential"}, {"hwtag", "nobicycle"}, {"hwtag", "yesfoot"}}), ());
}

UNIT_TEST(Eligibility_RejectsNonLineGeometry)
{
  classificator::Load();

  TEST(!Explorable({{"highway", "residential"}}, feature::GeomType::Point), ());
  TEST(!Explorable({{"highway", "residential"}}, feature::GeomType::Area), ());
}

UNIT_TEST(Eligibility_RejectsNonHighwayRoutes)
{
  classificator::Load();

  TEST(!Explorable({{"route", "ferry"}}), ());
  TEST(!Explorable({{"route", "ferry"}, {"hwtag", "yesfoot"}, {"hwtag", "yesbicycle"}}), ());
  TEST(!Explorable({{"aerialway", "cable_car"}}), ());
  TEST(!Explorable({{"aerialway", "gondola"}}), ());
  TEST(!Explorable({{"railway", "rail"}}), ());
  TEST(!Explorable({{"man_made", "pier"}}), ());
}

UNIT_TEST(Eligibility_ParkingAisleAndBuswayStillIncluded)
{
  classificator::Load();

  TEST(Explorable({{"highway", "service", "parking_aisle"}}), ());
  TEST(Explorable({{"highway", "busway"}}), ());
  TEST(Explorable({{"highway", "busway", "bridge"}}), ());
}
