#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

#include "coding/file_writer.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

std::vector<ExplorationArea> AdmitAll(std::vector<AreaCandidateInput> const & inputs, CountryPolicy const & policy)
{
  std::vector<ExplorationArea> areas;
  for (auto const & input : inputs)
  {
    auto result = FilterExplorationCandidate(input, policy);
    TEST_EQUAL(result.m_reason, RejectReason::Accepted, (DebugPrint(result.m_reason)));
    areas.push_back(*result.m_area);
  }
  return areas;
}

void RemoveIfExists(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
}

struct FixtureSpa
{
  std::string m_dir;
  std::string m_leaf;
  std::string m_path;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  CountryPolicy m_policy;
  uint32_t m_policyVersion = 0;
};

FixtureSpa MakeFixture()
{
  auto const config = FinlandConfig();
  FixtureSpa fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_policyVersion = config.GetPolicyVersion();
  fx.m_dir = GetPlatform().WritableDir();
  fx.m_leaf = "Finland_Southern Finland_Helsinki";
  fx.m_path = ExplorationSidecarPath(fx.m_dir, fx.m_leaf);
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(11, 10, "Nested", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(23.5, 59.5, 25.5, 61.5)),
          MakePlaceCandidate(90, "neighbourhood", "Hood", LonLatBox(24.3, 60.3, 24.4, 60.4)),
      },
      fx.m_policy);
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.35, 60.35),
      MercatorFromLonLat(30.0, 70.0),
  };
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = fx.m_policyVersion;
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = fx.m_leaf;
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}
}  // namespace

UNIT_TEST(ExplorationSidecar_PathBesideMwmLeaf)
{
  std::string const dir = GetPlatform().WritableDir();
  std::string const leaf = "Finland_Southern Finland_Helsinki";
  std::string const expected = base::JoinPath(dir, leaf + SPA_FILE_EXTENSION);
  TEST_EQUAL(ExplorationSidecarPath(dir, leaf), expected, ());

  std::string const mwmPath = base::JoinPath(dir, leaf + DATA_FILE_EXTENSION);
  TEST_EQUAL(ExplorationSidecarPathBesideMwm(mwmPath), expected, ());
}

UNIT_TEST(ExplorationSidecar_TryLoadFixtureAndAssignRoundTrip)
{
  auto fx = MakeFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, (DebugPrint(loaded.m_status)));
  TEST_EQUAL(loaded.m_file.m_header.m_mwmId, fx.m_leaf, ());
  TEST_EQUAL(loaded.m_file.m_header.m_mapDataVersion, fx.m_params.m_mapDataVersion, ());
  TEST_EQUAL(loaded.m_file.m_header.m_policyVersion, fx.m_params.m_policyVersion, ());

  uint32_t const sentinel = NoSubdivisionSentinel(loaded.m_file.m_header.m_indexWidth);
  auto const expected = BuildDenseAssignments(fx.m_samples, loaded.m_file.m_areas, fx.m_policy, sentinel);
  TEST_EQUAL(DenseAssignments(loaded.m_file), expected, ());

  auto const verified =
      TryLoadAndVerifyExplorationSidecar(fx.m_path, fx.m_params.m_mapDataVersion, fx.m_params.m_policyVersion);
  TEST_EQUAL(verified.m_status, SpaLoadStatus::Ok, (DebugPrint(verified.m_status)));
  TEST_EQUAL(DenseAssignments(verified.m_file), expected, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(ExplorationSidecar_MissingIsEmptySafe)
{
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "missing_sidecar_leaf");
  RemoveIfExists(path);

  auto const loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Missing, ());
  TEST(loaded.m_file.m_areas.empty(), ());
  TEST(loaded.m_file.m_assignments.empty(), ());
  TEST_EQUAL(loaded.m_file.m_header.m_areaCount, 0u, ());
}

UNIT_TEST(ExplorationSidecar_CorruptIsEmptySafe)
{
  std::string const path = ExplorationSidecarPath(GetPlatform().WritableDir(), "corrupt_sidecar_leaf");
  RemoveIfExists(path);
  {
    FileWriter writer(path);
    std::string const junk = "not-a-valid-spa-container";
    writer.Write(junk.data(), junk.size());
  }

  auto const loaded = TryLoadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Corrupt, (DebugPrint(loaded.m_status)));
  TEST(loaded.m_file.m_areas.empty(), ());
  TEST(loaded.m_file.m_assignments.empty(), ());

  RemoveIfExists(path);
}

UNIT_TEST(ExplorationSidecar_VersionMismatchIsEmptySafe)
{
  auto fx = MakeFixture();

  auto const badMap =
      TryLoadAndVerifyExplorationSidecar(fx.m_path, fx.m_params.m_mapDataVersion + 1, fx.m_params.m_policyVersion);
  TEST_EQUAL(badMap.m_status, SpaLoadStatus::VersionMismatch, (DebugPrint(badMap.m_status)));
  TEST(badMap.m_file.m_areas.empty(), ());
  TEST(badMap.m_file.m_assignments.empty(), ());

  auto const badPolicy =
      TryLoadAndVerifyExplorationSidecar(fx.m_path, fx.m_params.m_mapDataVersion, fx.m_params.m_policyVersion + 1);
  TEST_EQUAL(badPolicy.m_status, SpaLoadStatus::VersionMismatch, (DebugPrint(badPolicy.m_status)));
  TEST(badPolicy.m_file.m_areas.empty(), ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(ExplorationSidecar_DisplayNameNeverFallsBackToMwmId)
{
  auto fx = MakeFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  for (auto const & area : loaded.m_file.m_areas)
  {
    TEST_NOT_EQUAL(DisplayName(area), fx.m_leaf, (StableOsmId(area)));
    TEST_NOT_EQUAL(DisplayName(area), loaded.m_file.m_header.m_mwmId, (StableOsmId(area)));
    TEST(!DisplayName(area).empty(), (StableOsmId(area)));
  }

  TEST_EQUAL(DisplayName(loaded.m_file.m_areas[0]), "District", ());
  TEST_EQUAL(StableOsmId(loaded.m_file.m_areas[0]), 10u, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(ExplorationSidecar_SettlementRingsFromSidecar)
{
  auto fx = MakeFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  auto const settlements = SettlementAreas(loaded.m_file);
  TEST_EQUAL(settlements.size(), 1u, ());
  TEST_EQUAL(settlements[0]->m_role, AreaRole::Settlement, ());
  TEST_EQUAL(DisplayName(*settlements[0]), "City", ());
  TEST_EQUAL(StableOsmId(*settlements[0]), 8u, ());
  TEST(!settlements[0]->m_rings.empty(), ());
  TEST(settlements[0]->m_rings[0].size() >= 3u, ());

  // True-ring containment — not three-box CitiesBoundariesTable.
  TEST(settlements[0]->Contains(MercatorFromLonLat(24.5, 60.5)), ());
  TEST(!settlements[0]->Contains(MercatorFromLonLat(30.0, 70.0)), ());

  auto const subdivisions = AreasByRole(loaded.m_file, AreaRole::Subdivision);
  TEST_EQUAL(subdivisions.size(), 2u, ());

  auto const places = AreasByRole(loaded.m_file, AreaRole::PlaceBoundary);
  TEST_EQUAL(places.size(), 1u, ());

  // Settlement compact indices are never assignment targets.
  uint32_t const sentinel = NoSubdivisionSentinel(loaded.m_file.m_header.m_indexWidth);
  for (uint32_t value : DenseAssignments(loaded.m_file))
  {
    if (value == sentinel)
      continue;
    auto const * area = FindAreaByCompactIndex(loaded.m_file, value);
    TEST(area != nullptr, (value));
    TEST(area->IsAssignable(), (value, DebugPrint(area->m_role)));
  }

  RemoveIfExists(fx.m_path);
}
