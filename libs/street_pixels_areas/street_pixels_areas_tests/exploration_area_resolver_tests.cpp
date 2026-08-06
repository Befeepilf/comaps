#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/subdivision_assignment.hpp"

#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

struct ResolverFixture
{
  std::string m_path;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  std::vector<int64_t> m_universe;
  CountryPolicy m_policy;
};

ResolverFixture MakeSettlementOnlyFixture()
{
  auto const config = FinlandConfig();
  ResolverFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp029_settlement_only");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(8, 8, "Town", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {100, 200};
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp029_settlement_only";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

ResolverFixture MakeSubdivOverSettlementFixture()
{
  auto const config = FinlandConfig();
  ResolverFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp029_subdiv_wins");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.1, 60.1),
  };
  fx.m_universe = {10, 20};
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp029_subdiv_wins";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

ResolverFixture MakePlaceOverSettlementFixture()
{
  auto const config = FinlandConfig();
  ResolverFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp029_place_wins");
  fx.m_areas = AdmitAll(
      {
          MakePlaceCandidate(50, "neighbourhood", "Quarter", LonLatBox(24.3, 60.3, 24.7, 60.7)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.1, 60.1),
  };
  fx.m_universe = {10, 20};
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp029_place_wins";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

ResolverFixture MakeMultiSettlementTieFixture()
{
  auto const config = FinlandConfig();
  ResolverFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp029_settlement_tie");
  // Same box → equal area; lower OSM id must win. Nested smaller also covered below.
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(800, 8, "LargeOuter", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(700, 8, "SmallInner", LonLatBox(24.3, 60.3, 24.7, 60.7)),
          MakeAdminCandidate(900, 8, "EqualB", LonLatBox(23.0, 59.0, 23.5, 59.5)),
          MakeAdminCandidate(850, 8, "EqualA", LonLatBox(23.0, 59.0, 23.5, 59.5)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(23.25, 59.25),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {1, 2, 3};
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp029_settlement_tie";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}
}  // namespace

UNIT_TEST(SelectSettlement_SettlementOnlyAndRural)
{
  auto fx = MakeSettlementOnlyFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, (DebugPrint(loaded.m_status)));
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  // Dense column is subdivision-only: settlement-only samples are sentinel.
  TEST_EQUAL(LookupSubdivisionBySlot(loaded.m_file, 0), nullptr, ());
  TEST_EQUAL(LookupSubdivisionBySlot(loaded.m_file, 1), nullptr, ());

  auto const * town = SelectSettlementContaining(loaded.m_file, fx.m_samples[0]);
  TEST(town != nullptr, ());
  TEST_EQUAL(town->m_role, AreaRole::Settlement, ());
  TEST_EQUAL(town->m_name, "Town", ());
  TEST_EQUAL(StableOsmId(*town), 8u, ());

  TEST_EQUAL(SelectSettlementContaining(loaded.m_file, fx.m_samples[1]), nullptr, ());

  auto const * layeredIn = LookupExplorationArea(loaded.m_file, 0, fx.m_samples[0]);
  TEST_EQUAL(layeredIn, town, ());
  TEST_EQUAL(LookupExplorationArea(loaded.m_file, 1, fx.m_samples[1]), nullptr, ());

  TEST_EQUAL(LookupExplorationArea(loaded.m_file, fx.m_universe, 100, fx.m_samples[0]), town, ());
  TEST_EQUAL(LookupExplorationArea(loaded.m_file, fx.m_universe, 200, fx.m_samples[1]), nullptr, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(LookupExplorationArea_SubdivisionWinsOverSettlement)
{
  auto fx = MakeSubdivOverSettlementFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  auto const * nested = LookupExplorationArea(loaded.m_file, 0, fx.m_samples[0]);
  TEST(nested != nullptr, ());
  TEST_EQUAL(nested->m_role, AreaRole::Subdivision, ());
  TEST_EQUAL(nested->m_name, "District", ());
  TEST_EQUAL(LookupSubdivisionBySlot(loaded.m_file, 0), nested, ());

  // Outside subdiv but inside settlement → settlement fallback.
  auto const * city = LookupExplorationArea(loaded.m_file, 1, fx.m_samples[1]);
  TEST(city != nullptr, ());
  TEST_EQUAL(city->m_role, AreaRole::Settlement, ());
  TEST_EQUAL(city->m_name, "City", ());
  TEST_EQUAL(LookupSubdivisionBySlot(loaded.m_file, 1), nullptr, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(LookupExplorationArea_PlaceWinsOverSettlement)
{
  auto fx = MakePlaceOverSettlementFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  auto const * place = LookupExplorationArea(loaded.m_file, 0, fx.m_samples[0]);
  TEST(place != nullptr, ());
  TEST_EQUAL(place->m_role, AreaRole::PlaceBoundary, ());
  TEST_EQUAL(place->m_name, "Quarter", ());
  TEST(place->IsAssignable(), ());

  auto const * city = LookupExplorationArea(loaded.m_file, 1, fx.m_samples[1]);
  TEST(city != nullptr, ());
  TEST_EQUAL(city->m_role, AreaRole::Settlement, ());
  TEST_EQUAL(city->m_name, "City", ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(SelectSettlement_MultiSettlementTieBreak)
{
  auto fx = MakeMultiSettlementTieFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  // Nested: smallest area wins.
  auto const * small = SelectSettlementContaining(loaded.m_file, fx.m_samples[0]);
  TEST(small != nullptr, ());
  TEST_EQUAL(small->m_name, "SmallInner", ());
  TEST_EQUAL(StableOsmId(*small), 700u, ());
  auto const * large = FindAreaByCompactIndex(loaded.m_file, 0);
  TEST(large != nullptr, ());
  TEST_EQUAL(large->m_name, "LargeOuter", ());
  TEST_LESS(small->m_area, large->m_area, ());

  // Equal area: lower OSM id wins.
  auto const * equal = SelectSettlementContaining(loaded.m_file, fx.m_samples[1]);
  TEST(equal != nullptr, ());
  TEST_EQUAL(equal->m_name, "EqualA", ());
  TEST_EQUAL(StableOsmId(*equal), 850u, ());

  TEST_EQUAL(SelectSettlementContaining(loaded.m_file, fx.m_samples[2]), nullptr, ());

  TEST_EQUAL(LookupExplorationArea(loaded.m_file, fx.m_universe, 1, fx.m_samples[0]), small, ());
  TEST_EQUAL(LookupExplorationArea(loaded.m_file, fx.m_universe, 2, fx.m_samples[1]), equal, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(ExplorationAreaResolver_FailClosedAndLayering)
{
  auto fx = MakeSettlementOnlyFixture();

  auto ok = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                             fx.m_params.m_policyVersion);
  TEST(ok.has_value(), ());

  auto const * town = ok->LookupByHealpix(100, fx.m_samples[0]);
  TEST(town != nullptr, ());
  TEST_EQUAL(town->m_role, AreaRole::Settlement, ());
  TEST_EQUAL(town->m_name, "Town", ());
  TEST_EQUAL(ok->LookupByHealpix(200, fx.m_samples[1]), nullptr, ());
  TEST_EQUAL(ok->LookupBySlot(0, fx.m_samples[0]), town, ());
  // Unknown healpix: fail closed — no settlement invent from a wrong centre.
  TEST_EQUAL(ok->LookupByHealpix(999, fx.m_samples[0]), nullptr, ());

  // Subdivision table remains subdivision-only.
  TEST_EQUAL(ok->SubdivisionTable().LookupByHealpix(100), nullptr, ());

  auto badMap = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion + 1,
                                                 fx.m_params.m_policyVersion);
  TEST(!badMap.has_value(), ());

  auto badPolicy = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                    fx.m_params.m_policyVersion + 1);
  TEST(!badPolicy.has_value(), ());

  auto badUniverse =
      ExplorationAreaResolver::TryLoad(fx.m_path, {1}, fx.m_params.m_mapDataVersion, fx.m_params.m_policyVersion);
  TEST(!badUniverse.has_value(), ());

  auto notAscending = ExplorationAreaResolver::TryLoad(fx.m_path, {200, 100}, fx.m_params.m_mapDataVersion,
                                                       fx.m_params.m_policyVersion);
  TEST(!notAscending.has_value(), ());

  auto duplicates = ExplorationAreaResolver::TryLoad(fx.m_path, {100, 100}, fx.m_params.m_mapDataVersion,
                                                     fx.m_params.m_policyVersion);
  TEST(!duplicates.has_value(), ());

  auto missing = ExplorationAreaResolver::TryLoad(
      ExplorationSidecarPath(GetPlatform().WritableDir(), "missing_sp029"), fx.m_universe,
      fx.m_params.m_mapDataVersion, fx.m_params.m_policyVersion);
  TEST(!missing.has_value(), ());

  // Free-function healpix path also fails closed on bad U (no silent settlement).
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  TEST_EQUAL(LookupExplorationArea(loaded.m_file, {200, 100}, 100, fx.m_samples[0]), nullptr, ());
  TEST_EQUAL(LookupExplorationArea(loaded.m_file, {100, 100}, 100, fx.m_samples[0]), nullptr, ());
  TEST_EQUAL(LookupExplorationArea(loaded.m_file, fx.m_universe, 999, fx.m_samples[0]), nullptr, ());

  RemoveIfExists(fx.m_path);
}
