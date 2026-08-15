#include "testing/testing.hpp"

#include "map/street_pixels_file.hpp"
#include "map/street_pixels_manager.hpp"
#include "map/street_pixels_tests/street_pixels_test_helpers.hpp"

#include "routing/geometry.hpp"
#include "routing/routing_options.hpp"
#include "routing/segment.hpp"

#include "indexer/data_source.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

using namespace street_pixels_tests;

namespace
{
std::string_view constexpr kModeKey = "street_exploration_routing_mode";
std::string_view constexpr kEnabledKey = "street_exploration_routing_enabled";
std::string_view constexpr kStrengthKey = "street_exploration_routing_strength";

double constexpr kEps = 1e-12;
double constexpr kSampleStepMeters = 15.0;
double constexpr kLat = 60.17;
double constexpr kLon = 24.94;

class StreetExplorationRoutingOptionsGuard
{
public:
  StreetExplorationRoutingOptionsGuard()
  {
    m_hadMode = settings::Get(kModeKey, m_mode);
    m_hadEnabled = settings::Get(kEnabledKey, m_enabled);
    m_hadStrength = settings::Get(kStrengthKey, m_strength);
    settings::Delete(kModeKey);
    settings::Delete(kEnabledKey);
    settings::Delete(kStrengthKey);
  }

  ~StreetExplorationRoutingOptionsGuard()
  {
    settings::Delete(kModeKey);
    settings::Delete(kEnabledKey);
    settings::Delete(kStrengthKey);
    if (m_hadMode)
      settings::Set(kModeKey, m_mode);
    if (m_hadEnabled)
      settings::Set(kEnabledKey, m_enabled);
    if (m_hadStrength)
      settings::Set(kStrengthKey, m_strength);
  }

private:
  bool m_hadMode = false;
  bool m_hadEnabled = false;
  bool m_hadStrength = false;
  std::string m_mode;
  std::string m_enabled;
  std::string m_strength;
};

void SaveExplorationMode(routing::StreetExplorationRoutingMode mode, double strength)
{
  routing::StreetExplorationRoutingOptions options;
  options.m_mode = mode;
  options.m_strength = strength;
  routing::StreetExplorationRoutingOptions::SaveToSettings(options);
}

routing::Segment MakeRealSegment()
{
  return routing::Segment(0, 0, 0, true);
}

routing::RoadGeometry MakeSegmentRoad(double lat0, double lon0, double lat1, double lon1)
{
  return routing::RoadGeometry(false, 1.0, 1.0,
                               {mercator::FromLatLon(lat0, lon0), mercator::FromLatLon(lat1, lon1)});
}

std::vector<std::int64_t> CollectSamplePixelIds(double lat0, double lon0, double lat1, double lon1)
{
  m2::PointD const p1 = mercator::FromLatLon(lat0, lon0);
  m2::PointD const p2 = mercator::FromLatLon(lat1, lon1);
  std::vector<m2::PointD> samples;
  samples.push_back(p1);
  if (!m2::AlmostEqualAbs(p1, p2, 1e-6))
  {
    m2::PointD const p12 = p2 - p1;
    m2::PointD const p12Norm = p12.Normalize();
    double const distanceMercator = p12.Length();
    double const distanceMeters = mercator::DistanceOnEarth(p1, p2);
    size_t const numSegments = static_cast<size_t>(std::ceil(distanceMeters / kSampleStepMeters));
    if (numSegments > 1)
    {
      double const segmentSizeMercator = distanceMercator / static_cast<double>(numSegments);
      for (size_t i = 1; i < numSegments; ++i)
        samples.push_back(p1 + p12Norm * (static_cast<double>(i) * segmentSizeMercator));
    }
    samples.push_back(p2);
  }

  std::unordered_set<std::int64_t> uniqueIds;
  for (auto const & pt : samples)
  {
    auto const latlon = mercator::ToLatLon(pt);
    uniqueIds.insert(PixelIdForLatLon(latlon.m_lat, latlon.m_lon));
  }
  return {uniqueIds.begin(), uniqueIds.end()};
}

std::vector<df::StreetPixel> PixelsForIds(std::vector<std::int64_t> const & ids, bool explored, bool everLive)
{
  std::vector<df::StreetPixel> pixels;
  pixels.reserve(ids.size());
  for (std::int64_t id : ids)
    pixels.push_back(MakeStreetPixel(id, explored, everLive));
  return pixels;
}

double QueryMultiplier(StreetPixelsManager & manager, std::string const & country, double lat0, double lon0,
                       double lat1, double lon1)
{
  return manager.GetSegmentExplorationWeightMultiplier(country, MakeRealSegment(),
                                                       MakeSegmentRoad(lat0, lon0, lat1, lon1));
}
}  // namespace

UNIT_TEST(ExplorationWeight_PreferFullyExploredMaxStrength)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  TEST(!ids.empty(), ());
  manager.SetStreetPixelsOverlayForTesting("A", PixelsForIds(ids, true, true));
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Prefer, 100.0);
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second), 10.0, kEps, ());
}

UNIT_TEST(ExplorationWeight_PreferUnexploredIsOne)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  manager.SetStreetPixelsOverlayForTesting("A", PixelsForIds(ids, false, false));
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Prefer, 100.0);
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second), 1.0, kEps, ());
}

UNIT_TEST(ExplorationWeight_NeitherIsOneWhenExplored)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  manager.SetStreetPixelsOverlayForTesting("A", PixelsForIds(ids, true, true));
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Neither, 100.0);
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second), 1.0, kEps, ());
}

UNIT_TEST(ExplorationWeight_AvoidStoredDoesNotChangeMultiplier)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  manager.SetStreetPixelsOverlayForTesting("A", PixelsForIds(ids, true, true));
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Avoid, 100.0);
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second), 1.0, kEps, ());
}

UNIT_TEST(ExplorationWeight_ImportedOnlyCountsLikeLive)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Prefer, 100.0);

  manager.SetStreetPixelsOverlayForTesting("A", PixelsForIds(ids, true, false));
  TEST(manager.IsPixelExploredForTesting(ids.front()), ());
  TEST(!manager.IsPixelEverLiveForTesting(ids.front()), ());
  double const imported = QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second);

  manager.SetStreetPixelsOverlayForTesting("A", PixelsForIds(ids, true, true));
  TEST(manager.IsPixelEverLiveForTesting(ids.front()), ());
  double const live = QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second);

  TEST_ALMOST_EQUAL_ABS(imported, live, kEps, ());
  TEST_ALMOST_EQUAL_ABS(imported, 10.0, kEps, ());
}

UNIT_TEST(ExplorationWeight_OverlayCountryDiffersButSegmentPixInstalled)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  TEST(!ids.empty(), ());

  std::string const overlayCountry = "sp056_overlay";
  std::string const segmentCountry = "sp056_segment";
  manager.SetStreetPixelsOverlayForTesting(overlayCountry, PixelsForIds(ids, false, false));

  std::string const pixPath = GetPlatform().WritablePathForFile(segmentCountry + ".pix");
  SCOPE_GUARD(cleanup, [&]()
  {
    FileWriter::DeleteFileX(pixPath);
    manager.ClearLeafPixCacheForTesting();
  });

  std::set<int64_t> universe(ids.begin(), ids.end());
  street_pixels_file::ExploredEverLiveMap exploredEverLive;
  for (std::int64_t id : ids)
    exploredEverLive[id] = true;
  TEST(street_pixels_file::SaveRematchedUniverse(pixPath, universe, exploredEverLive, 1), ());

  SaveExplorationMode(routing::StreetExplorationRoutingMode::Prefer, 100.0);
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, segmentCountry, kLat, kLon, offset.first, offset.second), 10.0, kEps,
                        ());
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, overlayCountry, kLat, kLon, offset.first, offset.second), 1.0, kEps,
                        ());
  TEST(!manager.IsPixelExploredForTesting(ids.front()), ());
  TEST_EQUAL(static_cast<int>(manager.GetState().status),
             static_cast<int>(StreetPixelsManager::StreetPixelsStatus::Ready), ());
}

UNIT_TEST(ExplorationWeight_MissingPixIsOne)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 20.0);
  auto const ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  manager.SetStreetPixelsOverlayForTesting("sp056_overlay", PixelsForIds(ids, true, true));
  manager.ClearLeafPixCacheForTesting();
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Prefer, 100.0);
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, "sp056_missing", kLat, kLon, offset.first, offset.second), 1.0, kEps,
                        ());
}

UNIT_TEST(ExplorationWeight_HalfExploredMidStrength)
{
  StreetExplorationRoutingOptionsGuard guard;
  FrozenDataSource dataSource;
  StreetPixelsManager manager(dataSource);
  auto const offset = OffsetLatLonByMeters(kLat, kLon, 0.0, 80.0);
  auto ids = CollectSamplePixelIds(kLat, kLon, offset.first, offset.second);
  TEST_GREATER(ids.size(), 1, ());
  std::sort(ids.begin(), ids.end());
  size_t const exploredCount = ids.size() / 2;
  std::vector<df::StreetPixel> pixels;
  pixels.reserve(ids.size());
  for (size_t i = 0; i < ids.size(); ++i)
    pixels.push_back(MakeStreetPixel(ids[i], i < exploredCount, true));
  manager.SetStreetPixelsOverlayForTesting("A", std::move(pixels));
  SaveExplorationMode(routing::StreetExplorationRoutingMode::Prefer, 50.0);
  double const exploredRatio = static_cast<double>(exploredCount) / static_cast<double>(ids.size());
  double const expected = 1.0 + 0.5 * 9.0 * exploredRatio;
  TEST_ALMOST_EQUAL_ABS(QueryMultiplier(manager, "A", kLat, kLon, offset.first, offset.second), expected, kEps, ());
}
