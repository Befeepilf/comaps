# SP-087 — Evidence log (Phase 9 exit)

**Plan:** [SP-087-validation-plan.md](SP-087-validation-plan.md)
**Branch:** `cursor/sp-087-phase9-validation-db9d`
**Status:** Evidence recorded — Phase 9 exit **awaiting maintainer** (agent
does not mark exit Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-28 |
| Client git SHA (suite run tip) | `5ed5e6df26c9eddf22090d1e77313d93ca047d64` (`[docs] Add SP-087 Phase 9 validation plan` on `cursor/sp-087-phase9-validation-db9d`) |
| Parent SHA | `d0d815832e7f695362fa1da0e4fba21dfbaf1188` (`Merge branch 'cursor/sp-086-pro-monetisation-analytics-db9d'`) |
| Build | `./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests kml_tests coding_tests` → `/home/ubuntu/omim-build-debug/` (`sp087_build.log`; binaries mtime 2026-08-28 17:04 UTC; suites started 17:05 against those binaries) |
| `data/classificator.txt` at run time | **MISSING** (`ls: cannot access '/workspace/data/classificator.txt': No such file or directory`) |
| `adb devices` | `adb: command not found` — no handset |
| `--filter='HistoricalImport'` | **27/27** All tests passed (substring also matches `IsolationHistoricalImport_*`: 11 `HistoricalImport_*` + 16 isolation) |
| `--filter='IsolationHistoricalImport'` | **16/16** All tests passed |
| `--filter='ExplorerPro_'` | **12/12** All tests passed |
| `--filter='GpxGate'` | **7/7** All tests passed |
| `--filter='ExplorerProAnalytics'` | **13/13** All tests passed |
| `--filter='EverLive'` | **18/18** All tests passed (substring also matches HistoricalImport/file/weekly/ownership ever-live names) |
| Phase 8 isolation regressions (B9) | **5/5** All tests passed |
| `kml_tests --filter='Gpx' --suppress='Gpx_ImportExport'` | **24/25** — fail `Gpx_ColorMapExport_Test` (`creator="CoMaps"` vs golden `Organic Maps`) |
| `kml_tests --filter='Gpx' --suppress='Gpx_ImportExport\|Gpx_ColorMapExport'` | **24/24** All tests passed |
| `kml_tests --filter='Gpx_ImportExport'` | **0/2** — `Gpx_ImportExport_Test`, `Gpx_ImportExportEmpty_Test` (same creator mismatch) |
| G4 unprefixed `gpx_tests.cpp` names | **16/18** — fail `ImportExportWptColor`, `PointWithPredefinedColor` (same creator family). Extra match `ColorParser_Smoke` OK |
| `coding_tests --filter='XmlParser_'` | **5/5** All tests passed |
| `street_pixels_tests --suppress=Eligibility` | **464/465** — Eligibility skipped (no `Eligibility_*` ran). One **non-Phase-9** fail: `PauseResume_TrackBoundary_SaveProducesSeparateLines` (missing `/workspace/data/sp010_gpstrack_test.bin`). Do not treat as full suite |
| JVM `ExplorerProGateTest` | **11/11** `tests=11 failures=0 errors=0` (`TEST-app.organicmaps.sdk.ExplorerProGateTest.xml`; inventory said 10) |
| JVM `ExplorerProAnalyticsTest` | **2/2** `tests=2 failures=0 errors=0` |
| JVM `GpxSettingsVisibilityTest` | **12/12** `tests=12 failures=0 errors=0` via `:app:testGoogleDebugUnitTest` |
| JVM `BookmarkManagerGpxGateTest` | **0/6** UnsatisfiedLinkError / ExceptionInInitializerError (`Framework.nativeGetBookmarksFilesExts()`). Residual SP-084 |
| Device walks / APK dump / `nm` | Deferred → Phase 10 |

### Suite command transcripts (counts)

Executed. `grep -c '^OK$'` matches `grep -c '^Running '` on each green client log. Do not treat these as guesses. (`kml_gpx_minus_importexport.log` contains a NUL from malformed-bytes fixture; counts used `grep -a`.)

```text
$ git -C /workspace rev-parse HEAD
5ed5e6df26c9eddf22090d1e77313d93ca047d64

$ git -C /workspace branch --show-current
cursor/sp-087-phase9-validation-db9d

$ adb devices || true
adb: command not found

$ ls -la /workspace/data/classificator.txt
ls: cannot access '/workspace/data/classificator.txt': No such file or directory
classificator.txt MISSING

$ ./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests kml_tests coding_tests
# /opt/cursor/artifacts/sp087_build.log; configure notes cp: cannot stat classificator.txt (and other data assets)
$ ls -la /home/ubuntu/omim-build-debug/street_pixels_tests /home/ubuntu/omim-build-debug/kml_tests /home/ubuntu/omim-build-debug/coding_tests
-rwxr-xr-x 1 ubuntu ubuntu 235960392 Aug 28 17:04 /home/ubuntu/omim-build-debug/street_pixels_tests
-rwxr-xr-x 1 ubuntu ubuntu  42416392 Aug 28 17:04 /home/ubuntu/omim-build-debug/kml_tests
-rwxr-xr-x 1 ubuntu ubuntu  43489616 Aug 28 17:04 /home/ubuntu/omim-build-debug/coding_tests

BIN=/home/ubuntu/omim-build-debug
DATA_ARGS="--data_path=/workspace/data --user_resource_path=/workspace/data"

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='HistoricalImport'
# grep -c '^OK$' → 27; grep -c '^Running ' → 27
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='IsolationHistoricalImport'
# grep -c '^OK$' → 16
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='ExplorerPro_'
# grep -c '^OK$' → 12
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='GpxGate'
# grep -c '^OK$' → 7
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='ExplorerProAnalytics'
# grep -c '^OK$' → 13
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='EverLive'
# grep -c '^OK$' → 18
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionUpload_ImportedOnlyZeroCompetitiveHttp|WeeklyCityLive_ImportOnlyDoesNot|CompetitionOwnership_ImportDoesNotWriteRecency|FirstGoal_ImportDoesNotAdvance|CompetitionHint_ImportDoesNotAdvance'
# grep -c '^OK$' → 5
All tests passed.

$ "$BIN/kml_tests" $DATA_ARGS --filter='Gpx' --suppress='Gpx_ImportExport'
# grep -a -c '^OK$' → 24; grep -a -c '^Running ' → 25
# 1 test failed: gpx_tests.cpp::Gpx_ColorMapExport_Test
# creator="CoMaps" vs golden creator="Organic Maps" (color_map_dst.gpx)
Some tests FAILED.

$ "$BIN/kml_tests" $DATA_ARGS --filter='Gpx' --suppress='Gpx_ImportExport|Gpx_ColorMapExport'
# grep -a -c '^OK$' → 24; grep -a -c '^Running ' → 24
All tests passed.

$ "$BIN/kml_tests" $DATA_ARGS --filter='Gpx_ImportExport'
# grep -c '^OK$' → 0; grep -c '^Running ' → 2
# 2 tests failed:
# gpx_tests.cpp::Gpx_ImportExport_Test
# gpx_tests.cpp::Gpx_ImportExportEmpty_Test
Some tests FAILED.

$ "$BIN/coding_tests" $DATA_ARGS --filter='XmlParser_'
# grep -c '^OK$' → 5
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --suppress=Eligibility
# grep -a -c '^OK$' → 464; grep -a -c '^Running ' → 465
# Eligibility_* did not run (file missing; do not weaken those tests)
# 1 test failed: pause_resume_semantics_tests.cpp::PauseResume_TrackBoundary_SaveProducesSeparateLines
# TEST(points.size() == 4) 3 4
# File operation error for file: /workspace/data/sp010_gpstrack_test.bin - No such file or directory
Some tests FAILED.

# RSS (paste; do not invent MiB). Budget 256 MiB. No chunking (SP-085 G10).
SP-085 HistoricalImport_TenThousandPointsCompletes n = 10000 VmRSS_before_kb = 33228 VmRSS_after_kb = 42384 VmHWM_kb = 42628 ru_maxrss_kb = 42628
SP-085 HistoricalImport_FiftyThousandPointsCompletes n = 50000 VmRSS_before_kb = 42148 VmRSS_after_kb = 82300 VmHWM_kb = 82300 ru_maxrss_kb = 82068
SP-085 Gpx_TenThousandPoints_ParseCompletes n = 10000 VmRSS_before_kb = 23676 VmRSS_after_kb = 24008 VmHWM_kb = 24008 ru_maxrss_kb = 23912
SP-085 Gpx_FiftyThousandPoints_ParseCompletes n = 50000 VmRSS_before_kb = 25492 VmRSS_after_kb = 27364 VmHWM_kb = 27444 ru_maxrss_kb = 27444

# JVM
$ cd /workspace/android && ./gradlew :sdk:testDebugUnitTest \
  --tests app.organicmaps.sdk.ExplorerProGateTest \
  --tests app.organicmaps.sdk.ExplorerProAnalyticsTest \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks
BUILD SUCCESSFUL in 2s
TEST-app.organicmaps.sdk.ExplorerProGateTest.xml tests=11 failures=0 errors=0 timestamp=2026-08-28T17:05:20.657Z
TEST-app.organicmaps.sdk.ExplorerProAnalyticsTest.xml tests=2 failures=0 errors=0 timestamp=2026-08-28T17:05:20.652Z

$ ./gradlew :app:testGoogleDebugUnitTest \
  --tests app.organicmaps.settings.GpxSettingsVisibilityTest \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks
BUILD SUCCESSFUL in 12s
TEST-app.organicmaps.settings.GpxSettingsVisibilityTest.xml tests=12 failures=0 errors=0 timestamp=2026-08-28T17:07:46.414Z

$ ./gradlew :sdk:testDebugUnitTest \
  --tests app.organicmaps.sdk.bookmarks.data.BookmarkManagerGpxGateTest \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks
BUILD FAILED
6 tests completed, 6 failed
BookmarkManagerGpxGateTest > isGpxFilename_trueForGpx FAILED
    java.lang.UnsatisfiedLinkError at BookmarkManagerGpxGateTest.java:13
# subsequent methods: NoClassDefFoundError caused by ExceptionInInitializerError
# class-load hits Framework.nativeGetBookmarksFilesExts() (BookmarkManager.java:55)
```

Logs: `/opt/cursor/artifacts/sp087_env.log`, `sp087_build.log`, `sp087_suite_summary.log`, `sp087_historical_import.log`, `sp087_isolation_historical_import.log`, `sp087_explorer_pro.log`, `sp087_gpx_gate.log`, `sp087_explorer_pro_analytics.log`, `sp087_ever_live.log`, `sp087_phase8_isolation_regressions.log`, `sp087_kml_gpx_minus_importexport.log`, `sp087_kml_gpx_minus_creator_goldens.log`, `sp087_kml_gpx_importexport.log`, `sp087_kml_gpx_unprefixed.log`, `sp087_coding_xmlparser.log`, `sp087_suppress_eligibility.log`, `sp087_jvm_sdk.log`, `sp087_jvm_gpx_settings.log`, `sp087_jvm_bookmark_gpx_gate.log`.

Δ RSS vs 256 MiB budget (from pasted lines): HistoricalImport 10k ≈ 9.0 MiB, 50k ≈ 39.2 MiB; Gpx parse 10k ≈ 0.3 MiB, 50k ≈ 1.8 MiB. Chunking not implemented and not required.

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 (`adb` absent) |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |
| D3 | optional | — | — | Deferred Phase 10 |

Map screenshots remain **forbidden**. No walks, packet captures, public APK
inflated settings dumps, share-sheet exercises, or public-APK `nm` were
produced.

Public walk APK (when Phase 10 runs): default capabilities **false**, no
`-PenableExplorerProCapabilities`, no `-PenableExplorerProDebugEntitle`.

Internal Pro walk: `-PenableExplorerProCapabilities=true`
`-PenableExplorerProDebugEntitle=true` on a **debug** build only.

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1–A6 Import marks imported + personal completion | agent | **Pass** | HistoricalImport 11/11 inside 27/27 filter; Isolation MarksExplored / PersonalCompletion; EverLive_TrackAloneLeavesClear / TrackAfterLiveRemainsSet |
| A7 / M1 Device multi-hour GPX | — | **Residual** | Phase 10. No handset |
| B1–B9 Isolation regardless of gate | agent | **Pass** | IsolationHistoricalImport **16/16**; B9 regressions **5/5**; four-cell gate tests included |
| B10 / M2–M3 Device competition chrome | — | **Residual** | Phase 10 |
| C1–C7 Pro gate C++ | agent | **Pass** | ExplorerPro_ **12/12**; GpxGate **7/7** |
| C8 JVM fail-closed when native not ready | agent | **Pass** | ExplorerProGateTest **11/11** |
| C9 Settings visibility four-cell | agent | **Pass** | GpxSettingsVisibilityTest **12/12** (`testGoogleDebugUnitTest`) |
| C10 / M5 Internal Pro device | — | **Residual** | Phase 10 |
| D1 BuildConfig defaults / release-beta `EXPLORER_PRO_*` false; debug-entitle `'false'` on release/beta | agent | **Pass** (code) | `android/sdk/build.gradle`: defaultConfig capabilities via `explorerProCapabilitiesEnabled('false')`; `EXPLORER_PRO_DEBUG_ENTITLE` `'false'` in defaultConfig, release, beta; debug type uses `explorerProDebugEntitleEnabled()` (default false) |
| D2 Public init never installs debug entitle | agent | **Pass** (code) | `OrganicMaps.java`: install only if `DEBUG_ENTITLE && (any capability)` then freeze |
| D3 No BillingClient / Upgrade / Buy / Restore in GPX/Pro surfaces | agent | **Pass** (code) | No `BillingClient` / Play Billing / Upgrade / Buy / Restore in `android/app/src/main` GPX/Pro surfaces. SPD-010. Do not fail for missing billing |
| D4 Settings public `showGpxScreen(false,false,false,false)` | agent | **Pass** | `showGpxScreen_hiddenWhenPublic`; `DataManagementSettingsFragment.initGpxToolsPref` removes nested entry when `!showScreen` |
| D5 Java import refuses `.gpx` when gate closed | agent | **Pass** (code) + **Residual** (JVM JNI) | `BookmarkManager.importBookmarksFile` returns false for GPX when `!isGpxImportEnabled()`; `allowGpxInBatch`. BookmarkManagerGpxGateTest ULE |
| D6 Export menus behind `isGpxExportEnabled()` | agent | **Pass** (code) | PlacePageView, BookmarksListFragment, BookmarkCategoriesFragment add `export_file_gpx` only when enabled |
| D7 Manifest VIEW/SEND GPX remain; handler no-ops (G6) | agent | **Pass** (code) | Manifest GPX mime filters remain. `Factory.KmzKmlProcessor` → `importBookmarksFiles`; Java gate no-ops `.gpx` |
| D8 Debug grant **symbols** still in native | — | **Residual** | `DebugEntitlementSource` / `InstallDebugEntitlementSource` in `libs/map/explorer_pro.*`; JNI `nativeInstallExplorerProDebugEntitlement`. Public APK `nm` → Phase 10. Not compiled out (SP-083 follow-up) |
| D9 / M4 Public APK dump / share-sheet | — | **Residual** | Phase 10. `prefs_gpx.xml` exists as a resource (expected; dump inflated tree, not aapt) |
| E1–E4 Large import/parse RSS | agent | **Pass** | 10k/50k HistoricalImport + Gpx parse complete; Δ RSS under 256 MiB (lines pasted above) |
| E5 Chunking | — | **Residual** | Not implemented (SP-085 G10). Not required after measurement |
| E6 Device multi-hour memory | — | **Residual** | Phase 10 |
| F1–F5 Malformed / skip / prefix log / billion laughs / XmlParser | agent | **Pass** | kml Gpx malformed/skip/log/entity + XmlParser_ **5/5** |
| F6 Historical skip invalid; no paint | agent | **Pass** | HistoricalImport_InvalidCoordinatesAreSkipped / EmptyOrInvalidGeometryDoesNotPaint |
| F7 Isolation malformed GPX no competition | agent | **Pass** | IsolationHistoricalImport_MalformedGpxDoesNotTouchCompetition |
| G1 Gpx family minus ImportExport / ColorMapExport | agent | **Pass** | **24/24** after suppress of creator goldens |
| G2 `Gpx_ImportExport_*` creator mismatch | — | **Residual** | CoMaps vs Organic Maps; do not change writer |
| G3 `Gpx_ColorMapExport_Test` | — | **Residual** | Same `creator=` byte vs `color_map_dst.gpx` |
| G4 Unprefixed names | agent | **Pass** (parse/color) + **Residual** (roundtrip creator) | GoMap, GpxStudio, OsmTrack, … OK. `ImportExportWptColor` / `PointWithPredefinedColor` same creator family |
| H1–H5 Analytics C++ | agent | **Pass** | ExplorerProAnalytics **13/13** |
| H6 JVM fail-closed getters | agent | **Pass** | ExplorerProAnalyticsTest **2/2** |
| H7 Analytics upload | — | **Residual** | Phase 10. Not Sentry |
| H8 / M7 Device readout | — | **Residual** | Phase 10 |
| M1–M7 Manual / device | — | **Residual** | `adb` absent. Phase 10 |
| N1–N12 Named suites | agent | **Pass** (named Phase 9) + residuals as above | Counts in baseline table |
| N13 BookmarkManagerGpxGateTest | — | **Residual** | JNI ULE |

## Phase 9 exit criteria table

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | GPX import marks pixels imported and personal completion | **Pass (automated) + Residual (device GPX walk Phase 10)** | A1–A6; N1 27/27; N2 16/16; N6 18/18; M1 → Phase 10 |
| 2 | Imported pixels never recency/weekly/ownership regardless of gate | **Pass (automated) + Residual (device competition chrome Phase 10)** | B1–B9; Isolation **16/16**; B9 **5/5**; four-cell gate; M2–M3 → Phase 10 |
| 3 | GPX import/export/track management gated by flag + entitlement | **Pass (automated + code review) + Residual (internal Pro device Phase 10)** | C1–C9; ExplorerPro_ **12/12**; GpxGate **7/7**; JVM Gate **11/11** + Settings **12/12**; M5–M6 → Phase 10 |
| 4 | Public-configured builds expose no GPX tooling and no purchase action | **Pass (automated + code review) + Residual (public APK dump, share-sheet, debug-entitle `nm` Phase 10)** | D1–D7 code + automated; D8 symbols remain (SP-083); D9/M4 → Phase 10. Do not Fail. Do not call the device half done |
| 5 | Large imports complete without memory exhaustion | **Pass (automated RSS) + Residual (device multi-hour Phase 10)** | E1–E4; RSS lines under 256 MiB; chunking not required; M1 → Phase 10 |
| 6 | Malformed input rejected cleanly | **Pass (automated)** | F1–F7; XmlParser_ **5/5**; HistoricalImport skip/empty; Isolation malformed |
| 7 | Existing GPX tests still pass | **Pass (Gpx family minus creator-golden roundtrips) + Residual (CoMaps vs Organic Maps golden)** | G1 **24/24**; G2/G3/G4 roundtrip residuals. Do not Fail solely on G2. SP-085 new parse/malformed/10k/50k pass. Do not change writer |
| 8 | Monetisation analytics only when Pro enabled in the build | **Pass (automated) + Residual (upload + device readout Phase 10)** | H1–H6; ExplorerProAnalytics **13/13**; JVM **2/2**; H7–H8 → Phase 10 |

Do not mark any exit Met at the phase level.

## Residuals

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | No handset: M1–M7 | Phase 10. Map screenshots remain forbidden. |
| R2 | `adb` / public APK inflated settings dump / share-sheet VIEW | Phase 10 |
| R3 | Debug-entitle grant symbols in native / public APK `nm` | SP-083 follow-up; Phase 10 APK. `DebugEntitlementSource`, `InstallDebugEntitlementSource`, `nativeInstallExplorerProDebugEntitlement` present. Release/beta BuildConfig hardcoded `'false'` |
| R4 | `Gpx_ImportExport_Test` / `Gpx_ImportExportEmpty_Test` CoMaps vs Organic Maps | Pre-existing residual; do not change writer (`libs/kml/serdes_gpx.cpp`) |
| R5 | `Gpx_ColorMapExport_Test` same creator byte | Same residual family (`color_map_dst.gpx`) |
| R5b | `ImportExportWptColor` / `PointWithPredefinedColor` same creator byte | Same residual family (G4 unprefixed roundtrip) |
| R6 | `BookmarkManagerGpxGateTest` UnsatisfiedLinkError | Environment residual (SP-084). Class static `Framework.nativeGetBookmarksFilesExts()`. Do not fix JNI loading here |
| R7 | `data/classificator.txt` Eligibility abort | Environment residual. File missing at run time. `--suppress=Eligibility` used. Eligibility tests not weakened, skipped, or deleted. No `Eligibility_*` ran |
| R8 | `prefs_gpx.xml` in public resources | Expected (`android/app/src/main/res/xml/prefs_gpx.xml`); dump inflated Data Management tree on device |
| R9 | Analytics upload | Phase 10. Count-only local counters exist. Not Sentry |
| R10 | Desktop/Qt ungated C++ GPX prepare | SP-083 residual. No `IsCapabilityEnabled` in `qt/` |
| R11 | iOS GPX ungated | Out of Android V1 |
| R12 | `ReloadBookmarkRoutine` omits `historicalTracks` | SP-081/085 follow-up |
| R13 | Multi-category “GPX” export is KMZ | SP-086 follow-up |
| R14 | G1–G10 still Open | Maintainer; coding used recommended locks (OQ-20–OQ-29 / draft SPD-067–076) |
| R15 | README §4 Phase 9 “Not started” vs SP-081–086 Accepted | Report; do not edit README §4. `docs/implementation/README.md` §4 table still lists Phase 9 **Not started** while SP-081–086 are Accepted 2026-08-28 |
| R16 | Phase 9 exit Met? | Maintainer only. Agent does not mark Met |
| R17 | Import on GUI thread | SP-085; no chunking after measurement |
| R18 | `WITH_SYSTEM_PROVIDED_3PARTY` expat GE/DTD | SP-085 residual |
| R19 | `DeserializerKml` still logs whole file | Later robustness |
| R20 | `--suppress=Eligibility` **464/465**: `PauseResume_TrackBoundary_SaveProducesSeparateLines` | Environment residual, **not a Phase 9 exit**. `TEST(points.size() == 4) 3 4`; missing `/workspace/data/sp010_gpstrack_test.bin`. Do not treat this command as a full-suite pass. Not Eligibility |

Dirty tree left unstaged (never committed): `3party/healpix/healpix`,
`data/area_milestones.db`, `data/live_recency.db`, `data/street_stats.db`,
`data/weekly_city_live.db`.

## Code-review notes (exit 4)

- Isolation is a data rule: `ImportHistoricalTrack` is not wrapped in
  `IsCapabilityEnabled`. Framework handler skips paint when the Java/C++
  gate is closed; `GpxGate_DirectImportClosedStillPaints` asserts the
  dedicated C++ path still paints (SPD-011).
- No purchase action is reachable in public-configured GPX/Pro surfaces.
  Missing Play Billing is **not** a fail (SPD-010).
- `bookmark_purchase_img_width` is bookmarks UI, not Explorer Pro IAP.

## Phase 9 exit recommendation (agent)

Named Phase 9 suites on client SHA `5ed5e6df2` are green except documented
GPX **creator-golden** residuals (R4, R5, R5b) and environment residuals
(R1 device, R6 JNI, R7 classificator, R20 pause-resume bin). Exit criteria
1–8 map as **Pass (automated [+ code review]) + Residual (Phase 10 device /
APK / upload / G1–G10 Open)** per the table. Chunking is not required.

**Maintainer decides** whether Phase 9 exit is Met with residuals. Agent
does **not** mark SP-087 or Phase 9 Accepted. Agent does **not** set phase
Status to Exit criteria met. G1–G10 remain **Open**.
