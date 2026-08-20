# SP-069 — Evidence log (Phase 7 exit)

**Plan:** [SP-069-validation-plan.md](SP-069-validation-plan.md)
**Branch:** `cursor/sp-069-phase7-end-to-end-validation-c417`
**Status:** Evidence recorded — Phase 7 exit **awaiting maintainer decision** (agent does not mark exit Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-20 |
| Git SHA (suite run tip) | `4c67ed4c9` (SP-069 plan docs on `04e390509` Merge pull request #48 — SP-068 Accepted on `street-pixels`) |
| Build | `./tools/unix/build_omim.sh -d -p /workspace street_pixels_tests street_pixels_areas_tests` → `/workspace/omim-build-debug/` |
| `street_pixels_areas_tests --filter=AreaMilestone` | **8/8** All tests passed |
| `street_pixels_tests --filter=AreaMilestone` | **18/18** All tests passed (4 manager + 14 presentation) |
| `street_pixels_tests --filter=FirstGoal` | **11/11** All tests passed (9 `FirstGoal_*` + 2 first-goal haptic plays) |
| `street_pixels_tests --filter=ExplorationHaptic` | **21/21** All tests passed |
| `street_pixels_tests --filter=CompletionCard_` | **10/10** All tests passed |
| `street_pixels_tests --filter=CompletionCardShare` | **9/9** All tests passed |
| Full `street_pixels_areas_tests` | **113/113** All tests passed |
| Full `street_pixels_tests` | **150** OK then abort at `Eligibility_IncludesCommonHighways` (missing `data/classificator.txt`) |
| Post-Eligibility remainder | Filters `EverLive`, `ExplorationMultiplier`, `ExplorationWeight`, `ExplorerPro`, `Focus`, `FocusedArea`, `Interrupted`, `LiveSample`, `PauseResume`, `RecordingSession` each **All tests passed**. `ExplorationHaptic` and `FirstGoal` already green above. Eligibility not re-run (environment). |
| Smoke / APK | Not run (agent desktop suites only) |
| Device walks | Deferred → Phase 10 |

### Suite command transcripts (counts)

```text
$ git rev-parse HEAD
4c67ed4c9f6b2be31366d366d29893395e716c6f

$ ./omim-build-debug/street_pixels_areas_tests --filter=AreaMilestone
# grep -c '^OK$' → 8
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=AreaMilestone
# grep -c '^OK$' → 18
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=FirstGoal
# grep -c '^OK$' → 11
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=ExplorationHaptic
# grep -c '^OK$' → 21
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=CompletionCard_
# grep -c '^OK$' → 10
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=CompletionCardShare
# grep -c '^OK$' → 9
All tests passed.

$ ./omim-build-debug/street_pixels_areas_tests
# grep -c '^OK$' → 113
All tests passed.

$ ./omim-build-debug/street_pixels_tests
# grep -c '^OK$' → 150 then abort
Running eligibility_tests.cpp::Eligibility_IncludesCommonHighways
FAILED <<<Exception thrown [ FileAbsentException ... "File classificator.txt doesn't exist ..." ]
```

Logs: `/opt/cursor/artifacts/sp069_suite_summary.log`, `sp069_areas_full.log`, `sp069_map_full.log`, `sp069_post_eligibility_summary.log`.

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |
| D3 | optional | — | — | Deferred Phase 10 |

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1 Fire once per threshold | agent | **Pass** | `AreaMilestone_FireOncePerThreshold` |
| A2 Triple-cross one update | agent | **Pass** | `AreaMilestone_TripleCrossOneUpdate` |
| A3 No re-fire after drop | agent | **Pass** | `AreaMilestone_NoRefireAfterDrop` |
| A4 Manager rebuild / no re-fire | agent | **Pass** | `AreaMilestoneManager_FiresOnRebuild` / `NoRefireAfterInvalidateRebuild` |
| A5 Queue 100 then 50 then 25 | agent | **Pass** | `AreaMilestonePresentation_QueueOrder100Then50Then25` |
| A6 One at a time | agent | **Pass** | `AreaMilestonePresentation_OneAtATime` |
| A7 City-summary does not fire area milestones | agent | **Pass** | store + presentation city-summary tests |
| B1 Recording not stopped | agent | **Pass** | `AreaMilestonePresentation_FollowingDoesNotStopRoute` |
| B2 Auto-ack delays; no modal | agent | **Pass** (code) | 2000 / 3500 / 4000 ms `postDelayed`; toast + card, not a dialog |
| B3 Never closes routing | agent | **Pass** (code) | `MapButtonsController` has no `nativeCloseRouting` / `nativeDisableFollowing` |
| B4 DisplayName never MWM id | agent | **Pass** | `DisplayNameNeverMwmId` / `BlankDisplayNameDropped` |
| C1–C7 First-100 m | agent | **Pass** | all `FirstGoal_*` |
| D1 Deny-list | agent | **Pass** | `CompletionCard_DenyListFieldsAbsent` |
| D2 Rings-only geometry | agent | **Pass** | `CompletionCard_RingsOnlyGeometryMatchesOutline` |
| D3 Empty rings no card | agent | **Pass** | `CompletionCard_EmptyRingsNoCard` |
| D4 Display name never ids | agent | **Pass** | `CompletionCard_DisplayNameNeverMwmId` |
| D5–D7 No nickname / competition stub | agent | **Pass** | compose + stub tests |
| D8 Date only when requested | agent | **Pass** | `CompletionCard_IncludeDateOnlyWhenRequested` |
| D9 Bind from 100% peek | agent | **Pass** | `CompletionCard_ManagerBindsFromHundredPercentPeek` |
| E1 100% appear does not share | agent | **Pass** | `AreaMilestonePresentation_HundredPercentDoesNotShare` |
| E2 Transient PNG not track | agent | **Pass** | `CompletionCardShare_PrepareUsesTransientPngNotTrack` |
| E3 Date default off | agent | **Pass** | `CompletionCardShare_DateOptInDefaultOff` |
| E4 Include date when requested | agent | **Pass** | `CompletionCardShare_IncludeDateWhenRequested` |
| E5 Share text no coordinates | agent | **Pass** | `CompletionCardShare_TextHasNoCoordinates` |
| E6 Prepare fails without 100% | agent | **Pass** | `CompletionCardShare_PrepareFailsWithoutHundredPercent` |
| E7 ACTION_SEND image/png | agent | **Pass** (code) | `CompletionCardShare.shareImage`; card path does not call `SharingUtils` |
| F1–F7 Haptics predicate | agent | **Pass** | `ExplorationHaptic_*` **21/21** |
| G1–G2 OSM id stable | agent | **Pass** | cache rebuild + compact-index change |
| G3 Previously completed below 100 | agent | **Pass** | manager + presentation focus flag |
| G4 Import can cross remaining threshold | agent | **Pass** | `AreaMilestoneManager_ImportCanCrossThreshold` |
| H1–H4 Count-only analytics | agent | **Pass** | share analytics 9/9 including key denylist |
| H5 Upload sink | — | **Residual** | SPD-055 Phase 10 |
| I1–I6 Device manual | D1 | **Residual** | Phase 10 — no handset in this environment |
| I7 Competition-on §22.10 | — | **Residual** | Phase 8 |
| J1–J6 Phase 7 filters | agent | **Pass** | 8+18+11+21+10+9 |
| J7 Full areas suite | agent | **Pass** | **113/113** |
| J8 Full map suite | agent | **Residual** | Eligibility abort; remainder filters green |

## Phase 7 exit criteria table

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Milestones fire once per area per threshold and are non-blocking | **Pass** (automated) + **Residual** (device celebration) | A1–A7; B2; J1; J2; I1 → Phase 10 |
| 2 | Milestones never interrupt active routing or demand immediate interaction | **Pass** (automated + code review) + **Residual** (device following) | B1–B4; I6 → Phase 10 |
| 3 | The first-100-metres goal appears and completes as specified | **Pass** (automated) + **Residual** (device chip) | C1–C7; J3 |
| 4 | Completion cards generate at 100% and contain no excluded content | **Pass** (automated deny-list) + **Residual** (device PNG eyeball) | D1–D4, D8–D9; J5; I2 → Phase 10 |
| 5 | Cards work with no competition profile and no nickname | **Pass** (automated stub) + **Residual** (device copy; live §22.10 → Phase 8) | D5–D7; I3; I7 |
| 6 | The share action is explicit; the system share sheet does not auto-open | **Pass** (automated + code review) + **Residual** (chooser eyeball) | E1–E7; J6; I4 → Phase 10 |
| 7 | Haptics respect recording, foreground, and the single toggle | **Pass** (automated) + **Residual** (device feel) | F1–F7; J4; I5 → Phase 10 |
| 8 | Milestone state survives map updates per spec §27.4 | **Pass** (automated) + **Residual** (device rematch walk) | G1–G4; J1 |
| 9 | Growth analytics record generation and share with no location data | **Pass** (local uint64) + **Residual** (upload) | H1–H4; J6; H5 → Phase 10 / SPD-055 |

## Residuals → Phase 10 / Phase 8 / polish

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | Device Phase 7 walks (100% celebration, card vs exclusion list, first-person copy, share sheet on tap, haptics screen-off/background/toggle, navigation not interrupted) | Phase 10 |
| R2 | Competition-on leading / not-leading copy (§22.10) | Phase 8 |
| R3 | Growth-counter upload | Phase 10 / SPD-055 |
| R4 | 4 s auto-ack can delete the transient PNG while a share target still reads it | Phase 10 (SP-068 left duration unchanged) |
| R5 | `onResume` rebind can increment generated and reset the date checkbox | Phase 10 / polish |
| R6 | Full `street_pixels_tests` aborts at Eligibility (`classificator.txt` absent in this environment) | Environment residual; not a Phase 7 product defect; do not weaken Eligibility. Post-Eligibility remainder filters passed. |
| R7 | Shared PNG is outline-only; date/name live in `EXTRA_TEXT` | Phase 10 eyeball; SPD-046 geometry is the lock |
| R8 | ClipData URI grant only on API ≤ 22 (same as `SharingUtils`) | Phase 10 OEM share-target check |

## Phase 7 exit recommendation (agent)

Automated Blocks A–H and J1–J7 are green on SHA `4c67ed4c9`. Exit criteria 1–9 are **Pass** on shared-core / Android-signal coverage with honest **device residuals** (R1), **Phase 8** competition copy (R2), and **Phase 10** upload (R3). J8 is an environment residual (R6), not a Phase 7 product fail.

**Maintainer decides** whether Phase 7 exit is Met with residuals, or blocked pending device walks. Agent does **not** mark Phase 7 Accepted.
