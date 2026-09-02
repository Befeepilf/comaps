# Phase 9 — GPX and feature gating

**Status:** Exit criteria met 2026-08-28 with residuals (device / APK / upload / Eligibility env → Phase 10).
**Depends on:** Phase 3, Phase 1 (SP-005)
**Blocks:** nothing; required for release

---

## Objective

Route GPX import through a dedicated historical-import path that marks pixels
as imported and never touches competition, and place GPX tooling behind the
build-flag plus entitlement gate so that public V1 exposes no non-functional
purchase action.

## Product-spec references

- §4.1 The dedicated explorer imports and exports tracks when Explorer Pro is
  enabled in the build.
- §7 Explorer Pro; imported GPX data is excluded from competition regardless of
  feature-flag or entitlement state.
- §15.3 Imported-pixel behaviour: green, counts toward personal completion,
  marked imported, no recency timestamp, no weekly contribution, no ownership.
- §16.1 Imported GPX data is processed separately as personal historical data.
- §22.2 Competitive pixel set excludes imports.
- §24.1 Weekly city leaderboard excludes imports.
- §29, §29.1, §29.2, §29.3, §29.4 Free versus Pro features; commercial model;
  monetisation principle.
- §30 Settings; GPX tools appear only when the flag and entitlement allow.
- §32.5 Monetisation analytics measured only when Pro is enabled in a build.
- §34 "Explorer Pro and monetization" launch requirements.

## Technical-audit references

- §13 GPX tooling: existing import and export, the gap table, and the
  recommendation to build a dedicated importer.
- §16 Monetization and sharing: no entitlement abstraction, no Android billing.
- Spike 9.

## Current code locations

Verified 2026-07-25 against the working tree. **Re-verified 2026-08-28
post SP-086 `d0d815832` / SP-087 evidence `5ed5e6df2`.** Extra detail in
[`notes/SP-080-gpx-feature-gating-architecture.md`](../notes/SP-080-gpx-feature-gating-architecture.md).
Observed-state only, re-verified 2026-08-28 after Phase 9 residual close-out.
G1–G10 are **SPD-067–076**. Phase 9 exit is **Met with residuals**.

| Concern | Location | Observed state |
| --- | --- | --- |
| GPX serialisation | `libs/kml/serdes_gpx.cpp` | Import and export present. Writer `creator="Streifzug"`. No file-size cap (10k/50k measured; no chunking). Parse failure logs size + prefix, not the whole file. Invalid lat/lon skipped before `FromLatLon`. Truncated/empty XML fails (`ParseXML` `isFinal`). Timestamp repair is display/metadata, not pixel placement. |
| KML parse-failure log | `libs/kml/serdes.hpp` `DeserializerKml` | Same `LogXmlParseFailurePrefix` as GPX (size + 256-byte prefix). |
| GPX tests | `libs/kml/kml_tests/gpx_tests.cpp` (binary **`kml_tests`**) | Substantial coverage: malformed, skip, entity, 10k/50k parse. Roundtrip goldens use `creator="Streifzug"`. |
| Android import | `Factory.KmzKmlProcessor`, `BookmarkManager.importBookmarksFile`, Favorites Import, manifest VIEW/SEND `application/gpx` | **Gated.** `BookmarkManager.importBookmarksFile` returns false for `.gpx` when `!ExplorerPro.isGpxImportEnabled()`. Batch: `allowGpxInBatch`. `getBookmarksExtensions()` is lazy (JVM gate tests do not load JNI). `Factory.KmzKmlProcessor` still forwards VIEW/SEND/SEND_MULTIPLE; handler no-ops GPX when closed. Manifest GPX filters remain (G6). KML/KMZ bookmark import remains. |
| Android export | `PlacePageView`, `BookmarksListFragment`, `BookmarkCategoriesFragment` `export_file_gpx` | **Ungated (SPD-097).** Those surfaces add `export_file_gpx` when `ExplorerPro.isGpxExportEnabled()` (native-ready, not Pro). JNI `Prepare*FileForSharing` writes GPX without `Capability::GpxExport`. C++ `ExportSingleFileGpx` serialises. Import remains gated. |
| Track-to-pixel replay | `StreetPixelsManager::UpdateExploredPixels`; dedicated `ImportHistoricalTrack` | `UpdateExploredPixels` is a no-op. Dedicated `ImportHistoricalTrack` samples 15 m per segment (`ComputeTrackPixels`), `MarkExploredPixelIds` (explored, never ever-live). Framework handler: GUI gate, then File-thread `RunHistoricalImportIfEnabled` (import + `RecordGpxImportUsage`). Tests call the helper synchronously. Direct `ImportHistoricalTrack` stays ungated (data rule). Live `OnLocationUpdate` remains the only free pixel writer. `ReloadBookmarkRoutine` loads gated GPX but does **not** pass `historicalTracks` (accepted residual). |
| Processed-track ledger | `street_stats.db` `processed_tracks(geometry_hash, country_id)` | Unchanged: mercator x,y hash per country. Identical geometry skips. |
| Imported marking | `.pix` ever-live bit (SPD-015 / SP-016) | Ever-live clear on first explore via dedicated path. Import cannot clear later live. No `source=imported` enum. |
| Recency / weekly / upload | `LiveRecencyStore`, `WeeklyCityLiveStore`, `CompetitionUploadService` | Isolation asserted on `ImportHistoricalTrack` (SP-082) as a **data rule** (not wrapped in `IsCapabilityEnabled`). Four-cell Available×Entitled matrix keeps recency/weekly/ownership/pending clean. Live path and recording Finished only. |
| Pro gate | `libs/map/explorer_pro.*` + JNI + Java `ExplorerPro` | `IsCapabilityEnabled` = available ∧ entitled. **Production call sites exist** (Framework handler, BookmarkManager load/share, Android menus/settings). BuildConfig `EXPLORER_PRO_*` default false; `-PenableExplorerProCapabilities=true` sets capabilities true. |
| Entitlement | `StubEntitlementSource` / `DebugEntitlementSource` | Stub always false. `DebugEntitlementSource` / `InstallDebugEntitlementSource` / JNI grant compiled out unless `DEBUG`. Installed only when `EXPLORER_PRO_DEBUG_ENTITLE` and any capability (debug buildConfig; release/beta hardcoded false). Freeze after init. `UnfreezeConfigurationForTesting` remains. |
| Settings GPX | `DataManagementSettingsFragment` + `GpxSettingsFragment` | Nested Data Management entry added when `GpxSettingsVisibility.showGpxScreen`. Export row uses native-ready **Enabled** (**SPD-097**, always on after init). Import/batch use Pro **Enabled**; G8 info page uses **Available**. Public builds show the screen for export only. |
| Monetisation analytics | `street_pixels::ExplorerProAnalytics` | Count-only uint64 (`Explore.ProInfoViewed`, `Explore.GpxImportUsage`, `Explore.GpxExportUsage`). Increment when matching capability is **Available**. Upload residual Phase 10. Not Sentry. |
| Android billing | — | **Not found** (SPD-010). |

**Difference from the technical audit (2026-07-20):** entitlement
abstraction **exists** (SP-005, 2026-07-27). Imported marking **exists**
as ever-live-clear (SP-016). Sampling is 15 m, not 10 m (SP-019). Dedicated
importer **exists** (`ImportHistoricalTrack`; catch-all `UpdateExploredPixels`
is a no-op). GPX UX **is gated** for import on Android (SP-083/084); **GPX export is
free (SPD-097)**. Public-configured BuildConfig defaults are false for
Pro capabilities. A debug grant path **exists** for internal
debug builds only; grant symbols are compiled out of non-debug Android.
Isolation is asserted on the dedicated path (SP-082) regardless of gate.
10k/50k RSS measured under 256 MiB; no chunking (SP-085). Worldwide; no
city allowlist. G1–G10 closed as **SPD-067–076**. KML parse-failure logs
use the same prefix helper as GPX.

## Intended outcome

- A dedicated GPX import pipeline distinct from live collection, which
  explores with ever-live clear (imported-only), never writes recency, and
  never enqueues a competition upload.
- GPX import gated by build flag plus entitlement, with public V1
  shipping the flag off. GPX export is free (**SPD-097**).
- No purchase action visible in public builds.
- Competition isolation proven by test, not by convention.

## Dependencies

- Phase 3, for the per-pixel source flag. Without it there is nothing to mark.
  **Met 2026-08-03.**
- Phase 1 SP-005, for the flag and entitlement abstraction.
  **Met 2026-07-27.**
- Phase 8 stores (recency, weekly, upload) are used by SP-082 assertions
  when present; they are not an entry gate for SP-081.

## Work-item breakdown

Work-item planning 2026-08-28. Locks G1–G10 live in
[`SP-080`](../work-items/SP-080-gpx-feature-gating-architecture-decisions.md)
as **SPD-067–076**. Audit Spike 9 is **not** a separate item (G10 / SPD-076).

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-080](../work-items/SP-080-gpx-feature-gating-architecture-decisions.md) | Architecture decisions (**entry gate**) |
| 2 | [SP-081](../work-items/SP-081-dedicated-historical-import-pipeline.md) | Dedicated historical-import pipeline |
| 3 | [SP-082](../work-items/SP-082-competition-isolation-historical-import.md) | Competition isolation on the dedicated path |
| 4 | [SP-083](../work-items/SP-083-apply-pro-gate-to-gpx-surfaces.md) | Apply Pro gate to import, export, batch, share-sheet |
| 5 | [SP-084](../work-items/SP-084-gpx-settings-surface.md) | Settings surface when the gate opens |
| 6 | [SP-085](../work-items/SP-085-historical-import-robustness.md) | Untrusted input and large-import measurement |
| 7 | [SP-086](../work-items/SP-086-explorer-pro-monetisation-analytics.md) | Count-only monetisation analytics |
| 8 | [SP-087](../work-items/SP-087-phase9-end-to-end-validation.md) | Phase 9 end-to-end validation (**exit gate**) |

## Data and migration concerns

- Existing installs may already have pixels explored through track replay,
  indistinguishable from live pixels because no ever-live bit exists. When the
  bit is introduced in Phase 3, those pixels have no known provenance. Since
  there is no public user base yet, the safe default is to treat pre-existing
  explored pixels as imported-only (ever-live clear), which excludes them from
  competition rather than inflating it. Recorded in SPD-015 / SP-016.
- A pixel can be explored first by import and later live. Spec §15.2 requires
  later live visits to become competition-eligible. Phase 3 stores this as a
  single **ever-live** bit (SPD-015): import clears it only on first explore;
  live sets it and import must not clear it. No separate `both` state.
- `processed_tracks` prevents duplicate processing by geometry hash.
  **G3 / SPD-069:** keep mercator x,y-only hash; timestamp-only re-export
  skips; geometry edits reprocess.
- Large imports (10,000-plus points) must not exhaust memory. **SP-085**
  measures; chunks only if needed (G10 / SPD-076).
- Historical GPX timestamps are sparse and irregular. Live interpolation
  rules from Phase 2 must not be applied blindly to imports. **G5 /
  SPD-071:** 15 m geometric sampling per segment; no cross-segment fill.
- Catch-all bookmark replay of live-saved tracks can paint across pause
  segment joins via `Track::GetGeometry`. **G1 / SPD-067:** stop that
  painter; live collection remains the only free pixel writer.

## Privacy and security implications

- GPX files come from outside the app. Treat them as untrusted input: malformed
  XML, absurd coordinates, absurd timestamps, and very large files.
- Imported tracks are personal historical location data. They stay local, exactly
  like recorded tracks (spec §25.1).
- The competition-isolation rule is a privacy and fairness rule at once. It must
  hold regardless of flag or entitlement state, which means it belongs in the
  data layer, not in the gate.
- Monetisation analytics must not exist as events in a build where Pro is off.
- The entitlement stub must never be a route to granting entitlement in a public
  build.

## Automated testing strategy

- Importing a GPX track marks pixels `imported`, increases personal completion,
  and creates no recency entry.
- Importing a GPX track enqueues nothing for competition upload, asserted
  against the queue rather than the network.
- A pixel first imported then explored live becomes competition-eligible
  (ever-live set) and
  does not double-count for personal completion.
- A pixel first explored live then imported keeps its recency.
- Gate matrix: flag off plus no entitlement, flag off plus entitlement, flag on
  plus no entitlement all deny; only flag on plus entitlement allows.
- Malformed and oversized GPX inputs fail cleanly.
- Large-import memory behaviour.
- Existing `gpx_tests` must continue to pass.

## Manual validation strategy

- Import a real multi-hour GPX track and confirm the pixels turn green and the
  area percentage rises.
- With competition enabled, confirm the imported area produces no ownership
  change and no weekly leaderboard movement.
- Import a track covering an area already explored live and confirm the
  competitive position is unchanged.
- In a public-configured build, confirm no GPX import, export, or purchase entry
  point is reachable anywhere, including deep links and share-sheet targets.
- In a Pro-enabled internal build, confirm the tools appear and work.
- Batch-import several files at once.

## Entry criteria

- Phase 3 exit criteria met, with the source flag in place.
  **Met 2026-08-03.**
- SP-005 merged.
  **Met 2026-07-27.**
- G1, G5, G6, G7 locked before binary coding (SP-080). **Met 2026-08-28**
  (SPD-067, SPD-071, SPD-072, SPD-073).

## Exit criteria

1. GPX import marks pixels `imported` and contributes to personal completion.
2. Imported pixels never create or refresh recency, never contribute to weekly
   counts, and never affect ownership — proven by test, and holding regardless
   of flag or entitlement state.
3. GPX import, export, and track management are gated by build flag plus
   entitlement.
4. Public-configured builds expose no GPX tooling and no purchase action.
5. Large imports complete without memory exhaustion.
6. Malformed input is rejected cleanly.
7. Existing GPX tests still pass.
8. Monetisation analytics exist only when Pro is enabled in the build.

**Exit: Met with residuals 2026-08-28** (product-owner lock). Automated
exits 1–8 Pass; the following stay Phase 10 or accepted-as-is:

| Residual | Disposition |
| --- | --- |
| Device walks, public APK dump, share-sheet VIEW, internal Pro UX, multi-hour GPX, analytics upload, public APK `nm` | Phase 10. Do not fabricate. |
| `data/classificator.txt` / `sp010_gpstrack_test.bin` missing | Environment. Do not invent. Eligibility not weakened. |
| Desktop/Qt ungated C++ GPX prepare | Accepted Android-V1 residual (SPD-072). |
| `ReloadBookmarkRoutine` loads gated GPX but omits `historicalTracks` | Accepted. No paint on reload. |
| Multi-category export is KMZ, not GPX usage | Accepted (SPD-070 / SPD-075). |
| Global `FromLatLon` clamp | Accepted. GPX parser already skips invalid lat/lon. |
| `WITH_SYSTEM_PROVIDED_3PARTY` system libexpat | Accepted. In-tree Expat keeps GE/DTD off. |
| iOS GPX ungated | Out of Android V1. |

## Explicit non-goals

- Google Play Billing, purchase flow, purchase restoration, pricing, and store
  entitlement validation. Deferred by SPD-010.
- iOS StoreKit work.
- Additional export formats beyond what already exists.
- Making competition a paid feature. Spec §29.1 lists competition as free.
- Server-side entitlement validation.
- Removing the existing free bookmark and track import used for ordinary map
  bookmarks. Only *exploration-affecting* GPX tooling is gated.

## Known uncertainties

- ~~How the `both` source state is represented.~~ Resolved by SPD-015: single
  ever-live bit; no `both` state.
- ~~How to treat pixels explored before the source flag existed.~~ Default
  imported-only (ever-live clear) per SPD-015 / SP-016.
- ~~Whether the existing bookmark import path can be reused with a flag, or
  whether a separate import entry point is cleaner.~~ **Closed by SPD-067
  (G1 / OQ-20):** dedicated historical path; free KML/KMZ does not paint;
  live-saved tracks do not replay.
- ~~Whether importing a GPX track should also create a stored track.~~
  **Closed by SPD-068 (G2 / OQ-21):** yes; delete does not un-explore.
- ~~Whether `geometry_hash` is robust enough.~~ **Closed by SPD-069
  (G3 / OQ-22):** keep x,y-only hash.
- ~~What the entitlement stub looks like in a public build.~~ SP-005 stub
  is always-false. **Closed by SPD-073 (G7 / OQ-26):** debug
  entitlement for internal Pro builds only; stub never grants; grant
  symbols compiled out of non-debug Android.
- ~~Historical sampling vs live GPS rules.~~ **Closed by SPD-071 (G5 /
  OQ-24):** 15 m per segment; no live filters; no cross-segment fill.
- ~~Share-sheet GPX when the gate is closed.~~ **Closed by SPD-072 (G6 /
  OQ-25):** refuse GPX; no purchase CTA.
- ~~V1 advanced track management scope.~~ **Closed by SPD-070 (G4 /
  OQ-23):** batch import; no merge/split; own recordings stay free.
- ~~Information page and monetisation counters.~~ **Closed by SPD-074–
  075 (G8–G9 / OQ-27–OQ-28).**
- ~~Separate Spike 9.~~ **Closed by SPD-076 (G10 / OQ-29):** no;
  isolation → SP-082; memory → SP-085.
