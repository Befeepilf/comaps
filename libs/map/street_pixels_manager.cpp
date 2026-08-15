#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
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
#include "map/street_pixels_file.hpp"
#include "map/street_stats_db.hpp"
#include "map/recording_session.hpp"
#include "map/live_sample_acceptance_filter.hpp"
#include "map/live_segment_interpolation.hpp"

#include "street_pixels_areas/area_completion_cache.hpp"
#include "street_pixels_areas/area_overlay.hpp"
#include "street_pixels_areas/areas_format.hpp"
#include "street_pixels_areas/city_completion_cache.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/exploration_sidecar.hpp"
#include "street_pixels_areas/focus_selection_engine.hpp"
#include "street_pixels_areas/sparse_assignment_store.hpp"

#include "drape_frontend/exploration_area_overlay.hpp"
#include "drape/color.hpp"

#include "street_pixels_config/country_config.hpp"

#include "base/timer.hpp"
#include "map/track.hpp"

#include "platform/country_file.hpp"
#include "platform/location.hpp"
#include "platform/platform.hpp"
#include "platform/vibration.hpp"

#include "routing/routing_helpers.hpp"
#include "routing/routing_options.hpp"
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

double constexpr kSegmentLengthMeters = kPathSamplingStepMeters;
double constexpr kExploreRadiusMeters = 25.0;
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

uint64_t CountPixBodyEntries(std::string const & path, uint64_t fileSize)
{
  auto const probe = street_pixels_file::ProbeFile(path);
  switch (probe.kind)
  {
  case street_pixels_file::FileKind::HeaderedV1:
  case street_pixels_file::FileKind::HeaderedV2:
    if (fileSize < street_pixels_file::kHeaderSize ||
        ((fileSize - street_pixels_file::kHeaderSize) % sizeof(int64_t)) != 0)
      return 0;
    return (fileSize - street_pixels_file::kHeaderSize) / sizeof(int64_t);
  case street_pixels_file::FileKind::Legacy:
    if ((fileSize % sizeof(int64_t)) != 0)
      return 0;
    return fileSize / sizeof(int64_t);
  case street_pixels_file::FileKind::UnsupportedFormat:
  case street_pixels_file::FileKind::Corrupt: return 0;
  }
  return 0;
}

m2::PointD MercatorCentreForHealpixNest(std::int64_t pixelId)
{
  pointing const ang = hp::GetHealpixBase().pix2ang(pixelId);
  double const lat = math::RadToDeg(M_PI_2 - ang.theta);
  double const lon = math::RadToDeg(ang.phi);
  return mercator::FromLatLon(lat, lon);
}

void CollectExploredAscendingWithCentres(street_pixels_file::ExploredEverLiveMap const & explored,
                                         std::vector<std::int64_t> & exploredAscending,
                                         std::vector<m2::PointD> & centres)
{
  exploredAscending.clear();
  centres.clear();
  exploredAscending.reserve(explored.size());
  centres.reserve(explored.size());
  std::vector<std::int64_t> ids;
  ids.reserve(explored.size());
  for (auto const & entry : explored)
    ids.push_back(entry.first);
  std::sort(ids.begin(), ids.end());
  for (std::int64_t id : ids)
  {
    exploredAscending.push_back(id);
    centres.push_back(MercatorCentreForHealpixNest(id));
  }
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
  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableExplorationAreaOverlay, enabled);
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

void StreetPixelsManager::SetRecordingSession(RecordingSession const * session)
{
  m_recordingSession = session;
}

void StreetPixelsManager::ResetSampleAcceptanceReference()
{
  m_acceptanceFilter.ResetAcceptedReference();
  m_segmentInterpolation.MarkInterpolationBarrier();
}

void StreetPixelsManager::MarkInterpolationBarrier()
{
  m_segmentInterpolation.MarkInterpolationBarrier();
}

SampleRejectReason StreetPixelsManager::GetLastSampleRejectReason() const
{
  return m_acceptanceFilter.GetLastRejectReason();
}

void StreetPixelsManager::SetVibrationHandler(VibrationHandler const & handler)
{
  m_vibrationHandler = handler;
}

void StreetPixelsManager::SetStreetPixelsForTesting(std::vector<df::StreetPixel> pixels)
{
  std::sort(pixels.begin(), pixels.end(),
            [](df::StreetPixel const & a, df::StreetPixel const & b) { return a.GetPixelId() < b.GetPixelId(); });
  m_testStreetPixelsStorage = std::move(pixels);
  std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
  m_mmapReader.reset();
  m_streetPixels = m_testStreetPixelsStorage;
  m_exploredPixelCount = CountExploredPixels(m_streetPixels);
}

size_t StreetPixelsManager::MarkImportedPixelsForTesting(std::set<std::int64_t> const & pixelIds)
{
  return MarkExploredPixelIds(pixelIds, 0.0);
}

size_t StreetPixelsManager::MarkTrackPixelsForTesting(std::set<std::int64_t> const & pixelIds)
{
  return MarkImportedPixelsForTesting(pixelIds);
}

bool StreetPixelsManager::IsPixelExploredForTesting(std::int64_t pixelId) const
{
  std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
  auto const * pixel = FindStreetPixel(pixelId);
  return pixel != nullptr && pixel->IsExplored();
}

bool StreetPixelsManager::IsPixelEverLiveForTesting(std::int64_t pixelId) const
{
  std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
  auto const * pixel = FindStreetPixel(pixelId);
  return pixel != nullptr && pixel->IsEverLive();
}

size_t StreetPixelsManager::MarkExploredPixelIds(std::set<std::int64_t> const & pixelIds, double eventTimeSec)
{
  size_t statsNew = 0;
  std::string countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }

  {
    std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
    for (auto const pix : pixelIds)
    {
      auto * pixel = FindStreetPixel(pix);
      if (pixel == nullptr)
        continue;
      if (!pixel->IsExplored())
      {
        pixel->SetExplored(true);
        msync(pixel, sizeof(df::StreetPixel), MS_ASYNC);
        ++m_exploredPixelCount;
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

  InvalidateAreaCompletionCache();

  if (statsNew > 0 && m_explorationListener)
  {
    ExplorationDelta d;
    d.m_regionId = countryId;
    d.m_newPixels = static_cast<uint32_t>(statsNew);
    d.m_eventTimeSec = eventTimeSec;
    m_explorationListener(d);
  }

  return statsNew;
}

void StreetPixelsManager::TriggerCollectionVibration(size_t numNewlyExploredPixels)
{
  if (numNewlyExploredPixels == 0)
    return;

  if (m_vibrationHandler)
  {
    m_vibrationHandler(numNewlyExploredPixels);
    return;
  }

  if (numNewlyExploredPixels == 1)
    platform::Vibrate(50);
  else
  {
    size_t const maxPixels = 10;
    size_t const count = std::min(numNewlyExploredPixels, maxPixels);

    std::vector<uint32_t> durations(count, 30);
    std::vector<uint32_t> delays(count, 20);

    platform::VibratePattern(durations.data(), delays.data(), count);
  }
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

  std::int64_t const mapDataVersion = localFile ? localFile->GetVersion() : 0;
  std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
  std::string const archivePath = GetPlatform().WritablePathForFile(countryId + ".pixr");
  bool const hasArchive = Platform::IsFileExistsByFullPath(archivePath);
  auto const probe = street_pixels_file::ProbeFile(filePath);

  auto const tryRematchFromLocalMap = [&]() -> bool
  {
    if (!localFile || !localFile->OnDisk(MapFileType::Map))
    {
      LOG(LWARNING, ("Cannot rematch street pixels without local map", countryId));
      return false;
    }
    try
    {
      std::string const mwmPath = localFile->GetPath(MapFileType::Map);
      FeaturesVectorTest featuresVector(mwmPath);
      auto const newIds = DeriveStreetPixelsFromFeatures(featuresVector, countryId);
      std::string const spaPath = street_pixels::ExplorationSidecarPathBesideMwm(mwmPath);
      return RematchStreetPixelsWithNewUniverseUnlocked(countryId, newIds, mapDataVersion, spaPath);
    }
    catch (std::exception const & e)
    {
      LOG(LWARNING, ("Rematch on load aborted", countryId, e.what()));
      return false;
    }
  };

  if ((probe.kind == street_pixels_file::FileKind::HeaderedV1 ||
       probe.kind == street_pixels_file::FileKind::HeaderedV2) &&
      probe.header.mapDataVersion != mapDataVersion)
  {
    LOG(LINFO, ("Street pixels map-data version mismatch; rematching", countryId, "file",
                probe.header.mapDataVersion, "mwm", mapDataVersion));
    if (tryRematchFromLocalMap())
      return;
  }

  if ((probe.kind == street_pixels_file::FileKind::Corrupt ||
       probe.kind == street_pixels_file::FileKind::UnsupportedFormat) &&
      hasArchive)
  {
    LOG(LINFO, ("Street pixels file unusable; rematching from explored archive", countryId));
    if (tryRematchFromLocalMap())
      return;
    LOG(LWARNING, ("Rematch from explored archive failed; refusing blank derive", countryId));
    return;
  }

  try
  {
    LoadStreetPixelsFromFile(countryId, mapDataVersion);
  }
  catch (street_pixels_file::UnsupportedStreetPixelsFormat const & e)
  {
    LOG(LWARNING, ("Unsupported street pixels format:", e.what()));
    if (Platform::IsFileExistsByFullPath(archivePath))
    {
      LOG(LINFO, ("Recovering street pixels from explored archive", countryId));
      if (tryRematchFromLocalMap())
        return;
      LOG(LWARNING, ("Rematch from explored archive failed; refusing blank derive", countryId));
      return;
    }
  }
  catch (street_pixels_file::StreetPixelsMigrationException const & e)
  {
    LOG(LWARNING, ("Street pixels migration failed:", e.what()));
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Failed to memory-map pix file:", e.what()));
    if (Platform::IsFileExistsByFullPath(archivePath))
    {
      LOG(LINFO, ("Recovering street pixels from explored archive", countryId));
      if (tryRematchFromLocalMap())
        return;
      LOG(LWARNING, ("Rematch from explored archive failed; refusing blank derive", countryId));
      return;
    }
    auto const recoverProbe = street_pixels_file::ProbeFile(filePath);
    if (!street_pixels_file::MayRecoverByDerive(recoverProbe.kind))
    {
      LOG(LWARNING, ("Not re-deriving street pixels over existing file", filePath,
                     static_cast<int>(recoverProbe.kind)));
    }
    else
    {
      LOG(LINFO, ("Calculating street pixels for region:", countryId));
      std::string const mwmPath = localFile->GetPath(MapFileType::Map);
      FeaturesVectorTest featuresVector(mwmPath);
      auto newStreetPixels = DeriveStreetPixelsFromFeatures(featuresVector, countryId);
      SaveStreetPixelsToFile(newStreetPixels, mapDataVersion);
      LoadStreetPixelsFromFile(countryId, mapDataVersion);
    }
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

  if (localFile && localFile->OnDisk(MapFileType::Map))
  {
    std::string const spaPath = street_pixels::ExplorationSidecarPathBesideMwm(localFile->GetPath(MapFileType::Map));
    RefreshSparseAssignmentsBestEffortUnlocked(countryId, spaPath, mapDataVersion, false /* policyOnly */);
  }
}

void StreetPixelsManager::LoadStreetPixelsFromFile(storage::CountryId const & countryId,
                                                   std::int64_t mapDataVersion)
{
  LOG(LINFO, ("LoadStreetPixelsFromFile", countryId));

  std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
  LOG(LINFO, ("Trying to memory-map existing pix file for", countryId));

  auto const probe = street_pixels_file::ProbeFile(filePath);
  switch (probe.kind)
  {
  case street_pixels_file::FileKind::UnsupportedFormat:
    MYTHROW(street_pixels_file::UnsupportedStreetPixelsFormat,
            ("Unsupported street pixels format version", probe.header.formatVersion, filePath));
  case street_pixels_file::FileKind::Legacy:
    street_pixels_file::MigrateLegacyFile(filePath, mapDataVersion);
    break;
  case street_pixels_file::FileKind::Corrupt:
    MYTHROW(street_pixels_file::CorruptStreetPixelsFile, ("Corrupt or missing street pixels file", filePath));
  case street_pixels_file::FileKind::HeaderedV1:
  case street_pixels_file::FileKind::HeaderedV2: break;
  }

  std::unique_ptr<MmapReader> mmapReader =
      std::make_unique<MmapReader>(filePath, MmapReader::Advice::Sequential, true);
  if (mmapReader->Size() < street_pixels_file::kHeaderSize ||
      ((mmapReader->Size() - street_pixels_file::kHeaderSize) % sizeof(df::StreetPixel)) != 0)
  {
    MYTHROW(street_pixels_file::CorruptStreetPixelsFile, ("Invalid headered street pixels size", filePath));
  }

  auto const header = street_pixels_file::ReadHeader(mmapReader->Data());
  bool const supportedVersion = header.formatVersion == street_pixels_file::kFormatVersionV1 ||
                                header.formatVersion == street_pixels_file::kFormatVersionV2;
  if (header.magic != street_pixels_file::kMagic || !supportedVersion ||
      (header.flags & street_pixels_file::kFlagsHasHeaderBit) == 0)
  {
    MYTHROW(street_pixels_file::CorruptStreetPixelsFile, ("Invalid street pixels header after open", filePath));
  }

  size_t const entryCount =
      static_cast<size_t>((mmapReader->Size() - street_pixels_file::kHeaderSize) / sizeof(df::StreetPixel));
  auto * const body = reinterpret_cast<df::StreetPixel *>(mmapReader->Data() + street_pixels_file::kHeaderSize);
  std::span<df::StreetPixel> const streetPixels(body, entryCount);
  LOG(LINFO, ("Mapped", streetPixels.size(), "pixels for", countryId));

  size_t const exploredCount = CountExploredPixels(streetPixels);

  {
    std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
    m_mmapReader = std::move(mmapReader);
    m_streetPixels = streetPixels;
    m_exploredPixelCount = exploredCount;
    m_pixMapDataVersion = header.mapDataVersion;
  }
}

void StreetPixelsManager::SaveStreetPixelsToFile(std::set<std::int64_t> const & streetPixels,
                                                 std::int64_t mapDataVersion)
{
  LOG(LINFO, ("SaveStreetPixelsToFile", streetPixels.size()));

  storage::CountryId countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }

  LOG(LINFO, ("Saving street pixels for", countryId));
  std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
  if (!street_pixels_file::SaveUnexploredIds(filePath, streetPixels, mapDataVersion))
    MYTHROW(street_pixels_file::StreetPixelsFileException, ("Failed to save street pixels file", filePath));
}

std::int64_t StreetPixelsManager::GetPixMapDataVersion() const
{
  std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
  return m_pixMapDataVersion;
}

void StreetPixelsManager::CleanupStreetPixels(storage::CountryId const & countryId)
{
  GetPlatform().RunTask(Platform::Thread::Background, [this, countryId]()
  {
    std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
    CleanupStreetPixelsUnlocked(countryId);
  });
}

void StreetPixelsManager::CleanupStreetPixelsForTesting(storage::CountryId const & countryId)
{
  std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
  CleanupStreetPixelsUnlocked(countryId);
}

void StreetPixelsManager::CleanupStreetPixelsUnlocked(storage::CountryId const & countryId)
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

  std::string const pixPath = GetPlatform().WritablePathForFile(countryId + ".pix");
  std::string const archivePath = GetPlatform().WritablePathForFile(countryId + ".pixr");
  std::string const accountedPath = GetPlatform().WritablePathForFile(countryId + ".pixa");
  std::string const pixfPath = GetPlatform().WritablePathForFile(countryId + ".pixf");

  uint64_t pixSize = 0;
  bool const pixExists = Platform::GetFileSizeByFullPath(pixPath, pixSize) && pixSize > 0;
  if (!pixExists)
    return;

  auto const scanned = street_pixels_file::ScanExploredEverLive(pixPath);
  if (!scanned)
  {
    LOG(LWARNING, ("Street pixels scan failed during cleanup; keeping .pix", countryId));
    return;
  }

  if (scanned->empty())
  {
    Platform::RemoveFileIfExists(archivePath);
  }
  else
  {
    int64_t mapDataVersion = 0;
    auto const probe = street_pixels_file::ProbeFile(pixPath);
    if (probe.kind == street_pixels_file::FileKind::HeaderedV1 ||
        probe.kind == street_pixels_file::FileKind::HeaderedV2)
    {
      mapDataVersion = probe.header.mapDataVersion;
    }
    if (!street_pixels_file::SaveExploredArchive(archivePath, *scanned, mapDataVersion))
    {
      LOG(LERROR, ("Failed to write explored archive; keeping .pix", countryId));
      return;
    }
  }

  Platform::RemoveFileIfExists(pixPath);
  Platform::RemoveFileIfExists(accountedPath);
  Platform::RemoveFileIfExists(pixfPath);
  Platform::RemoveFileIfExists(street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), countryId));
}

void StreetPixelsManager::RematchStreetPixelsOnMapUpdate(storage::CountryId const & countryId,
                                                         storage::LocalFilePtr const & localFile)
{
  GetPlatform().RunTask(Platform::Thread::Background, [this, countryId, localFile]()
  {
    std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
    if (!localFile || !localFile->OnDisk(MapFileType::Map))
    {
      LOG(LWARNING, ("Rematch skipped; local map missing", countryId));
      return;
    }

    try
    {
      std::int64_t const mapDataVersion = localFile->GetVersion();
      std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
      std::string const archivePath = GetPlatform().WritablePathForFile(countryId + ".pixr");
      auto const probe = street_pixels_file::ProbeFile(filePath);
      if ((probe.kind == street_pixels_file::FileKind::HeaderedV1 ||
           probe.kind == street_pixels_file::FileKind::HeaderedV2) &&
          probe.header.mapDataVersion == mapDataVersion)
      {
        LOG(LINFO, ("Rematch skipped; map-data version already current", countryId, mapDataVersion));
        Platform::RemoveFileIfExists(archivePath);
        // .pix is current; still best-effort refresh .spx (missing/corrupt/stale policy).
        std::string const spaPath =
            street_pixels::ExplorationSidecarPathBesideMwm(localFile->GetPath(MapFileType::Map));
        RefreshSparseAssignmentsBestEffortUnlocked(countryId, spaPath, mapDataVersion, false /* policyOnly */);
        return;
      }

      std::string const mwmPath = localFile->GetPath(MapFileType::Map);
      FeaturesVectorTest featuresVector(mwmPath);
      auto const newIds = DeriveStreetPixelsFromFeatures(featuresVector, countryId);
      std::string const spaPath = street_pixels::ExplorationSidecarPathBesideMwm(mwmPath);
      RematchStreetPixelsWithNewUniverseUnlocked(countryId, newIds, mapDataVersion, spaPath);
    }
    catch (std::exception const & e)
    {
      LOG(LWARNING, ("Rematch aborted; leaving previous street pixels intact", countryId, e.what()));
    }
  });
}

bool StreetPixelsManager::RematchStreetPixelsWithNewUniverseForTesting(storage::CountryId const & countryId,
                                                                    std::set<std::int64_t> const & newIds,
                                                                    std::int64_t mapDataVersion)
{
  std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
  return RematchStreetPixelsWithNewUniverseUnlocked(countryId, newIds, mapDataVersion);
}

std::optional<StreetPixelsManager::RematchFractionChange> StreetPixelsManager::TakePendingRematchFractionChange(
    storage::CountryId const & forCountryId)
{
  std::lock_guard<std::mutex> lock(m_pendingRematchFractionMutex);
  if (!m_pendingRematchFractionChange)
    return std::nullopt;
  if (!forCountryId.empty() && m_pendingRematchFractionChange->countryId != forCountryId)
    return std::nullopt;
  auto change = std::move(m_pendingRematchFractionChange);
  m_pendingRematchFractionChange.reset();
  return change;
}

std::optional<StreetPixelsManager::AssignmentRematchSignal> StreetPixelsManager::TakePendingAssignmentRematch(
    storage::CountryId const & forCountryId)
{
  std::lock_guard<std::mutex> lock(m_pendingAssignmentRematchMutex);
  if (!m_pendingAssignmentRematch)
    return std::nullopt;
  if (!forCountryId.empty() && m_pendingAssignmentRematch->countryId != forCountryId)
    return std::nullopt;
  auto signal = std::move(m_pendingAssignmentRematch);
  m_pendingAssignmentRematch.reset();
  return signal;
}

bool StreetPixelsManager::RematerializeAssignmentsOnPolicyBump(storage::CountryId const & countryId,
                                                               std::string const & spaPath,
                                                               std::int64_t mapDataVersion,
                                                               uint32_t expectedPolicyVersion)
{
  std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
  auto verified = street_pixels::TryLoadAndVerifyExplorationSidecar(spaPath, mapDataVersion, expectedPolicyVersion);
  if (verified.m_status != street_pixels::SpaLoadStatus::Ok)
  {
    LOG(LWARNING, ("Policy rematerialize skipped; sidecar unavailable", countryId,
                   street_pixels::DebugPrint(verified.m_status)));
    InvalidateAreaCompletionCache();
    return false;
  }
  RefreshSparseAssignmentsBestEffortUnlocked(countryId, spaPath, mapDataVersion, true /* policyOnly */);
  auto const spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), countryId);
  auto loaded = street_pixels::TryLoadAndVerifySparseAssignmentStore(spxPath, mapDataVersion, expectedPolicyVersion);
  return loaded.m_status == street_pixels::SpxLoadStatus::Ok;
}

bool StreetPixelsManager::ReloadStreetPixelsAfterRematchUnlocked(storage::CountryId const & countryId,
                                                                 std::int64_t mapDataVersion)
{
  try
  {
    LoadStreetPixelsFromFile(countryId, mapDataVersion);
    {
      std::shared_lock<std::shared_mutex> lock(m_streetPixelsMutex);
      m_drapeEngine.SafeCall(&df::DrapeEngine::UpdateStreetPixels, m_streetPixels);
    }
    LoadAccountedBits();
    ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::Ready});
    return true;
  }
  catch (std::exception const & e)
  {
    LOG(LERROR, ("Failed to reload street pixels after rematch", countryId, e.what()));
    ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::NotReady});
    return false;
  }
}

bool StreetPixelsManager::RematchStreetPixelsWithNewUniverseUnlocked(storage::CountryId const & countryId,
                                                                     std::set<std::int64_t> const & newIds,
                                                                     std::int64_t mapDataVersion,
                                                                     std::string const & spaPath)
{
  LOG(LINFO, ("RematchStreetPixels", countryId, "newUniverse", newIds.size(), "mapDataVersion", mapDataVersion));

  bool const isActiveCountry = [&]()
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    return m_countryId == countryId;
  }();

  std::string const filePath = GetPlatform().WritablePathForFile(countryId + ".pix");
  std::string const archivePath = GetPlatform().WritablePathForFile(countryId + ".pixr");

  auto const probe = street_pixels_file::ProbeFile(filePath);
  if ((probe.kind == street_pixels_file::FileKind::HeaderedV1 ||
       probe.kind == street_pixels_file::FileKind::HeaderedV2) &&
      probe.header.mapDataVersion == mapDataVersion)
  {
    LOG(LINFO, ("Rematch skipped; map-data version already current", countryId, mapDataVersion));
    Platform::RemoveFileIfExists(archivePath);
    if (!spaPath.empty())
      RefreshSparseAssignmentsBestEffortUnlocked(countryId, spaPath, mapDataVersion, false /* policyOnly */);
    if (isActiveCountry)
      return ReloadStreetPixelsAfterRematchUnlocked(countryId, mapDataVersion);
    return true;
  }

  if (isActiveCountry)
  {
    ClearPixels();
    ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::Loading});
  }

  street_pixels_file::ExploredEverLiveMap seed;
  uint64_t previousTotal = 0;
  uint64_t pixSize = 0;
  bool const pixExists = Platform::GetFileSizeByFullPath(filePath, pixSize) && pixSize > 0;
  bool const archiveExists = Platform::IsFileExistsByFullPath(archivePath);
  if (pixExists)
  {
    previousTotal = CountPixBodyEntries(filePath, pixSize);
    auto const scanned = street_pixels_file::ScanExploredEverLive(filePath);
    if (scanned)
    {
      seed = *scanned;
    }
    else if (archiveExists)
    {
      auto const archived = street_pixels_file::LoadExploredArchive(archivePath);
      if (!archived)
      {
        LOG(LWARNING, ("Rematch archive load failed; leaving previous street pixels intact", countryId));
        if (isActiveCountry)
          ReloadStreetPixelsAfterRematchUnlocked(countryId, mapDataVersion);
        return false;
      }
      seed = *archived;
    }
    else
    {
      LOG(LWARNING, ("Rematch scan failed; leaving previous street pixels intact", countryId));
      if (isActiveCountry)
        ReloadStreetPixelsAfterRematchUnlocked(countryId, mapDataVersion);
      return false;
    }
  }
  else if (archiveExists)
  {
    auto const archived = street_pixels_file::LoadExploredArchive(archivePath);
    if (!archived)
    {
      LOG(LWARNING, ("Rematch archive load failed; leaving explored archive intact", countryId));
      if (isActiveCountry)
        ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::NotReady});
      return false;
    }
    seed = *archived;
  }

  if (!street_pixels_file::SaveRematchedUniverse(filePath, newIds, seed, mapDataVersion))
  {
    LOG(LERROR, ("Rematch failed before commit; leaving previous street pixels intact", countryId));
    if (isActiveCountry)
      ReloadStreetPixelsAfterRematchUnlocked(countryId, mapDataVersion);
    return false;
  }

  {
    uint64_t const previousExplored = seed.size();
    uint64_t const newTotal = newIds.size();
    uint64_t newExplored = 0;
    for (auto const & entry : seed)
    {
      if (newIds.find(entry.first) != newIds.end())
        ++newExplored;
    }
    double const previousFraction =
        previousTotal == 0 ? 0.0 : static_cast<double>(previousExplored) / static_cast<double>(previousTotal);
    double const newFraction =
        newTotal == 0 ? 0.0 : static_cast<double>(newExplored) / static_cast<double>(newTotal);
    bool const decreasedDueToUniverseGrowth = newFraction < previousFraction && newTotal > previousTotal;
    std::lock_guard<std::mutex> lock(m_pendingRematchFractionMutex);
    if (decreasedDueToUniverseGrowth)
    {
      RematchFractionChange change;
      change.countryId = countryId;
      change.previousTotal = previousTotal;
      change.previousExplored = previousExplored;
      change.newTotal = newTotal;
      change.newExplored = newExplored;
      change.previousFraction = previousFraction;
      change.newFraction = newFraction;
      change.decreasedDueToUniverseGrowth = true;
      m_pendingRematchFractionChange = std::move(change);
    }
    else if (m_pendingRematchFractionChange && m_pendingRematchFractionChange->countryId == countryId)
    {
      m_pendingRematchFractionChange.reset();
    }
  }

  Platform::RemoveFileIfExists(GetPlatform().WritablePathForFile(countryId + ".pixa"));
  Platform::RemoveFileIfExists(GetPlatform().WritablePathForFile(countryId + ".pixf"));
  Platform::RemoveFileIfExists(archivePath);
  street_stats::StreetStatsDB::Instance().ReconcileStatsAfterRematch(countryId);

  InvalidateAreaCompletionCache();
  if (!spaPath.empty())
    RefreshSparseAssignmentsBestEffortUnlocked(countryId, spaPath, mapDataVersion, false /* policyOnly */);

  bool const stillActive = [&]()
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    return m_countryId == countryId;
  }();

  if (stillActive)
  {
    if (m_state.status == StreetPixelsStatus::Ready)
      ChangeState(StreetPixelsState{m_state.enabled, StreetPixelsStatus::Loading});
    return ReloadStreetPixelsAfterRematchUnlocked(countryId, mapDataVersion);
  }

  return true;
}

void StreetPixelsManager::RefreshSparseAssignmentsBestEffortUnlocked(storage::CountryId const & countryId,
                                                                     std::string const & spaPath,
                                                                     std::int64_t mapDataVersion,
                                                                     bool policyOnly)
{
  if (spaPath.empty())
  {
    InvalidateAreaCompletionCache();
    return;
  }

  std::string const pixPath = GetPlatform().WritablePathForFile(countryId + ".pix");
  std::string const spxPath = street_pixels::SparseAssignmentPath(GetPlatform().WritableDir(), countryId);

  base::Timer refreshTimer;
  auto const universe = street_pixels_file::ScanUniverseAscending(pixPath);
  auto const exploredMap = street_pixels_file::ScanExploredEverLive(pixPath);
  LOG(LINFO, ("StreetPixels refresh pix scan ms", refreshTimer.ElapsedMilliseconds(), countryId));
  if (!universe || !exploredMap)
  {
    LOG(LWARNING, ("Sparse assignment refresh skipped; .pix unreadable", countryId));
    InvalidateAreaCompletionCache();
    return;
  }

  base::Timer spaTimer;
  auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
  if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok)
  {
    LOG(LINFO, ("Sparse assignment refresh skipped; no sidecar", countryId,
                street_pixels::DebugPrint(sidecar.m_status)));
    InvalidateAreaCompletionCache();
    return;
  }
  if (sidecar.m_file.m_header.m_mapDataVersion != mapDataVersion)
  {
    LOG(LINFO, ("Sparse assignment refresh deferred; sidecar map-data mismatch", countryId,
                sidecar.m_file.m_header.m_mapDataVersion, mapDataVersion));
    InvalidateAreaCompletionCache();
    return;
  }

  auto prior = street_pixels::TryLoadSparseAssignmentStore(spxPath);
  // TryLoad never returns VersionMismatch; treat Ok with wrong versions as stale.
  bool const hadDurablePrior = prior.m_status == street_pixels::SpxLoadStatus::Ok ||
                               prior.m_status == street_pixels::SpxLoadStatus::Corrupt;
  bool const versionsMatch =
      prior.m_status == street_pixels::SpxLoadStatus::Ok &&
      prior.m_store.MatchesVersions(sidecar.m_file.m_header.m_mapDataVersion,
                                    sidecar.m_file.m_header.m_policyVersion);

  auto resolver = street_pixels::ExplorationAreaResolver::TryLoad(
      spaPath, *universe, sidecar.m_file.m_header.m_mapDataVersion, sidecar.m_file.m_header.m_policyVersion);
  LOG(LINFO, ("StreetPixels refresh spa+resolver ms", spaTimer.ElapsedMilliseconds(), countryId));
  if (!resolver)
  {
    LOG(LWARNING, ("Sparse assignment refresh skipped; resolver load failed", countryId));
    InvalidateAreaCompletionCache();
    return;
  }

  std::vector<std::int64_t> exploredAscending;
  std::vector<m2::PointD> centres;
  CollectExploredAscendingWithCentres(*exploredMap, exploredAscending, centres);

  base::Timer spxTimer;
  auto ensured = street_pixels::EnsureSparseAssignmentStore(spxPath, *resolver, exploredAscending, centres);
  LOG(LINFO, ("StreetPixels refresh sparse ensure ms", spxTimer.ElapsedMilliseconds(), countryId,
              "explored", exploredAscending.size()));
  if (!ensured)
  {
    LOG(LWARNING, ("Sparse assignment refresh failed", countryId));
    InvalidateAreaCompletionCache();
    return;
  }

  RebuildAreaCompletionCacheFromLoadedUnlocked(*universe, exploredAscending, *resolver);
  RefreshFocusedAreaFractionUnlocked();

  // Signal version rematch / corrupt rebuild. Quiet exploration catch-up under the
  // same (map, policy) pair does not set the pending signal. policyOnly only
  // annotates the signal when a rematch was requested via the policy-bump API.
  if (hadDurablePrior && (!versionsMatch || prior.m_status == street_pixels::SpxLoadStatus::Corrupt))
  {
    std::lock_guard<std::mutex> lock(m_pendingAssignmentRematchMutex);
    AssignmentRematchSignal signal;
    signal.countryId = countryId;
    signal.mapDataVersion = ensured->GetHeader().m_mapDataVersion;
    signal.policyVersion = ensured->GetHeader().m_policyVersion;
    signal.policyOnly = policyOnly;
    m_pendingAssignmentRematch = std::move(signal);
  }
}

std::set<std::int64_t> StreetPixelsManager::DeriveStreetPixelsFromFeatures(FeaturesVectorTest & featuresVector)
{
  storage::CountryId countryId;
  {
    std::lock_guard<std::mutex> lock(m_countryIdMutex);
    countryId = m_countryId;
  }
  return DeriveStreetPixelsFromFeatures(featuresVector, countryId);
}

std::set<std::int64_t> StreetPixelsManager::DeriveStreetPixelsFromFeatures(FeaturesVectorTest & featuresVector,
                                                                          storage::CountryId const & countryId)
{
  LOG(LINFO, ("DeriveStreetPixelsFromFeatures", countryId));

  if (countryId.empty())
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

  routing::StreetExplorationRoutingOptions const options =
      routing::StreetExplorationRoutingOptions::LoadFromSettings();
  if (!options.m_enabled)
    return 1.0;

  if (GetState().status != StreetPixelsStatus::Ready)
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
  double const strength = options.m_strength / routing::StreetExplorationRoutingOptions::kMaxStrength;
  double constexpr kMaxExplorationPenalty = 9.0;
  return 1.0 + strength * kMaxExplorationPenalty * exploredRatio;
}

bool IsExplorableFeature(feature::GeomType geomType, feature::TypesHolder const & types)
{
  if (geomType != feature::GeomType::Line)
    return false;

  bool isHighway = false;
  bool isPrivate = false;
  bool isBikeAccessible = true;
  bool isPedestrianAccessible = true;
  bool isMotorwayFamily = false;
  bool hasYesBicycle = false;
  bool hardExclude = false;
  Classificator const & c = classif();
  for (uint32_t type : types)
  {
    std::vector<std::string> const path = c.GetFullObjectNamePath(type);
    if (!path.empty() && path[0] == "highway")
    {
      if (path.size() >= 2 &&
          (path[1] == "construction" || path[1] == "elevator" || path[1] == "raceway"))
      {
        hardExclude = true;
      }
      else if (path.size() >= 3 &&
               (path[2] == "driveway" || path[2] == "tunnel" || path[2] == "no-access"))
      {
        hardExclude = true;
      }
      else
      {
        isHighway = true;
        if (path.size() >= 2 && (path[1] == "motorway" || path[1] == "motorway_link"))
          isMotorwayFamily = true;
      }
    }
    if (path.size() >= 2 && path[0] == "hwtag")
    {
      if (path[1] == "private")
        isPrivate = true;
      else if (path[1] == "nobicycle")
        isBikeAccessible = false;
      else if (path[1] == "yesbicycle")
      {
        isBikeAccessible = true;
        hasYesBicycle = true;
      }
      else if (path[1] == "nofoot")
        isPedestrianAccessible = false;
      else if (path[1] == "yesfoot")
        isPedestrianAccessible = true;
    }
  }

  if (!isHighway || isPrivate || hardExclude)
    return false;
  if (isMotorwayFamily)
    return hasYesBicycle;
  return isBikeAccessible || isPedestrianAccessible;
}

bool StreetPixelsManager::IsExplorable(FeatureType & ft) const
{
  return IsExplorableFeature(ft.GetGeomType(), feature::TypesHolder(ft));
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
      MarkExploredPixelIds(trackPixels, static_cast<double>(kml::ToSecondsSinceEpoch(ti.ts)));

      street_stats::StreetStatsDB::Instance().MarkTrackProcessed(geometryHash, countryId);
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
    m2::PointD const curr = geometry::GetPoint(line[i]);
    ForEachMercatorSegmentSample(prev, curr, kPathSamplingStepMeters,
                                 [this](double lat, double lon) { UpdateStreetStats(lat, lon, 1); });
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
    m2::PointD const curr = geometry::GetPoint(trackInfo.geom[i]);
    ForEachMercatorSegmentSample(prev, curr, kPathSamplingStepMeters,
                                 [this, &pixels](double lat, double lon)
                                 { AddPixelsInRadius(lat, lon, pixels); });
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
  if (m_recordingSession == nullptr || !m_recordingSession->IsRecording())
  {
    ResetSampleAcceptanceReference();
    m_filterSessionId = 0;
    return;
  }

  if (m_filterSessionId != m_recordingSession->GetSessionId())
  {
    ResetSampleAcceptanceReference();
    m_filterSessionId = m_recordingSession->GetSessionId();
  }

  double const nowSec = static_cast<double>(base::SecondsSinceEpoch());
  auto const result = m_acceptanceFilter.Evaluate(info, nowSec);
  if (!result.accepted)
  {
    m_segmentInterpolation.MarkInterpolationBarrier();
    return;
  }

  std::set<std::int64_t> pixels;
  if (m_segmentInterpolation.CanInterpolateTo(info))
  {
    ForEachInterpolationSample(m_segmentInterpolation.GetInterpolationOrigin(), info,
                               [this, &pixels](double lat, double lon) { AddPixelsInRadius(lat, lon, pixels); });
  }
  else
  {
    AddPixelsInRadius(info.m_latitude, info.m_longitude, pixels);
  }
  m_segmentInterpolation.SetInterpolationOrigin(info);
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
    bool newlyExplored = false;
    {
      std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
      auto * pixel = FindStreetPixel(pix);
      if (pixel == nullptr || (pixel->IsExplored() && pixel->IsEverLive()))
        continue;
      idx = GetPixelIndexWhileLocked(pixel);
      if (!pixel->IsExplored())
      {
        pixel->SetExplored(true);
        pixel->SetEverLive(true);
        msync(pixel, sizeof(df::StreetPixel), MS_ASYNC);
        ++numNewlyExploredPixels;
        ++m_exploredPixelCount;
        newlyExplored = true;
      }
      else
      {
        pixel->SetEverLive(true);
        msync(pixel, sizeof(df::StreetPixel), MS_ASYNC);
      }
    }

    if (!newlyExplored)
      continue;

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

  if (numNewlyExploredPixels > 0)
    InvalidateAreaCompletionCache();

  if (numNewlyExploredPixels > 0 && m_explorationListener)
  {
    ExplorationDelta d;
    d.m_regionId = countryId;
    d.m_newPixels = static_cast<uint32_t>(numNewlyExploredPixels);
    d.m_eventTimeSec = info.m_timestamp;
    m_explorationListener(d);
  }

  TriggerCollectionVibration(numNewlyExploredPixels);
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
    std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
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

std::optional<street_pixels::AreaCompletionCounts> StreetPixelsManager::GetAreaCompletion(
    uint32_t compactIndex) const
{
  std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
  return m_areaCompletionCache.Get(compactIndex);
}

double StreetPixelsManager::GetAreaCompletionFraction(uint32_t compactIndex) const
{
  std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
  return m_areaCompletionCache.GetFraction(compactIndex);
}

std::optional<street_pixels::AreaCompletionCounts> StreetPixelsManager::GetCityCompletion(
    uint32_t settlementCompactIndex) const
{
  std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
  return m_cityCompletionCache.Get(settlementCompactIndex);
}

double StreetPixelsManager::GetCityCompletionFraction(uint32_t settlementCompactIndex) const
{
  std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
  return m_cityCompletionCache.GetFraction(settlementCompactIndex);
}

bool StreetPixelsManager::IsAreaCompletionCacheValid() const
{
  std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
  return m_areaCompletionCache.IsValid();
}

void StreetPixelsManager::InvalidateAreaCompletionCache()
{
  {
    std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
    InvalidateAreaCompletionCacheUnlocked();
  }
  RefreshFocusedAreaFractionUnlocked();
}

void StreetPixelsManager::InvalidateAreaCompletionCacheUnlocked()
{
  m_areaCompletionCache.Invalidate();
  m_cityCompletionCache.Invalidate();
}

void StreetPixelsManager::RefreshFocusedAreaFractionUnlocked()
{
  std::lock_guard<std::mutex> focusLock(m_focusedAreaMutex);
  if (!m_focusedAreaProgress.m_hasFocus)
    return;

  std::lock_guard<std::mutex> cacheLock(m_areaCompletionMutex);
  if (!m_areaCompletionCache.IsValid())
  {
    m_focusedAreaProgress.m_fractionValid = false;
    m_focusedAreaProgress.m_fraction = 0.0;
    m_focusedAreaProgress.m_areaCompleted = false;
    return;
  }

  std::optional<street_pixels::AreaCompletionCounts> counts;
  if (m_focusedAreaProgress.m_citySummary)
  {
    if (!m_cityCompletionCache.IsValid())
    {
      m_focusedAreaProgress.m_fractionValid = false;
      m_focusedAreaProgress.m_fraction = 0.0;
      m_focusedAreaProgress.m_areaCompleted = false;
      return;
    }
    counts = m_cityCompletionCache.Get(m_focusedAreaProgress.m_compactIndex);
  }
  else
  {
    counts = m_areaCompletionCache.Get(m_focusedAreaProgress.m_compactIndex);
  }

  if (!counts)
  {
    m_focusedAreaProgress.m_fractionValid = false;
    m_focusedAreaProgress.m_fraction = 0.0;
    m_focusedAreaProgress.m_areaCompleted = false;
    return;
  }
  m_focusedAreaProgress.m_fraction = street_pixels::AreaCompletionFraction(*counts);
  m_focusedAreaProgress.m_fractionValid = true;
  m_focusedAreaProgress.m_areaCompleted =
      counts->m_total > 0 && counts->m_explored >= counts->m_total;
}

void StreetPixelsManager::ClearFocusedAreaUnlocked()
{
  m_focusedAreaProgress = street_pixels::FocusedAreaProgress{};
  m_focusedAreaProgress.m_noExplorationArea = true;
}

street_pixels::FocusedAreaProgress StreetPixelsManager::GetFocusedAreaProgress() const
{
  std::lock_guard<std::mutex> lock(m_focusedAreaMutex);
  return m_focusedAreaProgress;
}

void StreetPixelsManager::ClearFocusedArea()
{
  std::lock_guard<std::mutex> lock(m_focusedAreaMutex);
  ClearFocusedAreaUnlocked();
  m_explicitFocusSticky = false;
}

bool StreetPixelsManager::SetFocusedArea(uint32_t compactIndex, std::string const & spaPath, bool citySummary)
{
  auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
  if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok)
  {
    ClearFocusedArea();
    return false;
  }
  auto const * area = street_pixels::FindAreaByCompactIndex(sidecar.m_file, compactIndex);
  if (area == nullptr)
  {
    ClearFocusedArea();
    return false;
  }
  std::string const name = street_pixels::DisplayName(*area);
  if (name.empty())
  {
    ClearFocusedArea();
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(m_focusedAreaMutex);
    m_focusedAreaProgress = street_pixels::FocusedAreaProgress{};
    m_focusedAreaProgress.m_hasFocus = true;
    m_focusedAreaProgress.m_citySummary = citySummary;
    m_focusedAreaProgress.m_compactIndex = area->m_compactIndex;
    m_focusedAreaProgress.m_osmId = street_pixels::StableOsmId(*area);
    m_focusedAreaProgress.m_displayName = name;
  }
  RefreshFocusedAreaFractionUnlocked();
  return true;
}

void StreetPixelsManager::SetFocusedAreaForTesting(uint32_t compactIndex, std::string displayName, uint64_t osmId,
                                                   bool citySummary)
{
  if (displayName.empty())
  {
    ClearFocusedArea();
    return;
  }
  {
    std::lock_guard<std::mutex> lock(m_focusedAreaMutex);
    m_focusedAreaProgress = street_pixels::FocusedAreaProgress{};
    m_focusedAreaProgress.m_hasFocus = true;
    m_focusedAreaProgress.m_citySummary = citySummary;
    m_focusedAreaProgress.m_compactIndex = compactIndex;
    m_focusedAreaProgress.m_osmId = osmId;
    m_focusedAreaProgress.m_displayName = std::move(displayName);
  }
  RefreshFocusedAreaFractionUnlocked();
}

bool StreetPixelsManager::SelectFocusedAreaExplicit(uint32_t compactIndex, std::string const & spaPath)
{
  bool const ok = SetFocusedArea(compactIndex, spaPath, false);
  m_explicitFocusSticky = ok;
  return ok;
}

bool StreetPixelsManager::SelectFocusedAreaAtPoint(m2::PointD const & mercator, std::string const & spaPath,
                                                   int64_t mapDataVersion)
{
  auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
  if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok ||
      sidecar.m_file.m_header.m_mapDataVersion != mapDataVersion)
  {
    ClearFocusedArea();
    return false;
  }

  street_pixels::CountryPolicy policy;
  try
  {
    std::string const policyPath =
        base::JoinPath(GetPlatform().ResourcesDir(), street_pixels::kCountryPoliciesRelativePath);
    auto const config = street_pixels::CountryConfig::LoadFromFile(policyPath);
    policy = config.GetByIso(sidecar.m_file.m_header.m_isoCode);
  }
  catch (RootException const &)
  {
    ClearFocusedArea();
    return false;
  }

  auto const * area = street_pixels::LookupExplorationAreaAtPoint(sidecar.m_file, policy, mercator);
  if (area == nullptr)
  {
    ClearFocusedArea();
    return false;
  }

  street_pixels::FocusSelectionRequest req;
  req.m_event = street_pixels::FocusEvent::ExplicitSelect;
  req.m_explicitAreaCompactIndex = area->m_compactIndex;
  bool const ok = ApplyFocusSelection(req, spaPath, mapDataVersion);
  m_explicitFocusSticky = ok;
  return ok;
}

bool StreetPixelsManager::ApplyFocusSelection(street_pixels::FocusSelectionRequest const & request,
                                              std::string const & spaPath, int64_t mapDataVersion)
{
  auto const decision = street_pixels::SelectFocusedArea(request);
  if (decision.m_kind == street_pixels::FocusTargetKind::None || !decision.m_compactIndex.has_value())
  {
    ClearFocusedArea();
    return false;
  }

  if (mapDataVersion != 0)
  {
    auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
    if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok ||
        sidecar.m_file.m_header.m_mapDataVersion != mapDataVersion)
    {
      ClearFocusedArea();
      return false;
    }
  }

  bool const citySummary = decision.m_kind == street_pixels::FocusTargetKind::CitySummary;
  return SetFocusedArea(*decision.m_compactIndex, spaPath, citySummary);
}

bool StreetPixelsManager::RefreshFocusFromViewport(m2::PointD const & mapCentre,
                                                   std::optional<m2::PointD> const & userPos, bool recordingActive,
                                                   bool followingMyPosition, int drawScale,
                                                   std::string const & spaPath, int64_t mapDataVersion)
{
  auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
  if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok ||
      sidecar.m_file.m_header.m_mapDataVersion != mapDataVersion)
  {
    ClearFocusedArea();
    return false;
  }

  street_pixels::CountryPolicy policy;
  try
  {
    std::string const policyPath =
        base::JoinPath(GetPlatform().ResourcesDir(), street_pixels::kCountryPoliciesRelativePath);
    auto const config = street_pixels::CountryConfig::LoadFromFile(policyPath);
    policy = config.GetByIso(sidecar.m_file.m_header.m_isoCode);
  }
  catch (RootException const &)
  {
    ClearFocusedArea();
    return false;
  }

  auto resolve = [&](m2::PointD const & pt) -> std::optional<uint32_t>
  {
    auto const * area = street_pixels::LookupExplorationAreaAtPoint(sidecar.m_file, policy, pt);
    if (area == nullptr)
      return std::nullopt;
    return area->m_compactIndex;
  };

  auto resolveCity = [&](m2::PointD const & pt) -> std::optional<uint32_t>
  {
    auto const * city = street_pixels::SelectSettlementContaining(sidecar.m_file, pt);
    if (city == nullptr)
      return std::nullopt;
    return city->m_compactIndex;
  };

  street_pixels::FocusSelectionRequest req;
  req.m_recordingActive = recordingActive;
  req.m_atCityScale = street_pixels::IsCityScaleDrawScale(drawScale);
  req.m_mapCentreAreaCompactIndex = resolve(mapCentre);
  if (userPos.has_value())
  {
    req.m_userAreaCompactIndex = resolve(*userPos);
    req.m_cityCompactIndex = resolveCity(*userPos);
  }
  if (!req.m_cityCompactIndex.has_value())
    req.m_cityCompactIndex = resolveCity(mapCentre);

  if (req.m_atCityScale)
    req.m_event = street_pixels::FocusEvent::ZoomChanged;
  else if (recordingActive)
    req.m_event = street_pixels::FocusEvent::RecordingOrUserLocation;
  else if (followingMyPosition)
    req.m_event = street_pixels::FocusEvent::Recentre;
  else
    req.m_event = street_pixels::FocusEvent::MapPan;

  if (m_explicitFocusSticky && req.m_event == street_pixels::FocusEvent::MapPan && !req.m_atCityScale)
  {
    RefreshFocusedAreaFractionUnlocked();
    return true;
  }

  m_explicitFocusSticky = false;
  return ApplyFocusSelection(req, spaPath, mapDataVersion);
}

bool StreetPixelsManager::TryFocusAtPoint(m2::PointD const & mercator, std::string const & spaPath,
                                          int64_t mapDataVersion)
{
  street_pixels::FocusSelectionRequest req;
  req.m_event = street_pixels::FocusEvent::MapPan;
  req.m_recordingActive = false;
  req.m_atCityScale = false;

  auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
  if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok ||
      sidecar.m_file.m_header.m_mapDataVersion != mapDataVersion)
  {
    ClearFocusedArea();
    return false;
  }

  street_pixels::CountryPolicy policy;
  try
  {
    std::string const policyPath =
        base::JoinPath(GetPlatform().ResourcesDir(), street_pixels::kCountryPoliciesRelativePath);
    auto const config = street_pixels::CountryConfig::LoadFromFile(policyPath);
    policy = config.GetByIso(sidecar.m_file.m_header.m_isoCode);
  }
  catch (RootException const &)
  {
    ClearFocusedArea();
    return false;
  }

  auto const * area = street_pixels::LookupExplorationAreaAtPoint(sidecar.m_file, policy, mercator);
  if (area == nullptr)
  {
    ClearFocusedArea();
    return false;
  }
  req.m_mapCentreAreaCompactIndex = area->m_compactIndex;
  return ApplyFocusSelection(req, spaPath, mapDataVersion);
}

bool StreetPixelsManager::RebuildAreaCompletionCache(storage::CountryId const & countryId,
                                                    std::string const & spaPath, int64_t mapDataVersion)
{
  std::lock_guard<std::mutex> pixLock(m_pixFileMutex);
  bool const ok = RebuildAreaCompletionCacheUnlocked(countryId, spaPath, mapDataVersion);
  if (ok)
    RefreshFocusedAreaFractionUnlocked();
  return ok;
}

bool StreetPixelsManager::RebuildAreaCompletionCacheUnlocked(storage::CountryId const & countryId,
                                                             std::string const & spaPath, int64_t mapDataVersion)
{
  auto failClosed = [this]()
  {
    std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
    InvalidateAreaCompletionCacheUnlocked();
  };

  if (spaPath.empty() || countryId.empty())
  {
    failClosed();
    return false;
  }

  std::string const pixPath = GetPlatform().WritablePathForFile(countryId + ".pix");
  base::Timer scanTimer;
  auto const universe = street_pixels_file::ScanUniverseAscending(pixPath);
  auto const exploredMap = street_pixels_file::ScanExploredEverLive(pixPath);
  LOG(LINFO, ("StreetPixels rebuild pix scan ms", scanTimer.ElapsedMilliseconds(), countryId));
  if (!universe || !exploredMap)
  {
    failClosed();
    return false;
  }

  base::Timer spaTimer;
  auto sidecar = street_pixels::TryLoadExplorationSidecar(spaPath);
  if (sidecar.m_status != street_pixels::SpaLoadStatus::Ok ||
      sidecar.m_file.m_header.m_mapDataVersion != mapDataVersion)
  {
    failClosed();
    return false;
  }

  auto resolver = street_pixels::ExplorationAreaResolver::TryLoad(
      spaPath, *universe, mapDataVersion, sidecar.m_file.m_header.m_policyVersion);
  LOG(LINFO, ("StreetPixels rebuild spa+resolver ms", spaTimer.ElapsedMilliseconds(), countryId));
  if (!resolver)
  {
    failClosed();
    return false;
  }

  std::vector<std::int64_t> exploredAscending;
  std::vector<m2::PointD> ignoredCentres;
  CollectExploredAscendingWithCentres(*exploredMap, exploredAscending, ignoredCentres);

  return RebuildAreaCompletionCacheFromLoadedUnlocked(*universe, exploredAscending, *resolver);
}

bool StreetPixelsManager::RebuildAreaCompletionCacheFromLoadedUnlocked(
    std::vector<std::int64_t> const & universeAscending, std::vector<std::int64_t> const & exploredAscending,
    street_pixels::ExplorationAreaResolver const & resolver)
{
  base::Timer buildTimer;
  // Empty centres: Build computes Mercator centres only for sentinel slots.
  auto built = street_pixels::AreaCompletionCache::Build(resolver, universeAscending, {}, exploredAscending);
  size_t sentinelSlots = 0;
  {
    uint32_t const sentinel =
        street_pixels::NoSubdivisionSentinel(resolver.GetFile().m_header.m_indexWidth);
    for (uint32_t assign : resolver.GetFile().m_assignments)
    {
      if (assign == sentinel)
        ++sentinelSlots;
    }
  }
  LOG(LINFO, ("StreetPixels AreaCompletionCache::Build ms", buildTimer.ElapsedMilliseconds(), "universe",
              universeAscending.size(), "explored", exploredAscending.size(), "sentinelSlots", sentinelSlots,
              "settlements", resolver.Settlements().Size()));

  base::Timer cityTimer;
  auto cityBuilt = street_pixels::CityCompletionCache::Build(resolver.GetFile(), built);
  LOG(LINFO, ("StreetPixels CityCompletionCache::Build ms", cityTimer.ElapsedMilliseconds()));
  {
    std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
    m_areaCompletionCache = std::move(built);
    m_cityCompletionCache = std::move(cityBuilt);
  }

  base::Timer overlayTimer;
  PushExplorationAreaOverlayUnlocked(resolver.GetFile());
  LOG(LINFO, ("StreetPixels overlay push ms", overlayTimer.ElapsedMilliseconds()));
  return true;
}

void StreetPixelsManager::PushExplorationAreaOverlayUnlocked(street_pixels::SpaFile const & file)
{
  uint32_t maxIndex = 0;
  for (auto const & area : file.m_areas)
    maxIndex = std::max(maxIndex, area.m_compactIndex);

  std::vector<std::optional<double>> fractions(static_cast<size_t>(maxIndex) + 1);
  {
    std::lock_guard<std::mutex> lock(m_areaCompletionMutex);
    for (auto const & area : file.m_areas)
    {
      auto const counts = m_areaCompletionCache.Get(area.m_compactIndex);
      if (counts)
        fractions[area.m_compactIndex] = street_pixels::AreaCompletionFraction(*counts);
    }
  }

  auto geometries = street_pixels::BuildAreaOverlayGeometry(file, fractions, nullptr);
  std::vector<df::ExplorationAreaOverlayItem> items;
  items.reserve(geometries.size());
  for (auto & geom : geometries)
  {
    auto const style =
        street_pixels::StyleForCompletion(geom.m_fraction, street_pixels::AreaOverlayZoomBand::Neighbourhood);
    df::ExplorationAreaOverlayItem item;
    item.m_compactIndex = geom.m_compactIndex;
    item.m_fraction = geom.m_fraction;
    item.m_completed = style.m_completed;
    item.m_rings = std::move(geom.m_rings);
    item.m_triangles = std::move(geom.m_triangles);
    item.m_bounds = geom.m_bounds;
    if (style.m_showFill)
      item.m_fillColor = dp::Color(style.m_fill.m_r, style.m_fill.m_g, style.m_fill.m_b, style.m_fill.m_a);
    else
      item.m_fillColor = dp::Color(0, 0, 0, 0);
    item.m_outlineColor =
        dp::Color(style.m_outline.m_r, style.m_outline.m_g, style.m_outline.m_b, style.m_outline.m_a);
    item.m_outlineWidthPx = style.m_outlineWidthPx;
    items.push_back(std::move(item));
  }
  m_drapeEngine.SafeCall(&df::DrapeEngine::UpdateExplorationAreaOverlay, std::move(items));
}

void StreetPixelsManager::ClearPixels()
{
  LOG(LINFO, ("Clearing pixels and unmapping pix file"));
  m_drapeEngine.SafeCall(&df::DrapeEngine::ClearStreetPixels);
  m_drapeEngine.SafeCall(&df::DrapeEngine::ClearExplorationAreaOverlay);
  {
    std::lock_guard<std::shared_mutex> lock(m_streetPixelsMutex);
    m_streetPixels = {};
    m_mmapReader.reset();
    m_exploredPixelCount = 0;
    m_pixMapDataVersion = 0;
  }
  m_accountedBits.clear();
  m_accountedDirty = false;
  InvalidateAreaCompletionCache();
  ClearFocusedArea();

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
