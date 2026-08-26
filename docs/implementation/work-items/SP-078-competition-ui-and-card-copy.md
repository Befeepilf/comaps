# SP-078 — Competition UI, card copy, and 30-pixel hint

**Phase:** 8 — Competition
**Status:** Accepted
**Branch:** `cursor/sp-078-competition-ui-and-card-copy-f95c`
**Depends on:** SP-071 identity; SP-072 scores; SP-076 reads (stubs
  allowed with stale/offline labels); SPD-052 stub; SPD-066
**Unblocks:** SP-079

---

## Objective

Ship Android competition chrome: Explore vs Competition control, area
snapshot, ranking, overtaking hints, §22.10 completion-card lines, and
the §10 step 10 hint at 30 newly explored live pixels.

## Motivation

Phase 7 left `competitionLine` empty (SPD-052). Spec §10 step 10, §22.10,
and §23 are V1. Friends UI must stay hidden (SPD-061, SP-071).

## In-scope behavior

- Compact map control: **Explore** (default) vs **Competition**. Turning
  Competition on does not change red/green pixels into a territory skin
  (§23.1–§23.2).
- Area snapshot from SP-076: boss, contested, unclaimed, user score, gap
  to next relevant participant, personal completion (SPD-026, distinct
  from ownership). Offline/stale labelling (§26.2).
- Ranking snapshot: top three + current user, no duplicate (§23.3).
- Sparse-area copy when the server omits nicknames (§23.4). Never “someone
  is nearby”.
- Rate-limited overtaking hints from delayed aggregates (§23.5).
- Completion card: fill leading / not-leading sentences (§22.10). Card
  still works with no profile (SPD-052). Never imply personal 100% is
  invalid.
- Once-per-install hint after **30 newly explored live pixels**
  (SPD-066), non-blocking, no nearby-user language. Independent of the
  first-goal 10-pixel tracker.
- Competition settings: leave/delete actions from SP-077.

## Out-of-scope behavior

- Boss haptic (SPD-054).
- Friends (SPD-061).
- iOS.
- Drawing other users on the map.

## Relevant product requirements

- Spec §10 steps 10–12, §22.10, §23, §24 presentation, §26.2.
- SPD-052, SPD-054, SPD-061, SPD-066.

## Relevant source files or symbols

- `CompletionCardSource::m_competitionLine`, `ComposeCompletionCard`
- `FirstGoalTracker` (do not reuse threshold 10)
- Android map buttons / `MwmActivity` menu; area detail sheet from
  Phase 5
- `MyAccountDialogFragment` after SP-071

## Implementation notes / constraints

- Shared copy strings in C++ or Android resources; tests should lock
  §22.10 meaning, not only English pixels.
- Hints must not interrupt `IsRoutingFollowing` (same rule as SPD-050).
- Do not screenshot the map for competition chrome.

## Acceptance criteria

1. Explore remains default; competition overlay is readable on top of
   red/green pixels.
2. Card leading / not-leading copy; anonymous card still valid.
3. 30-pixel hint once; 10-pixel first-goal unchanged.
4. Sparse-area UI never shows other nicknames if the payload omitted
   them.
5. No nearby/live-location language in strings tests.

## Required automated tests

- Card copy: leading, not-leading, no-profile stub.
- Hint fires at 30 newly explored live pixels; not on import; does not
  reset first-goal.
- Ranking de-dup; stale flag presentation model.
- String deny-list: nearby, live location, exact coordinates.

## Required manual validation

- Device: opt-in walkthrough vs §20.2; card; hint; Explore/Competition
  toggle (SP-079).

## Failure and rollback considerations

- Prefer hiding competition chrome over showing nicknames from a sparse
  payload.
- Prefer empty `competitionLine` over copy that invalidates 100%.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-078-competition-ui-and-card-copy-f95c` at `179f2d0521b99a61185a8e2ba90ef0f47404956a` |
| Test output | See executed output below. Not Accepted. |
| Accepted by | Product owner |
| Accepted date | 2026-08-26 |

## Executed test output

Cwd `/workspace`. Binary `/home/ubuntu/omim-build-debug/street_pixels_tests`. Build: `./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests`. Filter as specified for SP-078. 58 tests. Summary line: `All tests passed.`

```
Running area_milestone_presentation_tests.cpp::AreaMilestonePresentation_CompetitionLineStubEmpty
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlEmptyWhenUnconfigured
Settings path: /workspace/data/settings.ini
Explore.FriendVisibilityEnabled : true
Explore.SyncEnabled : true
StreetPixels.CompetitionMapMode : 0
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlNeverUsesStatsUpload
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionRegisterUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionRegisterUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionNicknameUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionNicknameUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAreaSnapshotUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAreaSnapshotUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionWeeklyBoardUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionWeeklyBoardUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionAreaSnapshotRequestUrlHasProfileQuery
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionWeeklyBoardRequestUrlHasProfileQuery
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionDeleteUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionDeleteUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionReportUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionReportUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionLeaveUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionLeaveUrlWhenConfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionExportUrlEmptyWhenUnconfigured
OK
Test took 0 ms

Running backend_config_tests.cpp::BackendConfig_CompetitionExportUrlWhenConfigured
OK
Test took 0 ms

Running completion_card_tests.cpp::CompletionCard_CompetitionLineStubEmpty
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionCard_LeadingLineWhenEligibleBoss
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionCard_NotLeadingWhenEligibleButNotBoss
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionCard_EmptyWithoutConsent
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionCard_EmptyWithoutProfile
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionRanking_UserInTop3NoDuplicate
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionRanking_UserFourthAppendsRow
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionRanking_NullNicknamesStayNull
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionSparse_Nlt3AnonymousBossYouLead
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionSparse_Nlt3AnonymousBossOther
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionSparse_NeverSomeoneIsNearby
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionChrome_StaleFlagFromSnapshot
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionChrome_OfflineWhenNoSnapshot
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionCopy_DenyList
OK
Test took 1 ms

Running first_goal_tests.cpp::FirstGoal_AppearsOnFirstRecordingStart
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_CompletesAtTenNewlyExploredLivePixels
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 7 ms

Running first_goal_tests.cpp::FirstGoal_ImportDoesNotAdvance
StreetPixels increment skipped n 2 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_PauseDoesNotIncrement
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_IncompleteSurvivesSecondSession
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_CompleteDoesNotReturn
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 3 ms

Running first_goal_tests.cpp::FirstGoal_PersistsAcrossNewTracker
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_LiveVisitOfImportedPixelsDoesNotAdvance
StreetPixels increment skipped n 2 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_DebugTriggerShowsBadgeAndMilestoneQueue
OK
Test took 0 ms

Running first_goal_tests.cpp::FirstGoal_SinglePulseCanComplete
OK
Test took 0 ms

Running competition_hint_tests.cpp::CompetitionHint_FiresAtThirtyNewlyExploredLivePixels
OK
Test took 0 ms

Running competition_hint_tests.cpp::CompetitionHint_ImportDoesNotAdvance
StreetPixels increment skipped n 2 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running competition_hint_tests.cpp::CompetitionHint_DoesNotResetFirstGoalTen
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
StreetPixels increment skipped n 1 cacheValid 0 hasResolver 0
OK
Test took 11 ms

Running competition_hint_tests.cpp::CompetitionHint_DoesNotPresentWhileRoutingFollowing
OK
Test took 0 ms

Running competition_hint_tests.cpp::CompetitionHint_SkippedWhenAlreadyConsented
OK
Test took 0 ms

Running competition_hint_tests.cpp::CompetitionHint_OncePerInstall
OK
Test took 0 ms

Running competition_hint_tests.cpp::CompetitionHint_LiveVisitOfImportedPixelsDoesNotAdvance
StreetPixels increment skipped n 2 cacheValid 0 hasResolver 0
OK
Test took 0 ms

Running competition_snapshot_tests.cpp::CompetitionSnapshot_ParseNullNickname
OK
Test took 0 ms

Running competition_snapshot_tests.cpp::CompetitionSnapshot_ParseNamedNickname
OK
Test took 0 ms

Running competition_snapshot_tests.cpp::CompetitionSnapshot_EmptyApiBaseNoGet
OK
Test took 0 ms

Running competition_snapshot_tests.cpp::CompetitionSnapshot_HttpFailUsesCacheAndStaleOrOffline
OK
Test took 0 ms

Running competition_snapshot_tests.cpp::CompetitionSnapshot_UrlHasProfileQueryNoFriendsHeaders
OK
Test took 0 ms

All tests passed.
```

Planner filter `CompetitionHint_` does not match `CompetitionHintCopy_*`. Those two UNIT_TESTs were run separately and passed:

```
Running competition_presentation_tests.cpp::CompetitionHintCopy_CompareGenericWithoutArea
OK
Test took 0 ms

Running competition_presentation_tests.cpp::CompetitionHintCopy_CompareWithAreaWithoutSnapshot
OK
Test took 0 ms

All tests passed.
```

Unity build initially failed because `competition_hint.cpp` reused anonymous-namespace identifiers `kCollectedKey` / `kCompleteKey` from `first_goal.cpp`. Settings string values were unchanged. Fixed in `6aa8d6319`. Not Accepted.

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Device walkthrough vs §20.2, card, hint, toggle | SP-079 only |
| `friends_signup_*` still used for nickname claim toasts | Later copy cleanup; do not resurrect friends API |
| Weekly board city OSM when focus is a subdivision not a settlement | Show weekly block only when `citySummary` is true; do not invent a city OSM. Weekly GET is not JNI-wired; sheet shows `competition_weekly_empty` |
| Boss haptic | Still out (SPD-054) |
| iOS | Out of V1 |
| Drawing other users | Out |
| Map screenshots of competition chrome | Forbidden; SP-079 must not capture the map either |
| Leftover `pref_explore_username_summary` friends flavor | Do not rebrand in this item |
| Planner `--filter` misses `CompetitionHintCopy_*` | Residual; both tests passed when run |
| Unity-build redefinition of `kCollectedKey` / `kCompleteKey` | Fixed: renamed C++ identifiers only |
