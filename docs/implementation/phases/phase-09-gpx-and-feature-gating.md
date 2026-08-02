# Phase 9 — GPX and feature gating

**Status:** Not started
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

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| GPX serialisation | `libs/kml/serdes_gpx.cpp` | Import and export both present |
| GPX tests | `libs/kml/kml_tests/gpx_tests.cpp` | Substantial existing coverage |
| Android import | multi-URI `importBookmarksFiles` | Batch import path exists |
| Track-to-pixel replay | `libs/map/street_pixels_manager.cpp` `UpdateExploredPixels`, `UpdateStreetStatsForTrack` | Imported tracks set explored (ever-live clear per SP-016). Sampling currently every 10 m via `kInterpolationStepMeters` / hard-coded `10.0`; **SP-019 / SPD-019 unifies to 15 m** with derivation. |
| Processed-track ledger | `street_stats.db` table `processed_tracks(geometry_hash, country_id)` | Prevents reprocessing the same track geometry |
| Imported marking | — | Not found. No source distinction exists. |
| Pro gate | — | Not found. GPX import and export are currently free and always available. |
| Entitlement | — | Not found |
| Android billing | — | Not found |

**Difference from the technical audit:** none material.

## Intended outcome

- A dedicated GPX import pipeline distinct from live collection, which sets
  `source = imported`, never writes recency, and never enqueues a competition
  upload.
- GPX import and export gated by build flag plus entitlement, with public V1
  shipping the flag off.
- No purchase action visible in public builds.
- Competition isolation proven by test, not by convention.

## Dependencies

- Phase 3, for the per-pixel source flag. Without it there is nothing to mark.
- Phase 1 SP-005, for the flag and entitlement abstraction.

## Proposed work-item breakdown

Not yet decomposed. Likely shape:

1. Dedicated imported-track pipeline setting `source = imported`.
2. Competition isolation: imports never create or refresh recency and never
   enqueue an upload.
3. Apply the build-flag plus entitlement gate to import, export, and
   track-management surfaces.
4. Settings surface for GPX tooling, shown only when the gate opens.
5. Chunked processing for large imports, if measurement shows it is needed.
6. Monetisation analytics, active only when the flag is on.

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
- `processed_tracks` prevents duplicate processing by geometry hash. Confirm the
  hash is stable and that reimporting a modified track behaves sensibly.
- Large imports (10,000-plus points) must not exhaust memory.
- Historical GPX timestamps are sparse and irregular. Live interpolation rules
  from Phase 2 must not be applied blindly to imports.

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
- SP-005 merged.

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
- Whether the existing bookmark import path can be reused with a flag, or
  whether a separate import entry point is cleaner. Ordinary bookmark import
  must remain free.
- Whether importing a GPX track should also create a stored track the user can
  inspect and delete, or only affect pixels.
- Whether `geometry_hash` in `processed_tracks` is robust enough to prevent
  duplicate imports of a re-exported track.
- What the entitlement stub looks like such that it is obviously inert in a
  public build.
