#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/area_completion_cache.hpp"
#include "street_pixels_areas/areas_writer.hpp"
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

struct CompletionFixture
{
  std::string m_path;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  std::vector<int64_t> m_universe;
  CountryPolicy m_policy;
};

CompletionFixture MakeDistrictCityFixture()
{
  auto const config = FinlandConfig();
  CompletionFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp034_completion");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  // slot0: inside district → subdivision District
  // slot1: outside district, inside city → settlement City
  // slot2: outside both → no-area
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.1, 60.1),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 20, 30};
  fx.m_params.m_mapDataVersion = 340;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp034_completion";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

CompletionFixture MakeEmptyAreaFixture()
{
  auto const config = FinlandConfig();
  CompletionFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp034_empty_area");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "EmptyDistrict", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  // All samples outside the district box → district total stays 0.
  fx.m_samples = {
      MercatorFromLonLat(24.1, 60.1),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {20, 30};
  fx.m_params.m_mapDataVersion = 341;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp034_empty_area";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}
}  // namespace

UNIT_TEST(AreaCompletion_KnownTotalsAndFractions)
{
  auto fx = MakeDistrictCityFixture();
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());

  // Explore district pixel only → 1/1 district; city settlement 0/1; no-area ignored.
  std::vector<int64_t> explored = {10};
  auto cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  TEST(cache.IsValid(), ());
  TEST_EQUAL(cache.MapDataVersion(), 340, ());
  TEST_EQUAL(cache.PolicyVersion(), fx.m_params.m_policyVersion, ());
  TEST_EQUAL(cache.Rows().size(), 2, ());

  auto district = cache.Get(0);
  TEST(district.has_value(), ());
  TEST_EQUAL(district->m_compactIndex, 0u, ());
  TEST_EQUAL(district->m_osmId, 10u, ());
  TEST_EQUAL(district->m_total, 1u, ());
  TEST_EQUAL(district->m_explored, 1u, ());
  TEST_EQUAL(cache.GetFraction(0), 1.0, ());

  auto city = cache.Get(1);
  TEST(city.has_value(), ());
  TEST_EQUAL(city->m_osmId, 8u, ());
  TEST_EQUAL(city->m_total, 1u, ());
  TEST_EQUAL(city->m_explored, 0u, ());
  TEST_EQUAL(cache.GetFraction(1), 0.0, ());

  // Explore both assignable pixels → district 100%, city 100%.
  explored = {10, 20};
  cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  TEST_EQUAL(cache.Get(0)->m_explored, 1u, ());
  TEST_EQUAL(cache.Get(1)->m_explored, 1u, ());
  TEST_EQUAL(cache.GetFraction(0), 1.0, ());
  TEST_EQUAL(cache.GetFraction(1), 1.0, ());

  // Explored no-area pixel does not invent a completion row or change fractions.
  explored = {10, 20, 30};
  cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  TEST_EQUAL(cache.Rows().size(), 2, ());
  TEST_EQUAL(cache.GetFraction(0), 1.0, ());
  TEST_EQUAL(cache.GetFraction(1), 1.0, ());

  // Explore one of two district pixels → 50%.
  explored = {10};
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.6, 60.6),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_universe = {10, 15, 30};
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                              fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());
  cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, explored);
  TEST_EQUAL(cache.Get(0)->m_total, 2u, ());
  TEST_EQUAL(cache.Get(0)->m_explored, 1u, ());
  TEST_EQUAL(cache.GetFraction(0), 0.5, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaCompletion_ZeroTotalSafeFraction)
{
  auto fx = MakeEmptyAreaFixture();
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());

  auto cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, /*explored=*/{});
  TEST(cache.IsValid(), ());
  auto emptyDistrict = cache.Get(0);
  TEST(emptyDistrict.has_value(), ());
  TEST_EQUAL(emptyDistrict->m_total, 0u, ());
  TEST_EQUAL(emptyDistrict->m_explored, 0u, ());
  TEST_EQUAL(AreaCompletionFraction(*emptyDistrict), 0.0, ());
  TEST_EQUAL(cache.GetFraction(0), 0.0, ());

  auto city = cache.Get(1);
  TEST(city.has_value(), ());
  TEST_EQUAL(city->m_total, 1u, ());
  TEST_EQUAL(city->m_explored, 0u, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaCompletion_InvalidateClearsRows)
{
  auto fx = MakeDistrictCityFixture();
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());

  auto cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, {10});
  TEST(cache.IsValid(), ());
  TEST(cache.Get(0).has_value(), ());

  cache.Invalidate();
  TEST(!cache.IsValid(), ());
  TEST(!cache.Get(0).has_value(), ());
  TEST_EQUAL(cache.GetFraction(0), 0.0, ());
  TEST(cache.Rows().empty(), ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(AreaCompletion_IgnoresExploredOutsideUniverse)
{
  auto fx = MakeDistrictCityFixture();
  auto resolver = ExplorationAreaResolver::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                   fx.m_params.m_policyVersion);
  TEST(resolver.has_value(), ());

  auto cache = AreaCompletionCache::Build(*resolver, fx.m_universe, fx.m_samples, {10, 999});
  TEST_EQUAL(cache.Get(0)->m_explored, 1u, ());
  TEST_EQUAL(cache.Get(1)->m_explored, 0u, ());

  RemoveIfExists(fx.m_path);
}
