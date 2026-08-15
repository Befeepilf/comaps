# SP-057 — Avoid-explored engine (strict pass + distinct no-route)

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Depends on:** SP-055 Group A Accepted; SP-054 recorded (R12); SP-056 mode
  model landed (or this item includes a minimal mode hook if SP-056 is not
  yet merged — prefer stacking after SP-056)
**Unblocks:** SP-058 fallback UX; SP-059; SP-061 exit #2

---

## Objective

Implement hard Avoid in the shared routing core: when Avoid is selected for
pedestrian or bicycle, the strict pass uses only edges with no matched
explored pixels, or returns a **distinct** no-route result. Never silently
degrade to Prefer or Standard.

## Motivation

SPD-009 and spec §17.3 require Avoid in V1. Today only a ≤10× soft
multiplier exists. A large penalty without a distinct failure still hides
explored edges inside a “successful” route. The engine must make
impossibility visible so SP-058 can offer the spec pair.

## In-scope behavior

- Avoid mode on pedestrian and bicycle estimators only (R2). Car remains
  Prefer-or-off.
- Strict pass per R5 + R12: exclude (or equivalent true skip) edges with
  `exploredRatio > 0`. Unmatched samples are not explored.
- If no path: new `RouterResultCode` (JNI-mirrored; `ResultCodes` Java).
  Must not reuse generic `RouteNotFound` as the Avoid-impossible signal.
- Weight / exclusion query uses R1 (`IsExplored()`).
- R11: segment-MWM `.pix` lookup when installed; missing file → not
  explored. Do not load extra leaves into the drape overlay.
- Fixture-graph tests in `routing_tests` and/or `street_pixels_tests`:
  unexplored path exists → it is used and explored edges are not;
  unexplored path absent → distinct code, no silent route.
- Production defaults: Avoid still needs SP-058 UI to be selectable; this
  item may expose a C++/JNI mode that Android wires in SP-058.

## Out-of-scope behavior

- Fallback dialog and min-connection retry (SP-058).
- Pre-use warning copy (SP-058).
- Mid-navigation freeze policy (SP-059) beyond not inventing a recompute
  loop in the estimator.
- Walk/bike toggle chrome (SP-056 / SP-058).
- Analytics (SP-060).
- True “infinite weight” without a no-path signal.

## Relevant product requirements

- Spec §17.3, §31, §34 Routing; SPD-009.
- SP-055 R1, R2, R5, R6, R11, R12.

## Relevant source files or symbols

- `IStreetExplorationWeights` (may need an avoid/exclude query or a
  multiplier of +inf / skip — additive extension preferred)
- `EdgeEstimator::ApplyStreetExplorationMultiplier`
- `IndexRouter` / A-star `NoPath` → `RouterResultCode`
- `routing_callbacks.hpp` enum (JNI mirror warning in-file)
- `android/sdk/.../ResultCodes.java`, `ResultCodesHelper.java`
- `StreetPixelsManager::GetSegmentExplorationWeightMultiplier`
- `libs/routing/routing_tests/index_graph_tools.*`

## Implementation notes / constraints

- Prefer additive hooks beside `IStreetExplorationWeights` over rewriting
  IndexRouter. If IndexRouter must distinguish Avoid-NoPath from other
  NoPath, say so in the PR: the result code is a product requirement (R6).
- ETA must remain unmultiplied (today’s `Purpose::Weight` only).
- Offline-only. No network for weights.
- Do not Pro-gate.

## Acceptance criteria

1. Fixture: unexplored route exists → Avoid chooses it; explored edges
   unused.
2. Fixture: no unexplored route → distinct result; no route returned as
   success.
3. Car / vehicle router does not apply Avoid exclusion.
4. R1: imported-only explored cells are avoided the same as live.
5. R11: installed `.pix` for the segment MWM is used.
6. `routing_tests` / `street_pixels_tests` named in the PR pass.

## Required automated tests

- Connected unexplored alternative.
- Disconnected unexplored graph → distinct code.
- Prefer mode still uses the soft multiplier (regression).
- Car estimator does not exclude on Avoid (or Avoid cannot be active).

## Required manual validation

- Deferred to SP-058 / SP-061 for UI; engine may be desktop-only here.

## Failure and rollback considerations

- Do not implement Avoid as “Prefer at strength 100”.
- Do not map Avoid-NoPath onto generic `RouteNotFound`.
- If R12 was revised after SP-054, implement the Accepted R12, not this
  file’s default.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| R12 implemented as | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
