# SP-072 — Recency store, ownership, eligibility, contested, unclaimed

**Phase:** 8 — Competition
**Status:** Accepted
**Branch:** `cursor/sp-072-recency-ownership-scoring-f95c`
**Depends on:** SP-070 (SPD-057, SPD-058, SPD-063); Phase 3 ever-live bit;
  Phase 4 area OSM ids
**Unblocks:** SP-074 (aggregates), SP-078 (area snapshot chrome)

---

## Objective

Store sparse last-live-visit recency, compute ownership scores from
SPD-057, and evaluate boss eligibility, unclaimed, and contested
(SPD-058) locally. Imported pixels never contribute.

## Motivation

No recency or ownership code exists. `.pix` has an ever-live bit only
(SPD-015). Competition scoring must stay local, offline, and live-only.

## In-scope behavior

- Sparse HEALPix → last-live-visit map for ever-live cells only
  (SPD-063). Not in `.pix`. Not a full-universe timestamp table.
- On first competition opt-in, seed `last_live_visit = consent time` for
  currently ever-live pixels. After that, only validated live sessions
  update recency.
- Ownership score per area: SPD-057. \(T = 0\) → 0.
- Eligibility: spec §22.5 (2% live coverage, 50 unique live pixels waived
  if \(T < 50\), score ≥ 0.5).
- Unclaimed: no eligible participant, or previous boss decayed below the
  minimum, or all eligible participants left.
- Contested: SPD-058 (runner-up ≥ 80% of leader among eligible).
- Query API: for an area OSM id, return local score, live coverage %,
  eligible, and local ranking inputs needed by SP-074 / SP-078.
- Crash-safe writes.

## Out-of-scope behavior

- Weekly city counts (SP-073).
- Upload (SP-074).
- Server-side decay (SP-075).
- UI (SP-078).
- Boss haptic (SPD-054).

## Relevant product requirements

- Spec §22.1–§22.9, §15.2–§15.3.
- SPD-015, SPD-026 (personal % is a different number), SPD-057, SPD-058,
  SPD-063.

## Relevant source files or symbols

- `StreetPixelsManager`, `IsEverLive()`, live collection path
- `street_pixels::ExplorationArea::m_osmId`, assignment sidecar
- `area_milestones.db` pattern (SQLite WAL) as a storage precedent — do
  not mix recency into milestone tables

## Implementation notes / constraints

- Shared C++. Android does not reimplement the formula.
- Imported-only cells: no timestamp, no score contribution, several
  tests.
- Server snapshot scores are not required for local computation; UI may
  show local score offline with a stale-ranking label (SP-078).

## Acceptance criteria

1. Recency weight ≈ 1.0 immediately, 0.5 at 30 days, 0.25 at 60, 0.125
   at 90; revisit restores ≈ 1.0.
2. Ownership fixtures match SPD-057; imported pixels never affect score,
   eligibility, or contested.
3. Eligibility conditions fail independently; \(T < 50\) waives the
   50-pixel rule only.
4. Contested holds at 80% and fails below; unclaimed when no eligible
   boss.
5. Opt-in seed covers current ever-live cells once.

## Required automated tests

- Decay table at 0 / 30 / 60 / 90 days and revisit restore.
- Score = 100 on just-visited full live coverage; 0 on imported-only.
- Eligibility: each of the three conditions independently; small-area
  waiver.
- Contested at 0.80 vs 0.79 relative gap.
- Seed-on-opt-in; second opt-in does not re-seed already timestamped
  cells.
- No `.pix` format change.

## Required manual validation

- Device residual → SP-079 / Phase 10.

## Failure and rollback considerations

- Prefer omitting competition chrome over writing timestamps into `.pix`.
- Do not count imported pixels to “make scores interesting”.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-072-recency-ownership-scoring-f95c` |
| Test output | Independent review re-run pasted under **Executed test output** below. Binaries in `/home/ubuntu/omim-build-debug/`. Debug build used `./tools/unix/build_omim.sh -d -p "$HOME"`. |
| Store location | `live_recency.db` via `GetPlatform().WritablePathForFile` (`LiveRecencyStore::DefaultDbPath()`). Not `area_milestones.db`. `.pix` format remains `kFormatVersionV2`. |
| Accepted by | Product owner |
| Accepted date | 2026-08-25 |

## Executed test output

Independent review re-run after fixes. Binaries in `/home/ubuntu/omim-build-debug/`.

```
=== /home/ubuntu/omim-build-debug/street_pixels_areas_tests --data_path=data --user_resource_path=data --filter=OwnershipScoring_|LiveRecency_|AreaCompletion_KnownTotals ===
Running area_completion_cache_tests.cpp::AreaCompletion_KnownTotalsAndFractions
OK
Test took 0 ms

Running live_recency_store_tests.cpp::LiveRecency_SeedInsertOrIgnoreKeepsFirstTimestamp
OK
Test took 8 ms

Running live_recency_store_tests.cpp::LiveRecency_TouchOverwritesTimestamp
OK
Test took 3 ms

Running live_recency_store_tests.cpp::LiveRecency_DefaultPathIsLiveRecencyNotMilestones
OK
Test took 0 ms

Running live_recency_store_tests.cpp::LiveRecency_TempDbRemovesWalAndShm
OK
Test took 1 ms

Running live_recency_store_tests.cpp::LiveRecency_GetLastLiveVisitsBatch
OK
Test took 1 ms

Running live_recency_store_tests.cpp::LiveRecency_ReopenSwitchesPath
OK
Test took 3 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_RecencyWeightDecayTable
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_RecencyWeightClampsNegativeDelta
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_RevisitRestoresFullWeight
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_JustVisitedFullLiveIs100
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_ZeroTotalIsZero
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_ImportedOnlyEmptyLiveIsZero
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_MissingTimestampContributesZeroToSum
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_EligibilityCoverageFailsIndependently
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_EligibilityUniqueLiveFailsIndependently
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_EligibilityScoreFailsIndependently
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_EligibilityScoreBoundary
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_EligibilityCoverageBoundary
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_SmallAreaWaivesMinLivePixels
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_FiftyPixelAreaDoesNotWaiveMinLive
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_ContestedAtEightyPercent
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_UnclaimedWhenNoEligible
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_OneEligibleIsNotContested
OK
Test took 0 ms

Running ownership_scoring_tests.cpp::OwnershipScoring_LocalViewUnclaimedAndNeverContested
OK
Test took 0 ms

All tests passed.

=== /home/ubuntu/omim-build-debug/street_pixels_tests --data_path=data --user_resource_path=data --filter=CompetitionOwnership_|EverLive_|CollectionGate_|StreetPixel_|StreetPixelsFile_|IdentityStore_|AreaMilestone_ ===
Running street_pixel_tests.cpp::StreetPixel_SetExploredPreservesIdentifier
OK
Test took 0 ms

Running street_pixel_tests.cpp::StreetPixel_IdentifierMaskExcludesFlagBits
OK
Test took 0 ms

Running street_pixel_tests.cpp::StreetPixel_MaximalHealpixIdRoundTrip
OK
Test took 0 ms

Running street_pixel_tests.cpp::StreetPixel_UnexploredByDefault
OK
Test took 0 ms

Running street_pixel_tests.cpp::StreetPixel_SetExploredFalseIsNoOp
OK
Test took 0 ms

Running street_pixel_tests.cpp::StreetPixel_SetEverLiveFalseIsNoOp
OK
Test took 0 ms

Running street_pixel_tests.cpp::StreetPixel_UnexploredKeepsEverLiveClear
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_ProbeHeaderedLegacyUnsupported
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_FlagsBit0ClearedTreatedAsLegacy
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_RoundTripHeaderedPreservesExploredMsbs
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_RoundTripHeaderedV2PreservesEverLive
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_SaveUnexploredIdsStampsVersion
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_LegacyMigratesPreservingExplored
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_UnsupportedFormatRejectedWithoutRewrite
Settings path: data/settings.ini
Explore.DeviceId : Hrc2V2FEny4EYbCwYYwVwGKe-hI4UUQy
StreetPixels.FirstGoalCollected : 10
StreetPixels.FirstGoalComplete : true
LoadStreetPixelsFromFile sp015_unsupported
Trying to memory-map existing pix file for sp015_unsupported
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_MayRecoverByDeriveOnlyCorrupt
OK
Test took 0 ms

Running street_pixels_file_tests.cpp::StreetPixelsFile_MigrateNonLegacyLeavesFileIntact
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Idle_CollectsNothing
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Paused_CollectsNothing
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Recording_CollectsExpected
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 4 ms

Running collection_gate_tests.cpp::CollectionGate_StartMidSequence_CollectsFromStartOnward
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_PauseMidSequence_CollectsUntilPause
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_TrackReplay_MarksRegardlessOfSession
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Rejected_NoVibration
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Finished_CollectsNothing
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Discarded_CollectsNothing
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Recording_TriggersVibration
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_LiveFirstVisitWritesRecency
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ImportDoesNotWriteRecency
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 1 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ImportedThenLiveWritesRecency
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 1 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_LiveThenImportLeavesRecencyUnchanged
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 1 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_RevisitUpdatesTimestamp
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 1 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_IdleWritesNoRecency
OK
Test took 3 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_PauseWritesNoRecency
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_RejectedSampleWritesNoRecency
OK
Test took 3 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_SeedOnOptInAndSecondOptInDoesNotReseed
OK
Test took 4 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_Score100JustVisitedFullLive
StreetPixels rebuild pix scan ms 0 sp072_score100
StreetPixels rebuild spa+resolver ms 0 sp072_score100
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 4 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ScoreUsesAreaTotalNotExploredCount
StreetPixels rebuild pix scan ms 0 sp072_score_total
StreetPixels rebuild spa+resolver ms 0 sp072_score_total
StreetPixels AreaCompletionCache::Build ms 0 universe 4 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 4 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ImportedOnlyScoreZeroFullPersonalCompletion
StreetPixels rebuild pix scan ms 0 sp072_imported_only
StreetPixels rebuild spa+resolver ms 0 sp072_imported_only
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ImportedOnlyDoesNotAffectEligibilityOrContested
StreetPixels rebuild pix scan ms 0 sp072_imported_elig
StreetPixels rebuild spa+resolver ms 0 sp072_imported_elig
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 5 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_UnknownOsmFailClosedZeros
StreetPixels rebuild pix scan ms 0 sp072_unknown_osm
StreetPixels rebuild spa+resolver ms 0 sp072_unknown_osm
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_PixFormatUnchangedV2
StreetPixels rebuild pix scan ms 0 sp072_pix_format
StreetPixels rebuild spa+resolver ms 0 sp072_pix_format
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 0 changed 0
OK
Test took 5 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_SeedScansPixFileEverLive
OK
Test took 4 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_SeedSkipsImportedOnlyPixCells
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_GrantHandlerSeedsWithoutExplicitCall
OK
Test took 4 ms

Running ever_live_tests.cpp::EverLive_FirstLiveSetsEverLive
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_FirstImportedClearThenLiveSets
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_LiveThenImportedRemainsSet
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_TrackAloneLeavesClear
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_TrackAfterLiveRemainsSet
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_LiveNeverClears
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_UpgradeDoesNotDoubleCount
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running ever_live_tests.cpp::EverLive_GetPixelIdMaskUnaffectedByFlags
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_LegacyConsentBooleanIsNotConsent
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_RejectsInvalidNicknames
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_AcceptsUnicodeNicknames
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_Collision409DoesNotPersistNickname
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_ConsentOffBlocksIdentityUpload
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_GrantInvokesHandlerWithStoredUnixTime
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_GrantWithoutHandlerDoesNotCrash
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_EmptyPolicyVersionIsNotConsent
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_InvalidUtf8NicknameRejected
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_ExistingAsciiUsernameNotAutoAccepted
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_RenameLimitLocalSevenDays
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_GenerateNicknameRetryIsNotNumericSuffix
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_UnsetHandlerKeepsDraftOnly
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_SetUsernameDoesNotAccept
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_SameNicknameReclaimIsNotLimited
OK
Test took 0 ms

All tests passed.

=== /home/ubuntu/omim-build-debug/street_pixels_areas_tests --data_path=data --user_resource_path=data --filter=AreaMilestone_ ===
Running area_milestone_store_tests.cpp::AreaMilestone_FireOncePerThreshold
OK
Test took 8 ms

Running area_milestone_store_tests.cpp::AreaMilestone_TripleCrossOneUpdate
OK
Test took 3 ms

Running area_milestone_store_tests.cpp::AreaMilestone_NoRefireAfterDrop
OK
Test took 4 ms

Running area_milestone_store_tests.cpp::AreaMilestone_ZeroTotalDoesNotFire
OK
Test took 1 ms

Running area_milestone_store_tests.cpp::AreaMilestone_ConsumePendingCrossings
OK
Test took 1 ms

Running area_milestone_store_tests.cpp::AreaMilestone_OsmIdStableAcrossCacheRebuild
OK
Test took 1 ms

Running area_milestone_store_tests.cpp::AreaMilestone_OsmIdStableAcrossCompactIndexChange
OK
Test took 5 ms

Running area_milestone_store_tests.cpp::AreaMilestone_CitySummaryDoesNotWriteAreaFiredState
OK
Test took 1 ms

All tests passed.

=== /home/ubuntu/omim-build-debug/street_pixels_tests --data_path=data --user_resource_path=data --filter=AreaMilestone ===
Running area_milestone_manager_tests.cpp::AreaMilestoneManager_FiresOnRebuild
Settings path: data/settings.ini
Explore.DeviceId : Hrc2V2FEny4EYbCwYYwVwGKe-hI4UUQy
StreetPixels.FirstGoalCollected : 10
StreetPixels.FirstGoalComplete : true
StreetPixels rebuild pix scan ms 0 sp063_mgr_fire
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_fire
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 5 ms

Running area_milestone_manager_tests.cpp::AreaMilestoneManager_NoRefireAfterInvalidateRebuild
StreetPixels rebuild pix scan ms 0 sp063_mgr_norefire
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_norefire
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels invalidate area completion cache
StreetPixels rebuild pix scan ms 0 sp063_mgr_norefire
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_norefire
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 5 ms

Running area_milestone_manager_tests.cpp::AreaMilestoneManager_ImportCanCrossThreshold
StreetPixels rebuild pix scan ms 0 sp063_mgr_import
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_import
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels rebuild pix scan ms 0 sp063_mgr_import
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_import
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 2 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 3 ms

Running area_milestone_manager_tests.cpp::AreaMilestoneManager_PreviouslyCompletedBelow100
StreetPixels rebuild pix scan ms 0 sp063_mgr_prev
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_prev
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels rebuild pix scan ms 0 sp063_mgr_prev
StreetPixels rebuild spa+resolver ms 0 sp063_mgr_prev
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 3 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_MapsThresholds
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_QueueOrder100Then50Then25
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_OneAtATime
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_SkipAlreadyShownThisCrossing
StreetPixels rebuild pix scan ms 0 sp065_skip_shown
StreetPixels rebuild spa+resolver ms 0 sp065_skip_shown
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels rebuild pix scan ms 0 sp065_skip_shown
StreetPixels rebuild spa+resolver ms 0 sp065_skip_shown
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 4 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_SkipDuplicateInQueue
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_DebugPreviewDoesNotBlockRealHundred
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_DisplayNameNeverMwmId
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_BlankDisplayNameDropped
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_CitySummaryDoesNotEnqueue
StreetPixels rebuild pix scan ms 0 sp065_city_sum
StreetPixels rebuild spa+resolver ms 0 sp065_city_sum
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels fraction reason ok citySummary 1 compactIndex 1 explored 1 total 2 fraction 0.5 fractionValid 1
OK
Test took 3 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_Haptic50And100Not25
StreetPixels rebuild pix scan ms 0 sp065_haptic
StreetPixels rebuild spa+resolver ms 0 sp065_haptic
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 2 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_DoesNotCallCollectionVibration
StreetPixels rebuild pix scan ms 0 sp065_novib
StreetPixels rebuild spa+resolver ms 0 sp065_novib
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 3 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_FollowingDoesNotStopRoute
StreetPixels rebuild pix scan ms 0 sp065_follow
StreetPixels rebuild spa+resolver ms 0 sp065_follow
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 2 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_HundredPercentDoesNotShare
StreetPixels rebuild pix scan ms 0 sp065_noshare
StreetPixels rebuild spa+resolver ms 0 sp065_noshare
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 2 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_CompetitionLineStubEmpty
OK
Test took 0 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_PreviouslyCompletedOnFocus
StreetPixels rebuild pix scan ms 0 sp065_prev_focus
StreetPixels rebuild spa+resolver ms 0 sp065_prev_focus
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels rebuild pix scan ms 0 sp065_prev_focus
StreetPixels rebuild spa+resolver ms 0 sp065_prev_focus
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels fraction reason ok citySummary 0 compactIndex 0 explored 0 total 1 fraction 0 fractionValid 1
StreetPixels fraction reason ok citySummary 1 compactIndex 1 explored 1 total 2 fraction 0.5 fractionValid 1
OK
Test took 3 ms

Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_DebugPreviewWithoutHundredPercent
StreetPixels rebuild pix scan ms 0 sp_dbg_card
StreetPixels rebuild spa+resolver ms 0 sp_dbg_card
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 53 ms

All tests passed.
```

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `QueryCompetitionOwnership` enumerates ever-live pixels only from the currently loaded `m_streetPixels` span. Opt-in / manager construction still scans WritableDir `*.pix` and `*.pixr`. Country load seeds only the mapped span. A multi-country local score can miss ever-live cells whose leaf is not loaded. | SP-074 / later: when querying, also scan other leaves (or the area’s MWM `.pix`) the same way opt-in seed does. |
| Query still walks the loaded universe checking `IsEverLive` (now under a shared lock; area lookup is outside the lock). | SP-074: index ever-live ids or enumerate recency-store keys instead of the full span. |
| `recency_meta` is created in `live_recency.db` but unused. | Keep for a later score/store version key, or drop if still empty after SP-074. |
| Unity builds collide on identical anonymous-namespace names (`kDatabaseFileName` vs `AreaMilestoneStore`). | Fixed in this item by renaming the recency constant. Watch for the same pattern in later sqlite stores. |
| `RevokeCompetitionConsent` does not delete `live_recency.db` rows. Re-opt-in `INSERT OR IGNORE` keeps prior timestamps. | Privacy / product: decide whether revoke should wipe local recency; do not upload either way. |
| Exclusive `m_streetPixelsMutex` during ownership query; full WritableDir rescan on every country load; consent handler lifetime after `Framework` teardown. | Fixed in independent review: shared lock + id collection only; country-load seed from mapped span; handler cleared in `~Framework`. |
| No test that local T is `AreaCompletionCache::m_total` when explored < total; imported-only cells skipped on seed; grant handler seeds without an extra `MaybeSeed` call. | Fixed in independent review (`CompetitionOwnership_ScoreUsesAreaTotalNotExploredCount`, `SeedSkipsImportedOnlyPixCells`, `GrantHandlerSeedsWithoutExplicitCall`). |
