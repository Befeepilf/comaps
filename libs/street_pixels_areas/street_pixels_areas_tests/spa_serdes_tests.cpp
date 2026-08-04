#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/areas_reader.hpp"
#include "street_pixels_areas/areas_writer.hpp"
#include "street_pixels_areas/exploration_filter.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

#include "coding/point_coding.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include "defines.hpp"

#include <string>
#include <vector>

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

std::string SpaPath(std::string const & leaf)
{
  return base::JoinPath(GetPlatform().WritableDir(), leaf + SPA_FILE_EXTENSION);
}

void RemoveIfExists(std::string const & path)
{
  Platform::RemoveFileIfExists(path);
}

bool RingsAlmostEqual(std::vector<m2::PointD> const & a, std::vector<m2::PointD> const & b)
{
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
  {
    if (!a[i].EqualDxDy(b[i], kMwmPointAccuracy))
      return false;
  }
  return true;
}

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
}  // namespace

UNIT_TEST(SpaSerdes_RoundTripRingsAndAssignments)
{
  auto const config = FinlandConfig();
  auto const policy = config.GetByIso("FI");
  auto areas = AdmitAll(
      {
          MakeAdminCandidate(10, 10, "District", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(11, 10, "Nested", LonLatBox(24.2, 60.2, 24.8, 60.8)),
          MakeAdminCandidate(8, 8, "City", LonLatBox(23.5, 59.5, 25.5, 61.5)),
          MakePlaceCandidate(90, "neighbourhood", "Hood", LonLatBox(24.3, 60.3, 24.4, 60.4)),
      },
      policy);

  std::vector<m2::PointD> const samples = {
      MercatorFromLonLat(24.5, 60.5),
      MercatorFromLonLat(24.35, 60.35),
      MercatorFromLonLat(30.0, 70.0),
  };

  std::string const path = SpaPath("Finland_Southern Finland_Helsinki");
  RemoveIfExists(path);

  SpaWriteParams params;
  params.m_mapDataVersion = 260417;
  params.m_policyVersion = config.GetPolicyVersion();
  params.m_isoCode = "FI";
  params.m_mwmId = "Finland_Southern Finland_Helsinki";

  WriteExplorationSidecar(path, areas, samples, policy, params);
  auto const loaded = ReadExplorationSidecar(path);

  TEST_EQUAL(loaded.m_header.m_magic, kSpaMagic, ());
  TEST_EQUAL(loaded.m_header.m_formatVersion, kSpaFormatVersion, ());
  TEST_EQUAL(loaded.m_header.m_mapDataVersion, params.m_mapDataVersion, ());
  TEST_EQUAL(loaded.m_header.m_policyVersion, params.m_policyVersion, ());
  TEST_EQUAL(loaded.m_header.m_isoCode, "FI", ());
  TEST_EQUAL(loaded.m_header.m_mwmId, params.m_mwmId, ());
  TEST_EQUAL(loaded.m_header.m_areaCount, 4u, ());
  TEST_EQUAL(loaded.m_header.m_assignCount, 3u, ());
  TEST_EQUAL(loaded.m_header.m_indexWidth, 2u, ());

  TEST_EQUAL(loaded.m_areas.size(), areas.size(), ());
  for (size_t i = 0; i < areas.size(); ++i)
  {
    TEST_EQUAL(loaded.m_areas[i].m_osmId, areas[i].m_osmId, ());
    TEST_EQUAL(loaded.m_areas[i].m_role, areas[i].m_role, ());
    TEST_EQUAL(loaded.m_areas[i].m_name, areas[i].m_name, ());
    TEST_EQUAL(loaded.m_areas[i].m_adminLevel, areas[i].m_adminLevel, ());
    TEST_EQUAL(loaded.m_areas[i].m_rings.size(), areas[i].m_rings.size(), ());
    TEST(RingsAlmostEqual(loaded.m_areas[i].m_rings[0], areas[i].m_rings[0]), (i));
    TEST(AlmostEqualAbs(loaded.m_areas[i].m_area, areas[i].m_area, 1e-9), ());
  }

  uint32_t const sentinel = NoSubdivisionSentinel(loaded.m_header.m_indexWidth);
  auto const expected = BuildDenseAssignments(samples, loaded.m_areas, policy, sentinel);
  TEST_EQUAL(loaded.m_assignments, expected, ());
  TEST_EQUAL(loaded.m_assignments[2], sentinel, ());

  // Settlement is present but never referenced by assignment column.
  bool settlementSeen = false;
  for (auto const & area : loaded.m_areas)
  {
    if (area.m_role == AreaRole::Settlement)
    {
      settlementSeen = true;
      for (uint32_t value : loaded.m_assignments)
        TEST_NOT_EQUAL(value, area.m_compactIndex, ());
    }
  }
  TEST(settlementSeen, ());

  RemoveIfExists(path);
}

UNIT_TEST(SpaSerdes_NoThreeBoxOrNodeInventedInOutput)
{
  auto const policy = FinlandPolicy();
  std::vector<AreaCandidateInput> inputs = {
      MakeAdminCandidate(1, 10, "Good", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      MakeAdminCandidate(2, 10, "Box", LonLatBox(24.0, 60.0, 25.0, 61.0), GeometrySource::ThreeBoxApprox),
      MakeAdminCandidate(3, 10, "Node", LonLatBox(24.0, 60.0, 25.0, 61.0),
                         GeometrySource::PlaceNodeInvented),
  };

  std::vector<ExplorationArea> areas;
  for (auto const & input : inputs)
  {
    auto result = FilterExplorationCandidate(input, policy);
    if (result.m_reason == RejectReason::Accepted)
      areas.push_back(*result.m_area);
  }
  TEST_EQUAL(areas.size(), 1u, ());
  TEST_EQUAL(areas[0].m_osmId, 1u, ());

  std::string const path = SpaPath("filter_output");
  RemoveIfExists(path);
  SpaWriteParams params;
  params.m_mapDataVersion = 1;
  params.m_policyVersion = 1;
  params.m_isoCode = "FI";
  params.m_mwmId = "filter_output";
  WriteExplorationSidecar(path, areas, {MercatorFromLonLat(24.5, 60.5)}, policy, params);

  auto const loaded = ReadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_areas.size(), 1u, ());
  TEST_EQUAL(loaded.m_areas[0].m_name, "Good", ());
  RemoveIfExists(path);
}

UNIT_TEST(SpaSerdes_KeyingMapDataAndPolicyVersion)
{
  auto const policy = FinlandPolicy();
  auto areas = AdmitAll({MakeAdminCandidate(1, 10, "A", LonLatBox(24.0, 60.0, 25.0, 61.0))}, policy);

  std::string const path = SpaPath("keying");
  RemoveIfExists(path);
  SpaWriteParams params;
  params.m_mapDataVersion = 111;
  params.m_policyVersion = 7;
  params.m_isoCode = "FI";
  params.m_mwmId = "keying";
  WriteExplorationSidecar(path, areas, {}, policy, params);

  auto const loaded = ReadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_header.m_mapDataVersion, 111, ());
  TEST_EQUAL(loaded.m_header.m_policyVersion, 7, ());
  TEST_EQUAL(loaded.m_header.m_assignCount, 0u, ());
  RemoveIfExists(path);
}

UNIT_TEST(SpaSerdes_CompactIndexWidthUint16)
{
  auto const policy = FinlandPolicy();
  auto areas = AdmitAll({MakeAdminCandidate(1, 10, "A", LonLatBox(24.0, 60.0, 25.0, 61.0))}, policy);
  TEST_EQUAL(ChooseIndexWidth(static_cast<uint32_t>(areas.size())), 2u, ());
  TEST_EQUAL(ChooseIndexWidth(1000), 2u, ());
  TEST_EQUAL(ChooseIndexWidth(kNoSubdivisionUint16), 4u, ());
  TEST_EQUAL(NoSubdivisionSentinel(2), kNoSubdivisionUint16, ());
  TEST_EQUAL(NoSubdivisionSentinel(4), kNoSubdivisionUint32, ());

  std::string const path = SpaPath("compact");
  RemoveIfExists(path);
  SpaWriteParams params;
  params.m_mapDataVersion = 1;
  params.m_policyVersion = 1;
  params.m_isoCode = "FI";
  params.m_mwmId = "compact";
  WriteExplorationSidecar(path, areas, {MercatorFromLonLat(24.5, 60.5)}, policy, params);
  auto const loaded = ReadExplorationSidecar(path);
  TEST_EQUAL(loaded.m_header.m_indexWidth, 2u, ());
  TEST(loaded.m_assignments[0] < kNoSubdivisionUint16, ());
  RemoveIfExists(path);
}
