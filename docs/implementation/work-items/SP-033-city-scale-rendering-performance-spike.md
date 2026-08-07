# SP-033 — Spike: city-scale street-pixel rendering performance

**Phase:** 5 — Area progress and map interaction
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** Phase 4 Accepted (exit criteria met 2026-08-07)
**Unblocks:** SP-034+ coding; SP-037 LOD decisions

---

## Objective

Measure street-pixel overlay rendering performance at city scale on a mid-tier
Android device (Spike 1 criteria), and record whether the current
one-circle-per-cell renderer meets the pass bar or needs LOD / aggregation
work. Desktop secondary measurement is acceptable if device access is deferred;
device residual honesty follows the Phase 4 R3 → Phase 10 pattern.

## Motivation

Phase 5 entry requires a rendering performance measurement before area UI and
boundary shading commit to a renderer strategy. Spec §34 Quality and audit
Spike 1 define the bar. Without numbers, SP-037 LOD and SP-041 exit #7 are
guesses.

## In-scope behavior

- Load a large city (Helsinki / Uusimaa-class preferred) with street-pixel
  overlay enabled.
- Measure frame times while panning/zooming at zoom **14–16**; report p95 FPS
  (or equivalent frame-time p95).
- Measure memory uplift attributable to the street-pixel overlay (&lt;150 MB
  pass criterion).
- Spike 1 pass criteria: **≥30 FPS at the 95th percentile** at zoom 14–16 with
  a city loaded; **memory uplift &lt;150 MB**.
- Record device model, OS, build type, city/MWM, procedure, and results in this
  work item's completion evidence (or a linked spike note under
  `docs/implementation/spikes/`).
- If mid-tier Android is unavailable: run a **desktop secondary** measurement,
  document methodology limits, and residual the device run to Phase 10. Do not
  fabricate device numbers.
- Recommendation inputs for SP-037: keep current renderer / add LOD /
  aggregate / other — grounded in the numbers.

## Out-of-scope behavior

- Shipping LOD or renderer rewrites (those land in SP-037+ if needed).
- Area progress UI, focus engine, completion cache (SP-034+).
- Marking Phase 5 entry rendering criterion Met without recorded measurement.
- Weakening Spike 1 criteria to pass.

## Relevant product requirements

- Spec §34 Quality; §12 zoom behaviour context.
- Audit Spike 1 pass criteria (≥30 FPS p95 zoom 14–16; &lt;150 MB uplift).
- Phase 5 entry criterion: rendering performance measurement.

## Relevant source files or symbols

- `libs/drape_frontend/street_pixel_renderer.cpp` / `.hpp`
- `libs/drape_frontend/street_pixel.cpp`
- `android/sdk/.../maplayer/Mode.java` `STREET_PIXELS`
- `StreetPixelsManager` pixel feed into drape

## Implementation notes / constraints

- Prefer mid-tier Android (Pixel-class or similar) as primary; desktop Qt/GL is
  secondary only.
- Mirror Phase 4 SP-023/024 gate: **no SP-034+ product coding until this
  measurement is recorded**.
- Offline-first: measurement uses installed map + local `.pix`; no network
  requirement beyond obtaining map data beforehand.

## Acceptance criteria

1. Written measurement (device and/or desktop secondary) with FPS/frame-time and
   memory numbers at zoom 14–16 for at least one large city.
2. Explicit pass / fail / residual against Spike 1 criteria.
3. Recommendation inputs for SP-037 LOD recorded.
4. If device deferred: Phase 10 residual stated explicitly; desktop numbers
   labeled secondary.

## Required automated tests

- None mandatory (spike). Prefer a reproducible procedure so the measurement
  can be re-run.

## Required manual validation

- Execute the pan/zoom procedure on the chosen target; record evidence.

## Failure and rollback considerations

- Failing the FPS/memory bar is a successful spike outcome if numbers and a
  recommended mitigation path are recorded — feed SP-037.
- Do not change product renderer behaviour on this item unless a tiny
  instrumentation hook is required for measurement and is clearly scoped.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Device / OS / build | |
| City / MWM | |
| FPS / frame-time (p95, zoom 14–16) | |
| Memory uplift | |
| Pass / fail / residual vs Spike 1 | |
| SP-037 recommendation inputs | |
| Desktop secondary (if any) | |
| Phase 10 residual (if device deferred) | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
