# SP-085 — Historical-import robustness

**Phase:** 9 — GPX and feature gating
**Status:** Planned
**Branch:** `cursor/phase-09-work-items-db9d`
**Depends on:** SP-080 G5, G10; SP-081 dedicated path
**Unblocks:** SP-087 exit 5–6 (large import, malformed input)

---

## Objective

Treat GPX as untrusted input: malformed XML, absurd coordinates, and
very large files fail cleanly without exhausting memory. Measure a
10,000-plus-point import; add chunked processing only if measurement
shows it is needed.

## Motivation

`DeserializerGpx` builds a full in-memory `FileData`. There is no size
cap. Parse failure may `ReadAsString` the whole file for logging. Replay
builds a full pixel `std::set` per track. Phase 9 exit requires clean
rejection and no memory exhaustion. Spike 9’s memory half was never
measured (G10).

## In-scope behavior

- Reject malformed GPX without crashing; no pixel writes; no competition
  side effects.
- Skip absurd coordinates (non-finite, |lat| > 90, |lon| > 180) per G5.
- Stop logging entire file bodies on parse failure for large input.
- Bound or stream work so a 10k-point (and a documented larger) fixture
  completes without allocator collapse on the desktop test host. Record
  peak RSS or an equivalent.
- If measurement shows a single `FileData` / pixel set cannot stay
  within a recorded budget, chunk by segment or point window **on the
  historical path only**. Do not silently drop points in the middle of a
  segment without a test.
- Existing `gpx_tests` continue to pass.
- Optional file-size or point-count cap: if added, fail cleanly with a
  user-visible error in Pro builds; never a purchase CTA.

## Out-of-scope behavior

- Changing live GPS interpolation (Phase 2).
- Pro UI (SP-083/084).
- A general XML sandbox rewrite of `ParseXML` unless the current reader
  cannot fail cleanly.

## Relevant product requirements

- Spec §16.1, §25.1, phase-09 privacy (untrusted GPX).
- Phase 9 exit 5–6, automated strategy bullets 6–8.

## Relevant source files or symbols

- `libs/kml/serdes_gpx.cpp` `DeserializerGpx`, `GpxParser`,
  `CheckAndCorrectTimestamps`
- `libs/map/bookmark_helpers.cpp` `LoadKmlFile`
- SP-081 `ComputeTrackPixels` / historical importer
- `libs/kml/kml_tests/gpx_tests.cpp`

## Implementation notes / constraints

- Untrusted input: do not execute entity expansion bombs if the parser
  allows them; record the parser’s entity policy.
- Historical timestamps stay unused for pixel placement (G5).
- Tests belong in `gpx_tests` and/or `street_pixels_tests`; do not
  require a device.
- If chunking is **not** needed, record the measurement in completion
  evidence and do not add speculative chunking.

## Acceptance criteria

1. Malformed GPX fails cleanly; pixels and competition stores unchanged.
2. Oversized / 10k-point import either completes within the recorded
   memory budget or is rejected cleanly; no abort/OOM in the test.
3. Chunking exists only if measurement required it; evidence says which.
4. `gpx_tests` still pass.
5. Parse-failure logging does not dump multi-megabyte payloads.

## Required automated tests

- Truncated XML, empty file, non-GPX bytes, namespace-odd but valid
  fixtures already in `gpx_tests` (keep).
- New: coordinates out of range skipped; 10k-point synthetic import
  completes or hits the cap.
- Import failure does not set upload pending or recency.

## Required manual validation

- Multi-hour real GPX in a Pro build (SP-087). Device residual → Phase
  10.

## Failure and rollback considerations

- Prefer reject-with-error over partial paint of a truncated file unless
  tests define prefix-complete semantics.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | — |
| 10k-point measurement | — |
| Chunking implemented? | — |
| Test output | — |
| Accepted by | — |
| Accepted date | — |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
