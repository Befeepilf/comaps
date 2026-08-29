# SP-094 — Evidence log

**Plan:** [SP-094-validation-plan.md](SP-094-validation-plan.md)
**Branch:** `cursor/sp-094-battery-protocol-6383`
**Status:** Protocol recorded 2026-08-29. **Every device scenario Residual
  (not executed).** Agent does **not** mark Accepted.

This Phase 10 coding slice records the H2 / Spike 1 / lifecycle
**protocol** only (product-owner lock 2026-08-29). No handset run. Do
not read empty cells as zero. Do not treat unit-test pointers as
device Pass.

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date (protocol recorded) | 2026-08-29 |
| Parent SHA (`street-pixels`) | `33ffff855c0dd90e7c7e06b6edd197d2c49a73e7` (`Merge branch 'cursor/sp-093-privacy-residual-6383'`) |
| Walk APK SHA | *(empty — not executed)* |
| `versionName` / `versionCode` | |
| Build type | Prefer release/beta when later executed. **Not built or installed in this slice.** |
| Flavor | |
| `adb` / handset | **Not used.** This slice must not run Spike 1, battery protocol, or lifecycle walks on a device. |
| `street_pixels_tests` (this slice) | **Not run.** Optional in the executing WI. Missing `data/classificator.txt` is an environment residual, not a product Fail. |
| Spike 1 FPS / memory | **Not measured.** Empty number cells below. |
| Battery %/hour / mAh | **Not measured.** No ceiling invented (**SPD-078**). |
| Cold start | **Not measured.** |

### Automated pointers (not a device substitute)

Cited from the tree at parent `33ffff855`. Names only. **Not re-run
in this slice.** Green unit tests, if later run, still do not close
Block R, Bat, CS, or L device rows.

| Area | File | Example names |
| --- | --- | --- |
| Rematch / §27.3 fraction | `libs/map/street_pixels_tests/rematch_tests.cpp` | `Rematch_UnchangedRemovedAddedMatrix`, `Rematch_EverLivePersistsForSurvivors`, `Rematch_InterruptBeforeRenameKeepsOld`, `Rematch_DenominatorGrowsFractionDrops`, `Rematch_PreviousVsNewFractionSignal` |
| Interruption / no gap fill | `libs/map/street_pixels_tests/interrupted_session_tests.cpp` | `InterruptedSession_PixelsBeforeInterruptionIntact`, `InterruptedSession_AfterEffects_NoInterpolate_DiscCollected`, `InterruptedSession_GapThreshold_Boundary` |
| Restart breadcrumb | `libs/map/street_pixels_tests/recording_session_tests.cpp` | `RecordingSession_BreadcrumbPersistenceAcrossRestart` |
| Delete competition profile | `libs/map/street_pixels_tests/competition_deletion_tests.cpp` | `CompetitionDeletion_SuccessClearsRecencyKeepsPix` |
| Week boundary / device TZ ignored | `libs/street_pixels_areas/street_pixels_areas_tests/weekly_city_week_tests.cpp`, `weekly_city_live_store_tests.cpp` | `WeeklyCityWeek_DeviceTzIgnored`, `WeeklyCityWeek_UtcMondayBoundary`, `WeeklyCityLive_MondayBoundarySeparateWeeks`, `WeeklyCityLive_TzChangesWeekIdVsUtc` |

`WeekBoundsFromUnix` currently ignores the IANA argument and always
sets UTC fallback. That is a code vs **SPD-060** gap for the later
L6 walk; it is not a device result. See the work-item discovered
follow-up.

## Device roster

| Slot | Model | OS / skin | Saver / exemptions | GPU / driver | Walker |
| --- | --- | --- | --- | --- | --- |
| D1 Pixel-class | | | | | |
| D2 aggressive OEM | | | | | |
| D3 optional | | | | | |

**SPD-077** locks D1 + D2. Slots are unnamed here because no handset
run exists. Do not copy Pixel 3a / Pixel 10a ids from SP-014 / SP-041
into this log as if they executed SP-094.

## Block R — Spike 1

| Scenario | Device | p95 FPS | Memory uplift (MB) | GPU / driver | Result | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| R1 pan/zoom z14–16 overlay on | D1 | | | | **Residual** | City loaded (Helsinki / Uusimaa-class). `.spa` required; if missing at execution → Blocked, do not fake FPS. |
| R2 overlay memory uplift | D1 | — | | | **Residual** | Bar &lt;150 MB. |
| R1 pan/zoom z14–16 overlay on | D2 | | | | **Residual** | Required by **SPD-077**. |
| R2 overlay memory uplift | D2 | — | | | **Residual** | |
| R3 fail-vs-bar disposition | — | — | — | — | **Residual** | If later Fail: report; do not add LOD here. |

SP-033 qualitative Pixel 3a (2026-08-07) is not copied here.

## Block Bat — H2 battery protocol

No %/hour ceiling. Maintainer accept/waive cells stay empty until
numbers exist.

| Scenario | Device | Duration | Start % | End % | %/hour | mAh (if any) | FGS survived | Pixels continued | Result | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Bat-A recording screen-off ≥2 h | D1 | | | | | | | | **Residual** | Same saver settings as Bat-B. |
| Bat-B control recording-off screen-off ≥2 h | D1 | | | | | | — | — | **Residual** | App installed; no navigation. |
| Bat-cmp A vs B | D1 | — | — | — | | | — | — | **Residual** | Delta only; no pass bar. |
| Bat-A recording screen-off ≥2 h | D2 | | | | | | | | **Residual** | |
| Bat-B control recording-off screen-off ≥2 h | D2 | | | | | | — | — | **Residual** | |
| Bat-cmp A vs B | D2 | — | — | — | | | — | — | **Residual** | |
| Bat-FGS (from Bat-A) | D1 | — | — | — | — | — | | — | **Residual** | Not SP-095 OEM-kill script. |
| Bat-FGS (from Bat-A) | D2 | — | — | — | — | — | | — | **Residual** | |
| Bat-pix (from Bat-A) | D1 | — | — | — | — | — | — | | **Residual** | |
| Bat-pix (from Bat-A) | D2 | — | — | — | — | — | — | | **Residual** | |

Maintainer accept / waive (battery): *(empty — numbers do not exist)*

## Block CS — Cold start

| Scenario | Device | Time to first interactive frame | Result | Notes |
| --- | --- | --- | --- | --- |
| CS1 large city cold start | D1 | | **Residual** | Recorded, not gated, unless a later SPD adds a number. |
| CS1 large city cold start | D2 | | **Residual** | |

## Block L — Lifecycle / data-loss

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| L1 Upgrade with existing `.pix` | D1 | **Residual** | |
| L1 Upgrade with existing `.pix` | D2 | **Residual** | |
| L2 Map update rematch + §27.3 message | D1 | **Residual** | Expect `street_pixels_more_to_explore` on fraction drop. Rematch duration empty. Pointer `Rematch_*` only. |
| L2 Map update rematch + §27.3 message | D2 | **Residual** | |
| L3 Force stop during recording | D1 | **Residual** | No gap fill. Pointer `InterruptedSession_*` only. |
| L3 Force stop during recording | D2 | **Residual** | |
| L4 Low-memory kill | D1 | **Residual** | |
| L4 Low-memory kill | D2 | **Residual** | |
| L5 Device restart, active session | D1 | **Residual** | |
| L5 Device restart, active session | D2 | **Residual** | |
| L6 Time-zone change / weekly boundary SPD-060 | D1 | **Residual** | Device TZ must not move the week. Pointer `WeeklyCityWeek_DeviceTzIgnored` only. |
| L6 Time-zone change / weekly boundary SPD-060 | D2 | **Residual** | |
| L7 Storage nearly full during derive/migration | D1 | **Residual** | |
| L7 Storage nearly full during derive/migration | D2 | **Residual** | |
| L8 Delete competition profile, local exploration intact | D1 | **Residual** | Pointer `CompetitionDeletion_SuccessClearsRecencyKeepsPix` only. |
| L8 Delete competition profile, local exploration intact | D2 | **Residual** | |
| L9 Clear app data vs policy claims | D1 | **Residual** | Coupled to SP-093 policy-landing residual. |
| L9 Clear app data vs policy claims | D2 | **Residual** | |

## Phase 10 exit mapping (not closed)

| Exit # | Criterion | Result in this slice | Evidence |
| --- | --- | --- | --- |
| 6 | Battery measured and accepted | **Residual** | Protocol in the plan; Bat-* rows empty |
| 7 | Rendering meets Spike 1 | **Residual** | R1/R2 empty |
| 8 | No critical exploration-data-loss path | **Residual** | L1–L9 Residual |

Do **not** mark Phase 10 exit met.

## Defects found this slice

None from device execution (none attempted). Code vs **SPD-060** IANA
stub is recorded on the work item as follow-up, not as a measured L6
Fail.
