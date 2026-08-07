#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/area_completion_cache.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/city_completion_cache.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"

#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

struct CityFx
{
  std::string m_path;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  std::vector<int64_t> m_universe;
  CountryPolicy m_policy;
};

CityFx MakeMultiAreaCityFx()
{
  auto const config = FinlandConfig();
  CityFx fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp039_multi");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.1, 60.1),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 20, 30};
  fx.m_params.m_mapDataVersion = 390;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp039_multi";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

CityFx MakeSettlementOnlyFx()
{
  auto const config = FinlandConfig();
  CityFx fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp039_settlement_only");
  fx.m_areas = AdmitAll({MakeAdminCandidate(8, 8, "Town", LonLatBox(24.0, 60.0, 25.0, 61.0))}, fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 30};
  fx.m_params.m_mapDataVersion = 391;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp039_settlement_only";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}
}  // namespace

UNIT_TEST(CityCompletion_MultiAreaSumsWithoutDoubleCount)
{
  auto fx = MakeMultiAreaCityFx();
  auto loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());

  std::vector<int64_t> explored = {10};
  auto areaCache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  TEST(areaCache.IsValid(), ());

  // District 1/1; settlement gap 0/1 → city rollup 1/2 (not settlement 0/1 alone).
  auto const city = AggregateCityCompletion(loaded.m_file, areaCache, /*settlement*/ 1);
  TEST_EQUAL(city.m_compactIndex, 1u, ());
  TEST_EQUAL(city.m_total, 2u, ());
  TEST_EQUAL(city.m_explored, 1u, ());
  TEST_EQUAL(AreaCompletionFraction(city), 0.5, ());

  explored = {10, 20};
  areaCache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  auto const cityAll = AggregateCityCompletion(loaded.m_file, areaCache, 1);
  TEST_EQUAL(cityAll.m_total, 2u, ());
  TEST_EQUAL(cityAll.m_explored, 2u, ());
  TEST_EQUAL(AreaCompletionFraction(cityAll), 1.0, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(CityCompletion_SettlementOnlyMatchesAreaRow)
{
  auto fx = MakeSettlementOnlyFx();
  auto loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());

  std::vector<int64_t> explored = {10};
  auto areaCache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  auto const town = AggregateCityCompletion(loaded.m_file, areaCache, 0);
  TEST_EQUAL(town.m_total, 1u, ());
  TEST_EQUAL(town.m_explored, 1u, ());
  TEST_EQUAL(AreaCompletionFraction(town), areaCache.GetFraction(0), ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(CityCompletion_CacheBuildAndEmpty)
{
  auto fx = MakeMultiAreaCityFx();
  auto loaded = TryLoadExplorationSidecar(fx.m_path);
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());
  auto areaCache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, {});
  auto cityCache = CityCompletionCache::Build(loaded.m_file, areaCache);
  TEST(cityCache.IsValid(), ());
  TEST_EQUAL(cityCache.Rows().size(), 1u, ());
  auto row = cityCache.Get(1);
  TEST(row.has_value(), ());
  TEST_EQUAL(row->m_total, 2u, ());
  TEST_EQUAL(row->m_explored, 0u, ());
  TEST_EQUAL(cityCache.GetFraction(1), 0.0, ());

  SpaFile empty;
  AreaCompletionCache invalid;
  TEST(!CityCompletionCache::Build(empty, invalid).IsValid(), ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(CityCompletion_NoCountryWorldAggregate)
{
  // AggregateCityCompletion only accepts Settlement compact indices from a sidecar —
  // never invents country/world rows (§12.4).
  SpaFile empty;
  AreaCompletionCache cache;
  auto const bogus = AggregateCityCompletion(empty, cache, 0);
  TEST_EQUAL(bogus.m_total, 0u, ());
  TEST_EQUAL(bogus.m_explored, 0u, ());
}
