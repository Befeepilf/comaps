# SP-054 — Spike: exploration-aware routing measurement

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Branch:** (this planning branch until the spike runs)
**Depends on:** Phase 3 exit met. Does **not** depend on Phase 5 or on OQ-2
  being Accepted (record which explored set was used).
**Unblocks:** SP-055 Group B (R12 / lookup cost); SP-056+ coding gate together
  with SP-055 Group A

---

## Objective

Record Spike 7 evidence: compare avoid-mode route length and compute time
against standard routing on real walk/bike data, including a forced
disconnected (no fully unexplored path) case, so SP-055 can lock the avoid
algorithm without guessing.

## Motivation

Phase 6 entry requires a routing measurement. Audit Spike 7 asked whether
avoid-explored can ship with safe fallback without pathological routes. Spec
§17.3 and SPD-009 require a strict unexplored route **or** an explicit
fallback — never a silent 10× prefer. Without numbers, SP-057 cannot choose
true exclusion vs a large finite penalty, and cannot bound lookup cost.

## In-scope behavior

- Measure on at least one real pedestrian (and if practical bicycle) OD pair
  in a partly explored city-scale graph (Helsinki / Uusimaa-class preferred).
- Compare, for the same OD:
  1. Standard routing (exploration weights off).
  2. Prefer-unexplored at current default strength 50 and at strength 100.
  3. Avoid prototype A: very large finite penalty on edges with
     `exploredRatio > 0`.
  4. Avoid prototype B: true exclusion of those edges (no path if
     disconnected).
- Forced disconnected case: mark a cut set explored so no unexplored OD path
  exists. Record that B returns no path, A still returns a route, and the
  extra latency versus standard.
- Record route length (and if cheap, explored metres) and wall-clock route
  computation for each variant.
- Spike 7 pass bar (audit): prefer mode stable; avoid always returns a route
  **or** an explicit fallback signal; extra compute **&lt;2 s** versus
  standard on the measured OD. Pathological detours are a recorded outcome,
  not an automatic V1 cut — SPD-009 already declined deferring Avoid.
- Primary explored set for measurement: **`IsExplored()`** (personal,
  including imported). Optionally report a live-only (`IsEverLive()`)
  sensitivity if cheap; do not block the spike on OQ-2.
- Desktop IndexRouter / existing routing test harness is acceptable as
  primary. Device residual → Phase 10; do not fabricate device numbers.
- Write results under `docs/implementation/spikes/SP-054-exploration-routing.md`
  (or this work item’s completion evidence).
- Recommendation inputs for SP-055 R12: exclusion vs large penalty for the
  strict pass; whether lookup cost needs a cache (R11).

Prototype hooks in this item must be clearly scoped for measurement. They
must not ship Avoid UX or silently change production defaults.

## Out-of-scope behavior

- Shipping Avoid to users (SP-057+).
- Android walk/bike options UI (SP-056 / SP-058).
- Accepting OQ-2 (SP-055 R1).
- World-dataset `routing_integration_tests` as a required gate.
- Weakening Spike 7 criteria to “pass”.
- Marking Phase 6 entry Met without recorded measurement **or** an explicit
  residual (SP-033 pattern).

## Relevant product requirements

- Spec §17.2–§17.3, §31, §34 Routing.
- SPD-009 (Avoid remains in V1; no silent degrade).
- Audit Spike 7; phase-06 entry criterion “routing measurement”.
- OQ-2 (measurement notes which set was used).

## Relevant source files or symbols

- `IStreetExplorationWeights`, `ApplyStreetExplorationMultiplier`
- `StreetPixelsManager::GetSegmentExplorationWeightMultiplier`
- `IndexRouter`, `PedestrianEstimator` / `BicycleEstimator`
- `libs/routing/routing_tests/index_graph_tools.*` (synthetic graphs)
- Optional: desktop routing against an installed MWM + `.pix`

## Implementation notes / constraints

- Offline-first: installed map + local `.pix`; no network beyond obtaining
  map data beforehand.
- Do not upload tracks or coordinates in evidence beyond what is needed to
  reproduce (place names / MWM id / approximate OD description are enough;
  no raw GPS dumps in the work item).
- If mid-tier Android is unavailable: desktop measurement is primary; label
  device as Phase 10 residual.
- Country-mismatch early-return (`m_countryId`) will bias cross-leaf ODs
  toward standard weights — prefer a **single-leaf** OD for the headline
  numbers, and note the bias if a multi-leaf OD is used.

## Acceptance criteria

1. Written measurement with length and compute-time for standard, prefer,
   avoid-penalty, and avoid-exclusion, plus a disconnected case.
2. Explicit pass / fail / residual against Spike 7 extra-latency bar.
3. R12 recommendation inputs recorded (exclusion vs penalty for strict pass).
4. Explored-set used (`IsExplored` vs optional live-only) stated.
5. If device deferred: Phase 10 residual stated; desktop labeled primary.

## Required automated tests

- None mandatory (spike). Prefer a reproducible command or harness so the
  measurement can be re-run.

## Required manual validation

- Execute the OD procedure on the chosen target; record evidence.

## Failure and rollback considerations

- A pathological detour or &gt;2 s extra is a **successful spike outcome** if
  numbers and a recommended mitigation (exclusion + fallback, cache, OD
  limits) are recorded. It does not by itself defer Avoid (SPD-009).
- Do not change production prefer defaults in this item except a reversible
  measurement hook.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Target (desktop / device) | |
| MWM / city | |
| OD description | |
| Explored-set used | |
| Standard length / time | |
| Prefer (50) length / time | |
| Prefer (100) length / time | |
| Avoid penalty length / time | |
| Avoid exclusion length / time or NoPath | |
| Disconnected case | |
| Pass / fail / residual vs &lt;2 s extra | |
| R12 recommendation inputs | |
| Phase 10 residual | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during the spike) | |
