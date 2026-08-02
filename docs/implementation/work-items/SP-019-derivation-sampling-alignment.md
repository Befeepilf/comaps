# SP-019 — Unify path sampling at 15 metres

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Make derivation, live segment interpolation, and track/import sampling share
one **15.0 m** path-sampling step (SPD-019). Eliminate the 10 m live/track
constant and the hard-coded `10.0` literal so the valid-pixel universe and
collection sampling agree without densifying on-disk `.pix` files.

## Motivation

`kSegmentLengthMeters = 15.0` already drives `DeriveStreetPixelsFromFeatures` /
`SegmentizeStreet`. Live and track sampling use `kInterpolationStepMeters =
10.0`, and `UpdateStreetStatsForTrack` hard-codes `10.0`. Dual steps diverge
collection density from the derived universe.

Phase 3 originally planned moving derivation to ~10 m (spec §14). After
Uusimaa `.pix` ≈ 50 MB, densifying the universe was rejected. SPD-019 locks
V1 at **15 m everywhere** — a recorded divergence from the spec’s ~10 m
figure in favour of storage headroom and one constant.

## In-scope behavior

- Live interpolation and active track replay sample at 15 m (same value as
  derivation).
- One sampling constant (or two equal aliases with a single definition) —
  record choice in evidence.
- Remove or align the legacy `10.0` literal in `UpdateStreetStatsForTrack` so
  no third value remains.
- Update automated tests that asserted 10 m (including SP-011-era segment
  interpolation tests) to 15 m.
- Determinism: derive twice from the same fixture → byte-identical `.pix`
  payload (entries). Derivation step itself is unchanged at 15 m.

## Out-of-scope behavior

- Densifying derivation to 10 m (explicitly rejected by SPD-019).
- Changing `nside` (SPD-017 locked).
- Rematch implementation (SP-017 already landed) — no universe rebuild is
  required solely for this sampling unify.
- Eligibility policy (SP-020).
- Renderer LOD.
- Editing the product spec text (§14 remains ~10 m as product intent).

## Relevant product requirements

- §14.1–§14.3 sampling / determinism (V1 implements 15 m per SPD-019).
- SPD-019.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.cpp` — `kSegmentLengthMeters`,
  `SegmentizeStreet`, `DeriveStreetPixelsFromFeatures`,
  `ComputeTrackPixels`, `UpdateStreetStatsForTrack`
- `libs/map/live_segment_interpolation.{hpp,cpp}` — `kInterpolationStepMeters`
- `libs/map/street_pixels_tests/segment_interpolation_tests.cpp` and related

## Implementation notes / constraints

- Prefer one constant name used by derivation and segment sampling.
- Do not bump `.pix` format or force rematch for this change; existing 15 m
  universes remain valid.
- **Size watch:** Uusimaa stays at current ~50 MB class because derivation is
  unchanged. Record that SPD-019 avoids the 10 m densification risk.

## Acceptance criteria

1. Live and track sampling use 15 m; no remaining 10 m sampling constant for
   Street Pixels path sampling.
2. One sampling constant (or two equal aliases with a single definition).
3. Repeat-derivation determinism test passes (still 15 m derive).
4. SP-011 / segment-interpolation tests updated and green.
5. Evidence records constant choice and that `.pix` size is unchanged by design.
6. Covered by `street_pixels_tests`.

## Required automated tests

- Sampling step constant assertion (= 15.0).
- Deterministic double derive.
- Fixture / segment tests updated for 15 m step (expected sample counts).

## Required manual validation

- Short recorded walk still paints greens; sampling feels consistent with the
  existing red universe (spot check; no denser red field expected).

## Failure and rollback considerations

- Rollback: restore 10 m live/track constants; derivation already 15 m.
- No explored-bit wipe risk from this change alone.

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
