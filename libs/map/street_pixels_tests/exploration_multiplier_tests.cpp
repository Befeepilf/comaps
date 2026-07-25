#include "testing/testing.hpp"

#include "routing/routing_options.hpp"

namespace
{
double ExplorationMultiplier(double strength, double exploredRatio)
{
  double const normalized = strength / routing::StreetExplorationRoutingOptions::kMaxStrength;
  double constexpr kMaxExplorationPenalty = 9.0;
  return 1.0 + normalized * kMaxExplorationPenalty * exploredRatio;
}

double constexpr kEps = 1e-12;
}  // namespace

UNIT_TEST(ExplorationMultiplier_RatioZeroYieldsOne)
{
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(100.0, 0.0), 1.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(50.0, 0.0), 1.0, kEps, ());
}

UNIT_TEST(ExplorationMultiplier_MaxStrengthFullRatioYieldsTen)
{
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(100.0, 1.0), 10.0, kEps, ());
}

UNIT_TEST(ExplorationMultiplier_MaxStrengthHalfRatioYieldsFivePointFive)
{
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(100.0, 0.5), 5.5, kEps, ());
}

UNIT_TEST(ExplorationMultiplier_ZeroStrengthYieldsOneRegardlessOfRatio)
{
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(0.0, 0.0), 1.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(0.0, 0.5), 1.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(ExplorationMultiplier(0.0, 1.0), 1.0, kEps, ());
}
