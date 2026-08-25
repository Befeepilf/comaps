#include "testing/testing.hpp"

#include "street_pixels_areas/ownership_scoring.hpp"

#include <vector>

namespace
{
using namespace street_pixels;

double constexpr kWeightEps = 1e-12;
double constexpr kScoreEps = 1e-9;
}  // namespace

UNIT_TEST(OwnershipScoring_RecencyWeightDecayTable)
{
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(0.0), 1.0, kWeightEps, ());
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(kHalfLifeDays * kSecondsPerDay), 0.5, kWeightEps, ());
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(2.0 * kHalfLifeDays * kSecondsPerDay), 0.25, kWeightEps, ());
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(3.0 * kHalfLifeDays * kSecondsPerDay), 0.125, kWeightEps, ());
}

UNIT_TEST(OwnershipScoring_RecencyWeightClampsNegativeDelta)
{
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(-100.0), 1.0, kWeightEps, ());
}

UNIT_TEST(OwnershipScoring_RevisitRestoresFullWeight)
{
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(2.0 * kHalfLifeDays * kSecondsPerDay), 0.25, kWeightEps, ());
  TEST_ALMOST_EQUAL_ABS(RecencyWeight(0.0), 1.0, kWeightEps, ());
}

UNIT_TEST(OwnershipScoring_JustVisitedFullLiveIs100)
{
  std::vector<double> const weights(10, 1.0);
  TEST_ALMOST_EQUAL_ABS(OwnershipScore(10, weights), 100.0, kScoreEps, ());
}

UNIT_TEST(OwnershipScoring_ZeroTotalIsZero)
{
  TEST_ALMOST_EQUAL_ABS(OwnershipScore(0, {1.0, 1.0}), 0.0, kScoreEps, ());
}

UNIT_TEST(OwnershipScoring_ImportedOnlyEmptyLiveIsZero)
{
  TEST_ALMOST_EQUAL_ABS(OwnershipScore(20, {}), 0.0, kScoreEps, ());
}

UNIT_TEST(OwnershipScoring_MissingTimestampContributesZeroToSum)
{
  std::vector<double> const weights = {1.0, 0.0};
  TEST_ALMOST_EQUAL_ABS(OwnershipScore(2, weights), 50.0, kScoreEps, ());
}

UNIT_TEST(OwnershipScoring_EligibilityCoverageFailsIndependently)
{
  uint64_t constexpr kTotal = 10000;
  uint64_t constexpr kUniqueLive = 50;
  double const coverage = LiveCoverageFraction(kTotal, kUniqueLive);
  TEST_LESS(coverage, kEligibilityLiveCoverage, ());
  double const score = OwnershipScore(kTotal, std::vector<double>(kUniqueLive, 1.0));
  TEST(score >= kEligibilityMinScore, ());
  auto const breakdown = EvaluateEligibility(kTotal, kUniqueLive, coverage, score);
  TEST(!breakdown.m_coverageMet, ());
  TEST(breakdown.m_minLivePixelsMet, ());
  TEST(breakdown.m_scoreMet, ());
  TEST(!IsEligible(breakdown), ());
}

UNIT_TEST(OwnershipScoring_EligibilityUniqueLiveFailsIndependently)
{
  uint64_t constexpr kTotal = 50;
  uint64_t constexpr kUniqueLive = 49;
  double const coverage = LiveCoverageFraction(kTotal, kUniqueLive);
  TEST(coverage >= kEligibilityLiveCoverage, ());
  double const score = OwnershipScore(kTotal, std::vector<double>(kUniqueLive, 1.0));
  TEST(score >= kEligibilityMinScore, ());
  auto const breakdown = EvaluateEligibility(kTotal, kUniqueLive, coverage, score);
  TEST(breakdown.m_coverageMet, ());
  TEST(!breakdown.m_minLivePixelsMet, ());
  TEST(breakdown.m_scoreMet, ());
  TEST(!IsEligible(breakdown), ());
}

UNIT_TEST(OwnershipScoring_EligibilityScoreFailsIndependently)
{
  uint64_t constexpr kTotal = 100;
  uint64_t constexpr kUniqueLive = 50;
  double const coverage = LiveCoverageFraction(kTotal, kUniqueLive);
  TEST(coverage >= kEligibilityLiveCoverage, ());
  double constexpr kScore = 0.499;
  auto const breakdown = EvaluateEligibility(kTotal, kUniqueLive, coverage, kScore);
  TEST(breakdown.m_coverageMet, ());
  TEST(breakdown.m_minLivePixelsMet, ());
  TEST(!breakdown.m_scoreMet, ());
  TEST(!IsEligible(breakdown), ());
}

UNIT_TEST(OwnershipScoring_EligibilityScoreBoundary)
{
  uint64_t constexpr kTotal = 100;
  uint64_t constexpr kUniqueLive = 50;
  double const coverage = LiveCoverageFraction(kTotal, kUniqueLive);
  auto const below = EvaluateEligibility(kTotal, kUniqueLive, coverage, 0.499);
  TEST(!below.m_scoreMet, ());
  TEST(!IsEligible(below), ());
  auto const at = EvaluateEligibility(kTotal, kUniqueLive, coverage, 0.5);
  TEST(at.m_scoreMet, ());
  TEST(IsEligible(at), ());
}

UNIT_TEST(OwnershipScoring_EligibilityCoverageBoundary)
{
  uint64_t constexpr kTotal = 100;
  uint64_t constexpr kUniqueLive = 50;
  double const score = OwnershipScore(kTotal, std::vector<double>(kUniqueLive, 1.0));
  auto const below = EvaluateEligibility(kTotal, kUniqueLive, 0.01999, score);
  TEST(!below.m_coverageMet, ());
  TEST(!IsEligible(below), ());
  auto const at = EvaluateEligibility(kTotal, kUniqueLive, 0.02, score);
  TEST(at.m_coverageMet, ());
  TEST(IsEligible(at), ());
}

UNIT_TEST(OwnershipScoring_SmallAreaWaivesMinLivePixels)
{
  uint64_t constexpr kTotal = 49;
  uint64_t constexpr kUniqueLive = 1;
  double const coverage = LiveCoverageFraction(kTotal, kUniqueLive);
  TEST(coverage >= kEligibilityLiveCoverage, ());
  double const score = OwnershipScore(kTotal, {1.0});
  TEST(score >= kEligibilityMinScore, ());
  auto const breakdown = EvaluateEligibility(kTotal, kUniqueLive, coverage, score);
  TEST(breakdown.m_coverageMet, ());
  TEST(breakdown.m_minLivePixelsMet, ());
  TEST(breakdown.m_scoreMet, ());
  TEST(IsEligible(breakdown), ());
}

UNIT_TEST(OwnershipScoring_FiftyPixelAreaDoesNotWaiveMinLive)
{
  uint64_t constexpr kTotal = 50;
  uint64_t constexpr kUniqueLive = 49;
  double const coverage = LiveCoverageFraction(kTotal, kUniqueLive);
  double const score = OwnershipScore(kTotal, std::vector<double>(kUniqueLive, 1.0));
  auto const breakdown = EvaluateEligibility(kTotal, kUniqueLive, coverage, score);
  TEST(breakdown.m_coverageMet, ());
  TEST(!breakdown.m_minLivePixelsMet, ());
  TEST(breakdown.m_scoreMet, ());
  TEST(!IsEligible(breakdown), ());
}

UNIT_TEST(OwnershipScoring_ContestedAtEightyPercent)
{
  TEST(IsContested({100.0, 80.0}), ());
  TEST(!IsContested({100.0, 79.0}), ());
}

UNIT_TEST(OwnershipScoring_UnclaimedWhenNoEligible)
{
  TEST(IsUnclaimed(false), ());
  TEST(!IsUnclaimed(true), ());
}

UNIT_TEST(OwnershipScoring_OneEligibleIsNotContested)
{
  TEST(!IsContested({12.0}), ());
  TEST(!IsContested({}), ());
}

UNIT_TEST(OwnershipScoring_LocalViewUnclaimedAndNeverContested)
{
  CompetitionAreaQuery query;
  query.m_eligibility = EvaluateEligibility(1, 1, 1.0, 100.0);
  ApplyLocalCompetitionView(query);
  TEST(query.m_eligible, ());
  TEST(!query.m_localUnclaimed, ());
  TEST(!query.m_localContested, ());
  TEST(query.m_localIsBoss, ());

  CompetitionAreaQuery empty;
  ApplyLocalCompetitionView(empty);
  TEST(!empty.m_eligible, ());
  TEST(empty.m_localUnclaimed, ());
  TEST(!empty.m_localContested, ());
  TEST(!empty.m_localIsBoss, ());
  TEST_EQUAL(empty.m_localUnclaimed, !empty.m_eligible, ());
}
