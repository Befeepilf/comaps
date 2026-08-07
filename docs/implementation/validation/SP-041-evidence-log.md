# SP-041 — Evidence log (Phase 5 exit)

**Plan:** [SP-041-validation-plan.md](SP-041-validation-plan.md)
**Branch:** `cursor/sp-041-phase5-validation-191e`
**Status:** Evidence recorded — Phase 5 exit **awaiting maintainer decision** (agent does not mark exit Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-07 |
| Git SHA (suite run tip) | `5b27b4227a07387299b7cd2a1020393614c09883` (`Merge pull request #20` — SP-040 Accepted on `street-pixels`) |
| Build | Existing `omim-build-debug` binaries matching SP-040 tip (`ninja street_pixels_areas_tests street_pixels_tests` previously green; re-run suites without code delta) |
| `street_pixels_areas_tests` | **67/67** All tests passed (`grep -c '^OK$'` → 67) |
| `street_pixels_tests` | **205/205** All tests passed (`grep -c '^OK$'` → 205) |
| `--filter=Focus` | **14/14** All tests passed |
| `--filter=AreaCompletion` | **5/5** All tests passed |
| `--filter=FocusedArea` | **6/6** All tests passed |
| `--filter=City` | **3/3** All tests passed |
| Smoke / APK | Not run (agent desktop suites only) |
| Device walks | Deferred → Phase 10 |

### Suite command transcripts (counts)

```text
$ git rev-parse HEAD
5b27b4227a07387299b7cd2a1020393614c09883

$ ./omim-build-debug/street_pixels_areas_tests
… (67 × OK) …
All tests passed.
# grep -c '^OK$' → 67

$ ./omim-build-debug/street_pixels_tests
… (205 × OK) …
All tests passed.
# grep -c '^OK$' → 205

$ ./omim-build-debug/street_pixels_tests --filter=Focus
… (14 × OK) …
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=AreaCompletion
… (5 × OK) …
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=FocusedArea
… (6 × OK) …
All tests passed.

$ ./omim-build-debug/street_pixels_tests --filter=City
… (3 × OK) …
All tests passed.
```

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1 Badge name + fraction | agent | **Pass** | `FocusedAreaBadge_SetFocusShowsNameAndFraction` (100% → `m_areaCompleted`) |
| A2 Blank name clears | agent | **Pass** | Clears + `m_noExplorationArea`; name ≠ leaf |
| A3 Focus change snapshot | agent | **Pass** | District → City fraction 0.0 |
| A4 Invalid cache | agent | **Pass** | Name kept; fraction invalid |
| B1–B6 §12.5 rules | agent | **Pass** | Areas focus_selection_engine_tests (7) + manager Focus* |
| B7 Sticky explicit | agent | **Pass** | `ExplicitStickyIgnoresIdlePanRefresh` |
| C1 Tap subdivision | agent | **Pass** | SelectAtPoint / Lookup |
| C2 Outside clears | agent | **Pass** | OutsideClears + no-area |
| C3 Polygon PIP | agent | **Pass** | LookupExplorationAreaAtPoint_* |
| D1 Area completion | agent | **Pass** | AreaCompletion cache + manager 5/5 |
| D2 City rollup 1/2 | agent | **Pass** | MultiAreaSums + CitySummaryUsesRollupFraction 0.5 |
| D3 Settlement-only | agent | **Pass** | CityCompletion_SettlementOnlyMatchesAreaRow |
| D4 Fail-closed no pix | agent | **Pass** | CitySummaryFailClosedWithoutPix |
| E1–E2 Completed style | agent | **Pass** | StyleCompletedDistinctFromInProgress (street outline-only) |
| E3 areaCompleted flag | agent | **Pass** | 100% badge fixture |
| E4–E5 No-area | agent | **Pass** | NoAreaSignalNeverUsesMwmId; NoAreaYieldsNone |
| F1 SP-033 qualitative | cite | **Pass (Partial)** | Pixel 3a qualitative OK (SP-033 Accepted) |
| F2 Overlay LOD keep circles | agent | **Pass** | AreaOverlay_* 5; one-circle-per-cell retained |
| F3 Spike 1 quantitative | — | **Residual** | Phase 10 (already carried) |
| F4 Device chrome re-spot | D1 | **Residual** | Phase 10 |
| G1 No country/world city | agent | **Pass** | CityCompletion_NoCountryWorldAggregate |
| G2 No country choropleth | agent | **Pass** | AreaOverlay_NoCountryChoropleth |
| G3 Badge binds area/city only | agent | **Pass** | FocusedAreaProgress; no country/world UI strings added in Phase 5 |
| H1–H6 Device manual | D1 | **Residual** | Phase 10 — boundary/tap/zoom/completed/empty/no-country UI |
| I1 areas suite | agent | **Pass** | **67/67** |
| I2 map suite | agent | **Pass** | **205/205** |
| I3–I6 filters | agent | **Pass** | Focus 14; AreaCompletion 5; FocusedArea 6; City 3 |

## Phase 5 exit criteria table

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Primary badge shows focused area name and correct % | **Pass** (automated) + **Residual** (device UI) | A1–A4; I5; H1 → Phase 10 |
| 2 | Focus behaviour matches all five §12.5 rules | **Pass** (automated) + **Residual** (device) | B1–B7; I3; H1 → Phase 10 |
| 3 | Tapping an area focuses it and reveals exact % | **Pass** (automated) + **Residual** (device) | C1–C3; I3; H2 → Phase 10 |
| 4 | Area and city completion correct for installed map version | **Pass** (automated) + **Residual** (device city zoom) | D1–D4; I4/I6; H3 → Phase 10 |
| 5 | Completed areas have distinct visual that survives zoom | **Pass** (style/signals automated) + **Residual** (device chrome; check glyph polish) | E1–E3; F2; H4 → Phase 10; `m_showCheck` not drawn yet |
| 6 | No-area state implemented and tested | **Pass** (automated) + **Residual** (device empty copy) | E4–E5; A2; H5 → Phase 10 |
| 7 | Rendering meets Spike 1 on mid-tier **or** LOD + re-measure | **Pass (Partial)** + **Residual** | F1 Partial SP-033; F2 LOD keep circles; F3 quantitative → Phase 10 |
| 8 | No country or world percentage calculated or displayed | **Pass** (automated) + **Residual** (device UI confirm) | G1–G3; H6 → Phase 10 |

**Note:** `GetTotalExploredFraction` remains MWM-scoped explore-stats helper (pre-area). It is **not** a country/world Phase 5 progress UI. ExploreStats weekly region keys are competition/stats (Phase 8), not neighbourhood %.

## Residuals → Phase 10 / polish

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | Device Helsinki Phase 5 walks (badge, focus, tap, city zoom, completed, empty, no country/world UI) | Phase 10 |
| R2 | Quantitative Spike 1 FPS/memory | Phase 10 (already listed; SP-033) |
| R3 | Completed check glyph not drawn in drape (`m_showCheck` reserved) | Polish / Phase 10 or small follow-up |
| R4 | Overlay push still bakes Neighbourhood band colors (SP-037 stub) | Optional retune; not exit-blocking |
| R5 | Completion-date persistence | Phase 7 (SP-040 explicit) |

## Phase 5 exit recommendation (agent)

Automated Blocks A–G and I are green on SHA `5b27b4227`. Exit criteria 1–6 and 8
are **Pass** on shared-core/Android-signal coverage with honest **device
residuals**. Exit 7 remains **Partial** (qualitative OK; quantitative Phase 10).

**Maintainer decides** whether Phase 5 exit is Met with residuals, or blocked
pending device walks. Agent does **not** mark Phase 5 Accepted.
