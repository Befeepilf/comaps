# SP-019 — Align derivation sampling to ~10 metres

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Make on-device street-pixel derivation sample at the spec's ~10 metre step and
eliminate divergent hard-coded sampling constants so one constant defines the
valid-pixel universe.

## Motivation

`kSegmentLengthMeters = 15.0` drives `DeriveStreetPixelsFromFeatures` /
`SegmentizeStreet`. Spec §14 requires ~10 m path sampling. Live and track
sampling already use `kInterpolationStepMeters = 10.0`. Dual constants also
tie `street_exploration` bitmask indices to 15 m buckets.

Phase 3 requires this change in the **same release** as rematch so denominator
shifts are absorbed by the update story.

## In-scope behavior

- Derivation sampling uses 10 m (shared named constant with the interpolation
  step, or a single derivation constant equal to it — record choice).
- Remove or align the legacy `10.0` literal in `UpdateStreetStatsForTrack` so
  no third value remains.
- Any stats bitmask indexing that assumed 15 m is updated or explicitly
  invalidated/regenerated under rematch.
- Determinism test: derive twice from the same fixture → byte-identical `.pix`
  payload (entries).
- Fixture asserting expected pixel-count behaviour at 10 m vs old 15 m
  (document expected direction: denser universe).

## Out-of-scope behavior

- Rematch implementation (SP-017) — coordinate landing order only.
- Eligibility policy (SP-020).
- Renderer LOD.
- Changing `nside` (SPD-017 locked).

## Relevant product requirements

- §14.1–§14.3 ~10 m sampling; determinism.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.cpp` — `kSegmentLengthMeters`,
  `SegmentizeStreet`, `DeriveStreetPixelsFromFeatures`,
  `UpdateStreetStatsForTrack`
- `libs/map/live_segment_interpolation.hpp` — `kInterpolationStepMeters`
- `libs/map/street_pixels_tests/*`

## Implementation notes / constraints

- Prefer one constant name used by derivation and segment sampling.
- Expect rematch (or first load after upgrade) to rebuild universes; do not
  silently keep 15 m files without a version bump story (SP-015 map-data /
  format interaction — record how rebuild is triggered).
- **Size watch:** Uusimaa `.pix` is already ~50 MB at 15 m derivation. 10 m
  sampling densifies the valid universe. Measure cell count / file size before
  and after on a Uusimaa-class (or largest available) region and record in
  evidence. If growth is severe, report — do not silently accept unbounded
  expansion; maintainer decides. `nside` stays locked (SPD-017); do not
  “fix” size by changing grid resolution.

## Acceptance criteria

1. Derivation samples at 10 m; no remaining 15 m derivation constant.
2. One sampling constant (or two equal aliases with a single definition).
3. Repeat-derivation determinism test passes.
4. Landing is ordered for the same release as SP-017 rematch.
5. Before/after size (or cell count) recorded for at least one regional-scale
   fixture or device region.
6. Covered by `street_pixels_tests`.

## Required automated tests

- Deterministic double derive.
- Sampling step constant assertion.
- Fixture geometry pixel-count at 10 m (expected value recorded in test).

## Required manual validation

- After update/rebuild, visual density of red pixels on a known street looks
  consistent with denser sampling (spot check; quantitative proof is
  automated).

## Failure and rollback considerations

- Universe change without rematch would desync explored bits; do not ship this
  without SP-017 in the same release train.
- Rollback requires reverting constant and regenerating.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Constant choice | |
| Test output | |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
