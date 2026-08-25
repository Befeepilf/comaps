# SP-074 — Competition upload queue

**Phase:** 8 — Competition
**Status:** In progress
**Branch:** `cursor/sp-074-competition-upload-queue-f95c`
**Depends on:** SP-070 (SPD-014, SPD-062, SPD-064, SPD-065); SP-071
  consent; SP-072 scores; SP-073 weekly counts
**Unblocks:** SP-075 (ingest), SP-079 (cadence checks)

---

## Objective

Queue and send **only** spec §25.2 aggregates, at most once per 15 minutes
plus up to 15 minutes jitter, with offline drain. Replace the 1-minute
`ExploreStatsService` poll. Do not reuse `/stats/upload`.

## Motivation

Spec §25.3 and SPD-014. Current client posts a different schema every
minute to an endpoint the backend does not implement. Delayed batching
exists so competition cannot act as a live-location signal.

## In-scope behavior

- Upload **only** when competition consent is on (SPD-064 record).
- Payload allow-list: pseudonymous profile id, nickname, area OSM id,
  aggregate ownership score, live coverage %, eligibility, weekly
  new-live-pixel count by city OSM id, map-data version,
  score-calc version (1), last update time (SPD-065).
- Deny-list tests: no lat/lon, GPS, tracks, per-pixel timestamps, live
  movement, device advertising ids, friends ids.
- Cadence: ≤ 1 upload / 15 minutes, plus jitter in `[0, 15]` minutes.
  No “sync now” control.
- Offline: queue; flush after connectivity. No interpolation across
  pauses (existing collection rules unchanged).
- URL: `{apiBase}/v1/competition/…` (SPD-062). Empty apiBase → no HTTP
  (SP-004).
- Discard `explore_stats.json` as a source of truth (SPD-064). Stop the
  1-minute stats poll for competition (remove or leave dead behind the
  consent gate — pick one and test that it does not fire).
- No upload when opted out.

## Out-of-scope behavior

- Backend ingest (SP-075).
- Read APIs (SP-076).
- Account deletion HTTP (SP-077) may share the client, but deletion is
  that item.

## Relevant product requirements

- Spec §25.1–§25.6, §26.2.
- SPD-014, SPD-062, SPD-064, SPD-065.

## Relevant source files or symbols

- `libs/map/explore_stats_service.{hpp,cpp}`
- `libs/map/backend_config.{hpp,cpp}` `GetStatsUploadUrl`
- `IdentityStore`, `StreetPixelsManager`

## Implementation notes / constraints

- Schema rejection is a backend duty; the client still must not send
  extra fields.
- Tests freeze time for cadence and jitter bounds.
- Fail closed if API unconfigured.

## Acceptance criteria

1. Payload contains only the allow-list; deny-list fields never appear.
2. No upload more than once per 15 minutes; jitter in range; no upload
   when opted out or apiBase empty.
3. Offline queue flushes later; no sync-now affordance.
4. `/stats/upload` is not used for competition.
5. Imported-only progress does not produce competitive payload values.

## Required automated tests

- Allow-list / deny-list JSON fixtures.
- Cadence + jitter with a fake clock.
- Opt-out and empty apiBase → zero HTTP.
- Offline enqueue then flush.
- Consent boolean-only (pre-SP-071) must not be testable as sufficient
  once SP-071 landed; if this item lands first, gate on the new record
  type.

## Required manual validation

- Capture traffic on device (SP-079): no coordinates; cadence holds.

## Failure and rollback considerations

- Prefer no upload over sending location-shaped fields.
- Do not add a debug “upload now” in public builds.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-074-competition-upload-queue-f95c` |
| Test output | See executed output below. Cwd `/workspace`. Binary `/home/ubuntu/omim-build-debug/street_pixels_tests`. `--data_path=data --user_resource_path=data`. `EXIT_UPLOAD=0`, `EXIT_REGRESSION=0`. Tests run at `3b3f3c07c`. |
| Endpoint path | `{apiBase}/v1/competition/aggregates` (`backend::GetCompetitionAggregatesUrl()`). Empty `apiBase` → empty URL. Does not call `GetStatsUploadUrl()`. |
| Accepted by | |
| Accepted date | |

## Executed test output

```
=== street_pixels_tests --filter='CompetitionUpload_|BackendConfig_|ExploreStats' ===
Running backend_config_tests.cpp::BackendConfig_DefaultEmptyWhenUnset
Settings path: data/settings.ini
Explore.FriendVisibilityEnabled : true
Explore.SyncEnabled : true
StreetPixels.FirstGoalCollected : 10
StreetPixels.FirstGoalComplete : true
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_SetNormalizesTrailingSlash
OK
Test took 1 ms

Running backend_config_tests.cpp::BackendConfig_ClearOnEmptySet
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_StatsUploadUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_StatsUploadUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlNeverUsesStatsUpload
OK
Test took 0 ms

Running backend_config_tests.cpp::ExploreStatsUpload_DecisionGateWhenApiUnconfigured
OK
Test took 0 ms

Running competition_upload_tests.cpp::CompetitionUpload_PayloadAllowList
OK
Test took 0 ms

Running competition_upload_tests.cpp::CompetitionUpload_PayloadDenyList
OK
Test took 0 ms

Running competition_upload_tests.cpp::CompetitionUpload_MarkPendingDoesNotPost
OK
Test took 2 ms

Running competition_upload_tests.cpp::CompetitionUpload_CadenceJitterZero
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_CadenceJitter900
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_OptOutZeroHttp
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_EmptyApiBaseZeroHttp
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_OfflineThenFlush
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_ConsentGivenBooleanOnlyZeroHttp
OK
Test took 0 ms

Running competition_upload_tests.cpp::CompetitionUpload_NicknameDraftOnlyZeroHttp
OK
Test took 0 ms

Running competition_upload_tests.cpp::CompetitionUpload_NoFriendsHeaders
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_StatsUploadUrlNeverUsed
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_EmptySnapshotSkipsHttpKeepsPending
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_SnapshotLiveOnlyOmitsUniqueCountsAndWeekId
StreetPixels rebuild pix scan ms 0 sp074_live_snap
StreetPixels rebuild spa+resolver ms 0 sp074_live_snap
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 5 ms

Running competition_upload_tests.cpp::CompetitionUpload_ImportedOnlyZeroCompetitiveHttp
StreetPixels rebuild pix scan ms 0 sp074_imported_only
StreetPixels rebuild spa+resolver ms 0 sp074_imported_only
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 1 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 12 ms

Running competition_upload_tests.cpp::CompetitionUpload_ExploreStatsTryUploadZeroHook
OK
Test took 1 ms

Running competition_upload_tests.cpp::CompetitionUpload_EnableSharingWithoutConsentZeroHttp
OK
Test took 3 ms

All tests passed.
EXIT_UPLOAD=0

=== street_pixels_tests --filter='CompetitionOwnership_ImportedOnly|WeeklyCityLive_|IdentityStore_LegacyConsent|IdentityStore_ConsentOff' ===
Running weekly_city_live_tests.cpp::WeeklyCityLive_FirstLiveVisitCounts
Settings path: data/settings.ini
Explore.FriendVisibilityEnabled : true
Explore.SyncEnabled : true
StreetPixels.FirstGoalCollected : 10
StreetPixels.FirstGoalComplete : true
StreetPixels rebuild pix scan ms 0 sp073_first_live
StreetPixels rebuild spa+resolver ms 0 sp073_first_live
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 15 ms

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
Test took 3 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_TrackReplayDoesNot
StreetPixels rebuild pix scan ms 0 sp073_track_replay
StreetPixels rebuild spa+resolver ms 0 sp073_track_replay
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 5 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_AlreadyEverLiveDoesNot
StreetPixels rebuild pix scan ms 0 sp073_already_live
StreetPixels rebuild spa+resolver ms 0 sp073_already_live
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 9 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_ImportedThenLiveCountsOnce
StreetPixels rebuild pix scan ms 0 sp073_import_then_live
StreetPixels rebuild spa+resolver ms 0 sp073_import_then_live
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
StreetPixels increment n 1 bumped 1 changed 1
OK
Test took 10 ms

Running weekly_city_live_tests.cpp::WeeklyCityLive_IdlePauseRejectedDoNot
StreetPixels rebuild pix scan ms 0 sp073_idle_pause_reject
StreetPixels rebuild spa+resolver ms 0 sp073_idle_pause_reject
StreetPixels AreaCompletionCache::Build ms 0 universe 3 explored 0 sentinelSlots 2 settlements 1
StreetPixels CityCompletionCache::Build ms 0
StreetPixels overlay push ms 0
OK
Test took 6 ms

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
Test took 8 ms

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
Test took 7 ms

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
Test took 6 ms

Running identity_store_tests.cpp::IdentityStore_LegacyConsentBooleanIsNotConsent
OK
Test took 0 ms

Running identity_store_tests.cpp::IdentityStore_ConsentOffBlocksIdentityUpload
OK
Test took 0 ms

All tests passed.
EXIT_REGRESSION=0
```

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `QueryCompetitionOwnership` (and therefore `BuildCompetitionUploadSnapshot`) only counts ever-live pixels currently loaded in `m_streetPixels`, not cells that exist only in on-disk `.pix` spans. Live pixels in unloaded leaves are omitted from the upload snapshot. | SP-072 loaded-span query gap. Do not paper over it by uploading unique counts or Healpix ids. |
| Competition POST sends no `X-Device-Id`, `X-Username`, or friends headers. Identity is `profile_id` + `nickname` in the closed JSON body. | Keep until SP-075 asks for a specific auth scheme. Do not add friends headers. |
| `backend::GetStatsUploadUrl()` (`/stats/upload`) remains in the tree and is unused by competition upload. `ExploreStatsService::TryUpload` is a no-op and does not call it. Consent does not re-enable `/stats/upload`. | Leave unused. Do not route competition through it. Delete or keep as a non-competition leftover in a later cleanup if product wants the symbol gone. |
| HTTP failure keeps pending and advances `next_allowed` by 900s+jitter so `MaybeUpload` cannot retry-storm in the same window. | Keep. Cadence gate is the retry control. |
