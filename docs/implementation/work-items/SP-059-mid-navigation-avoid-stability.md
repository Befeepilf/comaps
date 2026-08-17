# SP-059 — Mid-navigation stability when the route becomes explored

**Phase:** 6 — Exploration-aware routing
**Status:** In progress
**Branch:** `cursor/sp-059-mid-nav-avoid-35cf`
**Depends on:** SPD-043; SP-057 engine; SP-058 so Avoid can be followed
**Unblocks:** SP-061 exit #5

---

## Objective

Define and implement stable behaviour when the user explores streets that
are part of the active Avoid (or Prefer) route during navigation, so the
app does not thrash between recomputations.

## Motivation

Audit §12 lists mid-navigation instability as a hard-Avoid risk: as pixels
turn green, strict exclusion would invalidate the remaining path and
re-search. Spec §17.3 does not specify this case; SP-055 R9 is the lock.
Without an implementation, exit criterion 5 cannot pass.

## In-scope behavior

- While **following** a route built under Avoid: newly explored pixels on
  the **remaining followed geometry** do not trigger a strict re-search
  that abandons that path (R9).
- Off-route detection and user-requested recalculation **may** re-apply
  Avoid from the new position. If the strict pass fails, use the SP-058
  Prefer+strength control (SPD-042); do not silently inject fully explored
  edges.
- Prefer-following: keep today’s continuous recompute behaviour unless it
  produces a loop; do not invent a freeze for Prefer without evidence.
- Automated test: start following an Avoid route; mark remaining segments
  explored; assert the followed polyline is not replaced by a pathological
  new path in the same session without an off-route / explicit rebuild.
- Document the locked behaviour in this work item’s evidence (and phase-06
  if the wording still says “undefined”).

## Out-of-scope behavior

- Changing collection / interpolation (Phase 2).
- Completing Avoid UI (SP-058) except consuming following-state.
- Analytics (SP-060).
- Car navigation.

## Relevant product requirements

- Spec §17.3 (no silent abandon of the selected **rule**).
- SPD-042, SPD-043; audit §12 mid-navigation risk.
- Phase 6 exit #5.

## Relevant source files or symbols

- `routing::RoutingSession` following / rebuild / off-route
- `RoutingManager`
- `IndexRouter` rebuild entry points
- Street-pixel collection callbacks that currently do not notify routing

## Implementation notes / constraints

- Shared C++ owns the policy (SPD-002).
- Need a way to know “this rebuild is because pixels changed” vs “user went
  off-route”. If no signal exists, add a narrow one; do not disable all
  recalculation.
- Offline-only.

## Acceptance criteria

1. SPD-043 is implemented and described in evidence.
2. Automated case: exploring the remaining Avoid path does not by itself
   replace the route.
3. Off-route / explicit recalc still runs; Avoid-impossible uses the
   SP-058 Prefer+seekbar control, not a silent fully-explored route.
4. No recompute loop under a synthetic “pixels turn green along the route”
   sequence.

## Required automated tests

- Follow + paint remaining edges fully explored (`exploredRatio == 1`) →
  same route.
- Off-route rebuild still attempted (may use a session test double).

## Required manual validation

- Start Avoid navigation, walk the route, confirm no thrash. Device
  residual → SP-061 / Phase 10.

## Failure and rollback considerations

- Do not freeze Avoid across off-route rebuilds (that would strand the
  user).
- Do not re-enable strict exclusion on every GPS tick.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-059-mid-nav-avoid-35cf` |
| Test output | `routing_tests` 296/296 pass (2026-08-17); includes `AvoidFollowStability_ResearchAfterPaintingRemainingAbandonsPath`, `TestTrafficRebuildSkippedWhileFollowingAvoidRoute`, `TestTrafficRebuildRunsWhenNotFollowingAvoidRoute`, `TestOffRouteRebuildStillRunsWhileFollowingAvoidRoute` |
| Policy as implemented | While following **and on** a route built under Avoid (`Route::WasBuiltUnderAvoid`), traffic-driven rebuilds are skipped so newly explored pixels cannot invalidate the remaining path via re-search. Pixel collection does not notify routing. Off-route / explicit `RebuildRoute` still re-applies Avoid from the new position. GPS off-route `AvoidExploredNoRoute` is dropped (`AsyncRouter` `OnRemoveRoute` with a nullptr callback from `CheckLocationForRouting`); the SP-058 Prefer+seekbar dialog is not shown; fully explored edges are not injected. A new search with exclusion off is not used for follow-stability: that would pick a different shortest path. Exploration mode is snapshotted at rebuild start (`m_inFlightExplorationMode`) so `WasBuiltUnderAvoid` matches the search that produced the route. |
| Manual validation | Device residual → SP-061 / Phase 10 |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Pixel collection still does not notify routing. Follow-stability is skip-rebuild, not a pixel callback. | Keep; device walk residual → SP-061 / Phase 10 |
| Re-search with Avoid exclusion off after remaining edges turn green would pick a different shortest path, so it cannot be used to “keep the same route”. | Documented; do not add an exclusion-off follow rebuild |
| GPS off-route `RebuildRoute` that returns `AvoidExploredNoRoute` calls `AsyncRouter` `OnRemoveRoute` with a nullptr callback (`CheckLocationForRouting`). `OnRebuildRouteReady` is not invoked; `AssignRoute` does not run; state remains `RouteRebuilding` until a later GPS rematch. SP-058 Prefer+seekbar is not shown. Fully explored edges are not injected. | SP-061 / Phase 10. Not this item. |
