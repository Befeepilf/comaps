#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/subdivision_assigner.hpp"

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

std::vector<ExplorationArea> BuildAreas(std::vector<AreaCandidateInput> const & inputs,
                                         CountryPolicy const & policy)
{
  std::vector<ExplorationArea> areas;
  uint32_t index = 0;
  for (auto const & input : inputs)
  {
    auto result = FilterExplorationCandidate(input, policy);
    TEST_EQUAL(result.m_reason, RejectReason::Accepted, (DebugPrint(result.m_reason), input.m_name));
    result.m_area->m_compactIndex = index++;
    areas.push_back(*result.m_area);
  }
  return areas;
}
}  // namespace

UNIT_TEST(SubdivisionAssigner_PriorityPrefersConfiguredOrder)
{
  auto const policy = FinlandPolicy();
  // Nested: admin_11 (lower priority) inside admin_10 (higher priority for FI).
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(10, 10, "District10", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(11, 11, "District11", LonLatBox(24.2, 60.2, 24.8, 60.8)),
      },
      policy);

  auto const pt = MercatorFromLonLat(24.5, 60.5);
  uint32_t const sentinel = kNoSubdivisionUint16;
  TEST_EQUAL(AssignSubdivision(pt, areas, policy, sentinel), areas[0].m_compactIndex, ());
}

UNIT_TEST(SubdivisionAssigner_NestedSmallestAtSamePriority)
{
  auto const policy = FinlandPolicy();
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(20, 10, "Outer", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(21, 10, "Inner", LonLatBox(24.3, 60.3, 24.7, 60.7)),
      },
      policy);

  auto const pt = MercatorFromLonLat(24.5, 60.5);
  uint32_t const sentinel = kNoSubdivisionUint16;
  TEST_EQUAL(AssignSubdivision(pt, areas, policy, sentinel), areas[1].m_compactIndex, ());
  TEST_LESS(areas[1].m_area, areas[0].m_area, ());
}

UNIT_TEST(SubdivisionAssigner_TieBreakStableOsmId)
{
  auto const policy = FinlandPolicy();
  // Identical boxes → equal area; lower OSM id wins.
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(200, 10, "B", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(100, 10, "A", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      policy);

  auto const pt = MercatorFromLonLat(24.5, 60.5);
  uint32_t const sentinel = kNoSubdivisionUint16;
  TEST_EQUAL(AssignSubdivision(pt, areas, policy, sentinel), areas[1].m_compactIndex, ());
  TEST_EQUAL(areas[1].m_osmId, 100u, ());
}

UNIT_TEST(SubdivisionAssigner_SentinelWhenNoSubdivision)
{
  auto const policy = FinlandPolicy();
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(80, 8, "SettlementOnly", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      policy);

  auto const inside = MercatorFromLonLat(24.5, 60.5);
  auto const outside = MercatorFromLonLat(30.0, 70.0);
  uint32_t const sentinel = kNoSubdivisionUint16;

  TEST_EQUAL(AssignSubdivision(inside, areas, policy, sentinel), sentinel, ());
  TEST_EQUAL(AssignSubdivision(outside, areas, policy, sentinel), sentinel, ());
  TEST(!areas[0].IsAssignable(), ());
}

UNIT_TEST(SubdivisionAssigner_PlaceAfterAllAdminLevels)
{
  auto const policy = FinlandPolicy();
  // Point inside both admin_11 (lowest FI admin priority) and a place ring.
  // §8.3: place only when no suitable admin subdivision exists — any configured
  // admin containment wins over place.
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(11, 11, "FineAdmin", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakePlaceCandidate(90, "neighbourhood", "Hood", LonLatBox(24.2, 60.2, 24.8, 60.8)),
      },
      policy);

  auto const pt = MercatorFromLonLat(24.5, 60.5);
  uint32_t const sentinel = kNoSubdivisionUint16;
  TEST_EQUAL(AssignSubdivision(pt, areas, policy, sentinel), areas[0].m_compactIndex, ());
}

UNIT_TEST(SubdivisionAssigner_PlaceWhenNoAdminContains)
{
  auto const policy = FinlandPolicy();
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(10, 10, "Elsewhere", LonLatBox(20.0, 60.0, 21.0, 61.0)),
          MakePlaceCandidate(91, "suburb", "OnlyPlace", LonLatBox(24.0, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(8, 8, "Town", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      policy);

  auto const pt = MercatorFromLonLat(24.5, 60.5);
  uint32_t const sentinel = kNoSubdivisionUint16;
  TEST_EQUAL(AssignSubdivision(pt, areas, policy, sentinel), areas[1].m_compactIndex, ());
}

UNIT_TEST(SubdivisionAssigner_DeterminismAndDenseBuild)
{
  auto const policy = FinlandPolicy();
  auto areas = BuildAreas(
      {
          MakeAdminCandidate(1, 10, "Left", LonLatBox(24.0, 60.0, 24.5, 61.0)),
          MakeAdminCandidate(2, 10, "Right", LonLatBox(24.5, 60.0, 25.0, 61.0)),
          MakeAdminCandidate(3, 8, "Town", LonLatBox(24.0, 60.0, 25.0, 61.0)),
      },
      policy);

  std::vector<m2::PointD> const points = {
      MercatorFromLonLat(24.2, 60.5),
      MercatorFromLonLat(24.7, 60.5),
      MercatorFromLonLat(26.0, 60.5),
  };
  uint32_t const sentinel = kNoSubdivisionUint16;
  auto const a = BuildDenseAssignments(points, areas, policy, sentinel);
  auto const b = BuildDenseAssignments(points, areas, policy, sentinel);
  TEST_EQUAL(a, b, ());
  TEST_EQUAL(a.size(), 3u, ());
  TEST_EQUAL(a[0], areas[0].m_compactIndex, ());
  TEST_EQUAL(a[1], areas[1].m_compactIndex, ());
  TEST_EQUAL(a[2], sentinel, ());
}
