#include "street_pixels_areas/ownership_scoring.hpp"

#include <algorithm>
#include <cmath>

namespace street_pixels
{
double RecencyWeight(double deltaTSeconds)
{
  double const dt = std::max(0.0, deltaTSeconds);
  return std::pow(2.0, -dt / (kHalfLifeDays * kSecondsPerDay));
}

double OwnershipScore(uint64_t totalPixels, std::vector<double> const & recencyWeights)
{
  if (totalPixels == 0)
    return 0.0;
  double sum = 0.0;
  for (double const weight : recencyWeights)
    sum += weight;
  return 100.0 * sum / static_cast<double>(totalPixels);
}

double LiveCoverageFraction(uint64_t totalPixels, uint64_t uniqueLivePixels)
{
  if (totalPixels == 0)
    return 0.0;
  return static_cast<double>(uniqueLivePixels) / static_cast<double>(totalPixels);
}

EligibilityBreakdown EvaluateEligibility(uint64_t totalPixels, uint64_t uniqueLivePixels,
                                         double liveCoverageFraction, double ownershipScore)
{
  EligibilityBreakdown breakdown;
  breakdown.m_coverageMet = liveCoverageFraction >= kEligibilityLiveCoverage;
  breakdown.m_minLivePixelsMet =
      totalPixels < kEligibilityMinLivePixels || uniqueLivePixels >= kEligibilityMinLivePixels;
  breakdown.m_scoreMet = ownershipScore >= kEligibilityMinScore;
  return breakdown;
}

bool IsEligible(EligibilityBreakdown const & breakdown)
{
  return breakdown.m_coverageMet && breakdown.m_minLivePixelsMet && breakdown.m_scoreMet;
}

bool IsContested(std::vector<double> const & eligibleScores)
{
  if (eligibleScores.size() < 2)
    return false;
  double leader = 0.0;
  double runnerUp = 0.0;
  for (double const score : eligibleScores)
  {
    if (score > leader)
    {
      runnerUp = leader;
      leader = score;
    }
    else if (score > runnerUp)
      runnerUp = score;
  }
  return runnerUp >= kContestedRatio * leader;
}

bool IsUnclaimed(bool anyEligibleParticipant) { return !anyEligibleParticipant; }

void ApplyLocalCompetitionView(CompetitionAreaQuery & query)
{
  query.m_eligible = IsEligible(query.m_eligibility);
  query.m_localUnclaimed = !query.m_eligible;
  query.m_localContested = false;
  query.m_localIsBoss = query.m_eligible;
}
}  // namespace street_pixels
