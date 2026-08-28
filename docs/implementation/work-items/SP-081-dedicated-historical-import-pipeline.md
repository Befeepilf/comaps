# SP-081 — Dedicated historical-import pipeline

**Phase:** 9 — GPX and feature gating
**Status:** Accepted
**Branch:** `cursor/sp-081-historical-import-db9d`
**Depends on:** SP-080 locks for G1, G2, G3, G5 (**SPD-067–069**, **SPD-071**)
**Unblocks:** SP-082 (isolation proofs on this API), SP-083 (call-site
  gate), SP-085 (chunking on this path)

---

## Objective

Replace catch-all bookmark-track replay with a dedicated historical-import
path that marks pixels imported-only (ever-live clear), samples each
track segment at 15 m, stores a local track, and never writes live-only
state.

## Motivation

`UpdateExploredPixels` paints every bookmark track, including live-saved
recordings and free KML imports. `Track::GetGeometry` concatenates
segments, so replay can fill a pause or `trkseg` gap. Spec §16.1 and
§15.3 require a separate historical path. SP-016’s interim policy is
imported-only semantics on that catch-all; Phase 9 replaces the catch-all.

## In-scope behavior

- Named shared-C++ entry (name as fits surrounding style) that, given
  track segment polylines:
  - samples **each segment independently** at `kPathSamplingStepMeters`
    (15 m, SPD-019)
  - `AddPixelsInRadius` (25 m) then `MarkExploredPixelIds` (explored,
    never ever-live, never recency / weekly / hint / first-goal /
    `MarkPending`)
  - skips non-finite / out-of-range coordinates
  - uses `processed_tracks` + `ComputeGeometryHash` (G3) per country
- Stop using `UpdateExploredPixels` as a painter over all bookmark
  tracks. Live `OnLocationUpdate` remains the only free pixel writer.
- Import still materialises a bookmark/track (G2). Deleting it does not
  un-explore or drop the ledger row.
- Do not apply live GPS acceptance, pause barriers, or GPX timestamp
  interpolation to pixel placement (G5). Serdes timestamp repair may
  remain for display.
- Existing `MarkImportedPixelsForTesting` / `MarkTrackPixelsForTesting`
  keep imported-only semantics; point them at the new path if that
  removes duplication.
- Personal completion increments (SPD-026). Routing `IsExplored()`
  unchanged (SPD-040).

## Out-of-scope behavior

- Pro gate on Android surfaces (SP-083). This item may expose the C++
  API ungated; callers that would paint from free bookmark import must
  already be removed.
- Isolation assertion suite against recency/weekly/upload (SP-082).
- Settings UI (SP-084).
- File-size caps, chunking, malformed-XML policy (SP-085).
- Monetisation counters (SP-086).
- Billing (SPD-010).

## Relevant product requirements

- Spec §7, §15.2–§15.3, §16.1, §29.2.
- SPD-015, SPD-019, SPD-026, SPD-040, SPD-011 (isolation is a data rule).
- SP-080 G1, G2, G3, G5.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.{hpp,cpp}` `UpdateExploredPixels`,
  `ComputeTrackPixels`, `ComputeGeometryHash`, `MarkExploredPixelIds`
- `libs/map/track.cpp` `Track::GetGeometry`
- `libs/map/framework.cpp` bookmark-created → `UpdateExploredPixels`
- `libs/map/bookmark_manager.cpp` `LoadBookmarkRoutine`,
  `SaveTrackRecording`
- `libs/kml/serdes_gpx.*` (parse only; do not change export formats)
- `street_stats` `processed_tracks`
- `libs/map/street_pixels_tests/ever_live_tests.cpp` and related

## Implementation notes / constraints

- Do not guess G1/G5. If still Proposed, stop.
- Shared C++ owns marking (SPD-002). Android only invokes the API.
- Offline-only. GPX files stay local (spec §25.1).
- `SetEverLive(false)` remains a no-op; do not add a clear-ever-live
  path.
- Additive module beside existing bookmark import is preferred over
  restructuring `BookmarkManager` unless G1 cannot be met otherwise.
- Do not log coordinates in release.

## Acceptance criteria

1. Dedicated path marks first-import pixels explored and ever-live
   clear; later live visits set ever-live; import never clears it.
2. Catch-all bookmark-track replay no longer paints pixels.
3. Live-saved recordings do not gain imported pixels across pause
   segment joins.
4. Sampling is 15 m per segment; no sample across segment joins.
5. Duplicate identical geometry in the same country is skipped.
6. Stored track exists; delete track leaves pixels and ledger.
7. `gpx_tests` and existing `EverLive_*` / track-replay tests still
   pass or are updated to the new API without weakening isolation.

## Required automated tests

In `street_pixels_tests` (and `gpx_tests` regression):

- Multi-segment fixture: no pixels along the straight join between
  segments when the gap exceeds a trivial adjacent step.
- First import → explored, ever-live clear; live then import → ever-live
  remains set.
- Duplicate geometry hash skips second paint.
- Live recording save does not call the historical painter.
- Existing `EverLive_TrackAloneLeavesClear` and
  `EverLive_TrackAfterLiveRemainsSet` still hold via the new path.

## Required manual validation

- Import a short real GPX in a Pro-enabled internal build (after
  SP-083/G7): pixels green, area % up. Device residual → SP-087 /
  Phase 10 if no handset.

## Failure and rollback considerations

- Prefer no historical paint over catch-all replay that fills pauses.
- Rollback of this item must not re-enable live-saved pause-gap fill.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-081-historical-import-db9d` |
| Test output | See executed output below |
| Accepted by | Product owner |
| Accepted date | 2026-08-28 |

## Executed test output

Cwd `/workspace`. Binary `/home/ubuntu/omim-build-debug/street_pixels_tests`. SHA `b33e8bc58`. `--data_path=/workspace/data --user_resource_path=/workspace/data`.

- `--filter=HistoricalImport` **9/9** All tests passed
- `--filter=EverLive` **18/18** All tests passed (includes `EverLive_TrackAloneLeavesClear` and `EverLive_TrackAfterLiveRemainsSet`)
- `--suppress=Eligibility` **420/420** All tests passed (pre-review SHA `2287e0541`; Eligibility abort is missing `data/classificator.txt`)
- Device GPX import residual → SP-087 / Phase 10

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Import still runs on the GUI thread | **Closed** 2026-08-28: Framework handler gates on GUI then `Platform::Thread::File` |
| Isolation vs recency / weekly / upload not asserted on this API | SP-082 |
| Live-save non-paint covered by call-site (`SaveTrackRecording` never invokes the handler), not a BookmarkManager unit test | SP-087 if a device proof is needed |
| Public GPX surfaces still ungated | SP-083 |
| `ReloadBookmarkRoutine` omits `historicalTracks` | **Accepted residual** 2026-08-28: reload loads gated GPX but does not paint |
| A-NaN-B geometry hashes equal to A-B (paint does not fill the gap) | Leave; distinguishing would change `HistoricalImport_InvalidCoordinatesAreSkipped` |
