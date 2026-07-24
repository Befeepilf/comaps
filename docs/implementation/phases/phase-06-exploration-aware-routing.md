# Phase 6 — Exploration-aware routing

**Status:** Not started
**Depends on:** Phase 3
**Blocks:** nothing; required for release

---

## Objective

Let the map optimise for curiosity. Expose prefer-unexplored routing for
walking and cycling, and add a hard avoid-explored mode that either produces a
strictly unexplored route or explicitly asks the user what to do instead.

## Product-spec references

- §17.1 Standard routing.
- §17.2 Prefer unexplored streets; free; route stays reasonably practical.
- §17.3 Avoid explored streets; mandatory warning; explicit fallback offer;
  must not silently abandon the selected rule.
- §17.4 Deferred routing modes.
- §29.1 Exploration-aware routing including avoid-explored is a free feature.
- §31 "Route impossible under avoid-explored mode" error state.
- §34 "Routing" launch requirements.
- §32.2 Analytics for prefer-unexplored and avoid-explored usage.

## Technical-audit references

- §12 Routing feasibility, including the multiplier formula and the listed
  risks: disconnected unexplored components, extreme detours, instability as
  the user explores mid-navigation, per-segment pixel lookup cost.
- §12 note on imported exploration currently affecting the same explored bit
  that routing uses.
- Spike 7, with its recommendation to implement avoid as a very large finite
  penalty first and to add true exclusion only with a mandatory fallback.

## Current code locations

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Weight hook | `libs/routing/street_exploration_for_routing.hpp` `IStreetExplorationWeights::GetSegmentWeightMultiplier` | Interface exists |
| Multiplier application | `libs/routing/edge_estimator.cpp` `ApplyStreetExplorationMultiplier` | Applied only for `Purpose::Weight`, never for ETA |
| Multiplier formula | `libs/map/street_pixels_manager.cpp` | `1.0 + strength * 9.0 * exploredRatio`, where `strength` is `m_strength / kMaxStrength` and `exploredRatio` is the explored fraction of HEALPix samples matched along the segment. Maximum 10×. Returns 1.0 when no pixels match. |
| Options | `libs/routing/routing_options.hpp` `StreetExplorationRoutingOptions` | Enabled flag plus strength 0–100 |
| Adapter | `libs/map/street_exploration_routing_adapter.cpp` | Bridges the router to `StreetPixelsManager` |
| Android UI | `DrivingOptionsFragment` and the car-screen toggle | Present, but on the driving-options surface |
| Router | `libs/routing/` IndexRouter with `PedestrianModel` and `BicycleModel` | Present |
| Hard exclusion | — | **Not found anywhere in `libs/`.** Only continuous weight multiplication. |

**Difference from the technical audit:** none. The audit's description matches
the working tree exactly.

## Intended outcome

- Prefer-unexplored reachable from the walking and cycling route surfaces, not
  only from driving options.
- Avoid-explored implemented, with the spec's warning copy shown before use.
- When no strictly unexplored route exists, the user is explicitly offered
  "allow the minimum necessary explored connection" or "return to normal
  routing". Never a silent downgrade.
- Behaviour defined for the case where the user explores a street that is part
  of the active avoid-explored route.

## Dependencies

- Phase 3, for the live-versus-imported source flag, so that OQ-2 can be
  answered concretely rather than in principle.

## Proposed work-item breakdown

Not yet decomposed. Likely shape:

1. Expose prefer-unexplored on the pedestrian and bicycle route surfaces.
2. Add a hard-avoid weighting mode behind a feature flag, starting with a very
   large finite penalty.
3. No-route detection and the explicit fallback offer.
4. The "allow the minimum necessary explored connection" path.
5. Mid-navigation behaviour when the route becomes explored.
6. Warning copy and analytics events.

**Marked for phase-specific Plan Mode investigation.** Whether a large finite
penalty is sufficient, or true edge exclusion is needed, depends on measured
route quality. Decompose after the routing measurement exists.

## Data and migration concerns

- No new persisted data is expected. Routing options already persist through
  `StreetExplorationRoutingOptions`.
- The per-segment pixel lookup cost is already paid by prefer mode; avoid mode
  reuses it.
- If OQ-2 resolves toward live-only routing weights, the adapter needs the
  Phase 3 source flag on the query path, which may change lookup cost.

## Privacy and security implications

- Routing is entirely on device and offline. Nothing here uploads.
- Routing history is explicitly local-only under spec §25.1.
- Analytics for routing mode usage must be counts only, with no origin,
  destination, or geometry.

## Automated testing strategy

- Multiplier arithmetic: fully unexplored yields 1.0, fully explored at maximum
  strength yields 10.0, half explored yields the midpoint, no matched pixels
  yields 1.0.
- Avoid mode on a fixture graph where an unexplored route exists: it is chosen.
- Avoid mode on a fixture graph where no unexplored route exists: no route is
  returned silently; the fallback state is signalled.
- The minimum-explored-connection fallback returns a route that uses the fewest
  explored segments available on the fixture.
- Route stability: recomputing with an additional explored segment does not
  produce a pathological result.
- Regression: existing `routing_tests` and `routing_common_tests` still pass.

## Manual validation strategy

- Plan walking and cycling routes in a partly explored area and confirm
  prefer-unexplored visibly changes the route without making it absurd.
- Plan a route in a fully explored area with avoid-explored on and confirm the
  fallback offer appears with the specified warning.
- Accept "allow the minimum necessary explored connection" and confirm the
  resulting route is sensible.
- Choose "return to normal routing" and confirm the mode is clearly no longer
  active.
- Start navigation under avoid-explored, explore part of the route, and confirm
  the app does not thrash between recomputations.
- Confirm route computation time stays acceptable in both modes.

## Entry criteria

- Phase 3 exit criteria met.
- OQ-2 has been answered and recorded as a decision: does prefer-unexplored use
  the personal explored set including imported pixels, or live-only?
- A routing measurement exists comparing avoid-mode route length against normal
  routing on real data, including a forced disconnected case.

## Exit criteria

1. Prefer-unexplored is reachable and functional for walking and cycling.
2. Avoid-explored is implemented and, when a strictly unexplored route exists,
   produces one.
3. When no such route exists, the user sees the spec's explanation and both
   offered options. The selected rule is never silently abandoned.
4. The warning in spec §17.3 is shown before avoid mode is used.
5. Mid-navigation behaviour when the route becomes explored is defined,
   implemented, and observed to be stable.
6. Routing analytics record mode usage with no location data.
7. Existing routing tests pass.

## Explicit non-goals

- Maximising new pixels within a time limit. Spec §17.4.
- Completion loops and generated area-completion routes. Spec §17.4.
- Scenic-plus-unexplored routing and multiplayer routes. Spec §17.4.
- Exploration-specific route alternatives. The audit marks this unclear; it is
  not a V1 requirement.
- Making avoid-explored a paid feature. Spec §29.1 lists it as free.
- Car routing changes. The existing driving-options surface may keep its
  toggle, but V1 targets walking and cycling.

## Known uncertainties

- Whether a very large finite penalty is behaviourally sufficient, or true edge
  exclusion is needed. Only measurement can decide.
- What "the minimum necessary explored connection" means algorithmically:
  minimising explored segment count, explored distance, or explored pixel
  count.
- Whether recomputation during navigation should re-apply avoid mode at all, or
  freeze the route once started.
- Whether prefer-unexplored should live on the driving-options surface,
  alongside it, or on a dedicated exploration-routing surface.
- Per-segment pixel lookup cost on long routes across large countries.
- Whether imported pixels should influence routing (OQ-2). The product's
  position is that imported exploration is real personal exploration, which
  argues for including it; that has not been decided.
