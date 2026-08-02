# SP-014 — Evidence log

**Plan:** [SP-014-validation-plan.md](SP-014-validation-plan.md)
**Branch:** `street-pixels`
**Status:** In progress

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-02 |
| Git SHA (plan baseline) | `a4b17ef88a` (HEAD at automated run; plan commit follows) |
| `street_pixels_tests` | **109/109** All tests passed |
| `gps_track_*` (via `map_tests` filter GpsTrack) | `GpsTrackCollection_Simple`, `GpsTrackStorage_WriteRead`, `GpsTrack_Simple` — All tests passed |
| Smoke suite | Not completed in this environment (`run_tests.sh -s smoke` aborted: `Can't find test base_tests`). Re-run on a full smoke-capable build before Phase 2 exit. |

## Device roster

| Slot | Model | OS | Battery exemption | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | | | |
| D2 | TBD aggressive OEM | | | |

## Scenario results

One row per (scenario × device). Leave blank until walked.

| Scenario | Device | Date | Route / notes | Expected samples | Received | Result | Screenshots | Defect / WI |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| A1 | D1 Pixel 3a | | | n/a | n/a | | | |
| A2 | D1 Pixel 3a | | | | | | | |
| A3 | D1 Pixel 3a | | | | | | | |
| A4 | D1 Pixel 3a | | | | | | | |
| A5 | D1 Pixel 3a | | | | | | | |
| A6 | D1 Pixel 3a | | | | | | | |
| A7 | D1 Pixel 3a | | | | | | | |
| B4 | D1 Pixel 3a | | | ~1800 / 30 min | | | | |
| B5 | D1 Pixel 3a | | | ~900 / 15 min | | | | |
| B12 | D1 Pixel 3a | | | n/a | battery % | | | |
| B13 | D1 Pixel 3a | | | | | | | |
| C3 | D1 Pixel 3a | | | | | | | |
| D-open | D1 Pixel 3a | | | | | | | |
| D-canyon | D1 Pixel 3a | | | | | | | |
| D6 | D1 Pixel 3a | | | | | | **required** | |
| D7 | D1 Pixel 3a | | | | | | | |
| D8 | D1 Pixel 3a | | | | | | | |
| E9 | D1 Pixel 3a | | | | | | | |
| E-reboot | D1 Pixel 3a | | | | | | | |
| E-air | D1 Pixel 3a | | | | | | | |
| F-radius | D1 Pixel 3a | | | | | | | |
| F-poor | D1 Pixel 3a | | | | | | | |
| *(repeat critical rows for D2 when available)* | | | | | | | | |

## ABL recommendation (after B4/B5)

| Field | Value |
| --- | --- |
| Continuity on D1 without ABL | |
| Continuity on D2 without ABL | |
| Recommendation | |
| Phase 10 follow-up? | |

## Phase 2 exit status

| Exit # | Status | Evidence pointers |
| --- | --- | --- |
| 1 | | |
| 2 | | |
| 3 | | |
| 4 | | |
| 5 | | |
| 6 | | |
| 7 | Blocked — D2 aggressive OEM not named; B4 not run | Plan D1 = Pixel 3a only so far |
| 8 | Pending smoke re-run; `gps_track_*` green | GpsTrack_* OK; smoke blocked on missing `base_tests` binary locally |

## Defects found

| ID | Summary | Owning WI | Repro |
| --- | --- | --- | --- |
| SP014-1 | Top-right recording status FAB did nothing useful / did not open stop dialog while session active (menu path worked). Wired FAB to `showTrackSaveDialog`; pause/resume remains on notification. | SP-012 | Pixel 3a; start recording; tap top-right status FAB |
