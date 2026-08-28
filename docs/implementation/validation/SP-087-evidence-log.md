# SP-087 — Evidence log (Phase 9 exit)

**Plan:** [SP-087-validation-plan.md](SP-087-validation-plan.md)
**Branch:** `cursor/sp-087-phase9-validation-db9d`
**Status:** Template — counts filled after suite runs. Phase 9 exit
**awaiting maintainer** (agent does not mark exit Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | *(fill after suite run)* |
| Client git SHA (suite run tip) | *(fill `git rev-parse HEAD` after plan commit)* |
| Parent SHA | `d0d815832` (`Merge branch 'cursor/sp-086-pro-monetisation-analytics-db9d'`) or later plan-commit parent |
| Build | *(exact `build_omim.sh` line + binary mtimes)* |
| `data/classificator.txt` at run time | *(ls -la or MISSING)* |
| `adb devices` | *(verbatim)* |
| `HistoricalImport` | *(OK/Running)* |
| `IsolationHistoricalImport` | |
| `ExplorerPro_` | |
| `GpxGate` | |
| `ExplorerProAnalytics` | |
| `EverLive` | |
| Phase 8 isolation regressions (B9) | |
| `kml_tests --filter='Gpx' --suppress='Gpx_ImportExport'` | |
| `kml_tests --filter='Gpx_ImportExport'` | expect residual |
| `coding_tests --filter='XmlParser_'` | |
| `street_pixels_tests --suppress=Eligibility` | do not treat as full suite |
| JVM ExplorerProGateTest | |
| JVM ExplorerProAnalyticsTest | |
| JVM GpxSettingsVisibilityTest | |
| JVM BookmarkManagerGpxGateTest | residual JNI if UnsatisfiedLinkError |
| Device walks / APK dump | Deferred → Phase 10 |

### Suite command transcripts (counts)

*(Paste executed transcripts after rebuild. `grep -c '^OK$'` must match
`grep -c '^Running '`.)*

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 (`adb` absent unless filled) |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |
| D3 | optional | — | — | Deferred Phase 10 |

Map screenshots remain **forbidden**. No walks, packet captures, APK
dumps, or fabricated `nm` output until filled from execution.

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1–A6 Import marks imported + personal completion | | | |
| A7 / M1 Device multi-hour GPX | — | Residual | Phase 10 |
| B1–B9 Isolation regardless of gate | | | |
| B10 / M2–M3 Device competition chrome | — | Residual | Phase 10 |
| C1–C9 Pro gate automated + JVM | | | |
| C10 / M5 Internal Pro device | — | Residual | Phase 10 |
| D1–D7 Public-configured code review + automated | | | |
| D8 Debug-entitle symbols in native | — | Residual | SP-083 / Phase 10 `nm` |
| D9 / M4 Public APK dump / share-sheet | — | Residual | Phase 10 |
| E1–E5 Large import RSS / no chunking | | | |
| E6 Device multi-hour memory | — | Residual | Phase 10 |
| F1–F7 Malformed input | | | |
| G1 Gpx family minus ImportExport | | | |
| G2 `Gpx_ImportExport_*` creator mismatch | — | Residual | CoMaps vs Organic Maps; do not change writer |
| G3 `Gpx_ColorMapExport_Test` | | | residual if same creator byte |
| H1–H6 Analytics automated + JVM | | | |
| H7 Analytics upload | — | Residual | Phase 10 |
| H8 / M7 Device readout | — | Residual | Phase 10 |
| M1–M7 Manual / device | — | Residual | `adb` absent unless filled |
| N1–N13 Named suites | | | |

## Phase 9 exit criteria table

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | GPX import marks pixels imported and personal completion | *(fill)* | |
| 2 | Imported pixels never recency/weekly/ownership regardless of gate | | |
| 3 | GPX import/export/track management gated by flag + entitlement | | |
| 4 | Public-configured builds expose no GPX tooling and no purchase action | | |
| 5 | Large imports complete without memory exhaustion | | |
| 6 | Malformed input rejected cleanly | | |
| 7 | Existing GPX tests still pass | | |
| 8 | Monetisation analytics only when Pro enabled in the build | | |

Do not mark any exit Met at the phase level.

## Residuals

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | No handset: M1–M7 | Phase 10. Map screenshots remain forbidden. |
| R2 | `adb` / public APK inflated settings dump / share-sheet VIEW | Phase 10 |
| R3 | Debug-entitle grant symbols in native / public APK `nm` | SP-083 follow-up; Phase 10 APK |
| R4 | `Gpx_ImportExport_*` CoMaps vs Organic Maps | Pre-existing residual; do not change writer |
| R5 | `Gpx_ColorMapExport_Test` if same creator byte | Same residual family |
| R6 | `BookmarkManagerGpxGateTest` UnsatisfiedLinkError | Environment residual (SP-084) |
| R7 | `data/classificator.txt` Eligibility abort | Environment residual; do not weaken |
| R8 | `prefs_gpx.xml` in public resources | Expected; dump inflated tree on device |
| R9 | Analytics upload | Phase 10 |
| R10 | Desktop/Qt ungated C++ GPX prepare | SP-083 residual |
| R11 | iOS GPX ungated | Out of Android V1 |
| R12 | `ReloadBookmarkRoutine` omits `historicalTracks` | SP-081/085 follow-up |
| R13 | Multi-category “GPX” export is KMZ | SP-086 follow-up |
| R14 | G1–G10 still Open | Maintainer; coding used recommended locks |
| R15 | README §4 Phase 9 “Not started” vs SP-081–086 Accepted | Report; do not edit README §4 |
| R16 | Phase 9 exit Met? | Maintainer only |
| R17 | Import on GUI thread | SP-085; no chunking after measurement |
| R18 | `WITH_SYSTEM_PROVIDED_3PARTY` expat GE/DTD | SP-085 residual |
| R19 | `DeserializerKml` still logs whole file | Later robustness |

Dirty tree left unstaged (never committed): `3party/healpix/healpix`,
`data/area_milestones.db`, `data/live_recency.db`, `data/street_stats.db`,
`data/weekly_city_live.db`.

## Phase 9 exit recommendation (agent)

*(Fill after suite runs.)* **Maintainer decides** whether Phase 9 exit is
Met with residuals. Agent does **not** mark SP-087 or Phase 9 Accepted.
Agent does **not** set phase Status to Exit criteria met.
