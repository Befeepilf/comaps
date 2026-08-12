#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/settlement_containment.hpp"

#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

UNIT_TEST(SettlementContainment_SelectTieBreakAndOutside)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp_settlement_index");
  auto areas = AdmitAll(
      {
          MakeAdminCandidate(800, 8, "LargeOuter", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(700, 8, "SmallInner", LonLatBox(24.3, 60.3, 24.7, 60.7)),
          MakeAdminCandidate(900, 8, "EqualB", LonLatBox(23.0, 59.0, 23.5, 59.5)),
          MakeAdminCandidate(850, 8, "EqualA", LonLatBox(23.0, 59.0, 23.5, 59.5)),
      },
      policy);

  std::vector<m2::PointD> samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(23.25, 59.25),
      MercatorFromLonLat(30.0, 70.0),
  };
  SpaWriteParams params;
  params.m_mapDataVersion = 1;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp_settlement_index";
  RemoveIfExists(path);
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  SettlementContainmentIndex index(loaded.m_file.m_areas);
  TEST_GREATER(index.Size(), 0u, ());

  auto const * small = index.Select(MercatorFromLonLat(24.5, 60.5));
  TEST(small != nullptr, ());
  TEST_EQUAL(small->m_name, "SmallInner", ());
  TEST_EQUAL(StableOsmId(*small), 700u, ());

  auto const * equal = index.Select(MercatorFromLonLat(23.25, 59.25));
  TEST(equal != nullptr, ());
  TEST_EQUAL(equal->m_name, "EqualA", ());
  TEST_EQUAL(StableOsmId(*equal), 850u, ());

  TEST_EQUAL(index.Select(MercatorFromLonLat(30.0, 70.0)), nullptr, ());
  TEST(index.SettlementContains(small->m_compactIndex, MercatorFromLonLat(24.5, 60.5)), ());
  TEST(!index.SettlementContains(small->m_compactIndex, MercatorFromLonLat(30.0, 70.0)), ());

  RemoveIfExists(path);
}

UNIT_TEST(SettlementContainment_MatchesNaiveContainsLoop)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp_settlement_naive");
  auto areas = AdmitAll(
      {
          MakeAdminCandidate(800, 8, "LargeOuter", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(700, 8, "SmallInner", LonLatBox(24.3, 60.3, 24.7, 60.7)),
          MakeAdminCandidate(900, 8, "EqualB", LonLatBox(23.0, 59.0, 23.5, 59.5)),
          MakeAdminCandidate(850, 8, "EqualA", LonLatBox(23.0, 59.0, 23.5, 59.5)),
      },
      policy);
  std::vector<m2::PointD> samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(23.25, 59.25),
      MercatorFromLonLat(30.0, 70.0),
  };
  SpaWriteParams params;
  params.m_mapDataVersion = 3;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "sp_settlement_naive";
  RemoveIfExists(path);
  WriteExplorationSidecar(path, areas, samples, policy, params);

  auto loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  SettlementContainmentIndex index(loaded.m_file.m_areas);

  auto naiveSelect = [&](m2::PointD const & point) -> ExplorationArea const *
  {
    ExplorationArea const * best = nullptr;
    for (auto const & area : loaded.m_file.m_areas)
    {
      if (area.m_role != AreaRole::Settlement || !area.Contains(point))
        continue;
      if (best == nullptr || area.m_area < best->m_area ||
          (area.m_area == best->m_area &&
           (area.m_osmId < best->m_osmId ||
            (area.m_osmId == best->m_osmId && area.m_compactIndex < best->m_compactIndex))))
      {
        best = &area;
      }
    }
    return best;
  };

  for (auto const & pt : samples)
  {
    auto const * viaIndex = index.Select(pt);
    auto const * viaNaive = naiveSelect(pt);
    if (viaIndex == nullptr)
      TEST_EQUAL(viaNaive, nullptr, ());
    else
    {
      TEST(viaNaive != nullptr, ());
      TEST_EQUAL(viaIndex->m_compactIndex, viaNaive->m_compactIndex, ());
    }
  }

  RemoveIfExists(path);
}

}  // namespace
