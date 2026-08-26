# SP-079 — Validation plan (Phase 8 exit)

**Work item:** [SP-079](../work-items/SP-079-phase8-end-to-end-validation.md)
**Plan authored by:** Agent
**Plan review date:** 2026-08-26
**Branch:** `cursor/sp-079-phase8-end-to-end-validation-f95c` (lands on `street-pixels`)

Evidence-only. Write this plan and the evidence log, re-run named suites,
map exit criteria 1–12 to pass / fail / residual. **Do not** mark SP-079
or Phase 8 Accepted. **Do not** set phase Status to Exit criteria met.
**Do not** implement features. **Do not** merge competition into explorer
`main`.

## Approved decisions

| ID | Decision |
| --- | --- |
| Device walks / traffic capture | **Phase 10 residual** unless `adb devices` shows a handset. This cloud environment is assumed to have none. Do not fabricate walks, screenshots, or packet captures. |
| Map screenshots | **Forbidden**. Even if a device appears, do not capture the map. |
| Explorer checkout | **Present.** Run pytest on the existing SP-077 branch. Do not merge to explorer `main`. Do not create explorer commits unless a listed suite is blocked. |
| Explorer branch to test | `cursor/sp-077-nickname-moderation-deletion-f95c` at whatever `HEAD` is when tests run (baseline `a287577`). Record the executed SHA. |
| SP-071 not-accepted | **Honest residual on exits 1–3.** Docs still say SP-071 **In progress**. Map 1–3 as **Pass (automated + code review) + Residual (SP-071 not accepted; device walk Phase 10)**. Do not treat as Fail, and do not silently call SP-071 Accepted. |
| Phase 8 not self-accepted | After evidence: SP-079 work-item Status may move **In progress → In review**. Leave Accepted by / Accepted date empty. Do not edit README §4 status tables. |
| Weekly GET JNI | **Known residual.** Do not wire JNI on this branch. |
| Boss haptic | **Out** (SPD-054). Do not fail exit 6. |
| Friends | **Hidden** (SPD-061). `friends_signup_*` toasts are a copy residual. |
| Highway `Eligibility_*` | **Not Phase 8 exit 6.** If full `street_pixels_tests` aborts because `data/classificator.txt` is absent, record environment residual. Do not weaken Eligibility. Re-check file at run time. |
| Filter `CompetitionHintCopy_*` | **Mandatory extra command.** `--filter=CompetitionHint_` does NOT match `CompetitionHintCopy_*`. Always run `CompetitionHintCopy_` separately. |
| `street_pixels_areas_tests` | **In scope.** Recency/eligibility/contested/weekly live in the areas target. |
| Defects | Prefer record as follow-up. Fix on this branch only if a listed suite is blocked. |
| Dirty tree | Never stage `3party/healpix/healpix`, `data/area_milestones.db`, `data/live_recency.db`. |
| Counts | **Do not guess.** Paste executed transcripts. |
| Postgres prod | Deploy remains ops / Phase 10. |
| Spec vs SPD-059 | V1 nicknames **are unique**. Tests follow the SPD. |

## Scope

Evidence-only. No production behaviour changes on this branch except defect
fixes that block listed suites (prefer fix on owning SP-071–078). Map each
Phase 8 exit criterion (1–12) to pass / fail / residual with pointers into
the evidence log.

Phase 8 modules under test: SP-071 (consent + identity; **not accepted**),
SP-072 (recency / ownership / eligibility / contested / unclaimed),
SP-073 (weekly city new-live pixels), SP-074 (upload queue), SP-075
(backend register / ingest / decay), SP-076 (reads + sparse anonymity),
SP-077 (nickname moderation + deletion), SP-078 (UI / card copy / 30-pixel
hint). SP-070 is the lock set (SPD-057–066).

**Baselines (verify at start; do not treat as pass counts):**

- Client: `/workspace` on `cursor/sp-079-phase8-end-to-end-validation-f95c`
  at `dcad9abd2` (`[docs] Accept SP-078 and start SP-079`), from
  `street-pixels`. Includes SP-078 Accepted.
- Backend: `/home/ubuntu/explorer-src/explorer` on
  `cursor/sp-077-nickname-moderation-deletion-f95c` at `a287577`. **This is
  the branch to test.** `origin/main` is still friends-only.

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Finland / Helsinki | Same class as SP-014 / SP-041 / SP-069 |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log when walks run. This cloud environment is
assumed to have no handset (`adb devices` empty / `adb` absent). If a
device appears, still **do not capture the map**.

## Scenario catalogue

Exit 1–12 map to Blocks A–L plus manual M1–M8 (Phase 10 if no adb device).
Block N is the automated suite re-run that feeds all exits.

### Block A — Competition off by default / active confirmation (exit 1)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | Legacy `Explore.ConsentGiven == true` is not competition consent | `IdentityStore_LegacyConsentBooleanIsNotConsent` | 1 |
| A2 | Empty policy version is not consent | `IdentityStore_EmptyPolicyVersionIsNotConsent` | 1, 2 |
| A3 | Consent off blocks identity upload | `IdentityStore_ConsentOffBlocksIdentityUpload` | 1, 3 |
| A4 | Opt-out → zero competition HTTP | `CompetitionUpload_OptOutZeroHttp` | 1, 4, 5 |
| A5 | Boolean-only “consent given” is not sufficient for upload | `CompetitionUpload_ConsentGivenBooleanOnlyZeroHttp` | 1, 4 |
| A6 | Sharing enabled without consent → zero HTTP | `CompetitionUpload_EnableSharingWithoutConsentZeroHttp` | 1, 4 |
| A7 | Consent revoked during snapshot → zero HTTP | `CompetitionUpload_ConsentRevokedDuringSnapshotZeroHttp` | 1, 5 |
| A8 | 30-pixel hint skipped when already consented | `CompetitionHint_SkippedWhenAlreadyConsented` | 1 |
| A9 | Friends refresh is a no-op in public V1 | `FriendsManager_RefreshNoOpWhenFriendsHiddenInPublicV1` | 1 |
| A10 | Friends skip refresh when API unconfigured | `FriendsManager_SkipsRefreshWhenApiUnconfigured` | 1 |
| A11 | Explore vs Competition control default Explore (code review) | `StreetPixels.CompetitionMapMode` default 0; consent separate from location permission | 1 |
| A12 | Device opt-in walk vs §20.2 | M1 | 1 → Phase 10 if no handset |

Honest residual on this exit: SP-071 is still **In progress** / not
accepted. Map as **Pass (automated + code review) + Residual (SP-071 not
accepted; device walk Phase 10)**. Do not Fail, and do not silently call
SP-071 Accepted.

### Block B — Consent record: policy version + timestamp (exit 2)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | Grant stores unix time and invokes handler | `IdentityStore_GrantInvokesHandlerWithStoredUnixTime` | 2 |
| B2 | Grant without handler does not crash | `IdentityStore_GrantWithoutHandlerDoesNotCrash` | 2 |
| B3 | Empty policy version is not consent | `IdentityStore_EmptyPolicyVersionIsNotConsent` | 1, 2 |
| B4 | Legacy boolean is not the record | `IdentityStore_LegacyConsentBooleanIsNotConsent` | 1, 2 |
| B5 | Grant seeds recency without an extra explicit call | `CompetitionOwnership_GrantHandlerSeedsWithoutExplicitCall` | 2, 6 |
| B6 | Device: consent record includes version + timestamp on opt-in | M1 | 2 → Phase 10 if no handset |

Same SP-071 residual as Block A.

### Block C — Pseudonymous identity / unique nickname / no email (exit 3)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | Reject nicknames outside §21.1 | `IdentityStore_RejectsInvalidNicknames` | 3, 10 |
| C2 | Accept Unicode nicknames in range | `IdentityStore_AcceptsUnicodeNicknames` | 3, 10 |
| C3 | Invalid UTF-8 rejected | `IdentityStore_InvalidUtf8NicknameRejected` | 3, 10 |
| C4 | 409 collision does not persist nickname | `IdentityStore_Collision409DoesNotPersistNickname` | 3, 10 |
| C5 | Generated retry is not a numeric suffix | `IdentityStore_GenerateNicknameRetryIsNotNumericSuffix` | 3 |
| C6 | Existing ASCII username is not auto-accepted | `IdentityStore_ExistingAsciiUsernameNotAutoAccepted` | 3 |
| C7 | Unset handler keeps draft only | `IdentityStore_UnsetHandlerKeepsDraftOnly` | 3 |
| C8 | `SetUsername` does not accept | `IdentityStore_SetUsernameDoesNotAccept` | 3 |
| C9 | Empty API → no HTTP claim | `IdentityStore_ProductionClaimEmptyApiNoHttp` | 3 |
| C10 | Production claim POSTs register JSON without friends headers | `IdentityStore_ProductionClaimPostsRegisterJsonWithoutFriendsHeaders` | 3 |
| C11 | Default handler posts when API configured | `IdentityStore_DefaultHandlerPostsWhenApiConfigured` | 3 |
| C12 | Rename uses nickname URL | `IdentityStore_ProductionClaimRenameUsesNicknameUrl` | 3, 10 |
| C13 | Same-nickname reclaim is not limited | `IdentityStore_SameNicknameReclaimIsNotLimited` | 3, 10 |
| C14 | Local 7-day rename gate | `IdentityStore_RenameLimitLocalSevenDays` | 3, 10 |
| C15 | Blocked nickname invalid without HTTP | `IdentityStore_BlockedNicknameIsInvalidWithoutHttp` | 3, 10 |
| C16 | Backend register success | `test_register_success` | 3 |
| C17 | Case-insensitive collision | `test_register_collision_case_insensitive` | 3, 10 |
| C18 | Unicode nickname on server | `test_register_unicode_nickname` | 3 |
| C19 | ASCII friends-too-short rejected | `test_register_rejects_ascii_friends_too_short` | 3 |
| C20 | Extra field on register rejected | `test_register_rejects_extra_field` | 3, 4 |
| C21 | Register does not require friends headers | `test_register_does_not_require_friends_headers` | 3 |
| C22 | Nickname change collision | `test_nickname_change_collision` | 3, 10 |
| C23 | Nickname change success | `test_nickname_change_success` | 3, 10 |
| C24 | Casefold collision | `test_register_casefold_collision` | 3 |
| C25 | Integrity-error nickname is 409 | `test_register_integrity_error_nickname_is_409` | 3 |
| C26 | Duplicate profile id | `test_register_duplicate_profile_id` | 3 |
| C27 | No email or password on any identity path (code review) | Competition API `auth=None`; JSON `profile_id`; no password fields | 3 |

V1 nicknames **are unique** (SPD-059). Tests follow the SPD, not spec
§20.4 non-uniqueness. Same SP-071 residual as Block A.

### Block D — Upload payload allow-list / backend schema reject (exit 4)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | Client payload allow-list | `CompetitionUpload_PayloadAllowList` | 4 |
| D2 | Client payload deny-list | `CompetitionUpload_PayloadDenyList` | 4, 12 |
| D3 | No friends headers on upload | `CompetitionUpload_NoFriendsHeaders` | 4 |
| D4 | `/stats/upload` never used | `CompetitionUpload_StatsUploadUrlNeverUsed` | 4 |
| D5 | Snapshot live-only omits unique counts and week id | `CompetitionUpload_SnapshotLiveOnlyOmitsUniqueCountsAndWeekId` | 4, 8 |
| D6 | Imported-only → zero competitive HTTP | `CompetitionUpload_ImportedOnlyZeroCompetitiveHttp` | 4, 6 |
| D7 | Explore-stats `TryUpload` hook is zero | `CompetitionUpload_ExploreStatsTryUploadZeroHook` | 4 |
| D8 | Empty snapshot skips HTTP, keeps pending | `CompetitionUpload_EmptySnapshotSkipsHttpKeepsPending` | 4, 5 |
| D9 | Nickname draft only → zero HTTP | `CompetitionUpload_NicknameDraftOnlyZeroHttp` | 4 |
| D10 | Username cleared during snapshot → zero HTTP | `CompetitionUpload_UsernameClearedDuringSnapshotZeroHttp` | 4, 5 |
| D11 | Stats-upload URL empty / configured (leftover symbol) | `BackendConfig_StatsUploadUrlEmptyWhenUnconfigured` / `WhenConfigured`; leftover client symbol unused | 4 |
| D12 | Competition aggregates URL never uses stats-upload | `BackendConfig_CompetitionAggregatesUrlNeverUsesStatsUpload` | 4 |
| D13 | Backend ingest allow-list | `test_ingest_allow_list` | 4 |
| D14 | Extra lat/lon rejected | `test_ingest_extra_lat_lon_rejected` | 4, 12 |
| D15 | Nested schemas forbid extra | `test_nested_schemas_forbid_extra` | 4 |
| D16 | Non-finite schema rejected | `test_ingest_non_finite_schema_rejected` | 4 |
| D17 | `/stats/upload` does not exist | `test_stats_upload_does_not_exist` / `test_stats_upload_still_missing` / `test_stats_upload_is_not_an_area_read_alias` / `test_stats_upload_is_not_a_weekly_read_alias` | 4 |
| D18 | Device traffic capture: no coordinates | M2 | 4 → Phase 10 if no handset |

### Block E — Cadence ≤ 15 min + jitter + offline queue (exit 5)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | Cadence with jitter 0 | `CompetitionUpload_CadenceJitterZero` | 5 |
| E2 | Cadence with jitter 900 | `CompetitionUpload_CadenceJitter900` | 5 |
| E3 | Jitter clamped to closed range | `CompetitionUpload_JitterClampedToClosedRange` | 5 |
| E4 | Offline then flush | `CompetitionUpload_OfflineThenFlush` | 5 |
| E5 | Mark pending does not POST | `CompetitionUpload_MarkPendingDoesNotPost` | 5 |
| E6 | HTTP failure does not retry-storm | `CompetitionUpload_HttpFailureDoesNotRetryStorm` | 5 |
| E7 | Empty apiBase → zero HTTP | `CompetitionUpload_EmptyApiBaseZeroHttp` | 5 |
| E8 | Mark pending during in-flight keeps pending | `CompetitionUpload_MarkPendingDuringInFlightKeepsPending` | 5 |
| E9 | Opt-out zero HTTP | `CompetitionUpload_OptOutZeroHttp` | 1, 5 |
| E10 | Device: opt-out zero upload | M3 | 5 → Phase 10 if no handset |
| E11 | Device: offline queue then flush; stale labelled | M4 | 5 → Phase 10 if no handset |

### Block F — Ownership, eligibility, boss, contested, unclaimed (exit 6)

Highway `Eligibility_*` (classificator highways) is **not** Phase 8 exit 6.
Boss haptic is **out** (SPD-054); do not fail this exit for missing haptic.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Recency weight decay table 0 / 30 / 60 / 90 | `OwnershipScoring_RecencyWeightDecayTable` | 6, 7 |
| F2 | Negative delta clamped | `OwnershipScoring_RecencyWeightClampsNegativeDelta` | 6 |
| F3 | Revisit restores full weight | `OwnershipScoring_RevisitRestoresFullWeight` | 6 |
| F4 | Just-visited full live is 100 | `OwnershipScoring_JustVisitedFullLiveIs100` / `CompetitionOwnership_Score100JustVisitedFullLive` | 6 |
| F5 | Zero total is zero | `OwnershipScoring_ZeroTotalIsZero` | 6 |
| F6 | Imported-only empty live is zero | `OwnershipScoring_ImportedOnlyEmptyLiveIsZero` / `CompetitionOwnership_ImportedOnlyScoreZeroFullPersonalCompletion` | 6 |
| F7 | Missing timestamp contributes zero to sum | `OwnershipScoring_MissingTimestampContributesZeroToSum` | 6 |
| F8 | Eligibility coverage fails independently | `OwnershipScoring_EligibilityCoverageFailsIndependently` | 6 |
| F9 | Eligibility unique-live fails independently | `OwnershipScoring_EligibilityUniqueLiveFailsIndependently` | 6 |
| F10 | Eligibility score fails independently | `OwnershipScoring_EligibilityScoreFailsIndependently` | 6 |
| F11 | Eligibility score boundary | `OwnershipScoring_EligibilityScoreBoundary` | 6 |
| F12 | Eligibility coverage boundary | `OwnershipScoring_EligibilityCoverageBoundary` | 6 |
| F13 | Small-area waives min live pixels | `OwnershipScoring_SmallAreaWaivesMinLivePixels` | 6 |
| F14 | 50-pixel area does not waive | `OwnershipScoring_FiftyPixelAreaDoesNotWaiveMinLive` | 6 |
| F15 | Contested at 80% | `OwnershipScoring_ContestedAtEightyPercent` | 6 |
| F16 | Unclaimed when no eligible | `OwnershipScoring_UnclaimedWhenNoEligible` | 6 |
| F17 | One eligible is not contested | `OwnershipScoring_OneEligibleIsNotContested` | 6 |
| F18 | Local view unclaimed and never contested | `OwnershipScoring_LocalViewUnclaimedAndNeverContested` | 6 |
| F19 | Score uses area total, not explored count | `CompetitionOwnership_ScoreUsesAreaTotalNotExploredCount` | 6 |
| F20 | Imported-only does not affect eligibility or contested | `CompetitionOwnership_ImportedOnlyDoesNotAffectEligibilityOrContested` | 6 |
| F21 | Unknown OSM fail-closed zeros | `CompetitionOwnership_UnknownOsmFailClosedZeros` | 6 |
| F22 | Live first visit writes recency | `CompetitionOwnership_LiveFirstVisitWritesRecency` | 6 |
| F23 | Import does not write recency | `CompetitionOwnership_ImportDoesNotWriteRecency` | 6 |
| F24 | Imported then live writes recency | `CompetitionOwnership_ImportedThenLiveWritesRecency` | 6 |
| F25 | Live then import leaves recency unchanged | `CompetitionOwnership_LiveThenImportLeavesRecencyUnchanged` | 6 |
| F26 | Revisit updates timestamp | `CompetitionOwnership_RevisitUpdatesTimestamp` | 6 |
| F27 | Idle / pause / rejected sample write no recency | `CompetitionOwnership_IdleWritesNoRecency` / `PauseWritesNoRecency` / `RejectedSampleWritesNoRecency` | 6 |
| F28 | Seed on opt-in; second opt-in does not reseed | `CompetitionOwnership_SeedOnOptInAndSecondOptInDoesNotReseed` | 6 |
| F29 | Seed scans pix ever-live; skips imported-only | `CompetitionOwnership_SeedScansPixFileEverLive` / `SeedSkipsImportedOnlyPixCells` | 6 |
| F30 | `.pix` format unchanged v2 | `CompetitionOwnership_PixFormatUnchangedV2` | 6 |
| F31 | Recency store path / WAL / batch / reopen | `LiveRecency_SeedInsertOrIgnoreKeepsFirstTimestamp` / `TouchOverwritesTimestamp` / `DefaultPathIsLiveRecencyNotMilestones` / `TempDbRemovesWalAndShm` / `GetLastLiveVisitsBatch` / `ReopenSwitchesPath` | 6 |
| F32 | Server clamp score 101 → 100 | `test_ingest_clamp_score_101` | 6 |
| F33 | 1% coverage ineligible | `test_ingest_one_percent_coverage_ineligible` | 6 |
| F34 | Score below half ineligible | `test_ingest_score_below_half_ineligible` | 6 |
| F35 | Contested true/false around 0.80 | `test_contested_same_update_50_40_true` / `test_contested_same_update_50_399_false` | 6, 9 |
| F36 | Ineligible runner-up does not contest | `test_ineligible_runner_up_does_not_contest` | 6 |
| F37 | Ineligible leader is not boss | `test_ineligible_leader_is_not_boss` | 6 |
| F38 | Card leading / not-leading / empty without consent or profile | `CompetitionCard_LeadingLineWhenEligibleBoss` / `NotLeadingWhenEligibleButNotBoss` / `EmptyWithoutConsent` / `EmptyWithoutProfile` | 6 |
| F39 | Boss haptic | Out (SPD-054). Do not fail. | 6 |
| F40 | Highway `Eligibility_*` | Not this exit. Optional full-suite residual if `classificator.txt` absent. | — |
| F41 | `QueryCompetitionOwnership` loaded-span only | Known residual on SP-072 / SP-074 | 6 |

### Block G — Server-side decay between uploads (exit 7)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| G1 | Decay 30 / 60 / 90 days | `test_decay_30_60_90_days` | 7 |
| G2 | Decay does not grow for future last_update | `test_decay_does_not_grow_for_future_last_update` | 7 |
| G3 | Stored coverage is not decayed | `test_stored_aggregate_coverage_not_decayed` | 7 |
| G4 | Newer upload replaces decayed score | `test_newer_upload_replaces_decayed_score` | 7 |
| G5 | Older upload does not replace | `test_older_upload_does_not_replace` | 7 |
| G6 | Client recency weight table matches half-life | `OwnershipScoring_RecencyWeightDecayTable` | 6, 7 |
| G7 | Contested after decay (leader half-life vs runner 40 / 39.9) | `test_contested_after_decay_leader_half_life_runner_40` / `…_399_false` | 6, 7 |
| G8 | Stored 85 half-life ago vs 100 now is not contested | `test_stored_85_half_life_ago_vs_100_now_not_contested` | 6, 7 |
| G9 | Unclaimed when decayed below half | `test_unclaimed_when_decayed_below_half` | 6, 7 |
| G10 | Device: become boss, go inactive, decay without opening the app | M6 | 7 → Phase 10 if no handset |

### Block H — Weekly city leaderboard: no revisits / imports; weekly reset (exit 8)

Weekly GET is **not JNI-wired**. Record as known residual. Do not wire JNI
on this branch.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| H1 | First live visit counts | `WeeklyCityLive_FirstLiveVisitCounts` | 8 |
| H2 | Second visit same cell does not | `WeeklyCityLive_SecondVisitSameCellDoesNot` | 8 |
| H3 | Import-only does not | `WeeklyCityLive_ImportOnlyDoesNot` | 8 |
| H4 | Track replay does not | `WeeklyCityLive_TrackReplayDoesNot` | 8 |
| H5 | Already ever-live does not | `WeeklyCityLive_AlreadyEverLiveDoesNot` | 8 |
| H6 | Imported then live counts once | `WeeklyCityLive_ImportedThenLiveCountsOnce` | 8 |
| H7 | Idle / pause / rejected do not | `WeeklyCityLive_IdlePauseRejectedDoNot` | 8 |
| H8 | Two cities independent | `WeeklyCityLive_TwoCitiesIndependent` / `WeeklyCityLive_TwoCities` | 8 |
| H9 | Query by settlement not subdivision | `WeeklyCityLive_QueryBySettlementNotSubdivision` | 8 |
| H10 | No-area pixel does not invent city | `WeeklyCityLive_NoAreaPixelDoesNotInventCity` | 8 |
| H11 | Query week remaining | `WeeklyCityLive_QueryWeekRemaining` | 8 |
| H12 | Interpolation counts once | `WeeklyCityLive_InterpolationCountsOnce` | 8 |
| H13 | Store increment / schema has no GPS or HEALPix | `WeeklyCityLive_Increment` / `SchemaHasNoGpsOrHealpix` | 8, 12 |
| H14 | TZ changes week id vs UTC | `WeeklyCityLive_TzChangesWeekIdVsUtc` | 8 |
| H15 | Default db path / WAL cleanup / unknown city UTC 0 | `WeeklyCityLive_DefaultDbPathFilename` / `TempDbRemovesWalAndShm` / `UnknownCityUtcZero` | 8 |
| H16 | Monday boundary separate weeks | `WeeklyCityLive_MondayBoundarySeparateWeeks` | 8 |
| H17 | UTC Monday boundary | `WeeklyCityWeek_UtcMondayBoundary` | 8 |
| H18 | Fixed-offset Monday | `WeeklyCityWeek_FixedOffsetMonday` | 8 |
| H19 | Empty TZ is UTC | `WeeklyCityWeek_EmptyTzIsUtc` | 8 |
| H20 | Device TZ ignored | `WeeklyCityWeek_DeviceTzIgnored` | 8 |
| H21 | Remaining time | `WeeklyCityWeek_RemainingTime` | 8 |
| H22 | Server UTC / LA / Auckland week membership | `test_utc_week_membership_excludes_previous_week` / `test_los_angeles_week_membership` / `test_auckland_week_membership` | 8 |
| H23 | Last-write-wins does not sum | `test_last_write_wins_does_not_sum` | 8 |
| H24 | Ignores stored week_start_unix | `test_ignores_stored_week_start_unix` | 8 |
| H25 | Weekly N ignores previous-week rows | `test_weekly_n_ignores_previous_week_rows` | 8 |
| H26 | Empty city is 200 not 404 | `test_empty_city_is_200_not_404` | 8 |
| H27 | Week helper: UTC / LA / Auckland / DST / missing TZ never Helsinki | `test_utc_monday_boundary_remaining_is_one_week` / `test_america_los_angeles_week_is_not_utc` / `test_pacific_auckland_week_starts_before_utc` / `test_dst_spring_forward_composes_local_midnight` / `test_dst_fall_back_composes_local_midnight` / `test_missing_city_timezone_is_utc_never_helsinki` / `test_invalid_timezone_falls_back_to_utc` | 8 |
| H28 | `city_timezones.py` empty dict | Residual SP-076; SPD-060 keep | 8 |
| H29 | Weekly GET JNI not wired | Known residual. Do not wire. | 8 |
| H30 | Ever-live-flip vs newly-explored-only §24.1 | Residual SP-073 product lock | 8 |
| H31 | Weekly crash window drops increment | Residual SP-073 | 8 |

### Block I — Sparse-area anonymity server-side (exit 9)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| I1 | N=0 nickname visibility | `test_n0_nickname_visibility` | 9 |
| I2 | N=1 self keeps ranking nickname; boss hidden | `test_n1_self_keeps_ranking_nickname_boss_hidden` | 9 |
| I3 | N=1 other viewer hides nicknames and gaps | `test_n1_other_viewer_hides_nicknames_and_gaps` | 9 |
| I4 | N=1 raw body hides other nickname and profile | `test_n1_raw_body_hides_other_nickname_and_profile` | 9 |
| I5 | N=2 only current-user ranking nickname | `test_n2_only_current_user_ranking_nickname` | 9 |
| I6 | N=3 shows nicknames including boss | `test_n3_shows_nicknames_including_boss` | 9 |
| I7 | N=4 top-three de-dup | `test_n4_top_three_dedup_in_top3_and_fourth` | 9 |
| I8 | Weekly N=1 self vs other | `test_weekly_n1_self_vs_other_viewer` | 9 |
| I9 | Weekly N=2/3/4 nickname and top-3 de-dup | `test_weekly_n2_n3_n4_nickname_and_top3_dedup` | 9 |
| I10 | Client parse null vs named nickname | `CompetitionSnapshot_ParseNullNickname` / `ParseNamedNickname` | 9 |
| I11 | Client sparse copy N&lt;3 anonymous boss you / other | `CompetitionSparse_Nlt3AnonymousBossYouLead` / `Nlt3AnonymousBossOther` | 9, 12 |
| I12 | Ranking null nicknames stay null | `CompetitionRanking_NullNicknamesStayNull` | 9, 12 |
| I13 | Ranking user in top 3 no duplicate / fourth appends | `CompetitionRanking_UserInTop3NoDuplicate` / `UserFourthAppendsRow` | 9 |
| I14 | Area / weekly schemas forbid extra | `test_area_schemas_forbid_extra` / `test_weekly_schemas_forbid_extra` | 9, 4 |
| I15 | Device: N&lt;3 no nicknames | M5 | 9 → Phase 10 if no handset |

### Block J — Nickname validation, filter, report, admin reset, 7-day (exit 10)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| J1 | Client rejects invalid / accepts Unicode | C1–C3 | 10 |
| J2 | Client 409 does not persist | `IdentityStore_Collision409DoesNotPersistNickname` | 10 |
| J3 | Local 7-day rename gate | `IdentityStore_RenameLimitLocalSevenDays` | 10 |
| J4 | Same-name reclaim not limited | `IdentityStore_SameNicknameReclaimIsNotLimited` | 10 |
| J5 | Blocked nickname invalid without HTTP | `IdentityStore_BlockedNicknameIsInvalidWithoutHttp` | 10 |
| J6 | Report posts JSON without friends headers | `IdentityStore_ReportNicknamePostsJsonWithoutFriendsHeaders` | 10 |
| J7 | Report blocked nickname still posts | `IdentityStore_ReportBlockedNicknameStillPosts` | 10 |
| J8 | Report empty API no HTTP | `IdentityStore_ReportNicknameEmptyApiNoHttp` | 10 |
| J9 | Rename too soon is 409 `rename_limited` | `test_rename_too_soon_is_409_rename_limited` | 10 |
| J10 | Rename after seven days is 200 | `test_rename_after_seven_days_is_200` | 10 |
| J11 | Collision after seven days is 409 `nickname_taken` | `test_rename_collision_after_seven_days_is_409_nickname_taken` | 10 |
| J12 | Blocked nicknames are 422 | `test_blocked_nicknames_are_422` | 10 |
| J13 | `comapsadmin` is allowed | `test_comapsadmin_is_allowed` | 10 |
| J14 | Same-name reclaim does not move rename clock | `test_same_name_reclaim_does_not_move_rename_clock` | 10 |
| J15 | Admin reset allows immediate user rename | `test_admin_reset_allows_immediate_user_rename` | 10 |
| J16 | Report persists with year expiry | `test_report_persists_with_year_expiry` | 10 |
| J17 | Report unknown target / reporter is 404 | `test_report_unknown_target_is_404` / `test_report_unknown_reporter_is_404` | 10 |
| J18 | Report extra lat is 422; bad reason 422; schema forbids extra | `test_report_extra_lat_is_422` / `test_report_bad_reason_is_422` / `test_report_schema_forbids_extra` | 10, 4 |
| J19 | Eleventh report is 429 | `test_eleventh_report_is_429` | 10 |
| J20 | Report does not require friends headers; blocked target still reportable | `test_report_does_not_require_friends_headers` / `test_report_blocked_target_still_reportable` | 10 |
| J21 | Client 7-day-gates after admin reset | Residual on SP-077 | 10 |
| J22 | HTTP 409 `rename_limited` mapped to Collision | Residual on SP-077 | 10 |
| J23 | V1 blocked-list small whole-token set | Residual | 10 |
| J24 | Unicode script-table vs server | Server remains authority | 10 |

### Block K — Profile / aggregate deletion; local exploration intact (exit 11)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| K1 | Delete success does not clear `.pix` or recency | `CompetitionDeletion_SuccessDoesNotClearPixOrRecency` | 11 |
| K2 | Failed delete keeps pix, recency, and identity | `CompetitionDeletion_FailedKeepsPixRecencyAndIdentity` | 11 |
| K3 | Delete clears username | `IdentityStore_DeleteCompetitionProfileClearsUsername` | 11 |
| K4 | Failed delete keeps identity | `IdentityStore_DeleteCompetitionProfileFailedKeepsIdentity` | 11 |
| K5 | Empty API delete keeps identity | `IdentityStore_DeleteCompetitionProfileEmptyApiKeepsIdentity` | 11 |
| K6 | Leave retain keeps username | `IdentityStore_LeaveCompetitionRetainKeepsUsername` | 11 |
| K7 | Leave failed HTTP still revokes | `IdentityStore_LeaveCompetitionRetainFailedHttpStillRevokes` | 11 |
| K8 | Server delete removes profile and aggregates | `test_delete_removes_profile_and_aggregates` | 11 |
| K9 | Second delete is 404 | `test_second_delete_is_404` | 11 |
| K10 | Delete extra lat is 422 | `test_delete_extra_lat_is_422` | 11, 4 |
| K11 | Leave extra lat is 422 | `test_leave_extra_lat_is_422` | 11, 4 |
| K12 | Leave keeps aggregates; ingest clears flag | `test_leave_keeps_aggregates_and_ingest_clears_flag` | 11 |
| K13 | Leave unknown profile is 404 | `test_leave_unknown_profile_is_404` | 11 |
| K14 | Purge silent profiles respects retention | `test_purge_silent_profiles_respects_retention_window` | 11 |
| K15 | Purge expired reports | `test_purge_expired_reports` | 11 |
| K16 | Export shape and unknown profile | `test_export_shape_and_unknown_profile` | 11 |
| K17 | Lifecycle does not require friends headers | `test_lifecycle_does_not_require_friends_headers` | 11 |
| K18 | Failed `POST /leave` has no retry queue | Residual on SP-077 | 11 |
| K19 | Revoke does not delete `live_recency.db` rows | Residual on SP-072 | 11 |
| K20 | Device: delete profile; local exploration intact | M7 | 11 → Phase 10 if no handset |

### Block L — No live location / exact location / presence (exit 12)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| L1 | Client copy deny-list | `CompetitionCopy_DenyList` | 12 |
| L2 | Never “someone is nearby” | `CompetitionSparse_NeverSomeoneIsNearby` | 12 |
| L3 | Sparse anonymous boss copy | `CompetitionSparse_Nlt3AnonymousBossYouLead` / `Nlt3AnonymousBossOther` | 9, 12 |
| L4 | Ranking null nicknames stay null | `CompetitionRanking_NullNicknamesStayNull` | 9, 12 |
| L5 | Chrome stale / offline | `CompetitionChrome_StaleFlagFromSnapshot` / `CompetitionChrome_OfflineWhenNoSnapshot` | 12 |
| L6 | Hint copy generic / with area without snapshot | `CompetitionHintCopy_CompareGenericWithoutArea` / `CompareWithAreaWithoutSnapshot` | 12 |
| L7 | 30-pixel hint: fires at 30 newly explored live; not import; does not reset first-goal 10; not while routing-following; once per install; live visit of imported does not advance | `CompetitionHint_FiresAtThirtyNewlyExploredLivePixels` / `ImportDoesNotAdvance` / `DoesNotResetFirstGoalTen` / `DoesNotPresentWhileRoutingFollowing` / `OncePerInstall` / `LiveVisitOfImportedPixelsDoesNotAdvance` | 12, 1 |
| L8 | Snapshot URL has profile query, no friends headers | `CompetitionSnapshot_UrlHasProfileQueryNoFriendsHeaders` | 12 |
| L9 | Empty apiBase no GET; HTTP fail uses cache and stale/offline | `CompetitionSnapshot_EmptyApiBaseNoGet` / `HttpFailUsesCacheAndStaleOrOffline` | 12 |
| L10 | Weekly store schema has no GPS or HEALPix | `WeeklyCityLive_SchemaHasNoGpsOrHealpix` | 8, 12 |
| L11 | Backend privacy deny-keys on responses | `assert_no_denied_keys` via area/weekly/export tests | 12 |
| L12 | Ingest extra lat/lon rejected | `test_ingest_extra_lat_lon_rejected` | 4, 12 |
| L13 | `friends_signup_*` nickname toasts | Copy residual; SPD-061 friends hidden | 12 |
| L14 | Device: no presence copy eyeball | M8 | 12 → Phase 10 if no handset. Map screenshots remain forbidden. |

### Block M — Manual / device (all exits)

Phase 10 if `adb devices` shows no handset. Do not fabricate walks,
screenshots, or packet captures. Map screenshots remain **forbidden** even
if a device appears.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| M1 | Opt-in walk vs spec §20.2 item by item | Visual + copy vs aggregates, nickname when enough participants, no routes, no raw GPS, no live location, no nearby discovery, can leave later | 1, 2, 3 |
| M2 | Traffic capture during recording with competition enabled | No coordinates; cadence and jitter hold | 4, 5 |
| M3 | Opt-out zero upload | No competition HTTP with competition disabled | 1, 5 |
| M4 | Offline queue then flush; stale rankings labelled | Queued aggregates upload; stale chrome | 5 |
| M5 | N &lt; 3 nicknames | No other nicknames | 9 |
| M6 | Decay without opening the app | Boss eligibility lost server-side | 7 |
| M7 | Delete profile; local exploration intact | Server removal; personal green remains | 11 |
| M8 | Presence eyeball | No surface indicates another participant’s location or presence | 12 |

### Block N — Automated suites (feeds all exits)

`--filter=CompetitionHint_` does **not** match `CompetitionHintCopy_*`.
Always run `CompetitionHintCopy_` separately.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| N1 | `street_pixels_tests --filter='IdentityStore_|FriendsManager_|ExploreStatsUpload_'` | All pass; count recorded | 1–3, 10, 11 |
| N2 | `street_pixels_tests --filter='BackendConfig_|ExploreStatsUpload_|FriendsManager_'` | All pass; count recorded | 1, 3, 4 |
| N3 | `street_pixels_tests --filter='CompetitionUpload_'` | All pass; count recorded | 4, 5 |
| N4 | `street_pixels_tests --filter='CompetitionOwnership_'` | All pass; count recorded | 6 |
| N5 | `street_pixels_tests --filter='CompetitionDeletion_'` | All pass; count recorded | 11 |
| N6 | `street_pixels_tests --filter='CompetitionSnapshot_'` | All pass; count recorded | 9, 12 |
| N7 | `street_pixels_tests --filter='CompetitionHint_'` | All pass; count recorded (does **not** include HintCopy) | 1, 12 |
| N8 | `street_pixels_tests --filter='CompetitionHintCopy_'` | All pass; count recorded | 12 |
| N9 | `street_pixels_tests --filter='CompetitionCard_|CompetitionRanking_|CompetitionSparse_|CompetitionChrome_|CompetitionCopy_|CompetitionHintCopy_'` | All pass; count recorded | 6, 9, 12 |
| N10 | `street_pixels_tests --filter='WeeklyCityLive_'` | All pass; count recorded | 8 |
| N11 | `street_pixels_tests --filter='Competition'` | All pass; count recorded | 4–12 |
| N12 | `street_pixels_areas_tests --filter='OwnershipScoring_|LiveRecency_|WeeklyCity'` | All pass; count recorded | 6, 7, 8 |
| N13 | Full `street_pixels_areas_tests` | All pass or residual recorded | 6, 8 |
| N14 | Full `street_pixels_tests` (optional if `data/classificator.txt` exists) | All pass or Eligibility environment residual | 1–12 |
| N15 | Explorer `uv run pytest -q` on SP-077 branch | All pass; count recorded. Do not merge to `main`. | 3, 4, 6–11 |
| N16 | Throttle smoke | `test_sixth_register_is_throttled` / `test_nickname_and_ingest_have_separate_throttle_scopes` / `test_thirty_first_competition_read_is_throttled` / `test_area_and_weekly_reads_share_throttle_scope` / `test_throttle_instances_keep_distinct_scopes` / `test_thirty_reads_do_not_throttle_ingest` | 4, 9, 10 |
| N17 | Prod settings reject SQLite | `test_production_database_requires_url` / `rejects_sqlite` / `accepts_postgres`. Deploy remains ops / Phase 10. | — |

## Exit criteria mapping (fill in evidence log)

| # | Criterion | Evidence blocks |
| --- | --- | --- |
| 1 | Competition off by default; active confirmation separate from location permission | A, M1, N1, N2 |
| 2 | Consent record includes privacy-policy version and timestamp | B, M1, N1 |
| 3 | Pseudonymous identity and nickname; no email or password | C, N1, N15 |
| 4 | Uploads contain only §25.2 fields; backend rejects anything else | D, M2, N3, N15 |
| 5 | Cadence at most once per 15 minutes plus jitter; offline queueing | E, M3, M4, N3 |
| 6 | Ownership, eligibility, boss, contested, unclaimed | F, N4, N12, N13. Boss haptic out (SPD-054). Highway `Eligibility_*` not this exit. |
| 7 | Server-side decay between uploads | G, M6, N15 |
| 8 | Weekly city leaderboard excludes revisits and imports; weekly reset | H, N10, N12, N15. Weekly GET JNI residual. |
| 9 | Sparse-area anonymity enforced server-side | I, M5, N6, N9, N15 |
| 10 | Nickname validation, filtering, reporting, admin reset, seven-day rename | J, N1, N15 |
| 11 | Profile and aggregate deletion; local exploration intact | K, M7, N5, N15 |
| 12 | No surface reveals another user’s live location, exact location, or presence | L, M8, N7, N8, N9 |

Exits 1–3: **Pass (automated + code review) + Residual (SP-071 not
accepted; device walk Phase 10)** when the named suites are green. Do not
Fail, and do not silently call SP-071 Accepted.

## 4. Test commands

Record SHA + `adb devices` first. Save transcripts under
`/opt/cursor/artifacts/` (include `sp079_street_pixels_hintcopy.log`).
Do not guess counts.

```bash
cd /workspace
git rev-parse HEAD
adb devices || true
./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests street_pixels_areas_tests
BIN=/home/ubuntu/omim-build-debug
DATA_ARGS="--data_path=/workspace/data --user_resource_path=/workspace/data"

"$BIN/street_pixels_tests" $DATA_ARGS --filter='IdentityStore_|FriendsManager_|ExploreStatsUpload_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='BackendConfig_|ExploreStatsUpload_|FriendsManager_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionUpload_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionOwnership_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionDeletion_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionSnapshot_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionHint_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionHintCopy_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionCard_|CompetitionRanking_|CompetitionSparse_|CompetitionChrome_|CompetitionCopy_|CompetitionHintCopy_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='WeeklyCityLive_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='Competition'
"$BIN/street_pixels_areas_tests" $DATA_ARGS --filter='OwnershipScoring_|LiveRecency_|WeeklyCity'
"$BIN/street_pixels_areas_tests" $DATA_ARGS
# optional full street_pixels_tests if classificator.txt exists

cd /home/ubuntu/explorer-src/explorer
git rev-parse HEAD
git branch --show-current
uv run pytest -q
```

Re-check `data/classificator.txt` at run time before the optional full
`street_pixels_tests`. If absent, record environment residual. Do not
weaken `Eligibility_*`.

## Commit strategy

1. Commit 1 (this plan only): `[docs] Add SP-079 Phase 8 validation plan`.
   Body includes `Work item: SP-079`. `git commit -s`. Then
   `git push -u origin cursor/sp-079-phase8-end-to-end-validation-f95c`.
2. Run §4 commands. Save transcripts. Fill the evidence log and work-item
   completion evidence. Optional one-liner on
   `docs/implementation/phases/phase-08-competition.md`.
3. Commit 2: `[docs] Record SP-079 Phase 8 validation evidence`. Push.
   Stop.

Never stage `3party/healpix/healpix`, `data/area_milestones.db`,
`data/live_recency.db`. Stay on
`cursor/sp-079-phase8-end-to-end-validation-f95c`. Do not create GitHub
PRs. Do not start another work item.

## Explorer rules

- Checkout is **present** at `/home/ubuntu/explorer-src/explorer`.
- Test branch: `cursor/sp-077-nickname-moderation-deletion-f95c` (baseline
  `a287577`). Record the executed SHA.
- Run `uv run pytest -q` there.
- **Do not** merge to explorer `main`.
- **Do not** create explorer commits unless a listed suite is blocked.
- `origin/main` is still friends-only; that is not the branch under test.

## Out of scope / non-goals for this gate

- New features beyond defect fixes that block listed suites.
- Marking SP-079 or Phase 8 **Accepted**.
- Setting phase Status to **Exit criteria met**.
- Editing README §4 status tables.
- Changing SP-071 from **In progress**.
- Weakening, skipping, deleting, or narrowing tests.
- Friends feature revival (SPD-061).
- Wiring weekly GET JNI.
- Boss haptic (SPD-054).
- Map screenshots.
- Fabricating device walks, traffic captures, or packet captures.
- Postgres production deploy (ops / Phase 10).
- Merging competition into explorer `main`.
- Editing `docs/STREET_PIXELS_PRODUCT_SPEC.md` or
  `docs/street-pixels-technical-audit.md`.
- Editing `DECISIONS.md` or owning SP-071–078 unless a new defect must be
  recorded on the owner.
- Starting another work item.

## Discovered-follow-up placeholders (pre-fill; do not silently drop)

| Finding | Disposition |
| --- | --- |
| No handset: opt-in walk, traffic capture, opt-out zero upload, offline queue, N&lt;3, decay-without-app, delete+local intact, presence eyeball | Phase 10. Map screenshots remain forbidden. |
| SP-071 still In progress / not accepted | Maintainer; residual on exits 1–3 |
| friends_signup_* nickname toasts | Later copy cleanup; SPD-061 |
| Weekly GET not JNI-wired | Later client WI / Phase 10 |
| Boss haptic out | SPD-054 |
| CompetitionHint_ misses CompetitionHintCopy_ | Mitigated by extra filter |
| Client 7-day-gates after admin reset | SP-077 follow-up still open |
| HTTP 409 rename_limited mapped to Collision | Residual on SP-077 |
| Failed POST /leave has no retry queue | Residual on SP-077 |
| V1 blocked-list small whole-token set | Residual |
| Unicode script-table vs server | Server remains authority |
| city_timezones.py empty dict | SP-076; SPD-060 keep |
| Postgres production deploy | Ops / Phase 10 |
| Exact EU region string | SPD-062 ops |
| QueryCompetitionOwnership loaded-span only | SP-072 / SP-074 |
| Revoke does not delete live_recency.db rows | SP-072 |
| Weekly crash window drops increment | SP-073 |
| Ever-live-flip vs newly-explored-only §24.1 | SP-073 product lock |
| /stats/upload leftover client symbol | Leave unused |
| classificator.txt Eligibility abort if absent | Environment residual |
| Phase 8 exit Met? | Maintainer only |
