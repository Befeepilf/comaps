#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/src_point.hpp"

#include "coding/mmap_reader.hpp"

#include "drape/color.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/message.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source_helpers.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/features_vector.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "kml/type_utils.hpp"

#include "map/street_pixels_manager.hpp"
#include "map/street_stats_db.hpp"
#include "map/track.hpp"

#include "platform/country_file.hpp"
#include "platform/location.hpp"
#include "platform/platform.hpp"
#include "platform/vibration.hpp"

#include "routing/routing_helpers.hpp"
#include "routing_common/bicycle_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "search/reverse_geocoder.hpp"

#include <healpix_base.h>
#include <healpix_tables.h>
#include <sys/mman.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// File types:
// .pix: list of explorable healpix ids; left most bit indicates if pixel has been explored
// .pixa: bitmap of healpixels; each bit corresponds to index in the .pix file; used to calculate exploration stats by
// tracking which pixels have already been accounted for in the stats

namespace hp
{
T_Healpix_Base<std::int64_t> const & GetHealpixBase()
{
  static T_Healpix_Base<std::int64_t> base(1048576, Healpix_Ordering_Scheme::NEST, SET_NSIDE);
  return base;
}
}  // namespace hp

double constexpr kSegmentLengthMeters = 15.0;
double constexpr kExploreRadiusMeters = 20.0;
double constexpr kEarthRadiusMeters = 6371000.0;
double constexpr kRadiusRads = kExploreRadiusMeters / kEarthRadiusMeters;

namespace
{
size_t CountExploredPixels(std::span<df::StreetPixel const> streetPixels)
{
  size_t explored = 0;
  for (auto const & pixel : streetPixels)
    if (pixel.IsExplored())
      ++explored;
  return explored;
}
}  // namespace

StreetPixelsManager::StreetPixelsManager(DataSource const & dataSource) : m_dataSource(dataSource) {}

StreetPixelsManager::StreetPixelsState StreetPixelsManager::GetState() const
{
  return m_state;
}

void StreetPixelsManager::SetStateListener(StreetPixelsStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void StreetPixelsManager::ChangeState(StreetPixelsState newState)
{
  if (m_state.enabled == newState.enabled && m_state.status == newState.status)
    return;

  LOG(LINFO, ("Setting status. Is loading:", newState.status == StreetPixelsStatus::Loading));

  m_state = newState;
  if (m_onStateChangedFn != nullptr)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this]()
    {
      std::lock_guard<std::mutex> lock(m_countryIdMutex);
      m_onStateChangedFn(m_state.enabled, m_state.status, m_countryId);
    });
  }
}

void StreetPixelsManager::SetEnabled(bool enabled)
{
  ChangeState(StreetPixelsState{enabled, m_state.status});
  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableStreetPixels, enabled);
}

bool StreetPixelsManager::IsEnabled() const
{
  return m_state.enabled;
}

void StreetPixelsManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void StreetPixelsManager::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

void StreetPixelsManager::OnBookmarksCreated()
{
  LOG(LINFO, ("OnBookmarksCreated"));
  m_tracksLoaded = true;
  UpdateExploredPixels();
}

void StreetPixelsManager::SetExplorationListener(ExplorationListener const & listener)
{
  m_explorationListener = listener;
}

void StreetPixelsManager::LoadStreetPixels(storage::LocalFilePtr const & localFile)
{
  LOG(LINFO, ("LoadStreetPixels"));

  storage::CountryId countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }

  if (countryId == "World" || countryId == "WorldCoasts")
  {
    LOG(LINFO, ("Skipping country file for", countryId));
    return;
  }

  try
  {
    LoadStreetPixelsFromFile(countryId);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Failed to memory-map pix file:", e.what()));
    LOG(LINFO, ("Calculating street pixels for region:", countryId));
    std::string const mwmPath = localFile->GetPath(MapFileType::Map);
    FeaturesVectorTest featuresVector(mwmPath);
    auto newStreetPixels = DeriveStreetPixelsFromFeatures(featuresVector);
    SaveStreetPixelsToFile(newStreetPixels);
    LoadStreetPixelsFromFile(countryId);
  }

  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    if (m_countryId != countryId)
    {
      LOG(LWARNING, ("Country changed while loading street pixels. Aborting."));
      return;
    }
  }

  {
    std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
    m_drapeEngine.SafeCall(&df::DrapeEngine::UpdateStreetPixels, m_streetPixels);
    LOG(LINFO, ("Loaded", m_streetPixels.size(), "total street pixels"));
  }
  LoadAccountedBits();
}

void StreetPixelsManager::LoadStreetPixelsFromFile(storage::CountryId const & countryId)
{
  LOG(LINFO, ("LoadStreetPixelsFromFile", countryId));

  std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
  LOG(LINFO, ("Trying to memory-map existing pix file for", countryId));

  std::unique_ptr<MmapReader> mmapReader = std::make_unique<MmapReader>(filePath, MmapReader::Advice::Sequential, true);
  std::span<df::StreetPixel const> const streetPixels = mmapReader->DataSpan<df::StreetPixel>();
  LOG(LINFO, ("Mapped", streetPixels.size(), "pixels for", countryId));

  size_t const exploredCount = CountExploredPixels(streetPixels);

  {
    std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
    m_mmapReader = std::move(mmapReader);
    m_streetPixels = m_mmapReader->DataSpan<df::StreetPixel>();
    m_exploredPixelCount = exploredCount;
  }
}

void StreetPixelsManager::SaveStreetPixelsToFile(std::set<std::int64_t> const & streetPixels)
{
  LOG(LINFO, ("SaveStreetPixelsToFile", streetPixels.size()));

  storage::CountryId countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }

  LOG(LINFO, ("Saving street pixels for", countryId));
  std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
  {
    std::unique_ptr<FileWriter> writer(new FileWriter(filePath, FileWriter::OP_WRITE_TRUNCATE));
    for (auto const & pixel : streetPixels)
      writer->Write(&pixel, sizeof(int64_t));
    writer->Flush();
  }
}

void StreetPixelsManager::CleanupStreetPixels(storage::CountryId const & countryId)
{
  GetPlatform().RunTask(Platform::Thread::Background, [this, countryId]()
  {
    LOG(LINFO, ("Cleaning up street pixels files for", countryId));

    {
      std::lock_guard<std::mutex> lock(m_countryIdMutex);
      if (m_countryId == countryId)
      {
        m_countryId.clear();
        ClearPixels();
      }
    }

    street_stats::StreetStatsDB::Instance().DeleteMwmData(countryId);

    std::vector<std::string> extensions = {".pix", ".pixa", ".pixf"};
    for (auto const & ext : extensions)
      Platform::RemoveFileIfExists(GetPlatform().WritablePathForFile(countryId + ext));
  });
}

std::set<std::int64_t> StreetPixelsManager::DeriveStreetPixelsFromFeatures(FeaturesVectorTest & featuresVector)
{
  LOG(LINFO, ("DeriveStreetPixelsFromFeatures"));

  MwmSet::MwmId mwmId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    if (m_countryId.empty())
      return std::set<std::int64_t>{};
    mwmId = m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(m_countryId));
  }

  if (!mwmId.IsAlive())
    return std::set<std::int64_t>{};

  auto const & healpix = hp::GetHealpixBase();

  size_t const totalFeatures = featuresVector.GetVector().GetNumFeatures();
  int numStreets = 0;
  std::vector<m2::PointD> points;
  std::set<std::int64_t> pixelIds;
  featuresVector.GetVector().ForEach([&](FeatureType & feature, std::uint64_t const featureIndex)
  {
    if (totalFeatures != 0)
    {
      unsigned const pct = static_cast<unsigned>(((featureIndex + 1) * 100) / totalFeatures);
      unsigned const prevPct = featureIndex > 0 ? static_cast<unsigned>((featureIndex * 100) / totalFeatures) : 0U;
      if (pct / 5 > prevPct / 5)
        LOG(LINFO, ("DeriveStreetPixelsFromFeatures progress", (std::min)(pct, 100U), "%", featureIndex + 1, "of",
                    totalFeatures));
    }

    if (!IsExplorable(feature))
      return;

    numStreets++;

    feature.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const numPoints = feature.GetPointsCount();
    if (numPoints < 2)
      return;

    points.clear();
    points.reserve(numPoints * 4);
    m2::PointD prevPoint = feature.GetPoint(0);
    for (size_t i = 1; i < numPoints; ++i)
    {
      auto const point = feature.GetPoint(i);
      points.push_back(prevPoint);

      SegmentizeStreet(prevPoint, point, [&](m2::PointD const & segmentPoint, double distFromPrevPoint)
      { points.push_back(segmentPoint); });

      prevPoint = point;
    }

    for (auto const & point : points)
    {
      auto const latlon = mercator::ToLatLon(point);
      double const lat_rad = math::DegToRad(latlon.m_lat);
      double const lon_rad = math::DegToRad(latlon.m_lon);
      pointing ptg(M_PI_2 - lat_rad, lon_rad);
      std::int64_t const pixelId = healpix.ang2pix(ptg);
      pixelIds.insert(pixelId);
    }
  });

  LOG(LINFO, ("Found", pixelIds.size(), "street pixels for", numStreets, "streets"));
  return pixelIds;
}

void StreetPixelsManager::SegmentizeStreet(m2::PointD const & p1, m2::PointD const & p2,
                                           std::function<void(m2::PointD const &, double)> const & callback) const
{
  if (m2::AlmostEqualAbs(p1, p2, 1e-6))
    return;

  m2::PointD const p12 = p2 - p1;
  m2::PointD const p12Norm = p12.Normalize();

  double const distanceMercator = p12.Length();
  double const distanceMeters = mercator::DistanceOnEarth(p1, p2);

  size_t const numSegments = std::ceil(distanceMeters / kSegmentLengthMeters);
  if (numSegments <= 1)
    return;

  double const segmentSizeMercator = distanceMercator / numSegments;
  for (size_t i = 1; i < numSegments; ++i)
  {
    m2::PointD const segmentPoint = p1 + p12Norm * (i * segmentSizeMercator);
    double const distFromP1 = mercator::DistanceOnEarth(p1, segmentPoint);
    callback(segmentPoint, distFromP1);
  }
}

double StreetPixelsManager::GetSegmentExplorationWeightMultiplier(std::string const & mwmCountryName,
                                                                  routing::Segment const & segment,
                                                                  routing::RoadGeometry const & road) const
{
  if (!segment.IsRealSegment() || !road.IsValid())
    return 1.0;

  if (GetState().status != StreetPixelsStatus::Ready || !IsEnabled())
    return 1.0;

  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    if (m_countryId != mwmCountryName)
    {
      static std::atomic<unsigned> s_countryMismatchLogs{0};
      if (++s_countryMismatchLogs <= 8U)
        LOG(LINFO,
            ("StreetExploration country mismatch", "segmentMwm", mwmCountryName, "streetPixelsLoadedFor", m_countryId));
      return 1.0;
    }
  }

  {
    std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
    if (m_streetPixels.empty())
      return 1.0;
  }

  uint32_t const j = segment.GetSegmentIdx();
  if (static_cast<size_t>(j) + 1 >= road.GetPointsCount())
    return 1.0;

  m2::PointD const pFrom = mercator::FromLatLon(road.GetPoint(j));
  m2::PointD const pTo = mercator::FromLatLon(road.GetPoint(j + 1));

  std::vector<m2::PointD> samples;
  samples.push_back(pFrom);
  SegmentizeStreet(pFrom, pTo, [&](m2::PointD const & p, double) { samples.push_back(p); });
  if (!m2::AlmostEqualAbs(pFrom, pTo, 1e-6))
    samples.push_back(pTo);

  auto const & healpix = hp::GetHealpixBase();
  std::unordered_set<std::int64_t> seenHealpix;
  seenHealpix.reserve(samples.size() * 10);

  size_t matched = 0;
  size_t exploredMatched = 0;
  size_t pixSpan = 0;

  for (auto const & pt : samples)
  {
    auto const latlon = mercator::ToLatLon(pt);
    double const lat_rad = math::DegToRad(latlon.m_lat);
    double const lon_rad = math::DegToRad(latlon.m_lon);
    pointing const ptg(M_PI_2 - lat_rad, lon_rad);
    std::int64_t const pixelId = healpix.ang2pix(ptg);
    seenHealpix.insert(pixelId);
  }

  {
    std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
    if (m_streetPixels.empty())
      return 1.0;
    pixSpan = m_streetPixels.size();
    for (auto const & pixelId : seenHealpix)
    {
      df::StreetPixel const * const sp = FindStreetPixel(pixelId);
      if (sp == nullptr)
        continue;
      ++matched;
      if (sp->IsExplored())
        ++exploredMatched;
    }
  }

  if (matched == 0)
    return 1.0;

  double const exploredRatio = static_cast<double>(exploredMatched) / static_cast<double>(matched);
  return 10 * exploredRatio;
  // return 1.0 - kStrength * (1.0 - exploredRatio);
}

bool StreetPixelsManager::IsExplorable(FeatureType & ft) const
{
  if (ft.GetGeomType() != feature::GeomType::Line)
    return false;

  bool isHighway = false;
  bool isPrivate = false;
  bool isBikeAccessible = true;
  bool isPedestrianAccessible = true;
  Classificator & c = classif();
  ft.ForEachType([&](std::uint64_t type)
  {
    std::vector<std::string> types = c.GetFullObjectNamePath(type);
    if (types.size() > 0 && types[0] == "highway")
    {
      if (types.size() < 3 || (types[2] != "driveway" && types[2] != "tunnel"))
        isHighway = true;
    }
    if (types.size() >= 2 && types[0] == "hwtag")
    {
      if (types[1] == "private")
        isPrivate = true;
      else if (types[1] == "nobicycle")
        isBikeAccessible = false;
      else if (types[1] == "yesbicycle")
        isBikeAccessible = true;
      else if (types[1] == "nofoot")
        isPedestrianAccessible = false;
      else if (types[1] == "yesfoot")
        isPedestrianAccessible = true;
    }
  });

  return isHighway && !isPrivate && (isBikeAccessible || isPedestrianAccessible);
}

df::StreetPixel const * StreetPixelsManager::FindStreetPixel(std::int64_t pixelId) const
{
  auto first = m_streetPixels.begin();
  auto last = m_streetPixels.end();
  auto it = std::lower_bound(first, last, pixelId,
                             [](df::StreetPixel const & p, std::int64_t id) { return p.GetPixelId() < id; });
  if (it != last && it->GetPixelId() == pixelId)
    return &(*it);
  return nullptr;
}

df::StreetPixel * StreetPixelsManager::FindStreetPixel(std::int64_t pixelId)
{
  return const_cast<df::StreetPixel *>(std::as_const(*this).FindStreetPixel(pixelId));
}

void StreetPixelsManager::UpdateExploredPixels()
{
  LOG(LINFO, ("UpdateExploredPixels"));

  if (m_bmManager == nullptr)
    return;

  {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_state.status != StreetPixelsStatus::Ready)
    {
      LOG(LWARNING, ("Street pixels not loaded"));
      return;
    }
  }

  if (!m_tracksLoaded)
  {
    LOG(LWARNING, ("Tracks not loaded"));
    return;
  }

  LOG(LINFO, ("Collecting tracks"));
  std::vector<TrackInfo> tracks;
  m_bmManager->ForEachTrackSortedByTimestamp([&](Track const & t)
  { tracks.push_back(TrackInfo{t.GetId(), t.GetGeometry(), t.GetData().m_timestamp}); });

  storage::CountryId countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }

  GetPlatform().RunTask(Platform::Thread::Background, [this, tracks = std::move(tracks), countryId]() mutable
  {
    if (countryId.empty())
      return;

    for (auto const & ti : tracks)
    {
      {
        std::lock_guard<std::mutex> lock(m_countryIdMutex);
        if (m_countryId != countryId)
        {
          LOG(LWARNING, ("Country changed while updating explored pixels. Aborting."));
          return;
        }
      }

      std::int64_t const geometryHash = ComputeGeometryHash(ti);
      if (street_stats::StreetStatsDB::Instance().IsTrackProcessed(geometryHash, countryId))
        continue;

      // UpdateStreetStatsForTrack(ti.geom);

      LOG(LINFO, ("Computing track pixels for", ti.id));

      auto trackPixels = ComputeTrackPixels(ti);
      size_t statsNew = 0;
      std::set<int64_t> renderNew;
      {
        std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
        for (auto pix : trackPixels)
        {
          auto * pixel = FindStreetPixel(pix);
          if (pixel == nullptr)
            continue;
          if (!pixel->IsExplored())
          {
            pixel->SetExplored(true);
            msync(pixel, sizeof(df::StreetPixel), MS_ASYNC);
            ++m_exploredPixelCount;
            renderNew.insert(pix);
          }
          if (!m_accountedBits.empty())
          {
            size_t const index = GetPixelIndexWhileLocked(pixel);
            if (!IsAccountedIndex(index))
            {
              ApplyAccountedIndex(index, m_streetPixels.size());
              ++statsNew;
            }
          }
        }
      }

      street_stats::StreetStatsDB::Instance().MarkTrackProcessed(geometryHash, countryId);

      if (statsNew > 0 && m_explorationListener)
      {
        ExplorationDelta d;
        d.m_regionId = countryId;
        d.m_newPixels = static_cast<uint32_t>(statsNew);
        d.m_eventTimeSec = static_cast<double>(kml::ToSecondsSinceEpoch(ti.ts));
        m_explorationListener(d);
      }
    }

    {
      std::lock_guard<std::mutex> lock(m_countryIdMutex);
      if (m_countryId != countryId)
      {
        LOG(LWARNING, ("Country changed while updating explored pixels. Aborting."));
        return;
      }
    }

    if (m_accountedDirty)
      SaveAccountedBits();

    // Notify UI that exploration data updated even if status unchanged.
    if (m_onStateChangedFn)
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [this]()
      {
        std::lock_guard<std::mutex> lock(m_countryIdMutex);
        m_onStateChangedFn(m_state.enabled, m_state.status, m_countryId);
      });
    }
  });
}

void StreetPixelsManager::UpdateStreetStatsForTrack(kml::MultiGeometry::LineT const & line)
{
  // TODO: this method is super slow
  LOG(LINFO, ("UpdateStreetStatsForTrack"));

  if (line.empty())
    return;

  m2::PointD prev = geometry::GetPoint(line[0]);
  for (size_t i = 1; i < line.size(); ++i)
  {
    auto const & ptWithAlt = line[i];
    m2::PointD curr = geometry::GetPoint(ptWithAlt);
    double distMerc = (curr - prev).Length();
    double distMeters = mercator::DistanceOnEarth(prev, curr);
    size_t segments = std::max<size_t>(1, static_cast<size_t>(std::ceil(distMeters / 10.0)));  // Sample every 10m
    m2::PointD dir = (curr - prev).Normalize();
    double step = distMerc / segments;
    for (size_t s = 0; s <= segments; ++s)
    {
      m2::PointD p = prev + dir * (s * step);
      auto const latlon = mercator::ToLatLon(p);
      UpdateStreetStats(latlon.m_lat, latlon.m_lon, 1);
    }
    prev = curr;
  }
}

std::int64_t StreetPixelsManager::ComputeGeometryHash(TrackInfo const & trackInfo)
{
  std::size_t seed = 0;
  for (auto const & point : trackInfo.geom)
  {
    boost::hash_combine(seed, point.GetPoint().x);
    boost::hash_combine(seed, point.GetPoint().y);
  }
  return static_cast<std::int64_t>(seed);
}

std::set<int64_t> StreetPixelsManager::ComputeTrackPixels(TrackInfo const & trackInfo) const
{
  std::set<int64_t> pixels;

  if (trackInfo.geom.empty())
    return pixels;

  m2::PointD prev = geometry::GetPoint(trackInfo.geom[0]);
  for (size_t i = 1; i < trackInfo.geom.size(); ++i)
  {
    auto const & ptWithAlt = trackInfo.geom[i];
    m2::PointD curr = geometry::GetPoint(ptWithAlt);
    double distMerc = (curr - prev).Length();
    double distMeters = mercator::DistanceOnEarth(prev, curr);
    size_t segments = std::max<size_t>(1, static_cast<size_t>(std::ceil(distMeters / 10.0)));
    m2::PointD dir = (curr - prev).Normalize();
    double step = distMerc / segments;
    for (size_t s = 0; s <= segments; ++s)
    {
      m2::PointD p = prev + dir * (s * step);
      auto const latlon = mercator::ToLatLon(p);
      AddPixelsInRadius(latlon.m_lat, latlon.m_lon, pixels);
    }
    prev = curr;
  }
  return pixels;
}

void StreetPixelsManager::AddPixelsInRadius(double lat, double lon, std::set<std::int64_t> & pixels) const
{
  double const lat_rad = math::DegToRad(lat);
  double const lon_rad = math::DegToRad(lon);
  pointing ang(M_PI_2 - lat_rad, lon_rad);
  auto disc = hp::GetHealpixBase().query_disc(ang, kRadiusRads);
  for (tsize r = 0; r < disc.nranges(); ++r)
  {
    std::int64_t first = disc.ivbegin(r);
    std::int64_t last = disc.ivend(r);
    for (std::int64_t pix = first; pix < last; ++pix)
      pixels.insert(pix);
  }
}

void StreetPixelsManager::OnLocationUpdate(location::GpsInfo const & info)
{
  std::set<std::int64_t> pixels;
  AddPixelsInRadius(info.m_latitude, info.m_longitude, pixels);
  size_t numNewlyExploredPixels = 0;
  std::vector<ExplorationDelta> perPixelExplorationDeltas;
  if (m_explorationListener)
    perPixelExplorationDeltas.reserve(pixels.size());

  std::string countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }

  for (auto const & pix : pixels)
  {
    size_t idx = 0;
    {
      std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
      auto * pixel = FindStreetPixel(pix);
      if (pixel == nullptr || pixel->IsExplored())
        continue;
      idx = GetPixelIndexWhileLocked(pixel);
      pixel->SetExplored(true);
      msync(pixel, sizeof(df::StreetPixel), MS_ASYNC);
      ++numNewlyExploredPixels;
      ++m_exploredPixelCount;
    }

    if (!m_accountedBits.empty())
    {
      if (!IsAccountedIndex(idx))
      {
        SetAccountedIndex(idx);
        if (m_explorationListener)
        {
          ExplorationDelta d;
          d.m_regionId = countryId;
          d.m_newPixels = 1;
          d.m_eventTimeSec = info.m_timestamp;
          perPixelExplorationDeltas.push_back(std::move(d));
        }
      }
    }
  }

  if (m_explorationListener)
    for (auto const & d : perPixelExplorationDeltas)
      m_explorationListener(d);

  UpdateStreetStats(info.m_latitude, info.m_longitude, numNewlyExploredPixels);

  if (numNewlyExploredPixels > 0 && m_explorationListener)
  {
    ExplorationDelta d;
    d.m_regionId = countryId;
    d.m_newPixels = static_cast<uint32_t>(numNewlyExploredPixels);
    d.m_eventTimeSec = info.m_timestamp;
    m_explorationListener(d);
  }

  if (numNewlyExploredPixels == 1)
    platform::Vibrate(50);
  else if (numNewlyExploredPixels > 1)
  {
    size_t const maxPixels = 10;
    size_t const count = std::min(numNewlyExploredPixels, maxPixels);

    std::vector<uint32_t> durations(count, 30);
    std::vector<uint32_t> delays(count, 20);

    platform::VibratePattern(durations.data(), delays.data(), count);
  }
}

void StreetPixelsManager::UpdateStreetStats(double lat, double lon, size_t numNewlyExploredPixels)
{
  // LOG(LINFO, ("UpdateStreetStats", lat, lon, numNewlyExploredPixels));

  if (numNewlyExploredPixels == 0)
    return;

  MwmSet::MwmId mwmId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    if (m_countryId.empty())
      return;
    mwmId = m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(m_countryId));
  }

  if (!mwmId.IsAlive())
    return;

  m2::PointD centerMercator = mercator::FromLatLon(lat, lon);

  std::map<FeatureID, std::set<uint32_t>> featureUpdates;

  indexer::ForEachFeatureAtPoint(m_dataSource, [&](FeatureType & ft)
  {
    if (!IsExplorable(ft))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = ft.GetPointsCount();
    if (pointsCount < 2)
      return;

    double minSqDist = std::numeric_limits<double>::max();
    double distanceAlongFeatureM = -1.0;
    double accumulatedLengthM = 0.0;

    for (size_t i = 1; i < pointsCount; ++i)
    {
      m2::PointD const p1 = ft.GetPoint(i - 1);
      m2::PointD const p2 = ft.GetPoint(i);

      m2::ParametrizedSegment<m2::PointD> segment(p1, p2);
      m2::PointD const closestPoint = segment.ClosestPointTo(centerMercator);
      double const sqDist = centerMercator.SquaredLength(closestPoint);

      if (sqDist < minSqDist)
      {
        minSqDist = sqDist;
        distanceAlongFeatureM = accumulatedLengthM + mercator::DistanceOnEarth(p1, closestPoint);
      }
      accumulatedLengthM += mercator::DistanceOnEarth(p1, p2);
    }

    if (distanceAlongFeatureM >= 0)
    {
      uint32_t const pixelIndex = static_cast<uint32_t>(distanceAlongFeatureM / kSegmentLengthMeters);
      featureUpdates[ft.GetID()].insert(pixelIndex);
    }
  }, centerMercator, 0.0);

  if (featureUpdates.empty())
    return;

  auto & db = street_stats::StreetStatsDB::Instance();
  db.WithTransaction([&]()
  {
    for (auto const & [fid, pixelIndices] : featureUpdates)
    {
      auto bitmask = db.GetBitmask(fid.m_mwmId, fid.m_index);
      // If bitmask does not exist, it means the stats for this MWM have not been generated.
      // We should not create it on the fly, as we don't know the full feature length.
      if (!bitmask)
        continue;

      bool updated = false;
      for (uint32_t pixelIndex : pixelIndices)
      {
        size_t const byteIndex = pixelIndex / 8;
        if (byteIndex < bitmask->size())
        {
          uint8_t const bitIndex = pixelIndex % 8;
          if (!((*bitmask)[byteIndex] & (1 << bitIndex)))
          {
            (*bitmask)[byteIndex] |= (1 << bitIndex);
            updated = true;
          }
        }
      }

      if (updated)
        db.SaveBitmask(fid.m_mwmId, fid.m_index, *bitmask);
    }
  });
}

std::string StreetPixelsManager::GetCurrentCountryId() const
{
  std::lock_guard<std::mutex> lock(m_countryIdMutex);
  return m_countryId;
}

void StreetPixelsManager::OnUpdateCurrentCountry(storage::CountryId const & countryId,
                                                 storage::LocalFilePtr const & localFile)
{
  LOG(LINFO, ("OnUpdateCurrentCountry", countryId));

  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    LOG(LINFO, ("Country changed from", m_countryId, "to", countryId));
    m_countryId = countryId;
  }

  ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::Loading});

  GetPlatform().RunTask(Platform::Thread::Background, [this, countryId, localFile]()
  {
    ClearPixels();
    if (countryId.empty() || !localFile || !localFile->OnDisk(MapFileType::Map))
    {
      ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::NotReady});
      return;
    }

    LOG(LINFO, ("Loading street pixels in background thread because country changed to", countryId));
    LoadStreetPixels(localFile);
    ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::Ready});
    GetPlatform().RunTask(Platform::Thread::Gui, [this]() { UpdateExploredPixels(); });
  });
}

double StreetPixelsManager::GetTotalExploredFraction() const
{
  std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
  size_t const total = m_streetPixels.size();
  if (total == 0)
    return 0.0;
  return static_cast<double>(m_exploredPixelCount) / static_cast<double>(total);
}

void StreetPixelsManager::ClearPixels()
{
  LOG(LINFO, ("Clearing pixels and unmapping pix file"));
  m_drapeEngine.SafeCall(&df::DrapeEngine::ClearStreetPixels);
  {
    std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
    m_streetPixels = {};
    m_mmapReader.reset();
    m_exploredPixelCount = 0;
  }
  m_accountedBits.clear();
  m_accountedDirty = false;

  {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::NotReady});
  }
}

std::string StreetPixelsManager::GetAccountedFilePath() const
{
  storage::CountryId country;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    country = m_countryId;
  }
  return GetPlatform().WritablePathForFile(country + ".pixa");
}

size_t StreetPixelsManager::GetPixelIndex(df::StreetPixel const * ptr) const
{
  std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
  return GetPixelIndexWhileLocked(ptr);
}

size_t StreetPixelsManager::GetPixelIndexWhileLocked(df::StreetPixel const * ptr) const
{
  if (m_streetPixels.empty())
    return 0;
  return static_cast<size_t>(ptr - m_streetPixels.data());
}

bool StreetPixelsManager::IsAccountedIndex(size_t idx) const
{
  if (m_accountedBits.empty() || idx >= m_accountedBits.size() * 8)
    return false;
  size_t byteIdx = idx / 8;
  size_t bitIdx = idx % 8;
  return (m_accountedBits[byteIdx] & (1 << bitIdx)) != 0;
}

void StreetPixelsManager::SetAccountedIndex(size_t idx)
{
  size_t totalPixels = 0;
  {
    std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
    totalPixels = m_streetPixels.size();
  }
  ApplyAccountedIndex(idx, totalPixels);
}

void StreetPixelsManager::ApplyAccountedIndex(size_t idx, size_t totalPixels)
{
  if (idx >= totalPixels)
    return;

  size_t requiredBytes = (idx + 8) / 8;
  if (m_accountedBits.size() < requiredBytes)
    m_accountedBits.resize(requiredBytes, 0);

  size_t byteIdx = idx / 8;
  size_t bitIdx = idx % 8;
  m_accountedBits[byteIdx] |= (1 << bitIdx);
  m_accountedDirty = true;
}

void StreetPixelsManager::LoadAccountedBits()
{
  LOG(LINFO, ("LoadAccountedBits"));

  std::string path = GetAccountedFilePath();
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs.is_open())
  {
    LOG(LINFO, ("No accounted bits file for", GetCurrentCountryId()));
    return;
  }

  ifs.seekg(0, std::ios::end);
  size_t fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  m_accountedBits.resize(fileSize);
  ifs.read(reinterpret_cast<char *>(m_accountedBits.data()), fileSize);
  m_accountedDirty = false;

  LOG(LINFO, ("Loaded", fileSize, "bytes of accounted bits for", GetCurrentCountryId()));
}

void StreetPixelsManager::SaveAccountedBits()
{
  LOG(LINFO, ("SaveAccountedBits"));
  if (!m_accountedDirty)
    return;

  std::string path = GetAccountedFilePath();
  std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
  if (!ofs.is_open())
  {
    LOG(LWARNING, ("Failed to open accounted bits file for writing:", path));
    return;
  }

  ofs.write(reinterpret_cast<char const *>(m_accountedBits.data()), m_accountedBits.size());
  m_accountedDirty = false;

  LOG(LINFO, ("Saved", m_accountedBits.size(), "bytes of accounted bits for", GetCurrentCountryId()));
}
