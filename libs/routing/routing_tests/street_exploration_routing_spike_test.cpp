#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "routing/street_exploration_for_routing.hpp"

#include "base/timer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace street_exploration_routing_spike
{
using namespace routing;
using namespace routing_test;

size_t constexpr kTimedRuns = 101;
double constexpr kLengthEpsilon = 1e-6;

Segment MakeSegment(uint32_t featureId)
{
  return Segment(kTestNumMwmId, featureId, 0, true);
}

class SpikeStreetExplorationWeights final : public IStreetExplorationWeights
{
public:
  enum class Mode
  {
    Prefer,
    Avoid
  };

  SpikeStreetExplorationWeights(Mode mode, double strength, std::map<Segment, double> ratios)
    : m_mode(mode)
    , m_strength(strength)
    , m_ratios(std::move(ratios))
  {}

  double GetSegmentWeightMultiplier(NumMwmIds const &, NumMwmId, Segment const & segment,
                                    RoadGeometry const &) const override
  {
    ++m_lookupCount;
    if (m_mode == Mode::Avoid)
      return 1.0;
    return 1.0 + m_strength / 100.0 * 9.0 * GetRatio(segment);
  }

  bool IsAvoidExclusionActive() const override { return m_mode == Mode::Avoid; }

  bool IsSegmentExcluded(NumMwmIds const &, NumMwmId, Segment const & segment, RoadGeometry const &) const override
  {
    return IsExcluded(segment);
  }

  bool IsExcluded(Segment const & segment) const { return GetRatio(segment) == 1.0; }

  void ResetLookupCount() const { m_lookupCount = 0; }
  size_t GetLookupCount() const { return m_lookupCount; }

private:
  double GetRatio(Segment const & segment) const
  {
    auto const it = m_ratios.find(segment);
    if (it == m_ratios.end())
      return 0.0;
    return it->second;
  }

  Mode m_mode;
  double m_strength;
  std::map<Segment, double> m_ratios;
  mutable size_t m_lookupCount = 0;
};

enum class SpikeMode
{
  Standard,
  Prefer50,
  Prefer100,
  Avoid
};

struct EdgeDefinition
{
  TestIndexGraphTopology::Vertex m_from;
  TestIndexGraphTopology::Vertex m_to;
  double m_weight;
};

struct BenchmarkResult
{
  bool m_found = false;
  double m_length = 0.0;
  uint64_t m_medianUs = 0;
  uint64_t m_p95Us = 0;
  size_t m_lookupCalls = 0;
};

std::shared_ptr<SpikeStreetExplorationWeights> MakeProvider(SpikeMode mode,
                                                            std::map<Segment, double> const & ratios)
{
  if (mode == SpikeMode::Standard)
    return nullptr;
  if (mode == SpikeMode::Avoid)
    return std::make_shared<SpikeStreetExplorationWeights>(SpikeStreetExplorationWeights::Mode::Avoid, 0.0, ratios);
  double const strength = mode == SpikeMode::Prefer50 ? 50.0 : 100.0;
  return std::make_shared<SpikeStreetExplorationWeights>(SpikeStreetExplorationWeights::Mode::Prefer, strength, ratios);
}

void AddEdges(TestIndexGraphTopology & graph, std::vector<EdgeDefinition> const & edges,
              std::shared_ptr<SpikeStreetExplorationWeights> const & exclusionProvider)
{
  for (size_t i = 0; i < edges.size(); ++i)
  {
    auto const & edge = edges[i];
    graph.AddDirectedEdge(edge.m_from, edge.m_to, edge.m_weight);
    if (exclusionProvider && exclusionProvider->IsExcluded(MakeSegment(static_cast<uint32_t>(i))))
      graph.SetEdgeAccess(edge.m_from, edge.m_to, RoadAccess::Type::No);
  }
}

void CheckPath(bool found, double length, std::vector<TestIndexGraphTopology::Edge> const & path,
               bool expectedFound, double expectedLength,
               std::vector<TestIndexGraphTopology::Edge> const & expectedPath)
{
  TEST_EQUAL(found, expectedFound, ());
  if (!expectedFound)
    return;
  TEST_ALMOST_EQUAL_ABS(length, expectedLength, kLengthEpsilon, ());
  TEST_EQUAL(path, expectedPath, ());
}

BenchmarkResult RunFixture(uint32_t numVertices, TestIndexGraphTopology::Vertex finish,
                           std::vector<EdgeDefinition> const & edges, std::map<Segment, double> const & ratios,
                           SpikeMode mode, bool expectedFound, double expectedLength,
                           std::vector<TestIndexGraphTopology::Edge> const & expectedPath)
{
  auto provider = MakeProvider(mode, ratios);
  TestIndexGraphTopology graph(numVertices, provider);
  AddEdges(graph, edges, mode == SpikeMode::Avoid ? provider : nullptr);

  double warmupLength = 0.0;
  std::vector<TestIndexGraphTopology::Edge> warmupPath;
  bool const warmupFound = graph.FindPath(0, finish, warmupLength, warmupPath);
  CheckPath(warmupFound, warmupLength, warmupPath, expectedFound, expectedLength, expectedPath);

  if (provider)
    provider->ResetLookupCount();

  std::vector<uint64_t> timingsUs;
  timingsUs.reserve(kTimedRuns);
  for (size_t i = 0; i < kTimedRuns; ++i)
  {
    double length = 0.0;
    std::vector<TestIndexGraphTopology::Edge> path;
    base::HighResTimer timer;
    bool const found = graph.FindPath(0, finish, length, path);
    timingsUs.push_back(timer.TimeElapsedAs<std::chrono::microseconds>().count());

    TEST_EQUAL(found, warmupFound, ());
    if (found)
    {
      TEST_ALMOST_EQUAL_ABS(length, warmupLength, kLengthEpsilon, ());
      TEST_EQUAL(path, warmupPath, ());
    }
  }

  std::sort(timingsUs.begin(), timingsUs.end());
  size_t const p95Rank = (95 * timingsUs.size() + 99) / 100;

  BenchmarkResult result;
  result.m_found = warmupFound;
  result.m_length = warmupLength;
  result.m_medianUs = timingsUs[timingsUs.size() / 2];
  result.m_p95Us = timingsUs[p95Rank - 1];
  if (provider)
    result.m_lookupCalls = provider->GetLookupCount();
  return result;
}

void PrintResult(std::string const & fixture, std::string const & mode, BenchmarkResult const & result,
                 uint64_t standardMedianUs)
{
  int64_t const extraUs = static_cast<int64_t>(result.m_medianUs) - static_cast<int64_t>(standardMedianUs);
  std::cout << "SP054_RESULT fixture=" << fixture << " mode=" << mode
            << " status=" << (result.m_found ? "OK" : "NoPath") << " length_m=";
  if (result.m_found)
    std::cout << result.m_length;
  else
    std::cout << "NA";
  std::cout << " median_us=" << result.m_medianUs << " p95_us=" << result.m_p95Us << " extra_us=" << extraUs
            << " lookup_calls=" << result.m_lookupCalls << std::endl;
}

UNIT_TEST(StreetExplorationRoutingSpike_AvoidExclusionBoundary)
{
  std::map<Segment, double> const ratios = {{MakeSegment(0), 1.0}, {MakeSegment(1), 0.999}};
  SpikeStreetExplorationWeights provider(SpikeStreetExplorationWeights::Mode::Avoid, 0.0, ratios);
  TEST(provider.IsExcluded(MakeSegment(0)), ());
  TEST(!provider.IsExcluded(MakeSegment(1)), ());
}

UNIT_TEST(StreetExplorationRoutingSpike_Connected)
{
  std::vector<EdgeDefinition> const edges = {
      {0, 1, 100.0}, {1, 4, 100.0}, {0, 2, 150.0}, {2, 4, 150.0}, {0, 3, 400.0}, {3, 4, 400.0}};
  std::map<Segment, double> const ratios = {{MakeSegment(0), 1.0},  {MakeSegment(1), 1.0},
                                            {MakeSegment(2), 0.25}, {MakeSegment(3), 0.25},
                                            {MakeSegment(4), 0.0},  {MakeSegment(5), 0.0}};

  auto const standard =
      RunFixture(5, 4, edges, ratios, SpikeMode::Standard, true, 200.0, {{0, 1}, {1, 4}});
  PrintResult("connected", "standard", standard, standard.m_medianUs);

  auto const prefer50 =
      RunFixture(5, 4, edges, ratios, SpikeMode::Prefer50, true, 300.0, {{0, 2}, {2, 4}});
  PrintResult("connected", "prefer_50", prefer50, standard.m_medianUs);

  auto const prefer100 =
      RunFixture(5, 4, edges, ratios, SpikeMode::Prefer100, true, 800.0, {{0, 3}, {3, 4}});
  PrintResult("connected", "prefer_100", prefer100, standard.m_medianUs);

  auto const avoid = RunFixture(5, 4, edges, ratios, SpikeMode::Avoid, true, 300.0, {{0, 2}, {2, 4}});
  PrintResult("connected", "avoid", avoid, standard.m_medianUs);
}

UNIT_TEST(StreetExplorationRoutingSpike_ForcedCut)
{
  std::vector<EdgeDefinition> const edges = {{0, 1, 100.0}, {1, 3, 100.0}, {0, 2, 120.0}, {2, 3, 120.0}};
  std::map<Segment, double> const ratios = {{MakeSegment(0), 0.0},
                                            {MakeSegment(1), 1.0},
                                            {MakeSegment(2), 0.5},
                                            {MakeSegment(3), 1.0}};

  auto const standard =
      RunFixture(4, 3, edges, ratios, SpikeMode::Standard, true, 200.0, {{0, 1}, {1, 3}});
  PrintResult("forced_cut", "standard", standard, standard.m_medianUs);

  auto const avoid = RunFixture(4, 3, edges, ratios, SpikeMode::Avoid, false, 0.0, {});
  PrintResult("forced_cut", "avoid", avoid, standard.m_medianUs);
}
}
