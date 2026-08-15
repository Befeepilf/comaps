# SP-054 — Spike: exploration-aware routing measurement

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Branch:** (this planning branch until the spike runs)
**Depends on:** Phase 3 exit met; SPD-040–045 (algorithm shape locked)
**Unblocks:** SP-056+ coding gate (measurement / residual)

---

## Objective

Record Spike 7 evidence for the **locked** Avoid shape: true exclusion of
edges with `exploredRatio == 1` (SPD-042), compared with standard and
Prefer, including a forced no-route case, so SP-057 ships with measured
detour, latency, and no-route frequency.

## Motivation

Phase 6 entry requires a routing measurement. Product already locked
exclusion at `exploredRatio == 1` and Prefer+strength fallback (SPD-042).
The spike no longer chooses the algorithm. It records whether that shape
stays within the Spike 7 extra-latency bar and how often no-route fires.

## In-scope behavior

- Measure on at least one real pedestrian (and if practical bicycle) OD pair
  in a partly explored city-scale graph (Helsinki / Uusimaa-class preferred).
- Compare, for the same OD:
  1. Standard routing (exploration weights off).
  2. Prefer-unexplored at strength 50 and at strength 100.
  3. Avoid: true exclusion of edges with `exploredRatio == 1`.
- Forced no-route case: mark a cut set **fully** explored so every remaining
  path has an `exploredRatio == 1` edge. Record NoPath and extra latency
  versus standard.
- Optional extra (not a decision input): large finite penalty on
  `exploredRatio == 1` edges, labelled as a sensitivity, not a candidate
  Avoid implementation.
- Record route length and wall-clock route computation for each variant.
- Spike 7 pass bar (audit): prefer mode stable; avoid returns a route **or**
  an explicit no-route signal; extra compute **&lt;2 s** versus standard on
  the measured OD. Pathological detours are a recorded outcome, not an
  automatic V1 cut — SPD-009 declined deferring Avoid.
- Explored set: **`IsExplored()`** (SPD-040).
- Desktop IndexRouter is acceptable as primary. Device residual → Phase 10.
- Write results under `docs/implementation/spikes/SP-054-exploration-routing.md`
  (or this work item’s completion evidence).
- Record lookup-cost notes for SPD-045 (per-leaf `.pix`).

Prototype hooks must be scoped for measurement. They must not ship Avoid UX
or silently change production defaults.

## Out-of-scope behavior

- Shipping Avoid to users (SP-057+).
- Android walk/bike options UI (SP-056 / SP-058).
- Re-opening SPD-042 (`exploredRatio == 1`, no min-connection).
- World-dataset `routing_integration_tests` as a required gate.
- Weakening Spike 7 criteria to “pass”.
- Marking Phase 6 entry Met without recorded measurement **or** an explicit
  residual (SP-033 pattern).

## Relevant product requirements

- Spec §17.2–§17.3, §31, §34 Routing.
- SPD-009 (Avoid remains in V1; no silent degrade).
- SPD-040–045.
- Audit Spike 7; phase-06 entry criterion “routing measurement”.

## Relevant source files or symbols

- `IStreetExplorationWeights`, `ApplyStreetExplorationMultiplier`
- `StreetPixelsManager::GetSegmentExplorationWeightMultiplier`
- `IndexRouter`, `PedestrianEstimator` / `BicycleEstimator`
- `libs/routing/routing_tests/index_graph_tools.*`
- Optional: desktop routing against an installed MWM + `.pix`

## Implementation notes / constraints

- Offline-first: installed map + local `.pix`.
- Do not upload tracks or coordinates in evidence beyond what is needed to
  reproduce (place names / MWM id / approximate OD description).
- Prefer a **single-leaf** OD for headline numbers (SPD-045 mismatch bias).

## Acceptance criteria

1. Written measurement with length and compute-time for standard, prefer
   (50 and 100), and Avoid exclusion at `exploredRatio == 1`, plus a
   no-route case.
2. Explicit pass / fail / residual against Spike 7 extra-latency bar.
3. No-route frequency / detour notes recorded for SP-057.
4. Explored-set used stated (`IsExplored()`, SPD-040).
5. If device deferred: Phase 10 residual stated; desktop labeled primary.

## Required automated tests

- None mandatory (spike). Prefer a reproducible command or harness.

## Required manual validation

- Execute the OD procedure on the chosen target; record evidence.

## Failure and rollback considerations

- A pathological detour or &gt;2 s extra is a **successful spike outcome** if
  numbers are recorded. It does not by itself defer Avoid (SPD-009) or
  reopen `exploredRatio == 1` (SPD-042).
- Do not change production prefer defaults except a reversible measurement
  hook.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Target (desktop / device) | |
| MWM / city | |
| OD description | |
| Explored-set used | `IsExplored()` (SPD-040) |
| Standard length / time | |
| Prefer (50) length / time | |
| Prefer (100) length / time | |
| Avoid exclusion (`ratio == 1`) length / time or NoPath | |
| Forced no-route case | |
| Pass / fail / residual vs &lt;2 s extra | |
| Phase 10 residual | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during the spike) | |
