#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "routing/routing_callbacks.hpp"
#include "routing/street_exploration_for_routing.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace street_exploration_avoid_test
{
using namespace routing;
using namespace routing_test;

double constexpr kLengthEpsilon = 1e-6;

Segment MakeSegment(uint32_t featureId)
{
  return Segment(kTestNumMwmId, featureId, 0, true);
}

class AvoidTestStreetExplorationWeights final : public IStreetExplorationWeights
{
public:
  AvoidTestStreetExplorationWeights(bool avoidActive, double preferStrength, std::map<Segment, double> ratios)
    : m_avoidActive(avoidActive)
    , m_preferStrength(preferStrength)
    , m_ratios(std::move(ratios))
  {}

  double GetSegmentWeightMultiplier(NumMwmIds const &, NumMwmId, Segment const & segment,
                                    RoadGeometry const &) const override
  {
    if (m_avoidActive)
      return 1.0;
    return 1.0 + m_preferStrength / 100.0 * 9.0 * GetRatio(segment);
  }

  bool IsAvoidExclusionActive() const override { return m_avoidActive; }

  bool IsSegmentExcluded(NumMwmIds const &, NumMwmId, Segment const & segment, RoadGeometry const &) const override
  {
    return m_avoidActive && GetRatio(segment) == 1.0;
  }

private:
  double GetRatio(Segment const & segment) const
  {
    auto const it = m_ratios.find(segment);
    if (it == m_ratios.end())
      return 0.0;
    return it->second;
  }

  bool m_avoidActive = false;
  double m_preferStrength = 0.0;
  std::map<Segment, double> m_ratios;
};

std::map<Segment, double> const kConnectedRatios = {{MakeSegment(0), 1.0},  {MakeSegment(1), 1.0},
                                                    {MakeSegment(2), 0.25}, {MakeSegment(3), 0.25},
                                                    {MakeSegment(4), 0.0},  {MakeSegment(5), 0.0}};

void AddConnectedEdges(TestIndexGraphTopology & graph)
{
  graph.AddDirectedEdge(0, 1, 100.0);
  graph.AddDirectedEdge(1, 4, 100.0);
  graph.AddDirectedEdge(0, 2, 150.0);
  graph.AddDirectedEdge(2, 4, 150.0);
  graph.AddDirectedEdge(0, 3, 400.0);
  graph.AddDirectedEdge(3, 4, 400.0);
}

UNIT_TEST(StreetExplorationAvoid_MixedPathKeptFullyExploredUnused)
{
  auto provider = std::make_shared<AvoidTestStreetExplorationWeights>(true, 0.0, kConnectedRatios);
  TestIndexGraphTopology graph(5, provider);
  AddConnectedEdges(graph);

  double length = 0.0;
  std::vector<TestIndexGraphTopology::Edge> path;
  TEST(graph.FindPath(0, 4, length, path), ());
  TEST_ALMOST_EQUAL_ABS(length, 300.0, kLengthEpsilon, ());
  TEST_EQUAL(path, std::vector<TestIndexGraphTopology::Edge>({{0, 2}, {2, 4}}), ());
}

UNIT_TEST(StreetExplorationAvoid_AllPathsFullyExploredIsDistinctCode)
{
  std::map<Segment, double> const ratios = {{MakeSegment(0), 0.0},
                                            {MakeSegment(1), 1.0},
                                            {MakeSegment(2), 0.5},
                                            {MakeSegment(3), 1.0}};
  auto provider = std::make_shared<AvoidTestStreetExplorationWeights>(true, 0.0, ratios);
  TestIndexGraphTopology graph(4, provider);
  graph.AddDirectedEdge(0, 1, 100.0);
  graph.AddDirectedEdge(1, 3, 100.0);
  graph.AddDirectedEdge(0, 2, 120.0);
  graph.AddDirectedEdge(2, 3, 120.0);

  double length = 0.0;
  std::vector<TestIndexGraphTopology::Edge> path;
  TEST(!graph.FindPath(0, 3, length, path), ());
  TEST_EQUAL(MapAStarNoPath(true), RouterResultCode::AvoidExploredNoRoute, ());
  TEST(MapAStarNoPath(true) != RouterResultCode::RouteNotFound, ());
}

UNIT_TEST(StreetExplorationAvoid_PreferStillUsesSoftMultiplier)
{
  auto provider = std::make_shared<AvoidTestStreetExplorationWeights>(false, 50.0, kConnectedRatios);
  TestIndexGraphTopology graph(5, provider);
  AddConnectedEdges(graph);

  double length = 0.0;
  std::vector<TestIndexGraphTopology::Edge> path;
  TEST(graph.FindPath(0, 4, length, path), ());
  TEST_ALMOST_EQUAL_ABS(length, 300.0, kLengthEpsilon, ());
  TEST_EQUAL(path, std::vector<TestIndexGraphTopology::Edge>({{0, 2}, {2, 4}}), ());

  auto provider100 = std::make_shared<AvoidTestStreetExplorationWeights>(false, 100.0, kConnectedRatios);
  TestIndexGraphTopology graph100(5, provider100);
  AddConnectedEdges(graph100);
  double length100 = 0.0;
  std::vector<TestIndexGraphTopology::Edge> path100;
  TEST(graph100.FindPath(0, 4, length100, path100), ());
  TEST_ALMOST_EQUAL_ABS(length100, 800.0, kLengthEpsilon, ());
  TEST_EQUAL(path100, std::vector<TestIndexGraphTopology::Edge>({{0, 3}, {3, 4}}), ());
}

UNIT_TEST(StreetExplorationAvoid_CarEstimatorDoesNotExclude)
{
  auto provider = std::make_shared<AvoidTestStreetExplorationWeights>(true, 0.0, kConnectedRatios);
  TestIndexGraphTopology graph(5, provider, VehicleType::Car);
  AddConnectedEdges(graph);

  double length = 0.0;
  std::vector<TestIndexGraphTopology::Edge> path;
  TEST(graph.FindPath(0, 4, length, path), ());
  TEST_ALMOST_EQUAL_ABS(length, 200.0, kLengthEpsilon, ());
  TEST_EQUAL(path, std::vector<TestIndexGraphTopology::Edge>({{0, 1}, {1, 4}}), ());
}

UNIT_TEST(ConvertResult_AvoidNoPathIsAvoidExploredNoRoute)
{
  TEST_EQUAL(MapAStarNoPath(true), RouterResultCode::AvoidExploredNoRoute, ());
}

UNIT_TEST(ConvertResult_CarOrInactiveAvoidNoPathIsRouteNotFound)
{
  TEST_EQUAL(MapAStarNoPath(false), RouterResultCode::RouteNotFound, ());
}

UNIT_TEST(ToString_AvoidExploredNoRoute)
{
  TEST_EQUAL(ToString(RouterResultCode::AvoidExploredNoRoute), "AvoidExploredNoRoute", ());
}
}  // namespace street_exploration_avoid_test
