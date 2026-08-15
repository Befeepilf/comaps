# Phase 6 — Exploration-aware routing

**Status:** Not started (phase-entry planning 2026-08-15; coding gated on
SP-054 measurement + SP-055 Accepted locks)
**Depends on:** Phase 3
**Blocks:** nothing; required for release

Phase 5 review may continue in parallel. Phase 6 does **not** depend on
Phase 5 (roadmap §4.1). Do not wait for SP-041 acceptance to start SP-054/055.

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

Re-verified 2026-08-15 against the working tree (Phase 6 entry).

| Concern | Location | Observed state |
| --- | --- | --- |
| Weight hook | `libs/routing/street_exploration_for_routing.hpp` `IStreetExplorationWeights::GetSegmentWeightMultiplier` | Interface exists; multiplier only |
| Multiplier application | `libs/routing/edge_estimator.cpp` `ApplyStreetExplorationMultiplier` | Applied for `Purpose::Weight` only, never ETA. Wired into **Pedestrian, Bicycle, and Car** estimators |
| Multiplier formula | `StreetPixelsManager::GetSegmentExplorationWeightMultiplier` | `1.0 + strength * 9.0 * exploredRatio`. `strength` = `m_strength / kMaxStrength`. `exploredRatio` = explored / matched HEALPix samples on the segment. Max 10× at strength 100. Returns 1.0 when disabled, not Ready, no pixels, or **no matched samples** |
| Explored-set query | same function, `sp->IsExplored()` | Uses the **personal explored bit**, including imported-only cells. Does **not** consult `IsEverLive()`. Phase 3 ever-live bit exists (`df::StreetPixel::IsEverLive`) and is unused by routing |
| Single-MWM overlay | `StreetPixelsManager::m_countryId` | Overlay loads **one** country `.pix`. Adapter returns **1.0** when `mwmCountryName != m_countryId` (logged a few times). Cross-leaf routes silently ignore exploration |
| Options | `routing::StreetExplorationRoutingOptions` | `m_enabled` + `m_strength` 0–100 (default 50). Persisted in settings. No Avoid mode, no vehicle-type scope |
| Adapter | `libs/map/street_exploration_routing_adapter.cpp` | Bridges IndexRouter → `StreetPixelsManager` from `RoutingManager` for every non-ruler router |
| Walk/bike options UI | `WalkingOptionsFragment`, `CyclingOptionsFragment` | **No** prefer/avoid controls. Ferry/dirty/steps/paved only |
| Driving options UI | `DrivingOptionsFragment`, car `DrivingOptionsScreen` | Prefer toggle + strength seekbar (car screen: toggle only) |
| Options host | `RoutingOptionsFragment` | Three tabs (walk / cycle / drive); prefer lives only on the drive tab |
| No-route UX | `ResultCodesHelper` / `MwmActivity.onDrivingOptionsBuildError` | Generic `RouteNotFound` and “unable to calc — open settings”. **No** avoid-specific result code or fallback offer |
| Hard exclusion | — | **Not found** anywhere in `libs/`. Only continuous weight multiplication |
| Analytics | — | **Not found.** SP-003 explicitly deferred product-analytics events. No count sink for §32.2 |
| Feature flags | `explorer_pro::Capability` | GPX/track only. Prefer/avoid are free (§29.1); do not Pro-gate |
| Arithmetic tests | `street_pixels_tests` `ExplorationMultiplier_*` | Formula only; not wired to the manager or a graph |
| Graph / avoid tests | — | **Not found** |

**Difference from the technical audit (2026-07-20):** Phase 3 landed the
ever-live bit (SPD-015). Routing still uses `IsExplored()` only — the audit’s
“same explored bit” description remains true for the weight path. Walk/cycle
tabs exist now (`RoutingOptionsFragment`) but still omit prefer; the audit’s
“driving-options surface only” UI gap is unchanged. The single-MWM
`m_countryId` mismatch early-return was not called out in the 2026-07-25
phase snapshot.

**Difference from the product spec:** Prefer exists as a global soft
multiplier, but V1 requires it on **walking and cycling** surfaces (§17.2,
§34). Avoid-explored (§17.3, SPD-009) is absent. Strength slider is not in
the spec.

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

- Phase 3 exit **met** (2026-08-03). Ever-live vs imported is queryable, so
  OQ-2 can be locked as a product decision rather than a storage question.
- Phase 5 is **not** a prerequisite. Area polygons are unused by routing
  weights.

## Phase-entry investigation (2026-08-15)

### Confirmed gaps

- Prefer UI is on the **vehicle** tab / car screen, not walk/bike.
- Avoid mode, distinct no-route signal, fallback offer, and pre-use warning
  are absent.
- Options model is `enabled` + `strength`, not Standard / Prefer / Avoid.
- Routing weights ignore non-loaded MWM `.pix` files.
- No product-analytics event pipeline (SP-003 out-of-scope leftover).
- Spike 7 measurement **not recorded**.
- OQ-2 **not decided** (DECISIONS §15). Code currently includes imported
  pixels via `IsExplored()`.

### Blocking unknowns (must not be guessed in coding items)

Product/architecture locks live in SP-055 as recommended **R1–R12**. They are
**not** Accepted SPDs until the maintainer confirms. Coding SP-056+ does not
start before SP-055 Group A is Accepted and SP-054 has a recorded outcome
(or an explicit residual, same pattern as SP-033).

| Ref | Question | Blocks | Recommended lock (SP-055) | Needs spike? |
| --- | --- | --- | --- | --- |
| OQ-2 | Personal explored set including imported, or live-only? | Phase 6 entry; all weight tests | **R1** — personal `IsExplored()` including imported. Ever-live unused for routing. Competition isolation unchanged | No |
| R-UI | Walk/bike surface vs driving-options vs dedicated sheet | SP-056 | **R3** — walk and cycle routing-options tabs; mutually exclusive Standard / Prefer / Avoid | No |
| R-strength | Keep 0–100 seekbar? | SP-056 | **R4** — no walk/bike seekbar; prefer uses internal default 50 | No |
| R-vehicle | Does Avoid apply to car? | SP-057 | **R2** — Avoid pedestrian+bicycle only; car prefer toggle may remain | No |
| R-strict | Any explored pixel on an edge, or ratio threshold? | SP-057 | **R5** — exclude edge if `exploredRatio > 0`; unmatched samples are not explored | No |
| R-fallback | Distinct no-route vs generic `RouteNotFound` | SP-057/058 | **R6** — distinct result; never auto-switch to prefer/standard | No |
| R-min | Min explored **segments**, **distance**, or **pixels**? | SP-058 | **R7** — minimise explored **distance** (metres) | No |
| R-warn | When is §17.3 warning shown? | SP-058 | **R8** — before Avoid is applied | No |
| R-nav | Re-apply Avoid on every recompute, or freeze? | SP-059 | **R9** — do not abandon the active followed path because it just turned green; off-route recalc may re-apply Avoid and must use the same fallback offer | No |
| R-analytics | No event sink exists | Exit #6 / SP-060 | **R10** — count-only local API; no coordinates; upload residual → Phase 10 if no privacy-safe sink | No |
| R-cross-leaf | Silent 1.0 on `m_countryId` mismatch | Prefer/avoid correctness | **R11** — query the segment MWM’s `.pix` when installed | No (correctness) |
| R-algo | Large finite penalty vs true exclusion | SP-057 | **R12** — spike-gated. Product semantics are R5+R6 (strict unexplored **or** explicit fallback). Default recommendation: true exclusion for the strict pass; large finite / explored-distance cost for the min-connection pass | **Yes (SP-054)** |
| R-copy | §17.3 “minimum necessary explored connection” vs §31 “small amount of explored routing” | SP-058 strings | **R8/R7** — option label from §17.3; explanation from §31 | No |
| R-cost | Per-segment lookup on long country routes | Exit compute time | Measure in SP-054; residual slow paths → Phase 10 rather than dropping Avoid | Yes (SP-054) |

### Work-item breakdown

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-054](../work-items/SP-054-routing-spike.md) | Spike: exploration-aware routing measurement (**entry gate**) |
| 2 | [SP-055](../work-items/SP-055-routing-architecture-decisions.md) | Routing architecture decisions (**entry gate** for coding) |
| 3 | [SP-056](../work-items/SP-056-prefer-unexplored-walk-bike.md) | Prefer-unexplored on walking and cycling surfaces |
| 4 | [SP-057](../work-items/SP-057-avoid-explored-engine.md) | Avoid-explored engine (strict pass + distinct no-route) |
| 5 | [SP-058](../work-items/SP-058-avoid-fallback-and-warning.md) | Avoid warning, no-route fallback offer, min-connection retry |
| 6 | [SP-059](../work-items/SP-059-mid-navigation-avoid-stability.md) | Mid-navigation stability when the route becomes explored |
| 7 | [SP-060](../work-items/SP-060-routing-mode-analytics.md) | Count-only routing-mode analytics |
| 8 | [SP-061](../work-items/SP-061-phase6-end-to-end-validation.md) | Phase 6 end-to-end validation (**exit gate**) |

Gate: **do not start SP-056+ product coding until SP-054 has a recorded
outcome and SP-055 Group A is Accepted** (mirror Phase 4 SP-023/024 and
Phase 5 SP-033/034). SP-054 and SP-055 may proceed now, in parallel with
Phase 5 review.

### Open questions

See the blocking table above. OQ-2 remains in `DECISIONS.md` §15 until R1 is
Accepted as an SPD. Group B (R12, lookup cost) waits on SP-054 numbers.

## Data and migration concerns

- Options already persist via `StreetExplorationRoutingOptions`. Reshaping to
  a mode enum (SP-056, after R3/R4) needs a backward-compatible settings
  migration: existing `m_enabled == true` → Prefer; `false` → Standard.
  Strength may remain persisted for the car tab.
- No new exploration files. Avoid reuses the `.pix` explored bit.
- If R1 were live-only (not recommended), the adapter would read
  `IsEverLive()` and lookup cost would change. Recommended R1 avoids that.
- R11 may mmap additional leaf `.pix` files during a route; do not load them
  into the renderer overlay.

## Privacy and security implications

- Routing is entirely on device and offline. Nothing here uploads tracks.
- Routing history is explicitly local-only under spec §25.1.
- Analytics for routing mode usage must be counts only, with no origin,
  destination, geometry, or pixel ids (R10). Do not send routes to Sentry.

## Automated testing strategy

- Multiplier arithmetic: fully unexplored yields 1.0, fully explored at
  maximum strength yields 10.0, half explored yields the midpoint, no matched
  pixels yields 1.0 (existing `ExplorationMultiplier_*`; keep).
- Prefer on a fixture graph: an unexplored alternative is chosen when the
  practicality bound still holds; disabled mode matches standard.
- Avoid on a fixture graph where an unexplored route exists: it is chosen;
  explored edges are unused.
- Avoid on a fixture graph where no unexplored route exists: a **distinct**
  fallback state is signalled; not silent `RouteNotFound`.
- Min-connection fallback returns the fixture route with least explored
  **distance** (after R7).
- Imported-only vs ever-live cells: routing follows the Accepted OQ-2 lock.
- Cross-leaf: a segment whose MWM `.pix` is installed is weighted even when
  the overlay `m_countryId` is a different leaf.
- Route stability: exploring the followed remaining path does not produce a
  pathological recompute loop (SP-059).
- Regression: `routing_tests`, `routing_common_tests`, and
  `street_pixels_tests` still pass. Do not require
  `routing_integration_tests` (world dataset / `REQUIRE_SERVER`) for the
  unit gate.

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
- Device residual (no handset in this environment) → Phase 10, same honesty
  pattern as SP-014 / SP-041.

## Entry criteria

- Phase 3 exit criteria met. **Met 2026-08-03.**
- OQ-2 has been answered and recorded as a decision: does prefer-unexplored use
  the personal explored set including imported pixels, or live-only?
  **Not met** — recommended R1 in SP-055; not Accepted.
- A routing measurement exists comparing avoid-mode route length against normal
  routing on real data, including a forced disconnected case.
  **Not met** — SP-054.

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
  prefer toggle (R2); V1 targets walking and cycling.
- Pro capability flags for prefer/avoid.
- A new analytics backend or Sentry route events.
- Country/world exploration percentages, competition, GPX (Phases 7–9).

## Known uncertainties

Moved into the blocking table and SP-055 R1–R12. Remaining after those locks:

- SP-054 numbers for detour ratio, disconnected-case latency, and lookup cost
  (R12 / R-cost).
- Whether R11 full per-leaf `.pix` mmap is too heavy on a long multi-leaf
  walk — measure; if needed, cache recently used leaf maps, do not silently
  drop weights.
