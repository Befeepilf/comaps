# SP-022 — Evidence log

**Plan:** [SP-022-validation-plan.md](SP-022-validation-plan.md)
**Branch:** `street-pixels`
**Status:** Complete (device-walk residual → Phase 10)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date (plan baseline) | 2026-08-03 |
| Git SHA (plan baseline tip) | `949e04621e` (includes SP-021 accept) |
| Git SHA (walk APK) | *(executor fills)* |
| `versionName` / `versionCode` | *(executor fills)* |
| Build notes | Plan-time suite run used `../omim-build-debug/street_pixels_tests` on tip `949e04621e`. **Re-run full suite on the exact walk APK SHA** before Phase 3 exit. |
| `street_pixels_tests` (plan re-verify) | **171/171** All tests passed (2026-08-03) |
| `street_pixels_tests` (walk SHA) | *(executor fills)* |
| `--filter=Rematch` | *(executor fills)* |
| `--filter=Archive` | *(executor fills)* |
| Known flake | `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` — pre-existing intermittent; not Phase 3. Re-run once if sole failure. |
| Smoke suite | *(optional; Phase 10 if skipped)* |

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill)* | Prefer Uusimaa | *(fill)* |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |

## Uusimaa / large-region measurements

| Slot | Metric | Value | Notes |
| --- | --- | --- | --- |
| S1 | `.pix` size | | ~50 MB expected |
| S2 | Explored fraction / count | | |
| S3 | Rematch wall time | | |
| S4 | UI during rematch | | usable / jank / freeze |
| S5 | Memory / OOM | | |
| S6 | `.pixr` size after delete | | must be ≪ `.pix` |
| S7 | `.pix` size after redownload | | |
| S8 | SPD-019 densify check | | still ~50 MB class |

## Scenario results

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| A1 Live ever-live survives reopen | D1 | | |
| A2 Import-only then live upgrade | D1 | | |
| A3 Header + map-data version | D1 | | |
| A4 Legacy `.pix` migrate (if available) | D1 | | skip + reason if no legacy fixture |
| B1 Update rematch greens retained | D1 | | |
| B2 Update while viewing map | D1 | | |
| B3 §27.3 toast on fraction drop | D1 | | expect `street_pixels_more_to_explore` |
| B4 No false toast when no drop | D1 | | |
| B-time Uusimaa rematch timing | D1 | | see S3–S5 |
| C1 Force-stop mid-migration | D1 | | |
| C2 Kill mid large derive (optional) | D1 | | |
| D1 Delete → `.pix` gone, `.pixr` kept | D1 | | |
| D2 Redownload rematch from `.pixr` | D1 | | |
| E1 15 m unify / no densify | D1 + auto | | |
| E2 Eligibility spot-check vs SP-020 | D1 | | |
| F1 Full `street_pixels_tests` | agent | **171/171** plan baseline | re-run on walk SHA |
| F2 Determinism / repeat-derive | agent | | covered by suite |
| F3 Rematch/Archive/Eligib subsets | agent | | optional after F1 |

## Phase 3 exit criteria

| Exit # | Criterion | Pass / fail / residual | Evidence pointers |
| --- | --- | --- | --- |
| 1 | Ever-live vs imported-only recorded per explored pixel | | |
| 2 | Format version + map-data version on pixel files | | SP-015 Accepted? |
| 3 | Update rematches; surviving cells keep explored state | | |
| 4 | Migration crash-safe (interrupted) | | |
| 5 | Sampling unified at 15 m (SPD-019); no densify | | |
| 6 | Eligibility matches §13 or divergences recorded | | SP-020 register |
| 7 | Denominators recalculate; §27.3 messaging | | |
| 8 | Determinism via repeat-derivation test | | |
| — | Delete → redownload via `.pixr` (SPD-016) | | required phase manual |

## Residuals (Phase 10 or owning SP)

| ID | Summary | Disposition |
| --- | --- | --- |
| | | |

## Defects found

| ID | Summary | Owning WI | Disposition |
| --- | --- | --- | --- |
| | | | |

## Maintainer exit decision

| Field | Value |
| --- | --- |
| Phase 3 exit | *(Accepted / Accepted with residuals / Blocked)* |
| Accepted by | |
| Accepted date | |
| Notes | |
