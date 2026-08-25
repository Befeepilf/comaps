#pragma once

#include <cstdint>
#include <vector>

namespace street_pixels
{
double constexpr kHalfLifeDays = 30.0;
double constexpr kSecondsPerDay = 86400.0;
double constexpr kEligibilityLiveCoverage = 0.02;
uint64_t constexpr kEligibilityMinLivePixels = 50;
double constexpr kEligibilityMinScore = 0.5;
double constexpr kContestedRatio = 0.80;
int constexpr kScoreCalcVersion = 1;

struct EligibilityBreakdown
{
  bool m_coverageMet = false;
  bool m_minLivePixelsMet = false;
  bool m_scoreMet = false;
};

struct CompetitionAreaQuery
{
  uint64_t m_osmId = 0;
  int64_t m_mapDataVersion = 0;
  int m_scoreCalcVersion = kScoreCalcVersion;
  uint64_t m_totalPixels = 0;
  uint64_t m_uniqueLivePixels = 0;
  double m_liveCoverageFraction = 0.0;
  double m_liveCoveragePct = 0.0;
  double m_ownershipScore = 0.0;
  int64_t m_lastLocalUpdateUnix = 0;
  EligibilityBreakdown m_eligibility;
  bool m_eligible = false;
  bool m_localUnclaimed = true;
  bool m_localContested = false;
  bool m_localIsBoss = false;
};

double RecencyWeight(double deltaTSeconds);
double OwnershipScore(uint64_t totalPixels, std::vector<double> const & recencyWeights);
double LiveCoverageFraction(uint64_t totalPixels, uint64_t uniqueLivePixels);

EligibilityBreakdown EvaluateEligibility(uint64_t totalPixels, uint64_t uniqueLivePixels,
                                         double liveCoverageFraction, double ownershipScore);
bool IsEligible(EligibilityBreakdown const & breakdown);
bool IsContested(std::vector<double> const & eligibleScores);
bool IsUnclaimed(bool anyEligibleParticipant);

void ApplyLocalCompetitionView(CompetitionAreaQuery & query);
}  // namespace street_pixels
