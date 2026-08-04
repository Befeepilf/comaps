#include "testing/testing.hpp"

#include "street_pixels_areas/street_pixels_areas_tests/test_helpers.hpp"

#include "street_pixels_areas/exploration_filter.hpp"

#include "street_pixels_config/country_config.hpp"

namespace
{
using namespace street_pixels;
using namespace street_pixels::test_helpers;

ExplorationArea MustAdmit(AreaCandidateInput const & input, CountryPolicy const & policy)
{
  auto result = FilterExplorationCandidate(input, policy);
  TEST_EQUAL(result.m_reason, RejectReason::Accepted, (DebugPrint(result.m_reason)));
  TEST(result.m_area.has_value(), ());
  return *result.m_area;
}
}  // namespace

UNIT_TEST(ExplorationFilter_AdmitTrueClosedNamedSubdivision)
{
  auto const policy = FinlandPolicy();
  auto const area =
      MustAdmit(MakeAdminCandidate(100, 10, "Kallio", LonLatBox(24.9, 60.1, 25.0, 60.2)), policy);
  TEST_EQUAL(area.m_role, AreaRole::Subdivision, ());
  TEST_EQUAL(area.m_osmId, 100u, ());
  TEST_EQUAL(area.m_adminLevel, 10, ());
  TEST_EQUAL(area.m_rings.size(), 1u, ());
  TEST(area.m_area > 0.0, ());
}

UNIT_TEST(ExplorationFilter_RejectThreeBox)
{
  auto const policy = FinlandPolicy();
  auto input = MakeAdminCandidate(101, 10, "Boxy", LonLatBox(24.9, 60.1, 25.0, 60.2),
                                  GeometrySource::ThreeBoxApprox);
  auto const result = FilterExplorationCandidate(input, policy);
  TEST_EQUAL(result.m_reason, RejectReason::ThreeBox, ());
  TEST(!result.m_area.has_value(), ());
}

UNIT_TEST(ExplorationFilter_RejectPlaceNodeInvented)
{
  auto const policy = FinlandPolicy();
  auto input = MakeAdminCandidate(102, 10, "Invented", LonLatBox(24.9, 60.1, 25.0, 60.2),
                                  GeometrySource::PlaceNodeInvented);
  auto const result = FilterExplorationCandidate(input, policy);
  TEST_EQUAL(result.m_reason, RejectReason::PlaceNodeInvented, ());
  TEST(!result.m_area.has_value(), ());
}

UNIT_TEST(ExplorationFilter_RejectUnnamed)
{
  auto const policy = FinlandPolicy();
  auto input = MakeAdminCandidate(103, 10, "   ", LonLatBox(24.9, 60.1, 25.0, 60.2));
  auto const result = FilterExplorationCandidate(input, policy);
  TEST_EQUAL(result.m_reason, RejectReason::Unnamed, ());
}

UNIT_TEST(ExplorationFilter_PolicyFilterAdminLevels)
{
  auto const policy = FinlandPolicy();

  auto ok10 = FilterExplorationCandidate(
      MakeAdminCandidate(201, 10, "District", LonLatBox(24.9, 60.1, 25.0, 60.2)), policy);
  TEST_EQUAL(ok10.m_reason, RejectReason::Accepted, ());
  TEST_EQUAL(ok10.m_area->m_role, AreaRole::Subdivision, ());

  auto ok8 = FilterExplorationCandidate(
      MakeAdminCandidate(202, 8, "City", LonLatBox(24.8, 60.0, 25.1, 60.3)), policy);
  TEST_EQUAL(ok8.m_reason, RejectReason::Accepted, ());
  TEST_EQUAL(ok8.m_area->m_role, AreaRole::Settlement, ());

  auto bad7 = FilterExplorationCandidate(
      MakeAdminCandidate(203, 7, "County", LonLatBox(24.0, 60.0, 26.0, 61.0)), policy);
  TEST_EQUAL(bad7.m_reason, RejectReason::PolicyMismatch, ());

  auto placeOk = FilterExplorationCandidate(
      MakePlaceCandidate(204, "neighbourhood", "Punavuori", LonLatBox(24.93, 60.16, 24.95, 60.17)), policy);
  TEST_EQUAL(placeOk.m_reason, RejectReason::Accepted, ());
  TEST_EQUAL(placeOk.m_area->m_role, AreaRole::PlaceBoundary, ());

  auto placeBad = FilterExplorationCandidate(
      MakePlaceCandidate(205, "city", "NotAllowed", LonLatBox(24.93, 60.16, 24.95, 60.17)), policy);
  TEST_EQUAL(placeBad.m_reason, RejectReason::PolicyMismatch, ());
}

UNIT_TEST(ExplorationFilter_NamedOnlyAndUnconfigured)
{
  auto const policy = FinlandPolicy();
  auto unnamed = FilterExplorationCandidate(
      MakeAdminCandidate(301, 10, "", LonLatBox(24.9, 60.1, 25.0, 60.2)), policy);
  TEST_EQUAL(unnamed.m_reason, RejectReason::Unnamed, ());

  auto const & unconfigured = CountryConfig::UnconfiguredPolicy();
  auto rejected = FilterExplorationCandidate(
      MakeAdminCandidate(302, 10, "Anywhere", LonLatBox(24.9, 60.1, 25.0, 60.2)), unconfigured);
  TEST_EQUAL(rejected.m_reason, RejectReason::UnconfiguredCountry, ());
}
