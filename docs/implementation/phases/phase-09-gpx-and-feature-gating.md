# Phase 9 — GPX and feature gating

**Status:** SP-084 Accepted 2026-08-28; SP-085 next. G1–G10 still Open (OQ-20–OQ-29).
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
(Phase 9 work-item planning / SP-080).** Extra detail in
[`notes/SP-080-gpx-feature-gating-architecture.md`](../notes/SP-080-gpx-feature-gating-architecture.md).

| Concern | Location | Observed state |
| --- | --- | --- |
| GPX serialisation | `libs/kml/serdes_gpx.cpp` | Import and export both present. No file-size cap. Parse failure may `ReadAsString` the whole file. Timestamp repair is display/metadata, not pixel placement. |
| GPX tests | `libs/kml/kml_tests/gpx_tests.cpp` | Substantial existing coverage. No OOM / oversized cases. |
| Android import | `Factory.KmzKmlProcessor`, `BookmarkManager.importBookmarksFiles`, Favorites Import, manifest VIEW/SEND `application/gpx` | Ungated. Batch multi-URI exists. GPX re-saved as a KML category with tracks. |
| Android export | `PlacePageView`, `BookmarksListFragment`, `BookmarkCategoriesFragment` `export_file_gpx` | Ungated. |
| Track-to-pixel replay | `StreetPixelsManager::UpdateExploredPixels` → `ComputeTrackPixels` → `MarkExploredPixelIds` | Catch-all over every bookmark track. Explored only; never ever-live, recency, weekly, or upload. Sampling **15 m** (`kPathSamplingStepMeters`, SPD-019). `UpdateStreetStatsForTrack` exists but is commented out at the call site. `Track::GetGeometry` concatenates segments (possible corridor across a `trkseg` / pause gap). |
| Processed-track ledger | `street_stats.db` `processed_tracks(geometry_hash, country_id)` | Mercator x,y hash. Identical geometry skips; timestamp-only re-export skips; point edits reprocess. Track delete does not drop the row. |
| Imported marking | `.pix` ever-live bit (SPD-015 / SP-016) | Import-first → ever-live clear; live sets; import cannot clear. No `source=imported` enum. |
| Recency / weekly / upload | `LiveRecencyStore`, `WeeklyCityLiveStore`, `CompetitionUploadService` | Live path and recording **Finished** only. Import helpers already isolated; no GPX-file fixture yet. |
| Pro gate | `libs/map/explorer_pro.*` | `GpxImport` / `GpxExport` / `AdvancedTrackManagement`. `IsCapabilityEnabled` = available ∧ entitled. **No production call site.** Java setter JNI only. BuildConfig `EXPLORER_PRO_*` default false; `-PenableExplorerProCapabilities=true` sets all true. |
| Entitlement | `StubEntitlementSource` | Always false. Documented `ExplorerPro.Entitled` key unused. No debug grant path — internal capability-on builds still cannot open the gate. |
| Settings GPX | `DataManagementSettingsFragment` + `GpxSettingsFragment` | Nested Data Management entry added only when `GpxSettingsVisibility.showGpxScreen`. Tool rows use Enabled; G8 info page uses Available. Public (all Available false) adds nothing. |
| Monetisation analytics | — | **Not found.** Count-only pattern exists for cards and routing. |
| Android billing | — | **Not found** (SPD-010). |

**Difference from the technical audit (2026-07-20):** entitlement
abstraction **exists** (SP-005, 2026-07-27). Imported marking **exists**
as ever-live-clear (SP-016). Sampling is 15 m, not 10 m (SP-019). GPX UX
is still ungated. Catch-all bookmark replay is still not a dedicated
importer. Spike 9 isolation is largely a data-layer property; 10k-point
memory is unmeasured.

## Intended outcome

- A dedicated GPX import pipeline distinct from live collection, which
  explores with ever-live clear (imported-only), never writes recency, and
  never enqueues a competition upload.
- GPX import and export gated by build flag plus entitlement, with public V1
  shipping the flag off.
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

Work-item planning 2026-08-28. Recommended locks G1–G10 live in
[`SP-080`](../work-items/SP-080-gpx-feature-gating-architecture-decisions.md)
as draft **SPD-067–076** / **OQ-20–OQ-29**. Coding SP-081+ waits on
maintainer lock of G1, G5, G6, and G7 (those four block binary work).
Audit Spike 9 is **not** a separate item (G10).

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
  **Proposed G3:** keep mercator x,y-only hash; timestamp-only re-export
  skips; geometry edits reprocess.
- Large imports (10,000-plus points) must not exhaust memory. **SP-085**
  measures; chunks only if needed (G10).
- Historical GPX timestamps are sparse and irregular. Live interpolation
  rules from Phase 2 must not be applied blindly to imports. **Proposed
  G5:** 15 m geometric sampling per segment; no cross-segment fill.
- Catch-all bookmark replay of live-saved tracks can paint across pause
  segment joins via `Track::GetGeometry`. **Proposed G1:** stop that
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
- G1, G5, G6, G7 locked before binary coding (SP-080). **Not met.**

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
  whether a separate import entry point is cleaner.~~ **Proposed G1 / OQ-20
  (draft SPD-067):** dedicated historical path; free KML/KMZ does not paint;
  live-saved tracks do not replay. Awaiting maintainer lock.
- ~~Whether importing a GPX track should also create a stored track.~~
  **Proposed G2 / OQ-21 (draft SPD-068):** yes; delete does not un-explore.
- ~~Whether `geometry_hash` is robust enough.~~ **Proposed G3 / OQ-22
  (draft SPD-069):** keep x,y-only hash.
- ~~What the entitlement stub looks like in a public build.~~ SP-005 stub
  is always-false. **Proposed G7 / OQ-26 (draft SPD-073):** debug
  entitlement for internal Pro builds only; stub never grants.
- Historical sampling vs live GPS rules — **Proposed G5 / OQ-24 (draft
  SPD-071):** 15 m per segment; no live filters; no cross-segment fill.
- Share-sheet GPX when the gate is closed — **Proposed G6 / OQ-25 (draft
  SPD-072):** refuse GPX; no purchase CTA.
- V1 advanced track management scope — **Proposed G4 / OQ-23 (draft
  SPD-070):** batch import; no merge/split; own recordings stay free.
- Information page and monetisation counters — **Proposed G8–G9 / OQ-27–
  OQ-28.**
- Separate Spike 9 — **Proposed G10 / OQ-29:** no; isolation → SP-082;
  memory → SP-085.
