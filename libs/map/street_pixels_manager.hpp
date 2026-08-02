#pragma once

#include "map/bookmark_manager.hpp"

#include "map/live_sample_acceptance_filter.hpp"

#include "platform/location.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"
#include "drape_frontend/street_pixel.hpp"

#include "drape/color.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/mmap_reader.hpp"

#include "indexer/data_source.hpp"
#include "indexer/features_vector.hpp"

#include "routing/geometry.hpp"
#include "routing/segment.hpp"

#include "storage/storage.hpp"

#include <healpix_base.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace hp
{
T_Healpix_Base<std::int64_t> const & GetHealpixBase();
}  // namespace hp

struct TrackInfo
{
  kml::TrackId id;
  kml::MultiGeometry::LineT geom;
  kml::Timestamp ts;
};

class RecordingSession;

class StreetPixelsManager
{
public:
  enum class StreetPixelsStatus
  {
    NotReady,
    Loading,
    Ready,
  };

  struct StreetPixelsState
  {
    bool enabled = false;
    StreetPixelsStatus status = StreetPixelsStatus::NotReady;
  };

  using StreetPixelsStateChangedFn =
      std::function<void(bool enabled, StreetPixelsStatus status, std::string countryId)>;

  StreetPixelsManager(DataSource const & dataSource);

  StreetPixelsState GetState() const;
  void SetStateListener(StreetPixelsStateChangedFn const & onStateChangedFn);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetBookmarkManager(BookmarkManager * bmManager);

  void OnBookmarksCreated();
  void LoadStreetPixels(storage::LocalFilePtr const & localFile);

  std::set<std::int64_t> DeriveStreetPixelsFromFeatures(FeaturesVectorTest & featuresVector);
  void LoadStreetPixelsFromFile(storage::CountryId const & countryId);
  void SaveStreetPixelsToFile(std::set<std::int64_t> const & streetPixels);

  void CleanupStreetPixels(storage::CountryId const & countryId);

  void ClearPixels();

  void UpdateExploredPixels();

  double GetTotalExploredFraction() const;

  void OnUpdateCurrentCountry(storage::CountryId const & countryId, storage::LocalFilePtr const & localFile);

  void OnLocationUpdate(location::GpsInfo const & info);

  double GetSegmentExplorationWeightMultiplier(std::string const & mwmCountryName, routing::Segment const & segment,
                                               routing::RoadGeometry const & road) const;

  struct ExplorationDelta
  {
    std::string m_regionId;
    uint32_t m_newPixels = 0;
    double m_eventTimeSec = 0.0;
  };
  using ExplorationListener = std::function<void(ExplorationDelta const &)>;
  void SetExplorationListener(ExplorationListener const & listener);

  void SetRecordingSession(RecordingSession const * session);

  void ResetSampleAcceptanceReference();
  SampleRejectReason GetLastSampleRejectReason() const;

  using VibrationHandler = std::function<void(size_t newlyExplored)>;
  void SetVibrationHandler(VibrationHandler const & handler);

  void SetStreetPixelsForTesting(std::vector<df::StreetPixel> pixels);
  size_t MarkTrackPixelsForTesting(std::set<std::int64_t> const & pixelIds);
  bool IsPixelExploredForTesting(std::int64_t pixelId) const;

private:
  DataSource const & m_dataSource;

  StreetPixelsState m_state;
  StreetPixelsStateChangedFn m_onStateChangedFn;
  mutable std::mutex m_stateMutex;

  void ChangeState(StreetPixelsState newState);

  storage::CountryId m_countryId;
  mutable std::mutex m_countryIdMutex;

  df::DrapeEngineSafePtr m_drapeEngine;

  BookmarkManager * m_bmManager = nullptr;

  std::span<df::StreetPixel> m_streetPixels;
  mutable std::shared_mutex m_streetPixelsMutex;
  size_t m_exploredPixelCount = 0;

  std::unique_ptr<MmapReader> m_mmapReader;

  df::StreetPixel const * FindStreetPixel(std::int64_t pixelId) const;
  df::StreetPixel * FindStreetPixel(std::int64_t pixelId);

  bool m_tracksLoaded = false;

  void UpdateStreetStatsForTrack(kml::MultiGeometry::LineT const & line);

  void SegmentizeStreet(m2::PointD const & p1, m2::PointD const & p2,
                        std::function<void(m2::PointD const &, double)> const & callback) const;

  std::int64_t ComputeGeometryHash(TrackInfo const & trackInfo);
  std::set<std::int64_t> ComputeTrackPixels(TrackInfo const & trackInfo) const;
  void AddPixelsInRadius(double lat, double lon, std::set<std::int64_t> & pixels) const;
  bool IsExplorable(FeatureType & ft) const;

  std::string GetCurrentCountryId() const;
  ExplorationListener m_explorationListener;
  RecordingSession const * m_recordingSession = nullptr;
  LiveSampleAcceptanceFilter m_acceptanceFilter;
  uint64_t m_filterSessionId = 0;
  VibrationHandler m_vibrationHandler;
  std::vector<df::StreetPixel> m_testStreetPixelsStorage;

  void TriggerCollectionVibration(size_t numNewlyExploredPixels);
  size_t MarkExploredPixelIds(std::set<std::int64_t> const & pixelIds, double eventTimeSec);

  // Updates heuristic stats for each street in the explore radius. Needed for routing to prefer streets with more
  // unexplored pixels.
  void UpdateStreetStats(double lat, double lon, size_t numNewlyExploredPixels);

  // Accounted bitset (.pixa) for stats aggregation
  std::vector<uint8_t> m_accountedBits;
  bool m_accountedDirty = false;
  void LoadAccountedBits();
  void SaveAccountedBits();
  std::string GetAccountedFilePath() const;
  bool IsAccountedIndex(size_t idx) const;
  void SetAccountedIndex(size_t idx);
  void ApplyAccountedIndex(size_t idx, size_t totalPixels);
  size_t GetPixelIndex(df::StreetPixel const * ptr) const;
  size_t GetPixelIndexWhileLocked(df::StreetPixel const * ptr) const;
};