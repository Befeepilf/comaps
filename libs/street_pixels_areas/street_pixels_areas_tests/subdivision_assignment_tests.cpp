#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"
#include "street_pixels_areas/subdivision_assignment.hpp"

#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

struct AssignmentFixture
{
  std::string m_path;
  SpaWriteParams m_params;
  std::vector<ExplorationArea> m_areas;
  std::vector<m2::PointD> m_samples;
  // Synthetic ascending NEST ids parallel to m_samples (universe-order contract).
  std::vector<int64_t> m_universe;
  CountryPolicy m_policy;
};

AssignmentFixture MakeNestedFixture()
{
  auto const config = FinlandConfig();
  AssignmentFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp028_nested");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "Outer", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(11, 10, "Inner", LonLatBox(24.3, 60.3, 24.7, 60.7)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(23.5, 59.5, 25.5, 61.5)),
      },
      fx.m_policy);
  // Slots: nested centre, outside-all, outer-only.
  fx.m_samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(30.0, 70.0),
      MercatorFromLonLat(24.1, 60.1),
  };
  fx.m_universe = {100, 200, 300};
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp028_nested";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}

AssignmentFixture MakeTieFixture()
{
  auto const config = FinlandConfig();
  AssignmentFixture fx;
  fx.m_policy = config.GetByIso("FI");
  fx.m_path = ExplorationSidecarPath(GetPlatform().WritableDir(), "sp028_tie");
  fx.m_areas = AdmitAll(
      {
          MakeAdminCandidate(200, 10, "B", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(100, 10, "A", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      fx.m_policy);
  fx.m_samples = {MercatorFromLonLat(24.5, 60.5)};
  fx.m_universe = {42};
  fx.m_params.m_mapDataVersion = 260417;
  fx.m_params.m_policyVersion = config.GetPolicyVersion();
  fx.m_params.m_isoCode = "FI";
  fx.m_params.m_mwmId = "sp028_tie";
  RemoveIfExists(fx.m_path);
  WriteExplorationSidecar(fx.m_path, fx.m_areas, fx.m_samples, fx.m_policy, fx.m_params);
  return fx;
}
}  // namespace

UNIT_TEST(LookupSubdivision_NestedSmallestAndOutside)
{
  auto fx = MakeNestedFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, (DebugPrint(loaded.m_status)));
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  auto const * nested = LookupSubdivisionBySlot(loaded.m_file, 0);
  TEST(nested != nullptr, ());
  TEST_EQUAL(nested->m_name, "Inner", ());
  TEST_EQUAL(StableOsmId(*nested), 11u, ());

  TEST_EQUAL(LookupSubdivisionBySlot(loaded.m_file, 1), nullptr, ());

  auto const * outerOnly = LookupSubdivisionBySlot(loaded.m_file, 2);
  TEST(outerOnly != nullptr, ());
  TEST_EQUAL(outerOnly->m_name, "Outer", ());

  TEST_EQUAL(LookupSubdivisionByHealpix(loaded.m_file, fx.m_universe, 100), nested, ());
  TEST_EQUAL(LookupSubdivisionByHealpix(loaded.m_file, fx.m_universe, 200), nullptr, ());
  TEST_EQUAL(LookupSubdivisionByHealpix(loaded.m_file, fx.m_universe, 300), outerOnly, ());
  TEST_EQUAL(LookupSubdivisionByHealpix(loaded.m_file, fx.m_universe, 999), nullptr, ());
  TEST_EQUAL(LookupSubdivisionBySlot(loaded.m_file, 99), nullptr, ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(LookupSubdivision_TieBreakStableOsmId)
{
  auto fx = MakeTieFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  auto const * area = LookupSubdivisionByHealpix(loaded.m_file, fx.m_universe, 42);
  TEST(area != nullptr, ());
  TEST_EQUAL(StableOsmId(*area), 100u, ());
  TEST_EQUAL(area->m_name, "A", ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(LookupSubdivision_DeterminismAndNoDual)
{
  auto fx = MakeNestedFixture();
  auto const a = TryLoadExplorationSidecar(fx.m_path);
  auto const b = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(a.m_status, SpaLoadStatus::Ok, ());
  TEST_EQUAL(b.m_status, SpaLoadStatus::Ok, ());
  TEST_EQUAL(DenseAssignments(a.m_file), DenseAssignments(b.m_file), ());
  TEST(VerifyDenseAssignments(a.m_file, fx.m_samples, fx.m_policy), ());
  TEST(VerifyDenseAssignments(b.m_file, fx.m_samples, fx.m_policy), ());

  uint32_t const sentinel = NoSubdivisionSentinel(a.m_file.m_header.m_indexWidth);
  for (size_t i = 0; i < a.m_file.m_assignments.size(); ++i)
  {
    uint32_t const value = a.m_file.m_assignments[i];
    // Dense column encodes exactly one answer per slot (sentinel = none).
    if (value == sentinel)
    {
      TEST_EQUAL(LookupSubdivisionBySlot(a.m_file, i), nullptr, (i));
      continue;
    }
    auto const * area = LookupSubdivisionBySlot(a.m_file, i);
    TEST(area != nullptr, (i, value));
    TEST_EQUAL(area->m_compactIndex, value, (i));
    TEST(area->IsAssignable(), (i, DebugPrint(area->m_role)));
  }

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(LookupSubdivision_ClientMatchesGenerator)
{
  auto fx = MakeNestedFixture();
  auto const loaded = TryLoadExplorationSidecar(fx.m_path);
  TEST_EQUAL(loaded.m_status, SpaLoadStatus::Ok, ());

  uint32_t const sentinel = NoSubdivisionSentinel(loaded.m_file.m_header.m_indexWidth);
  auto const expected = BuildDenseAssignments(fx.m_samples, loaded.m_file.m_areas, fx.m_policy, sentinel);
  TEST_EQUAL(DenseAssignments(loaded.m_file), expected, ());
  TEST(VerifyDenseAssignments(loaded.m_file, fx.m_samples, fx.m_policy), ());

  // Tamper: wrong sample centres must fail verification.
  std::vector<m2::PointD> wrong = fx.m_samples;
  wrong[0] = MercatorFromLonLat(30.0, 70.0);
  TEST(!VerifyDenseAssignments(loaded.m_file, wrong, fx.m_policy), ());

  RemoveIfExists(fx.m_path);
}

UNIT_TEST(SubdivisionAssignmentTable_VersionMismatchFailClosed)
{
  auto fx = MakeNestedFixture();

  auto ok = SubdivisionAssignmentTable::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                fx.m_params.m_policyVersion);
  TEST(ok.has_value(), ());
  TEST_EQUAL(ok->LookupByHealpix(100)->m_name, "Inner", ());
  TEST_EQUAL(ok->LookupByHealpix(200), nullptr, ());
  TEST_EQUAL(ok->LookupBySlot(2)->m_name, "Outer", ());

  auto badMap = SubdivisionAssignmentTable::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion + 1,
                                                    fx.m_params.m_policyVersion);
  TEST(!badMap.has_value(), ());

  auto badPolicy = SubdivisionAssignmentTable::TryLoad(fx.m_path, fx.m_universe, fx.m_params.m_mapDataVersion,
                                                       fx.m_params.m_policyVersion + 1);
  TEST(!badPolicy.has_value(), ());

  auto badUniverseSize =
      SubdivisionAssignmentTable::TryLoad(fx.m_path, {1, 2}, fx.m_params.m_mapDataVersion, fx.m_params.m_policyVersion);
  TEST(!badUniverseSize.has_value(), ());

  auto notAscending = SubdivisionAssignmentTable::TryLoad(fx.m_path, {300, 200, 100}, fx.m_params.m_mapDataVersion,
                                                          fx.m_params.m_policyVersion);
  TEST(!notAscending.has_value(), ());

  auto missing = SubdivisionAssignmentTable::TryLoad(ExplorationSidecarPath(GetPlatform().WritableDir(), "missing_sp028"),
                                                     fx.m_universe, fx.m_params.m_mapDataVersion,
                                                     fx.m_params.m_policyVersion);
  TEST(!missing.has_value(), ());

  RemoveIfExists(fx.m_path);
}
