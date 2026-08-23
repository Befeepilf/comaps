# SP-069 — Validation plan (Phase 7 exit)

**Work item:** [SP-069](../work-items/SP-069-phase7-end-to-end-validation.md)
**Plan authored by:** Agent
**Plan review date:** 2026-08-20
**Branch:** `cursor/sp-069-phase7-end-to-end-validation-c417` (lands on `street-pixels`)

## Approved decisions

| ID | Decision |
| --- | --- |
| Device walks | Deferred to **Phase 10** if no handset (SP-014 / SP-022 / SP-031 / SP-041 pattern). Automated exit coverage remains mandatory. |
| Competition-on copy | §22.10 leading / not-leading sentences are **Phase 8**. Do not fail Phase 7 on live competition chrome. Stub must stay empty / first-person (SPD-052). |
| Growth upload | Local uint64 only (SPD-055). Upload residual remains Phase 10. |
| Phase 7 Accepted | Maintainer decides after reviewing evidence. Agent does **not** mark Phase 7 exit Met unilaterally. |
| Eligibility suite | If `data/classificator.txt` is absent, record environment residual. Do not weaken `Eligibility_*`. |

## Scope

Evidence-only. No production behaviour changes on this branch except defect
fixes that block listed suites (prefer fix on owning SP-062–068). Map each
Phase 7 exit criterion (1–9) to pass / fail / residual with pointers into the
evidence log.

Phase 7 modules under test: SP-063 (fired-state), SP-064 (first-goal),
SP-065 (presentation), SP-066 (haptics), SP-067 (card model), SP-068 (share
+ counters). SP-062 is the lock set (SPD-046–055).

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Finland / Helsinki | Same class as SP-014 / SP-041 |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log when walks run.

## Scenario catalogue

### Block A — Fire-once milestones (exit 1)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | 25 / 50 / 100 fire once per threshold | `AreaMilestone_FireOncePerThreshold` | 1 |
| A2 | Triple-cross in one update records all three | `AreaMilestone_TripleCrossOneUpdate` | 1 |
| A3 | Re-cross after drop does not re-fire | `AreaMilestone_NoRefireAfterDrop` | 1, 8 |
| A4 | Manager rebuild fires then does not re-fire | `AreaMilestoneManager_FiresOnRebuild` / `NoRefireAfterInvalidateRebuild` | 1 |
| A5 | Presentation queue 100 then 50 then 25 | `AreaMilestonePresentation_QueueOrder100Then50Then25` | 1 |
| A6 | One celebration at a time | `AreaMilestonePresentation_OneAtATime` | 1 |
| A7 | City-summary does not write area fired-state | `AreaMilestone_CitySummaryDoesNotWriteAreaFiredState` / `CitySummaryDoesNotEnqueue` | 1 |

### Block B — Non-blocking / no route interrupt (exit 2)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | Presentation does not stop recording | `AreaMilestonePresentation_FollowingDoesNotStopRoute` | 2 |
| B2 | Auto-ack exists (2 s / 3.5 s / 4 s); no modal | `MapButtonsController.applyAreaMilestonePresentation` delayMs | 2 |
| B3 | Milestone chrome never calls `nativeCloseRouting` / `nativeDisableFollowing` | Code review of `MapButtonsController` share + ack paths | 2 |
| B4 | Display names never MWM ids | `AreaMilestonePresentation_DisplayNameNeverMwmId` / `BlankDisplayNameDropped` | 1, 2 |

### Block C — First-100 m (exit 3)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | Appears on first recording start | `FirstGoal_AppearsOnFirstRecordingStart` | 3 |
| C2 | Completes at 10 newly explored live pixels | `FirstGoal_CompletesAtTenNewlyExploredLivePixels` | 3 |
| C3 | Import does not advance | `FirstGoal_ImportDoesNotAdvance` / `LiveVisitOfImportedPixelsDoesNotAdvance` | 3 |
| C4 | Pause does not increment | `FirstGoal_PauseDoesNotIncrement` | 3 |
| C5 | Incomplete survives a later session | `FirstGoal_IncompleteSurvivesSecondSession` | 3 |
| C6 | Complete does not return (once per install) | `FirstGoal_CompleteDoesNotReturn` / `PersistsAcrossNewTracker` | 3 |
| C7 | Single pulse can complete | `FirstGoal_SinglePulseCanComplete` | 3 |

### Block D — Completion card deny-list (exit 4, 5)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | Permit-list only; deny tokens absent | `CompletionCard_DenyListFieldsAbsent` | 4 |
| D2 | Rings-only geometry matches outline | `CompletionCard_RingsOnlyGeometryMatchesOutline` | 4 |
| D3 | Empty rings → no card | `CompletionCard_EmptyRingsNoCard` | 4 |
| D4 | Display name never MWM / osm id | `CompletionCard_DisplayNameNeverMwmId` | 4 |
| D5 | Compose without nickname or date | `CompletionCard_ComposeWithoutNicknameOrDate` | 5 |
| D6 | Nickname omitted when empty | `CompletionCard_NicknameOmittedWhenEmpty` | 5 |
| D7 | Competition line stub empty | `CompletionCard_CompetitionLineStubEmpty` / `AreaMilestonePresentation_CompetitionLineStubEmpty` | 5 |
| D8 | Date included only when requested | `CompletionCard_IncludeDateOnlyWhenRequested` | 4, 5 |
| D9 | Manager binds from 100% peek | `CompletionCard_ManagerBindsFromHundredPercentPeek` | 4 |

### Block E — Explicit share (exit 6)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | 100% appear does not share | `AreaMilestonePresentation_HundredPercentDoesNotShare` | 6 |
| E2 | Prepare uses transient PNG, not track | `CompletionCardShare_PrepareUsesTransientPngNotTrack` | 6 |
| E3 | Date opt-in default off | `CompletionCardShare_DateOptInDefaultOff` | 6 |
| E4 | Include date when requested | `CompletionCardShare_IncludeDateWhenRequested` | 6 |
| E5 | Share text has no coordinates | `CompletionCardShare_TextHasNoCoordinates` | 6, 9 |
| E6 | Prepare fails without 100% head | `CompletionCardShare_PrepareFailsWithoutHundredPercent` | 6 |
| E7 | Android `ACTION_SEND` image/png only | `CompletionCardShare.shareImage`; no `SharingUtils` | 6 |

### Block F — Haptics (exit 7)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Predicate all-true allows | `ExplorationHaptic_Predicate_AllTrue_Allows` | 7 |
| F2 | Not recording / background / toggle off deny | matching `Predicate_*_Denies` | 7 |
| F3 | One collection pulse for one or many new pixels | `CollectionPulse_OneOrMany_Same` + manager one-pulse tests | 7 |
| F4 | Zero new pixels / paused / background / toggle off: no collection pulse | matching `Manager_*` | 7 |
| F5 | 50 and 100 play once when allowed; 25 has no pattern | `P100ThenP50_PlayOnceEachWhenAllowed` / `P25_NoPattern` | 7 |
| F6 | Milestone and first-goal suppressed off-predicate | matching `Milestone_*` / `FirstGoalComplete_Background_NoPlay` | 7 |
| F7 | Toggle default on when key missing | `Toggle_DefaultOnWhenKeyMissing` | 7 |

### Block G — §27.4 survival (exit 8)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| G1 | OSM id stable across cache rebuild | `AreaMilestone_OsmIdStableAcrossCacheRebuild` | 8 |
| G2 | OSM id stable across compact-index change | `AreaMilestone_OsmIdStableAcrossCompactIndexChange` | 8 |
| G3 | Previously completed below 100 | `AreaMilestoneManager_PreviouslyCompletedBelow100` / `AreaMilestonePresentation_PreviouslyCompletedOnFocus` | 8 |
| G4 | Import can cross a threshold without re-fire of already-fired | `AreaMilestoneManager_ImportCanCrossThreshold` | 1, 8 |

### Block H — Growth analytics (exit 9)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| H1 | Counters default zero | `CompletionCardShare_AnalyticsDefaultZero` | 9 |
| H2 | Keys have no location / area / pixel / lat | `CompletionCardShare_AnalyticsKeysHaveNoLocationOrArea` | 9 |
| H3 | Generated increments on display get only | `CompletionCardShare_GeneratedIncrementsOnDisplayGet` | 9 |
| H4 | Share increments only on record | `CompletionCardShare_ShareIncrementsOnlyOnRecord` | 9 |
| H5 | Upload sink | None in V1 | 9 → Phase 10 |

### Block I — Manual / device (all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| I1 | Small-area 100% celebration; non-blocking | Visual | 1, 2 |
| I2 | Card image vs exclusion list | Visual PNG | 4 |
| I3 | Competition-off first-person copy | Visual | 5 |
| I4 | Share sheet only on tap | Visual chooser | 6 |
| I5 | Haptics screen-off / background / toggle off | Feel | 7 |
| I6 | Navigation not interrupted | Visual following | 2 |
| I7 | Competition-on §22.10 sentences | N/A in V1 | Phase 8 |

### Block J — Automated suites (feeds all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| J1 | `street_pixels_areas_tests --filter=AreaMilestone` | All pass; count recorded | 1, 8 |
| J2 | `street_pixels_tests --filter=AreaMilestone` | All pass | 1, 2, 6, 8 |
| J3 | `street_pixels_tests --filter=FirstGoal` | All pass | 3, 7 |
| J4 | `street_pixels_tests --filter=ExplorationHaptic` | All pass | 7 |
| J5 | `street_pixels_tests --filter=CompletionCard_` | All pass | 4, 5 |
| J6 | `street_pixels_tests --filter=CompletionCardShare` | All pass | 6, 9 |
| J7 | Full `street_pixels_areas_tests` | All pass or residual recorded | 1–9 |
| J8 | Full `street_pixels_tests` | All pass or Eligibility environment residual | 1–9 |

## Exit criteria mapping (fill in evidence log)

| # | Criterion | Evidence blocks |
| --- | --- | --- |
| 1 | Fire once; non-blocking | A, B2, J1, J2 |
| 2 | Never interrupt routing | B, I6 |
| 3 | First-100 m | C, J3 |
| 4 | Cards at 100%; deny-list | D, J5, I2 |
| 5 | No competition / nickname | D5–D7, I3, I7 |
| 6 | Explicit share | E, J6, I4 |
| 7 | Haptics predicate | F, J4, I5 |
| 8 | §27.4 survival | G, J1 |
| 9 | Count-only analytics | H, J6 |

## Non-goals for this gate

- Phase 8 competition overlays / live §22.10 sentences.
- Marking Phase 7 **Accepted** without maintainer decision.
- Weakening tests.
- Changing 4 s auto-ack duration, compositor type, or fire-once policy.
