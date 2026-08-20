#pragma once

#include "map/bookmark_manager.hpp"

#include "map/area_milestone_presentation.hpp"
#include "map/exploration_haptics.hpp"
#include "map/first_goal.hpp"
#include "map/live_sample_acceptance_filter.hpp"
#include "map/live_segment_interpolation.hpp"

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

#include "street_pixels_areas/area_completion_cache.hpp"
#include "street_pixels_areas/area_milestone_store.hpp"
#include "street_pixels_areas/areas_types.hpp"
#include "street_pixels_areas/city_completion_cache.hpp"
#include "street_pixels_areas/completion_card.hpp"
#include "street_pixels_areas/exploration_area_resolver.hpp"
#include "street_pixels_areas/focus_selection_engine.hpp"
#include "street_pixels_areas/focused_area_progress.hpp"

#include <healpix_base.h>

#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class ScreenBase;

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

bool IsExplorableFeature(feature::GeomType geomType, feature::TypesHolder const & types);

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
  using FocusedAreaProgressChangedFn = std::function<void(street_pixels::FocusedAreaProgress const &)>;
  using ExplorationAreaTappedFn = std::function<void(street_pixels::FocusedAreaProgress const &)>;

  StreetPixelsManager(DataSource const & dataSource);

  StreetPixelsState GetState() const;
  void SetStateListener(StreetPixelsStateChangedFn const & onStateChangedFn);
  void SetFocusedAreaProgressListener(FocusedAreaProgressChangedFn const & fn);
  void SetExplorationAreaTapListener(ExplorationAreaTappedFn const & fn);
  void NotifyExplorationAreaTapped();

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  void SetBookmarkManager(BookmarkManager * bmManager);

  void OnBookmarksCreated();
  void LoadStreetPixels(storage::LocalFilePtr const & localFile);

  std::set<std::int64_t> DeriveStreetPixelsFromFeatures(FeaturesVectorTest & featuresVector);
  std::set<std::int64_t> DeriveStreetPixelsFromFeatures(FeaturesVectorTest & featuresVector,
                                                       storage::CountryId const & countryId);
  void LoadStreetPixelsFromFile(storage::CountryId const & countryId, std::int64_t mapDataVersion);
  void SaveStreetPixelsToFile(std::set<std::int64_t> const & streetPixels, std::int64_t mapDataVersion);

  void RematchStreetPixelsOnMapUpdate(storage::CountryId const & countryId,
                                      storage::LocalFilePtr const & localFile);
  bool RematchStreetPixelsWithNewUniverseForTesting(storage::CountryId const & countryId,
                                                    std::set<std::int64_t> const & newIds,
                                                    std::int64_t mapDataVersion);

  struct RematchFractionChange
  {
    storage::CountryId countryId;
    uint64_t previousTotal = 0;
    uint64_t previousExplored = 0;
    uint64_t newTotal = 0;
    uint64_t newExplored = 0;
    double previousFraction = 0.0;
    double newFraction = 0.0;
    bool decreasedDueToUniverseGrowth = false;
  };

  std::optional<RematchFractionChange> TakePendingRematchFractionChange(
      storage::CountryId const & forCountryId = {});

  struct AssignmentRematchSignal
  {
    storage::CountryId countryId;
    int64_t mapDataVersion = 0;
    uint32_t policyVersion = 0;
    bool policyOnly = false;
  };

  // Optional pending signal when sparse assignments were rematerialized (no UI).
  std::optional<AssignmentRematchSignal> TakePendingAssignmentRematch(
      storage::CountryId const & forCountryId = {});

  // Policy-only rematerialize of `{countryId}.spx` from a versioned `.spa`.
  // Does not wipe `.pix` exploration. Returns false if sidecar/universe unavailable.
  bool RematerializeAssignmentsOnPolicyBump(storage::CountryId const & countryId, std::string const & spaPath,
                                            int64_t mapDataVersion, uint32_t expectedPolicyVersion);

  void CleanupStreetPixels(storage::CountryId const & countryId);
  void CleanupStreetPixelsForTesting(storage::CountryId const & countryId);

  void ClearPixels();

  void UpdateExploredPixels();

  std::int64_t GetPixMapDataVersion() const;

  double GetTotalExploredFraction() const;

  // Area-scoped personal completion (SPD-026). Keyed by compact area index, never MWM id.
  // Fail-closed: nullopt / 0 when cache is invalid or the area is unknown.
  std::optional<street_pixels::AreaCompletionCounts> GetAreaCompletion(uint32_t compactIndex) const;
  double GetAreaCompletionFraction(uint32_t compactIndex) const;
  // City rollup when focus is city-summary mode (SP-039). Fail-closed if cache invalid.
  std::optional<street_pixels::AreaCompletionCounts> GetCityCompletion(uint32_t settlementCompactIndex) const;
  double GetCityCompletionFraction(uint32_t settlementCompactIndex) const;
  bool IsAreaCompletionCacheValid() const;
  void InvalidateAreaCompletionCache();
  // Rebuild from `{countryId}.pix` + sidecar. Returns false if universe/sidecar unavailable.
  bool RebuildAreaCompletionCache(storage::CountryId const & countryId, std::string const & spaPath,
                                  int64_t mapDataVersion);

  std::optional<street_pixels::AreaMilestoneRecord> GetAreaMilestoneRecord(uint64_t osmId) const;
  std::optional<street_pixels::AreaMilestoneRecord> GetAreaMilestoneRecordByCompactIndex(
      uint32_t compactIndex) const;
  std::vector<street_pixels::AreaMilestoneCrossing> ConsumePendingAreaMilestoneCrossings();
  bool WasAreaPreviouslyCompletedBelow100(uint32_t compactIndex) const;
  void ConfigureAreaMilestoneStoreForTesting(std::string const & dbPath);

  // Focused-area progress for the primary badge (SP-035 / SP-036 §12.5).
  street_pixels::FocusedAreaProgress GetFocusedAreaProgress() const;
  void ClearFocusedArea();
  // Loads display name from sidecar; never falls back to countryId. Blank name → clear.
  bool SetFocusedArea(uint32_t compactIndex, std::string const & spaPath, bool citySummary = false);
  // §12.5 engine: resolve areas from spa and apply SelectFocusedArea.
  bool ApplyFocusSelection(street_pixels::FocusSelectionRequest const & request, std::string const & spaPath,
                           int64_t mapDataVersion);
  // Point → area lookup then SetFocusedArea (used by engine helpers / tests).
  bool TryFocusAtPoint(m2::PointD const & mercator, std::string const & spaPath, int64_t mapDataVersion);
  void SetFocusedAreaForTesting(uint32_t compactIndex, std::string displayName, uint64_t osmId,
                                bool citySummary = false);
  // Explicit tap selection (§12.5 rule 3). Hit-test wiring is SP-038.
  bool SelectFocusedAreaExplicit(uint32_t compactIndex, std::string const & spaPath);
  // Polygon hit-test at point → ExplicitSelect (not MapPan). Outside → clear focus.
  bool SelectFocusedAreaAtPoint(m2::PointD const & mercator, std::string const & spaPath, int64_t mapDataVersion);
  bool HasExplorationAreaAtPoint(m2::PointD const & mercator, std::string const & spaPath, int64_t mapDataVersion);
  std::optional<uint32_t> HitOverlayLabel(m2::PointD const & mercator, ScreenBase const & screen) const;
  // Resolve §12.5 inputs from viewport/session and apply the engine.
  bool RefreshFocusFromViewport(m2::PointD const & mapCentre, std::optional<m2::PointD> const & userPos,
                                bool recordingActive, bool followingMyPosition, int drawScale,
                                std::string const & spaPath, int64_t mapDataVersion,
                                storage::CountryId const & countryId = {});
  bool CanSkipFocusRefresh(m2::PointD const & mapCentre, int drawScale, bool recordingActive,
                           bool followingMyPosition);

  void OnUpdateCurrentCountry(storage::CountryId const & countryId, storage::LocalFilePtr const & localFile);

  void OnLocationUpdate(location::GpsInfo const & info);

  double GetSegmentExplorationWeightMultiplier(std::string const & mwmCountryName, routing::Segment const & segment,
                                               routing::RoadGeometry const & road) const;

  bool IsSegmentExcludedForAvoid(std::string const & mwmCountryName, routing::Segment const & segment,
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
  void MarkInterpolationBarrier();
  SampleRejectReason GetLastSampleRejectReason() const;

  using VibrationHandler = std::function<void(street_pixels::ExplorationHapticKind kind)>;
  void SetVibrationHandler(VibrationHandler const & handler);
  void SetApplicationForeground(bool foreground);

  using FirstGoalProgressChangedFn = std::function<void(street_pixels::FirstGoalProgress const &)>;
  using FirstGoalCompleteFn = std::function<void()>;
  void SetFirstGoalProgressListener(FirstGoalProgressChangedFn const & fn);
  void SetFirstGoalCompleteHandler(FirstGoalCompleteFn const & fn);
  street_pixels::FirstGoalProgress GetFirstGoalProgress() const;
  void OnRecordingSessionStateChanged();
  void ResetFirstGoalForTesting();

  using AreaMilestonePresentationChangedFn =
      std::function<void(std::optional<street_pixels::AreaMilestonePresentation> const &)>;
  using AreaMilestoneHapticFn = std::function<void(street_pixels::AreaMilestoneHapticEvent)>;
  void SetAreaMilestonePresentationListener(AreaMilestonePresentationChangedFn const & fn);
  void SetAreaMilestoneHapticHandler(AreaMilestoneHapticFn const & fn);
  std::optional<street_pixels::AreaMilestonePresentation> GetCurrentAreaMilestonePresentation() const;
  void AcknowledgeAreaMilestonePresentation();
  void ResetAreaMilestonePresentationForTesting();

  using CompletionCardGeneratedFn = std::function<void()>;
  void SetCompletionCardGeneratedHandler(CompletionCardGeneratedFn const & fn);
  std::optional<street_pixels::CompletionCardModel> GetCompletionCardForCurrentPresentation(
      bool includeDate = false, bool recordGenerated = true);
  std::optional<street_pixels::CompletionCardSharePayload> PrepareCompletionCardShare(bool includeDate);
  void RecordCompletionCardShareInitiated();

  void SetStreetPixelsForTesting(std::vector<df::StreetPixel> pixels);
  void SetStreetPixelsOverlayForTesting(storage::CountryId const & countryId, std::vector<df::StreetPixel> pixels);
  void ClearLeafPixCacheForTesting();
  void EvictLeafPixForTesting(storage::CountryId const & countryId);
  size_t MarkTrackPixelsForTesting(std::set<std::int64_t> const & pixelIds);
  size_t MarkImportedPixelsForTesting(std::set<std::int64_t> const & pixelIds);
  bool IsPixelExploredForTesting(std::int64_t pixelId) const;
  bool IsPixelEverLiveForTesting(std::int64_t pixelId) const;

private:
  DataSource const & m_dataSource;

  StreetPixelsState m_state;
  StreetPixelsStateChangedFn m_onStateChangedFn;
  FocusedAreaProgressChangedFn m_focusedAreaProgressListener;
  ExplorationAreaTappedFn m_explorationAreaTapListener;
  street_pixels::FocusedAreaProgress m_lastNotifiedFocusedAreaProgress;
  mutable std::mutex m_stateMutex;

  void ChangeState(StreetPixelsState newState);
  void NotifyFocusedAreaProgressIfChanged();
  bool LoadFocusSidecar(std::string const & spaPath, int64_t mapDataVersion);

  storage::CountryId m_countryId;
  mutable std::mutex m_countryIdMutex;

  df::DrapeEngineSafePtr m_drapeEngine;

  BookmarkManager * m_bmManager = nullptr;

  std::span<df::StreetPixel> m_streetPixels;
  mutable std::shared_mutex m_streetPixelsMutex;
  size_t m_exploredPixelCount = 0;
  std::int64_t m_pixMapDataVersion = 0;

  std::unique_ptr<MmapReader> m_mmapReader;

  df::StreetPixel const * FindStreetPixel(std::int64_t pixelId) const;
  df::StreetPixel * FindStreetPixel(std::int64_t pixelId);

  bool m_tracksLoaded = false;

  void UpdateStreetStatsForTrack(kml::MultiGeometry::LineT const & line);

  void SegmentizeStreet(m2::PointD const & p1, m2::PointD const & p2,
                        std::function<void(m2::PointD const &, double)> const & callback) const;

  double ExploredRatioForSegment(std::string const & mwmCountryName, routing::Segment const & segment,
                                 routing::RoadGeometry const & road) const;

  std::int64_t ComputeGeometryHash(TrackInfo const & trackInfo);
  std::set<std::int64_t> ComputeTrackPixels(TrackInfo const & trackInfo) const;
  void AddPixelsInRadius(double lat, double lon, std::set<std::int64_t> & pixels) const;
  bool IsExplorable(FeatureType & ft) const;

  std::string GetCurrentCountryId() const;
  ExplorationListener m_explorationListener;
  RecordingSession const * m_recordingSession = nullptr;
  LiveSampleAcceptanceFilter m_acceptanceFilter;
  LiveSegmentInterpolation m_segmentInterpolation;
  uint64_t m_filterSessionId = 0;
  VibrationHandler m_vibrationHandler;
  bool m_applicationForeground = false;
  FirstGoalProgressChangedFn m_firstGoalProgressListener;
  FirstGoalCompleteFn m_firstGoalCompleteHandler;
  street_pixels::FirstGoalTracker m_firstGoalTracker;
  street_pixels::FirstGoalProgress m_lastNotifiedFirstGoalProgress;
  AreaMilestonePresentationChangedFn m_areaMilestonePresentationListener;
  AreaMilestoneHapticFn m_areaMilestoneHapticHandler;
  street_pixels::AreaMilestonePresenter m_areaMilestonePresenter;
  CompletionCardGeneratedFn m_completionCardGeneratedFn;
  std::vector<df::StreetPixel> m_testStreetPixelsStorage;

  void TriggerCollectionVibration(size_t numNewlyExploredPixels);
  void PlayExplorationHaptic(street_pixels::ExplorationHapticKind kind);
  void NotifyFirstGoalProgressIfChanged();
  bool IsFirstGoalSessionActive() const;
  size_t MarkExploredPixelIds(std::set<std::int64_t> const & pixelIds, double eventTimeSec);

  bool RematchStreetPixelsWithNewUniverseUnlocked(storage::CountryId const & countryId,
                                                  std::set<std::int64_t> const & newIds,
                                                  std::int64_t mapDataVersion,
                                                  std::string const & spaPath = {});
  bool ReloadStreetPixelsAfterRematchUnlocked(storage::CountryId const & countryId, std::int64_t mapDataVersion);
  void CleanupStreetPixelsUnlocked(storage::CountryId const & countryId);
  void RefreshSparseAssignmentsBestEffortUnlocked(storage::CountryId const & countryId, std::string const & spaPath,
                                                  std::int64_t mapDataVersion, bool policyOnly);
  void InvalidateAreaCompletionCacheUnlocked();
  bool RebuildAreaCompletionCacheUnlocked(storage::CountryId const & countryId, std::string const & spaPath,
                                          int64_t mapDataVersion);
  bool RebuildAreaCompletionCacheFromLoadedUnlocked(std::vector<std::int64_t> const & universeAscending,
                                                    std::vector<std::int64_t> const & exploredAscending,
                                                    street_pixels::ExplorationAreaResolver const & resolver);
  void EvaluateAreaMilestonesUnlocked(int64_t nowSec);
  void IngestPendingAreaMilestonePresentations(street_pixels::SpaFile const & file);
  void NotifyAreaMilestonePresentationIfChanged(
      std::optional<street_pixels::AreaMilestonePresentation> const & before);
  void EmitAreaMilestoneHapticIfNeeded(std::optional<street_pixels::AreaMilestonePresentation> const & head);
  void PushExplorationAreaOverlayUnlocked(street_pixels::SpaFile const & file);
  void RefreshFocusedAreaFractionUnlocked();
  void ClearFocusedAreaUnlocked();

  struct OverlayLabel
  {
    uint32_t m_compactIndex = 0;
    m2::PointD m_labelPoint;
    m2::PointD m_halfSizePx;
  };
  std::vector<OverlayLabel> m_overlayLabels;

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

  mutable std::mutex m_pixFileMutex;

  struct LeafPixMapping
  {
    storage::CountryId countryId;
    std::unique_ptr<MmapReader> reader;
    std::span<df::StreetPixel const> pixels;
  };

  static constexpr size_t kMaxLeafPixCache = 4;
  mutable std::mutex m_leafPixMutex;
  mutable std::list<LeafPixMapping> m_leafPixLru;

  std::optional<LeafPixMapping> TryOpenLeafPix(storage::CountryId const & countryId) const;
  std::span<df::StreetPixel const> LookupLeafPixUnlocked(storage::CountryId const & countryId) const;
  void EvictLeafPix(storage::CountryId const & countryId);

  std::optional<RematchFractionChange> m_pendingRematchFractionChange;
  mutable std::mutex m_pendingRematchFractionMutex;

  std::optional<AssignmentRematchSignal> m_pendingAssignmentRematch;
  mutable std::mutex m_pendingAssignmentRematchMutex;

  street_pixels::AreaCompletionCache m_areaCompletionCache;
  street_pixels::CityCompletionCache m_cityCompletionCache;
  mutable std::mutex m_areaCompletionMutex;

  street_pixels::FocusedAreaProgress m_focusedAreaProgress;
  bool m_explicitFocusSticky = false;
  mutable std::mutex m_focusedAreaMutex;

  mutable std::mutex m_focusCacheMutex;
  std::string m_cachedFocusSpaPath;
  int64_t m_cachedFocusSpaVersion = 0;
  bool m_cachedFocusSpaValid = false;
  street_pixels::SpaFile m_cachedFocusSpaFile;
  street_pixels::CountryPolicy m_cachedFocusPolicy;

  m2::PointD m_lastFocusMapCentre{0.0, 0.0};
  int m_lastFocusDrawScale = 0;
  bool m_lastFocusRecording = false;
  bool m_lastFocusFollowing = false;
  bool m_hasLastFocusRefresh = false;
};