# SP-057 — Avoid-explored engine (strict pass + distinct no-route)

**Phase:** 6 — Exploration-aware routing
**Status:** In progress
**Branch:** `cursor/sp-057-avoid-engine-35cf`
**Depends on:** SPD-040–042, SPD-045; SP-054 recorded; SP-056 mode model
  landed (or this item includes a minimal mode hook — prefer stacking after
  SP-056)
**Unblocks:** SP-058 fallback UX; SP-059; SP-061 exit #2

---

## Objective

Implement hard Avoid in the shared routing core: when Avoid is selected for
pedestrian or bicycle, the strict pass excludes edges with
`exploredRatio == 1`, or returns a **distinct** no-route result. Never
silently degrade to Prefer.

## Motivation

SPD-009 and spec §17.3 require Avoid in V1. SPD-042 locks exclusion at
fully explored edges only, so mixed edges remain usable. A large penalty
without a distinct failure still hides fully explored edges inside a
“successful” route. The engine must make impossibility visible so SP-058
can offer Prefer+strength.

## In-scope behavior

- Avoid mode on pedestrian and bicycle estimators only (SPD-041). Car
  remains Prefer-or-off.
- Strict pass (SPD-042): exclude (true skip) edges with
  `exploredRatio == 1`. Mixed edges (`0 < ratio < 1`) remain. Unmatched
  samples are not explored.
- If no path: new `RouterResultCode` (JNI-mirrored). Must not reuse generic
  `RouteNotFound` as the Avoid-impossible signal.
- Weight / exclusion query uses SPD-040 (`IsExplored()`).
- SPD-045: segment-MWM `.pix` lookup when installed; missing file → not
  explored. Do not load extra leaves into the drape overlay.
- Fixture-graph tests: a path that only needs mixed/unexplored edges is
  chosen and fully explored edges are unused; when every path needs a fully
  explored edge → distinct code, no silent route.
- Production defaults: Avoid still needs SP-058 UI to be selectable; this
  item may expose a C++/JNI mode that Android wires in SP-058.

## Out-of-scope behavior

- Fallback dialog and Prefer switch (SP-058).
- Pre-use warning copy (SP-058).
- Min-connection second search (rejected, SPD-042).
- Mid-navigation freeze policy (SP-059).
- Walk/bike toggle chrome (SP-056 / SP-058).
- Analytics (SP-060).
- True “infinite weight” without a no-path signal.

## Relevant product requirements

- Spec §17.3, §31, §34 Routing; SPD-009; SPD-040; SPD-041; SPD-042;
  SPD-045.

## Relevant source files or symbols

- `IStreetExplorationWeights` (additive exclude query preferred)
- `EdgeEstimator::ApplyStreetExplorationMultiplier`
- `IndexRouter` / A-star `NoPath` → `RouterResultCode`
- `routing_callbacks.hpp` enum (JNI mirror warning in-file)
- `android/sdk/.../ResultCodes.java`, `ResultCodesHelper.java`
- `StreetPixelsManager::GetSegmentExplorationWeightMultiplier`
- `libs/routing/routing_tests/index_graph_tools.*`

## Implementation notes / constraints

- Prefer additive hooks beside `IStreetExplorationWeights` over rewriting
  IndexRouter. Distinguishing Avoid-NoPath from other NoPath is a product
  requirement (SPD-042).
- ETA must remain unmultiplied (`Purpose::Weight` only).
- Offline-only. Do not Pro-gate.

## Acceptance criteria

1. Fixture: a route exists that avoids fully explored edges → Avoid chooses
   it; `exploredRatio == 1` edges unused; mixed edges may be used.
2. Fixture: every path uses a fully explored edge → distinct result; no
   route returned as success.
3. Car / vehicle router does not apply Avoid exclusion.
4. SPD-040: imported-only explored cells count toward `exploredRatio`.
5. SPD-045: installed `.pix` for the segment MWM is used.
6. `routing_tests` / `street_pixels_tests` named in the PR pass.

## Required automated tests

- Mixed-edge path kept; fully explored edge excluded.
- All-paths fully explored → distinct code.
- Prefer mode still uses the soft multiplier (regression).
- Car estimator does not exclude on Avoid (or Avoid cannot be active).

## Required manual validation

- Deferred to SP-058 / SP-061 for UI; engine may be desktop-only here.

## Failure and rollback considerations

- Do not implement Avoid as “Prefer at strength 100”.
- Do not exclude on `exploredRatio > 0`.
- Do not map Avoid-NoPath onto generic `RouteNotFound`.
- Do not add a min-connection cost pass.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Exclusion rule | `exploredRatio == 1` (SPD-042) |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
