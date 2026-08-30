# SP-087 — Validation plan (Phase 9 exit)

**Work item:** [SP-087](../work-items/SP-087-phase9-end-to-end-validation.md)
**Plan authored by:** Agent
**Plan review date:** 2026-08-28
**Branch:** `cursor/sp-087-phase9-validation-db9d` (lands on `street-pixels`)

Evidence-only. Write this plan and the evidence log, re-run named suites,
map exit criteria 1–8 to pass / fail / residual. **Do not** mark SP-087
or Phase 9 Accepted. **Do not** set phase Status to Exit criteria met.
**Do not** implement features. **Do not** fill Accepted by (orchestrator
accepts). G1–G10 remain **Open**.

## Approved decisions

| ID | Decision |
| --- | --- |
| Device / APK | **Phase 10 residual** unless `adb devices` shows a handset. This cloud environment is assumed to have none (`adb: command not found` at plan time). Do not fabricate walks, screenshots, packet captures, or APK settings dumps. |
| Map screenshots | **Forbidden**. Even if a device appears, do not capture the map. |
| Phase 9 not self-accepted | After evidence: SP-087 Status → **In review**. Leave Accepted empty. Do **not** set phase-09 Status to exit Met. G1–G10 stay Open. |
| Highway `Eligibility_*` | **Not a Phase 9 exit.** If full `street_pixels_tests` aborts because `data/classificator.txt` is absent, environment residual. Re-check at run time. Do not weaken Eligibility. |
| `Gpx_ImportExport_*` | Pre-existing creator mismatch. Residual. Do not change the writer. Prefer `--suppress='Gpx_ImportExport'` on the named existing-GPX suite so exit 7 is not blocked by a Streifzug rebrand golden. |
| Debug-entitle compile-out | Known SP-083 follow-up: `DebugEntitlementSource` / `InstallDebugEntitlementSource` / `nativeInstallExplorerProDebugEntitlement` remain in the native binary. Public APK `nm`/dump is device/APK residual → Phase 10. Code-review of `build.gradle` (release/beta `EXPLORER_PRO_DEBUG_ENTITLE` hardcoded `'false'`) is in-scope. |
| `prefs_gpx.xml` in public APK | Strings/XML exist. SP-084: dump the **inflated Preference tree**, not `aapt` of prefs XML or a `strings.xml` grep. No handset → residual. |
| `BookmarkManagerGpxGateTest` | Class-load hits `Framework.nativeGetBookmarksFilesExts()` → `UnsatisfiedLinkError` in this JVM. Environment residual (SP-084). Do not “fix” JNI loading here. |
| Isolation | Data rule. Four-cell gate matrix must keep recency/weekly/ownership/pending clean. Do not wrap isolation in `IsCapabilityEnabled`. |
| Billing / iOS | Out. SPD-010. Do not fail exit 4 for missing Play Billing. Fail only if a **purchase action** is reachable in public-configured code. |
| Dirty tree | Never commit healpix or `data/*.db`. |
| Counts | Executed transcripts only. |
| Phase-09 table | Refresh **observed-state** column to the working tree. Do not rewrite intended outcome, exit criteria, or G1–G10. |
| Explorer / backend | Not in Phase 9. Do not run explorer pytest. |
| README §4 | Plan-time: do not edit README §4 (report contradiction). Independent review 2026-08-28 **did** refresh §4 to Phase 9 **In progress** (SP-081–086 Accepted; SP-087 In review / evidence recorded; exit awaiting maintainer) without claiming exit Met. |

## Scope

Evidence-only. No production behaviour changes on this branch except defect
fixes that block listed suites (prefer record as follow-up on owning
SP-081–086). Map each Phase 9 exit criterion (1–8) to pass / fail /
residual with pointers into the evidence log.

Phase 9 modules under test: SP-081 (dedicated historical-import pipeline),
SP-082 (competition isolation), SP-083 (Pro gate on GPX surfaces), SP-084
(settings surface), SP-085 (untrusted input / large-import measurement),
SP-086 (count-only monetisation analytics). SP-080 is the lock set
(G1–G10 / draft SPD-067–076 / OQ-20–OQ-29) and remains **Open**.

**Baselines (verify at start; do not treat as pass counts):**

- Client: `/workspace` on `cursor/sp-087-phase9-validation-db9d`. Parent of
  this work: `d0d815832` (`Merge branch
  'cursor/sp-086-pro-monetisation-analytics-db9d'`). Record `git rev-parse
  HEAD` after any plan commit.
- SP-081–086 **Accepted 2026-08-28** (product owner). SP-080 / G1–G10 still
  **Open**. Coding used recommended locks.
- `data/classificator.txt` at plan time: **MISSING**. Re-check before
  optional full `street_pixels_tests`.
- `adb`: not found at plan time.
- Prebuilt binaries exist under `/home/ubuntu/omim-build-debug/`.
  **Rebuild** so the evidence SHA matches the suite binaries. Do not reuse
  another item’s counts.
- Android SDK present at plan time: `ANDROID_HOME=/home/ubuntu/Android/Sdk`.
  JVM tests are **in scope** (skip native CMake).
- Explorer / backend pytest: **out of scope**.

SP-081–086 executed baselines (context only; re-run):

| Item | SHA (then) | Named filters |
| --- | --- | --- |
| SP-081 | `b33e8bc58` | HistoricalImport 9/9 (now 11 tests incl. 10k/50k); EverLive 18/18; `--suppress=Eligibility` 420/420 |
| SP-082 | `145cc7f65` | IsolationHistoricalImport 14/14 (now 16); Phase 8 regressions + ExplorerPro_ 12/12 |
| SP-083 | — | `ExplorerPro_\|GpxGate\|IsolationHistoricalImport_Gate` 23/23 |
| SP-084 | — | ExplorerProGateTest 10/10; GpxSettingsVisibilityTest 12/12; BookmarkManagerGpxGateTest JNI residual |
| SP-085 | — | XmlParser_ 5/5; Gpx_Malformed / skip / log / entity / 10k/50k parse; HistoricalImport 11; Isolation 16/16; ImportExport residual |
| SP-086 | — | ExplorerProAnalytics 13/13; Isolation 16/16; ExplorerPro_ 12/12; JVM ExplorerProAnalyticsTest 2/2 |

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Finland / Helsinki | Same class as SP-014 / SP-041 / SP-069 / SP-079 |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log when walks run. This cloud environment is
assumed to have no handset (`adb devices` empty / `adb` absent). If a
device appears, still **do not capture the map**.

Public walk APK: default capabilities **false**, no
`-PenableExplorerProCapabilities`, no `-PenableExplorerProDebugEntitle`.

Internal Pro walk: `-PenableExplorerProCapabilities=true`
`-PenableExplorerProDebugEntitle=true` on a **debug** build only.

## Scenario catalogue

Exit 1–8 map to Blocks A–H plus manual M1–M7 (Phase 10 if no adb device).
Block N is the automated suite re-run that feeds all exits.

Expected names below are **inventory**, not pass counts.

### Block A — Import marks imported + personal completion (exit 1)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | First import explored, ever-live clear | `HistoricalImport_FirstImportEverLiveClear` | 1 |
| A2 | Live then import remains ever-live | `HistoricalImport_LiveThenImportRemainsEverLive` | 1 |
| A3 | Dedicated path paints; catch-all does not | `HistoricalImport_UpdateExploredPixelsDoesNotPaint` | 1 |
| A4 | Import marks explored never live | `IsolationHistoricalImport_MarksExploredNeverLive` | 1, 2 |
| A5 | Personal completion increments | `IsolationHistoricalImport_PersonalCompletionIncrements` | 1 |
| A6 | Ever-live track helpers | `EverLive_TrackAloneLeavesClear` / `EverLive_TrackAfterLiveRemainsSet` | 1 |
| A7 | Device: multi-hour GPX → green + area % | M1 | 1 → Phase 10 |

Filter: `--filter='HistoricalImport_|IsolationHistoricalImport_MarksExplored|IsolationHistoricalImport_PersonalCompletion|EverLive_Track'`.

Named suite for exit 1: `--filter='HistoricalImport'` matches **11** `HistoricalImport_*` tests **and** 16 `IsolationHistoricalImport_*` (substring). Report 11+16, not “27 HistoricalImport tests”. `--filter='IsolationHistoricalImport'` feeds exits 1 and 2 (16 tests).

### Block B — Isolation regardless of gate (exit 2)

Isolation is a **data rule**. Gate matrix must not create recency,
weekly, ownership, or upload pending.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | No recency / weekly / ownership / pending | `IsolationHistoricalImport_NoRecencyWeeklyOwnershipOrPending` | 2 |
| B2 | Ownership 0 at full personal | `IsolationHistoricalImport_OwnershipZeroAtFullPersonal` | 2 |
| B3 | Import then live counts once | `IsolationHistoricalImport_ThenLiveCountsOnce` | 2 |
| B4 | Live then import leaves recency | `IsolationHistoricalImport_LiveThenImportLeavesRecencyUnchanged` | 2 |
| B5 | First-goal / hint / haptic | `IsolationHistoricalImport_FirstGoalDoesNotAdvance` / `CompetitionHintDoesNotAdvance` / `NotRecordingZeroHaptic` | 2 |
| B6 | Four-cell isolation | `IsolationHistoricalImport_GateUnavailableNotEntitled` / `GateUnavailableEntitled` / `GateAvailableNotEntitled` / `GateAvailableEntitled` | 2, 3 |
| B7 | Live then import when available+entitled | `IsolationHistoricalImport_LiveThenImportWhenAvailableEntitled` | 2 |
| B8 | Invalid / malformed GPX no competition | `IsolationHistoricalImport_InvalidGeometryDoesNotTouchCompetition` / `MalformedGpxDoesNotTouchCompetition` | 2, 6 |
| B9 | Phase 8 regressions | `CompetitionUpload_ImportedOnlyZeroCompetitiveHttp` / `WeeklyCityLive_ImportOnlyDoesNot` / `CompetitionOwnership_ImportDoesNotWriteRecency` / `FirstGoal_ImportDoesNotAdvance` / `CompetitionHint_ImportDoesNotAdvance` | 2 |
| B10 | Device: competition on, no ownership/weekly movement | M2, M3 | 2 → Phase 10 |

Filter: `--filter='IsolationHistoricalImport'`.
Regression filter:
`--filter='CompetitionUpload_ImportedOnlyZeroCompetitiveHttp|WeeklyCityLive_ImportOnlyDoesNot|CompetitionOwnership_ImportDoesNotWriteRecency|FirstGoal_ImportDoesNotAdvance|CompetitionHint_ImportDoesNotAdvance'`.

### Block C — Pro gate flag + entitlement (exit 3)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | Default unavailable; stub never grants | `ExplorerPro_DefaultCapabilitiesUnavailable` / `StubEntitlementNeverGrants` | 3, 4 |
| C2 | Four-cell IsCapabilityEnabled | `ExplorerPro_GateUnavailableNotEntitled` / `GateUnavailableEntitled` / `GateAvailableNotEntitled` / `GateAvailableEntitled` / `GateMatrixAllCapabilities` | 3 |
| C3 | Debug entitle used / not used / stub restored / freeze | `ExplorerPro_DebugEntitlementSourceUsed` / `NotUsed` / `StubRestored` / `FrozenKeepsEnabledState` / `FrozenIgnoresDebugInstall` | 3, 4 |
| C4 | Handler closed does not paint (three closed cells) | `GpxGate_HandlerClosedDoesNotPaint` / `HandlerUnavailableEntitledDoesNotPaint` / `HandlerAvailableNotEntitledDoesNotPaint` | 3, 4 |
| C5 | Handler open paints | `GpxGate_HandlerOpenPaints` | 3 |
| C6 | Export / batch four-cell | `GpxGate_ExportFourCell` / `GpxGate_BatchFourCell` | 3 |
| C7 | Direct C++ import still paints when Java gate closed (data rule) | `GpxGate_DirectImportClosedStillPaints` | 2, 3 |
| C8 | JVM fail-closed when native not ready | `ExplorerProGateTest` 10 methods (inventory); SP-087 executed **11** | 3, 4 |
| C9 | Settings visibility four-cell | `GpxSettingsVisibilityTest` 12 methods | 3, 4 |
| C10 | Device: internal Pro + debug-entitle tools work | M5 | 3 → Phase 10 |

Filters: `--filter='ExplorerPro_'` and `--filter='GpxGate'`.

### Block D — Public-configured builds: no GPX tooling, no purchase (exit 4)

No APK in this environment. Split: **Pass (automated + code review) +
Residual (public APK dump / share-sheet / compile-out nm → Phase 10)**.
Do not Fail. Do not call the device half done.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | BuildConfig defaults false; release/beta `EXPLORER_PRO_*` false; `EXPLORER_PRO_DEBUG_ENTITLE` `'false'` on release/beta | Code review `android/sdk/build.gradle` | 4 |
| D2 | Public init never installs debug entitle | `OrganicMaps.java`: install only if `DEBUG_ENTITLE && (any capability)` | 4 |
| D3 | No BillingClient / Upgrade / Buy / Restore in GPX/Pro surfaces | Code review (SPD-010). `bookmark_purchase_img_width` is bookmarks UI, not Pro IAP | 4 |
| D4 | Settings: public `showGpxScreen(false,false,false,false)` adds nothing | `GpxSettingsVisibilityTest.showGpxScreen_hiddenWhenPublic`; `DataManagementSettingsFragment.initGpxToolsPref` | 4 |
| D5 | Java import refuses `.gpx` when gate closed; KML/KMZ stays | `BookmarkManager.importBookmarksFile` / `allowGpxInBatch`; JVM `BookmarkManagerGpxGateTest` if it loads | 4 |
| D6 | Export menus behind `isGpxExportEnabled()` | `PlacePageView`, `BookmarksListFragment`, `BookmarkCategoriesFragment` | 4 |
| D7 | Manifest VIEW/SEND GPX may remain; handler no-ops (G6) | Code review `Factory.KmzKmlProcessor` → `importBookmarksFiles` | 4 |
| D8 | Debug grant **symbols** still in native | Residual (SP-083). Do not compile-out in this item. Public APK `nm` → Phase 10 | 4 |
| D9 | Device: public APK inflated settings dump, share-sheet, no purchase | M4 | 4 → Phase 10 |

`prefs_gpx.xml` and English GPX/Pro strings **exist** in the public APK
resources. That is not a fail. Inflated Data Management must add no
rows when all Available are false.

### Block E — Large imports without memory exhaustion (exit 5)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | Parse 10k completes; log Δ RSS | `Gpx_TenThousandPoints_ParseCompletes` | 5, 7 |
| E2 | Parse 50k completes; log Δ RSS | `Gpx_FiftyThousandPoints_ParseCompletes` | 5, 7 |
| E3 | Import 10k completes; log Δ RSS | `HistoricalImport_TenThousandPointsCompletes` | 5 |
| E4 | Import 50k completes; log Δ RSS | `HistoricalImport_FiftyThousandPointsCompletes` | 5 |
| E5 | Chunking | **Not implemented** (SP-085 G10). Do not add it. Record “no chunking; budget 256 MiB” if RSS lines stay under budget | 5 |
| E6 | Device multi-hour GPX memory | M1 | 5 → Phase 10 |

Filters: `--filter='Gpx_TenThousand|Gpx_FiftyThousand'` on **kml_tests**;
`--filter='HistoricalImport_TenThousand|HistoricalImport_FiftyThousand'`
on **street_pixels_tests**. Paste RSS log lines; do not invent MiB.

### Block F — Malformed input rejected cleanly (exit 6)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Truncated / empty / non-GPX | `Gpx_Malformed_TruncatedXml` / `TruncatedXmlClosedInnerTags` / `EmptyFile` / `NonGpxBytes` | 6, 7 |
| F2 | Out-of-range / non-finite skipped | `Gpx_OutOfRangeCoordinatesAreSkipped` / `NonFiniteCoordinatesAreSkipped` / `ValidPointAmongInvalidsIsKept` / `WaypointOutOfRangeIsSkipped` / `BoundaryCoordinatesAreKept` | 6, 7 |
| F3 | Parse-failure prefix log, not whole payload | `Gpx_ParseFailure_DoesNotLogWholePayload` | 6, 7 |
| F4 | Billion laughs does not expand | `Gpx_BillionLaughs_DoesNotExpand` | 6, 7 |
| F5 | XmlParser truncated/empty | `XmlParser_SmokeTest` / `LongTest` / `WellFormedPopsRoot` / `TruncatedDocument` / `EmptyDocument` | 6 |
| F6 | Historical skip invalid; no paint | `HistoricalImport_InvalidCoordinatesAreSkipped` / `EmptyOrInvalidGeometryDoesNotPaint` | 6 |
| F7 | Isolation: malformed GPX no competition | `IsolationHistoricalImport_MalformedGpxDoesNotTouchCompetition` | 2, 6 |

Filters: `kml_tests --filter='Gpx_Malformed_|Gpx_OutOfRange|Gpx_NonFinite|Gpx_ValidPoint|Gpx_Waypoint|Gpx_Boundary|Gpx_ParseFailure|Gpx_BillionLaughs'`.
`coding_tests --filter='XmlParser_'`.

### Block G — Existing GPX tests still pass (exit 7)

Binary: **`kml_tests`** (`libs/kml/kml_tests/gpx_tests.cpp`).

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| G1 | Named Gpx family except ImportExport | `kml_tests --filter='Gpx' --suppress='Gpx_ImportExport'` | 7 |
| G2 | Known golden fail | `kml_tests --filter='Gpx_ImportExport'` → **Residual** (`creator="Streifzug"` vs golden `Organic Maps` in `data/test_data/gpx/export_test.gpx` and `export_test_empty.gpx`). Do not change the writer. | 7 |
| G3 | ColorMap exact roundtrip | If `Gpx_ColorMapExport_Test` fails on the same `creator=` byte (`color_map_dst.gpx`), residual with G2. Do not change the writer. If it passes, record pass. | 7 |
| G4 | Other gpx_tests.cpp names without `Gpx_` prefix | `GoMap`, `GpxStudio`, `OsmTrack`, `TowerCollector`, `PointsOnly`, `Route`, `Color`, `ParseExportedGpxColor`, `MultiTrackNames`, `Empty`, `ImportExportWptColor`, `PointWithPredefinedColor`, `OsmandColor1`, `OsmandColor2`, `OpentracksColor`, `ParseFromString`, `MapGarminColor` — include in G1 via `--filter='Gpx'` which matches both `Gpx_` and `GpxStudio` | 7 |

`--filter='Gpx_'` does **not** match `GpxStudio` (no underscore after
Gpx). Always use `--filter='Gpx'` for G1 so unprefixed names run.

If `Gpx_ColorMapExport` fails, add it to suppress for the green run and
record G3 residual. Do not weaken the test.

Honest mapping: **Pass (parse / malformed / 10k / non-roundtrip) +
Residual (ImportExport creator mismatch)**. Do not Fail exit 7 solely on
G2 if G1 is green.

### Block H — Monetisation analytics only when Pro enabled (exit 8)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| H1 | Default zero; no keys | `ExplorerProAnalytics_DefaultZero` | 8 |
| H2 | Record when any/matching Available | `RecordInfoWhenAnyAvailable` / `RecordImportWhenAvailable` / `RecordImportWhenAvailableNotEntitled` / `RecordExportWhenAvailable` | 8 |
| H3 | Fail-closed when unavailable | `RecordInfoFailClosedWhenAllUnavailable` / `RecordExportNoOpWhenUnavailable` / `UnavailableLeavesStoredZero` | 8 |
| H4 | Persist / reset / no location keys | `PersistRoundTrip` / `ResetIsolatesTests` / `SnapshotHasNoLocationKeys` | 8 |
| H5 | Handler increments after import only when available | `HandlerDoesNotWrapManager` / `HandlerIncrementsAfterImport` | 8 |
| H6 | JVM fail-closed getters | `ExplorerProAnalyticsTest` | 8 |
| H7 | Upload | Residual Phase 10 (SPD-044 pattern). Not Sentry. | 8 |
| H8 | Device debug readout | M7 | 8 → Phase 10 |

Filter: `--filter='ExplorerProAnalytics'`.

### Block M — Manual / device (phase-09 strategy)

Phase 10 if no handset. Do not fabricate.

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| M1 | Real multi-hour GPX: green pixels, area % up | Visual; no map screenshot | 1, 5 |
| M2 | Competition on: no ownership change, no weekly movement | Competition chrome unchanged | 2 |
| M3 | Import over already-live area: competitive position unchanged | Same | 2 |
| M4 | Public-configured build: no GPX import/export/purchase, including share-sheet and VIEW intents; inflated settings dump | No tools, no buy | 4 |
| M5 | Pro-enabled internal build (G7 debug-entitle): tools appear and work | Import/export/batch | 3 |
| M6 | Batch-import several files | Several URIs | 3, 4 |
| M7 | Optional analytics readout after one import | Counters +1 when Available | 8 |

### Block N — Automated suites (feeds all exits)

| ID | Command | Exit # |
| --- | --- | --- |
| N1 | `street_pixels_tests --filter='HistoricalImport'` | 1, 5, 6 |
| N2 | `street_pixels_tests --filter='IsolationHistoricalImport'` | 1, 2, 6 |
| N3 | `street_pixels_tests --filter='ExplorerPro_'` | 3, 4 |
| N4 | `street_pixels_tests --filter='GpxGate'` | 3, 4 |
| N5 | `street_pixels_tests --filter='ExplorerProAnalytics'` | 8 |
| N6 | `street_pixels_tests --filter='EverLive'` | 1 |
| N7 | Phase 8 isolation regressions (B9 filter) | 2 |
| N8 | `kml_tests --filter='Gpx' --suppress='Gpx_ImportExport'` | 5, 6, 7 |
| N9 | `kml_tests --filter='Gpx_ImportExport'` (expect fail; residual) | 7 |
| N10 | `coding_tests --filter='XmlParser_'` | 6 |
| N11 | `street_pixels_tests --suppress=Eligibility` if classificator missing; else optional full | env |
| N12 | JVM: ExplorerProGateTest, ExplorerProAnalyticsTest, GpxSettingsVisibilityTest | 3, 4, 8 |
| N13 | BookmarkManagerGpxGateTest | residual JNI if UnsatisfiedLinkError |

`gpx_tests.cpp` is compiled into the **`kml_tests`** binary, not a
separate `gpx_tests` target.

## Exit criteria mapping (fill in evidence log)

| # | Criterion | Evidence | Honest result shape |
| --- | --- | --- | --- |
| 1 | GPX import marks pixels imported and personal completion | A, N1, N2, N6, M1 | **Pass (automated) + Residual (device GPX walk Phase 10)** |
| 2 | Imported pixels never recency/weekly/ownership regardless of gate | B, N2, N7, M2, M3 | **Pass (automated) + Residual (device competition chrome Phase 10)** |
| 3 | GPX import/export/track management gated by flag + entitlement | C, N3, N4, N12, M5, M6 | **Pass (automated + code review) + Residual (internal Pro device Phase 10)** |
| 4 | Public-configured builds expose no GPX tooling and no purchase action | D, N3, N4, N12, M4 | **Pass (automated + code review) + Residual (public APK dump, share-sheet, debug-entitle `nm` Phase 10)** |
| 5 | Large imports complete without memory exhaustion | E, N1, N8, M1 | **Pass (automated RSS) + Residual (device multi-hour Phase 10)**. Chunking not required. |
| 6 | Malformed input rejected cleanly | F, N1, N2, N8, N10 | **Pass (automated)** |
| 7 | Existing GPX tests still pass | G, N8, N9 | **Pass (parse / malformed / 10k / non-roundtrip) + Residual (`Gpx_ImportExport_Test`, `Gpx_ImportExportEmpty_Test`, and if they fail the same `creator=` byte: `Gpx_ColorMapExport_Test`, `ImportExportWptColor`, `PointWithPredefinedColor`)** |
| 8 | Monetisation analytics only when Pro enabled in the build | H, N5, N12, M7 | **Pass (automated) + Residual (upload + device readout Phase 10)** |

Do not mark any exit Met at the phase level.

## Test commands

Record SHA + `adb devices` first. Transcripts under
`/opt/cursor/artifacts/` as `sp087_*.log`. Rebuild so binaries match SHA.

```bash
cd /workspace
git rev-parse HEAD
git branch --show-current
adb devices || true
ls -la /workspace/data/classificator.txt || echo "classificator.txt MISSING"

./tools/unix/build_omim.sh -d -p "$HOME" \
  street_pixels_tests kml_tests coding_tests

BIN=/home/ubuntu/omim-build-debug
DATA_ARGS="--data_path=/workspace/data --user_resource_path=/workspace/data"
ls -la "$BIN/street_pixels_tests" "$BIN/kml_tests" "$BIN/coding_tests"

# Named street_pixels_tests
"$BIN/street_pixels_tests" $DATA_ARGS --filter='HistoricalImport'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='IsolationHistoricalImport'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='ExplorerPro_'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='GpxGate'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='ExplorerProAnalytics'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='EverLive'
"$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionUpload_ImportedOnlyZeroCompetitiveHttp|WeeklyCityLive_ImportOnlyDoesNot|CompetitionOwnership_ImportDoesNotWriteRecency|FirstGoal_ImportDoesNotAdvance|CompetitionHint_ImportDoesNotAdvance'

# kml_tests = gpx_tests.cpp + other kml
"$BIN/kml_tests" $DATA_ARGS --filter='Gpx' --suppress='Gpx_ImportExport'
"$BIN/kml_tests" $DATA_ARGS --filter='Gpx_ImportExport'   # expect creator mismatch

# coding_tests XmlParser_
"$BIN/coding_tests" $DATA_ARGS --filter='XmlParser_'

# Full street_pixels_tests: only if classificator present
if [ -f /workspace/data/classificator.txt ]; then
  "$BIN/street_pixels_tests" $DATA_ARGS
else
  "$BIN/street_pixels_tests" $DATA_ARGS --suppress=Eligibility
fi
```

Count method:

```text
grep -c '^OK$' logfile
grep -c '^Running ' logfile
# Must match. Paste “All tests passed.” or the FAIL body.
```

### JVM (SDK present at plan time)

Skip native CMake. Do not treat a missing flavor as a product fail.

```bash
cd /workspace/android
./gradlew :sdk:testDebugUnitTest \
  --tests app.organicmaps.sdk.ExplorerProGateTest \
  --tests app.organicmaps.sdk.ExplorerProAnalyticsTest \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks

# BookmarkManagerGpxGateTest: expect UnsatisfiedLinkError (class static JNI).
# Run once; residual if ULE. Do not change BookmarkManager to make it load.

./gradlew :app:testGoogleDebugUnitTest \
  --tests app.organicmaps.settings.GpxSettingsVisibilityTest \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks
```

If `testGoogleDebugUnitTest` is missing, try `:app:testWebDebugUnitTest`
or `:app:testFdroidDebugUnitTest` (same skip flags). Record the task
name that actually ran.

For Gradle: parse `TEST-*.xml` `tests=` `failures=` `errors=`.

If Gradle/SDK is broken at run time, residual JVM as environment; C++
suites remain mandatory.

## Commit strategy

1. Commit 1 (this plan + evidence-log template): `[docs] Add SP-087 Phase 9
   validation plan`. Body includes `Work item: SP-087`. `git commit -s`.
   Then `git push -u origin cursor/sp-087-phase9-validation-db9d`.
2. Run the test commands. Save transcripts. Fill the evidence log, set
   work-item Status to **In review**, refresh phase-09 **observed-state**
   column only. Do **not** mark phase exit Met.
3. Commit 2: `[docs] Record SP-087 Phase 9 evidence`. Body
   `Work item: SP-087`. Push. Stop.

Never stage `3party/healpix/healpix`, `data/area_milestones.db`,
`data/live_recency.db`, `data/street_stats.db`, `data/weekly_city_live.db`.
Stay on `cursor/sp-087-phase9-validation-db9d`. Do not create GitHub PRs.
Do not start another work item. Do not force-push. Do not amend.

## Out of scope / non-goals for this gate

- New features beyond defect fixes that block listed suites.
- Marking SP-087 or Phase 9 **Accepted**. Filling Accepted by.
- Setting phase Status to **Exit criteria met**.
- Editing README §4 to claim Phase 9 **Exit criteria met**. (Independent
  review may refresh §4 to In progress / evidence recorded without Met.)
- Changing G1–G10 from **Open**.
- Weakening, skipping, deleting, or narrowing Eligibility or
  `Gpx_ImportExport` tests.
- Inventing `data/classificator.txt`.
- Compiling out debug entitlement.
- Retargeting GPX goldens (`creator=`).
- Adding chunking.
- Fabricating device walks, screenshots, APK dumps, or `nm` of a public APK.
- Map screenshots.
- Billing / iOS (SPD-010).
- Explorer pytest / backend.
- Editing `docs/STREET_PIXELS_PRODUCT_SPEC.md` or
  `docs/street-pixels-technical-audit.md`.
- Starting SP-088 / Phase 10.

## Discovered-follow-up placeholders (pre-fill; do not silently drop)

| Finding | Disposition |
| --- | --- |
| No handset: M1–M7 | Phase 10. Map screenshots forbidden. |
| `adb` / public APK inflated settings dump / share-sheet VIEW | Phase 10 |
| Debug-entitle grant symbols in native / public APK `nm` | SP-083 follow-up; Phase 10 APK |
| `Gpx_ImportExport_*` Streifzug vs Organic Maps | Pre-existing residual; do not change writer |
| `Gpx_ColorMapExport_Test` if same creator byte | Same residual family |
| `BookmarkManagerGpxGateTest` UnsatisfiedLinkError | Environment residual (SP-084) |
| `data/classificator.txt` Eligibility abort | Environment residual; do not weaken |
| `prefs_gpx.xml` in public resources | Expected; dump inflated tree on device |
| Analytics upload | Phase 10 |
| Desktop/Qt ungated C++ GPX prepare | SP-083 residual |
| iOS GPX ungated | Out of Android V1 |
| `ReloadBookmarkRoutine` omits `historicalTracks` | SP-081/085 follow-up; not this item |
| Multi-category “GPX” export is KMZ | SP-086 follow-up |
| G1–G10 still Open | Maintainer; coding used recommended locks |
| README §4 Phase 9 “Not started” vs SP-081–086 Accepted | Independent review: refresh §4 to In progress / evidence recorded; **not** Met |
| Phase 9 exit Met? | Maintainer only |
| Import on GUI thread | SP-085; no chunking after measurement |
| `WITH_SYSTEM_PROVIDED_3PARTY` expat GE/DTD | SP-085 residual |
| `DeserializerKml` still logs whole file | Later robustness |

## Inventory of UNIT_TEST names (not pass counts)

**HistoricalImport (11):** FirstImportEverLiveClear,
LiveThenImportRemainsEverLive, MultiSegmentGapIsNotFilled,
DuplicateGeometryHashSkipsSecondPaint, UpdateExploredPixelsDoesNotPaint,
InvalidCoordinatesAreSkipped, EmptyOrInvalidGeometryDoesNotPaint,
SegmentBoundariesChangeGeometryHash, NotReadyDoesNotWriteLedger,
TenThousandPointsCompletes, FiftyThousandPointsCompletes.

**IsolationHistoricalImport (16):** MarksExploredNeverLive,
PersonalCompletionIncrements, NoRecencyWeeklyOwnershipOrPending,
OwnershipZeroAtFullPersonal, ThenLiveCountsOnce,
LiveThenImportLeavesRecencyUnchanged, FirstGoalDoesNotAdvance,
CompetitionHintDoesNotAdvance, NotRecordingZeroHaptic,
GateUnavailableNotEntitled, GateUnavailableEntitled,
GateAvailableNotEntitled, GateAvailableEntitled,
LiveThenImportWhenAvailableEntitled,
InvalidGeometryDoesNotTouchCompetition, MalformedGpxDoesNotTouchCompetition.

**ExplorerPro_ (12):** DefaultCapabilitiesUnavailable,
StubEntitlementNeverGrants, GateUnavailableNotEntitled,
GateUnavailableEntitled, GateAvailableNotEntitled, GateAvailableEntitled,
GateMatrixAllCapabilities, DebugEntitlementSourceUsed,
DebugEntitlementSourceNotUsed, DebugEntitlementSourceStubRestored,
FrozenKeepsEnabledState, FrozenIgnoresDebugInstall.

**GpxGate (7):** HandlerClosedDoesNotPaint,
HandlerUnavailableEntitledDoesNotPaint,
HandlerAvailableNotEntitledDoesNotPaint, HandlerOpenPaints,
ExportFourCell, BatchFourCell, DirectImportClosedStillPaints.

**ExplorerProAnalytics (13):** DefaultZero, RecordInfoWhenAnyAvailable,
RecordInfoFailClosedWhenAllUnavailable, RecordImportWhenAvailable,
RecordImportWhenAvailableNotEntitled, RecordExportWhenAvailable,
RecordExportNoOpWhenUnavailable, UnavailableLeavesStoredZero,
PersistRoundTrip, ResetIsolatesTests, SnapshotHasNoLocationKeys,
HandlerDoesNotWrapManager, HandlerIncrementsAfterImport.

**XmlParser_ (5):** SmokeTest, LongTest, WellFormedPopsRoot,
TruncatedDocument, EmptyDocument.

**JVM:** ExplorerProGateTest 10 (inventory; SP-087 evidence **11**);
GpxSettingsVisibilityTest 12;
ExplorerProAnalyticsTest 2; BookmarkManagerGpxGateTest 6 (JNI residual).
