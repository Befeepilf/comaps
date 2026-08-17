#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/routing_callbacks.hpp"
#include "routing/street_exploration_for_routing.hpp"

#include "testing/testing.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace street_exploration_avoid_follow_test
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
  AvoidTestStreetExplorationWeights(bool avoidActive, std::map<Segment, double> ratios)
    : m_avoidActive(avoidActive)
    , m_ratios(std::move(ratios))
  {}

  double GetSegmentWeightMultiplier(NumMwmIds const &, NumMwmId, Segment const &, RoadGeometry const &) const override
  {
    return 1.0;
  }

  bool IsAvoidExclusionActive() const override { return m_avoidActive; }

  bool IsSegmentExcluded(NumMwmIds const &, NumMwmId, Segment const & segment, RoadGeometry const &) const override
  {
    return m_avoidActive && GetRatio(segment) == 1.0;
  }

  void SetRatios(std::map<Segment, double> ratios) { m_ratios = std::move(ratios); }

private:
  double GetRatio(Segment const & segment) const
  {
    auto const it = m_ratios.find(segment);
    if (it == m_ratios.end())
      return 0.0;
    return it->second;
  }

  bool m_avoidActive = false;
  std::map<Segment, double> m_ratios;
};

std::map<Segment, double> const kMixedRatios = {{MakeSegment(0), 1.0},  {MakeSegment(1), 1.0},
                                                {MakeSegment(2), 0.25}, {MakeSegment(3), 0.25},
                                                {MakeSegment(4), 0.0},  {MakeSegment(5), 0.0}};

std::map<Segment, double> MakeAllExploredRatios()
{
  std::map<Segment, double> ratios;
  for (uint32_t i = 0; i < 6; ++i)
    ratios[MakeSegment(i)] = 1.0;
  return ratios;
}

void AddConnectedEdges(TestIndexGraphTopology & graph)
{
  graph.AddDirectedEdge(0, 1, 100.0);
  graph.AddDirectedEdge(1, 4, 100.0);
  graph.AddDirectedEdge(0, 2, 150.0);
  graph.AddDirectedEdge(2, 4, 150.0);
  graph.AddDirectedEdge(0, 3, 400.0);
  graph.AddDirectedEdge(3, 4, 400.0);
}

UNIT_TEST(AvoidFollowStability_ResearchAfterPaintingRemainingAbandonsPath)
{
  auto provider = std::make_shared<AvoidTestStreetExplorationWeights>(true, kMixedRatios);
  TestIndexGraphTopology graph(5, provider);
  AddConnectedEdges(graph);

  double length = 0.0;
  std::vector<TestIndexGraphTopology::Edge> path;
  TEST(graph.FindPath(0, 4, length, path), ());
  TEST_ALMOST_EQUAL_ABS(length, 300.0, kLengthEpsilon, ());
  TEST_EQUAL(path, std::vector<TestIndexGraphTopology::Edge>({{0, 2}, {2, 4}}), ());

  provider->SetRatios(MakeAllExploredRatios());
  TEST(!graph.FindPath(0, 4, length, path), ());
  TEST_EQUAL(MapAStarNoPath(true), RouterResultCode::AvoidExploredNoRoute, ());
}
}  // namespace street_exploration_avoid_follow_test
