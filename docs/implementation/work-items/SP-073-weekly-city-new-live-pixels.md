# SP-073 — Weekly city new-live-pixel counting

**Phase:** 8 — Competition
**Status:** Not started
**Branch:** `cursor/sp-073-weekly-city-new-live-pixels-f95c`
**Depends on:** SP-070 (SPD-060); SP-072 recency/live writes; Phase 4
  settlement OSM ids
**Unblocks:** SP-074 (weekly field in upload), SP-076 (city board)

---

## Objective

Count unique **new** live pixels per city per week for the weekly
leaderboard, using Monday 00:00 in the city IANA timezone, else UTC.

## Motivation

Spec §24.1–§24.2. Current `ExploreStatsService` buckets weekly
`regionId` pixel deltas that are not city-scoped, not live-only, and not
unique-new. SPD-064 discards that file; this item is a new counter.

## In-scope behavior

- Per settlement OSM id (city), count unique pixels whose first
  **live** exploration in that city occurs in the current week.
- Revisits of already-live pixels do not count. Imports do not count.
- Week boundary: Monday 00:00 in the city IANA zone when known; else UTC
  (SPD-060). Never device local time.
- If IANA tz is not yet on the city record, both client and server use
  UTC for that city (fail closed). Optional: persist tz from centroid
  lookup onto the city record as follow-up inside this item — not a
  Phase 4 reopen.
- Query: current week id, remaining time, local new-live count for a
  city.

## Out-of-scope behavior

- Server ranking (SP-076).
- Global / country boards (spec §6).
- Upload (SP-074).

## Relevant product requirements

- Spec §24; SPD-060; SPD-007.

## Relevant source files or symbols

- `CityCompletionCache`, settlement-role areas, `StableOsmId`
- `ExploreStatsService` / `explore_stats.json` — do not reuse as the
  V1 weekly store (SPD-064)

## Implementation notes / constraints

- Week id must be reproducible given (city id, timestamp, tz or UTC).
- Tests must not depend on the developer’s local zone.

## Acceptance criteria

1. New live pixels increment; revisits and imports do not.
2. Week rolls at Monday 00:00 in the fixture tz; unknown tz uses UTC.
3. Device zone does not change the bucket.
4. City key is settlement OSM id.

## Required automated tests

- First live visit in city counts; second visit same cell does not.
- Import-only write does not increment.
- Fixed-tz Monday boundary; UTC fallback; device TZ ignored.
- Two cities independently.

## Required manual validation

- Device residual → SP-079 / Phase 10.

## Failure and rollback considerations

- Prefer UTC fallback over guessing Europe/Helsinki.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-073-weekly-city-new-live-pixels-f95c` |
| Test output | Pasted under **Executed test output** below. Binaries in `/home/ubuntu/omim-build-debug/`. Debug build: `./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_areas_tests street_pixels_tests`. |
| Store location | `weekly_city_live.db` via `GetPlatform().WritablePathForFile` (`WeeklyCityLiveStore::DefaultDbPath()`). Not `live_recency.db`, not `area_milestones.db`, not `explore_stats.json`. |
| Accepted by | |
| Accepted date | |

## Executed test output

Cwd `/workspace`. Binaries in `/home/ubuntu/omim-build-debug/`. `--data_path=data --user_resource_path=data`. HEAD at evidence time: `d0b99f29b`.

```
=== street_pixels_areas_tests --filter=WeeklyCity ===
Running weekly_city_live_store_tests.cpp::WeeklyCityLive_Increment
OK
Test took 9 ms

Running weekly_city_live_store_tests.cpp::WeeklyCityLive_TwoCities
OK
Test took 4 ms

Running weekly_city_live_store_tests.cpp::WeeklyCityLive_TzChangesWeekIdVsUtc
OK
Test took 4 ms

Running weekly_city_live_store_tests.cpp::WeeklyCityLive_DefaultDbPathFilename
OK
Test took 0 ms

Running weekly_city_live_store_tests.cpp::WeeklyCityLive_TempDbRemovesWalAndShm
OK
Test took 1 ms

Running weekly_city_live_store_tests.cpp::WeeklyCityLive_UnknownCityUtcZero
OK
Test took 1 ms

Running weekly_city_week_tests.cpp::WeeklyCityWeek_UtcMondayBoundary
OK
Test took 0 ms

Running weekly_city_week_tests.cpp::WeeklyCityWeek_FixedOffsetMonday
OK
Test took 0 ms

Running weekly_city_week_tests.cpp::WeeklyCityWeek_EmptyTzIsUtc
OK
Test took 0 ms

Running weekly_city_week_tests.cpp::WeeklyCityWeek_DeviceTzIgnored
OK
Test took 0 ms

Running weekly_city_week_tests.cpp::WeeklyCityWeek_RemainingTime
OK
Test took 0 ms

All tests passed.
EXIT_AREAS=0
=== street_pixels_tests --filter=WeeklyCity ===
Running weekly_city_live_tests.cpp::WeeklyCityLive_FirstLiveVisitCounts
Settings path: data/settings.ini
Explore.DeviceId : Hrc2V2FEny4EYbCwYYwVwGKe-hI4UUQy
StreetPixels.FirstGoalCollected : 10
StreetPixels.FirstGoalComplete : true
StreetPixels rebuild pix scan ms 0 sp073_first_live
StreetPixels rebuild spa+resolver ms 0 sp073_first_live
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 10 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_SecondVisitSameCellDoesNot
StreetPixels rebuild pix scan ms 0 sp073_second_visit
StreetPixels rebuild spa+resolver ms 0 sp073_second_visit
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 4 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_ImportOnlyDoesNot
StreetPixels rebuild pix scan ms 0 sp073_import_only
StreetPixels rebuild spa+resolver ms 0 sp073_import_only
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 7 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_ImportedThenLiveCountsOnce
StreetPixels rebuild pix scan ms 0 sp073_import_then_live
StreetPixels rebuild spa+resolver ms 0 sp073_import_then_live
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 8 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_IdlePauseRejectedDoNot
StreetPixels rebuild pix scan ms 0 sp073_idle_pause_reject
StreetPixels rebuild spa+resolver ms 0 sp073_idle_pause_reject
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 4 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_TwoCitiesIndependent
StreetPixels rebuild pix scan ms 0 sp073_two_cities_mgr
StreetPixels rebuild spa+resolver ms 0 sp073_two_cities_mgr
StreetPixels AreaCompletionCache::Build ms 0 universe 4 explored 0 sentinelSlots 3 settlements 2
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 9 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_QueryBySettlementNotSubdivision
StreetPixels rebuild pix scan ms 0 sp073_settlement_key
StreetPixels rebuild spa+resolver ms 0 sp073_settlement_key
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 7 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_NoAreaPixelDoesNotInventCity
StreetPixels rebuild pix scan ms 0 sp073_no_area
StreetPixels rebuild spa+resolver ms 0 sp073_no_area
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 0 changed 0
OK
Test took 4 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_QueryWeekRemaining
OK
Test took 2 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_InterpolationCountsOnce
StreetPixels rebuild pix scan ms 0 sp073_interpolation
StreetPixels rebuild spa+resolver ms 0 sp073_interpolation
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
StreetPixels increment n 2 bumped 0 changed 0
OK
Test took 5 ms

All tests passed.
EXIT_WEEKLY=0
=== street_pixels_tests --filter=CompetitionOwnership_|EverLive_|CollectionGate_ ===
Running collection_gate_tests.cpp::CollectionGate_Idle_CollectsNothing
Settings path: data/settings.ini
Explore.DeviceId : Hrc2V2FEny4EYbCwYYwVwGKe-hI4UUQy
StreetPixels.FirstGoalCollected : 10
StreetPixels.FirstGoalComplete : true
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Paused_CollectsNothing
OK
Test took 0 ms

Running collection_gate_tests.cpp::CollectionGate_Recording_CollectsExpected
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 2 ms

Running collection_gate_tests.cpp::CollectionGate_StartMidSequence_CollectsFromStartOnward
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 1 ms

Running collection_gate_tests.cpp::CollectionGate_PauseMidSequence_CollectsUntilPause
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 1 ms

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
Test took 3 ms

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
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_IdleWritesNoRecency
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_PauseWritesNoRecency
OK
Test took 3 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_RejectedSampleWritesNoRecency
OK
Test took 1 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_SeedOnOptInAndSecondOptInDoesNotReseed
OK
Test took 3 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_Score100JustVisitedFullLive
StreetPixels rebuild pix scan ms 0 sp072_score100
StreetPixels rebuild spa+resolver ms 0 sp072_score100
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 6 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ScoreUsesAreaTotalNotExploredCount
StreetPixels rebuild pix scan ms 0 sp072_score_total
StreetPixels rebuild spa+resolver ms 0 sp072_score_total
StreetPixels AreaCompletionCache::Build ms 0 universe 4 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 6 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ImportedOnlyScoreZeroFullPersonalCompletion
StreetPixels rebuild pix scan ms 0 sp072_imported_only
StreetPixels rebuild spa+resolver ms 0 sp072_imported_only
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 5 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_ImportedOnlyDoesNotAffectEligibilityOrContested
StreetPixels rebuild pix scan ms 0 sp072_imported_elig
StreetPixels rebuild spa+resolver ms 0 sp072_imported_elig
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 3 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_UnknownOsmFailClosedZeros
StreetPixels rebuild pix scan ms 0 sp072_unknown_osm
StreetPixels rebuild spa+resolver ms 0 sp072_unknown_osm
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 4 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_PixFormatUnchangedV2
StreetPixels rebuild pix scan ms 0 sp072_pix_format
StreetPixels rebuild spa+resolver ms 0 sp072_pix_format
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 0 changed 0
OK
Test took 4 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_SeedScansPixFileEverLive
OK
Test took 3 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_SeedSkipsImportedOnlyPixCells
OK
Test took 2 ms

Running competition_ownership_tests.cpp::CompetitionOwnership_GrantHandlerSeedsWithoutExplicitCall
OK
Test took 2 ms

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

All tests passed.
EXIT_REGRESSION=0
```

`--filter=WeeklyCityLive_` is a subset of `--filter=WeeklyCity` (manager + store names). Not re-run separately.

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Spec §24.1 ranks “newly explored unique live pixels”. A first-exploration reading (`newlyExploredIds`) would give import-then-live **+0**, so imported exploration would suppress later live credit. This item counts the **ever-live flip** on the live path (imported-then-live **+1** once). Work-item “first live” and “imported exploration must not suppress later live credit” win over a newly-explored-only reading of §24.1. | Product lock: keep ever-live flip, or change spec §24.1 if newly-explored-only is intended. |
| IANA zone on the city record is stored (`city_tz`) but not resolved to an offset. App ICU is transliteration-only; no TimeZone. Empty / unknown tz → UTC (SPD-060 fail closed). `WeekBoundsAtFixedOffset` is the offset path for tests. Production does not populate tz (no centroid library). | Optional follow-up: persist tz from centroid lookup onto the city record (already allowed inside this item / sidecar metadata; not a Phase 4 reopen). |
| Device residual for weekly city display. | SP-079 / Phase 10 (already required manual validation). |
