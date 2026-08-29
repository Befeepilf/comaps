# SP-014 — Evidence log

**Plan:** [SP-014-validation-plan.md](SP-014-validation-plan.md)
**Branch:** `street-pixels`
**Status:** Accepted

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-02 |
| Git SHA (plan baseline) | `a4b17ef88a` |
| Git SHA (status FAB fix) | `aa1fc0fe9e` |
| `street_pixels_tests` | **109/109** All tests passed |
| `gps_track_*` (via `map_tests` filter GpsTrack) | `GpsTrackCollection_Simple`, `GpsTrackStorage_WriteRead`, `GpsTrack_Simple` — All tests passed |
| Smoke suite | Not completed in agent environment (`base_tests` missing). Maintainer accepted SP-014 on device evidence; re-run smoke before public release (Phase 10). |

## Device roster

| Slot | Model | OS | Battery exemption | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(device OS as tested)* | *(as tested)* | Maintainer |
| D2 | Aggressive OEM | — | — | **Deferred** to Phase 10 release hardening |

## Scenario results (D1 Pixel 3a)

Maintainer attestation 2026-08-03: **all planned checks passed on Pixel 3a**, except the status-FAB stop dialog defect (SP014-1) found and fixed before accept.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| A1–A7, B4–B5, B12–B13, C3, D-*, E-*, F-* | D1 Pixel 3a | **Pass** (maintainer) | Full catalogue per validation plan |
| Status FAB stop dialog | D1 Pixel 3a | Fail → **fixed** | SP014-1 → `aa1fc0fe9e` |
| D2 aggressive OEM matrix | — | Not run | Follow-up Phase 10 / exit criterion 7 residual |

## ABL recommendation (after Pixel 3a Block B)

| Field | Value |
| --- | --- |
| Continuity on D1 without ABL | Pass (maintainer; screen-off / background checks included in “all checks pass”) |
| Continuity on D2 without ABL | Not measured |
| Recommendation | Keep `ACCESS_BACKGROUND_LOCATION` **absent** pending aggressive-OEM measurement in Phase 10 |
| Phase 10 follow-up? | Yes — OEM screen-off continuity (SP-095 B4 / B-OEM on D2). **SPD-082** (2026-08-29) superseded “final ABL decision in Phase 10”: keep ABL absent; a D2 Fail needs a **new SPD**, not adding the permission. |

## Phase 2 exit status

| Exit # | Status | Evidence pointers |
| --- | --- | --- |
| 1 | Met | SP-007 tests + Pixel 3a A1/A2 |
| 2 | Met | SP-006–012 + Pixel 3a A6; SP014-1 FAB fix |
| 3 | Met | SP-009 + Pixel 3a Block D |
| 4 | Met | SP-011 + Pixel 3a C3/D6/D7 |
| 5 | Met | SP-013 + Pixel 3a Block E |
| 6 | Met | SP-008 + Pixel 3a F-radius |
| 7 | **Partial** | Pixel 3a background/screen-off met; aggressive OEM **not** run — deferred to Phase 10 |
| 8 | Met (gps_track); smoke deferred | GpsTrack_* OK; smoke → Phase 10 |

## Defects found

| ID | Summary | Owning WI | Disposition |
| --- | --- | --- | --- |
| SP014-1 | Top-right recording status FAB did not open stop dialog | SP-012 | Fixed in `aa1fc0fe9e`; pause/resume remains on notification |
