# Phase 6 — Exploration-aware routing

**Status:** In progress (SP-054 Accepted 2026-08-15 with city-scale residual;
SPD-040–045 locked; coding starts at SP-056)
**Depends on:** Phase 3
**Blocks:** nothing; required for release

Phase 5 review may continue in parallel. Phase 6 does **not** depend on
Phase 5 (roadmap §4.1). Do not wait for SP-041 acceptance to start SP-054/055.

---

## Objective

Let the map optimise for curiosity. Expose prefer-unexplored routing for
walking and cycling, and add a hard avoid-explored mode that excludes
fully explored edges or clearly asks the user to switch to Prefer.

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

Re-verified 2026-08-15 against the working tree (Phase 6 entry; SP-056 rows updated after implementation).

| Concern | Location | Observed state |
| --- | --- | --- |
| Weight hook | `libs/routing/street_exploration_for_routing.hpp` `IStreetExplorationWeights::GetSegmentWeightMultiplier` | Interface exists; multiplier only |
| Multiplier application | `libs/routing/edge_estimator.cpp` `ApplyStreetExplorationMultiplier` | Applied for `Purpose::Weight` only, never ETA. Wired into **Pedestrian, Bicycle, and Car** estimators |
| Multiplier formula | `StreetPixelsManager::GetSegmentExplorationWeightMultiplier` | `1.0 + strength * 9.0 * exploredRatio`. `strength` = `m_strength / kMaxStrength`. `exploredRatio` = explored / matched HEALPix samples on the segment. Max 10× at strength 100. Returns 1.0 when mode is not Prefer (Neither/Avoid), missing/corrupt/empty `{mwm}.pix`, or **no matched samples**. Overlay Ready no longer gates leaf lookup |
| Explored-set query | same function, `sp->IsExplored()` | Uses the **personal explored bit**, including imported-only cells. Does **not** consult `IsEverLive()`. Phase 3 ever-live bit exists (`df::StreetPixel::IsEverLive`) and is unused by routing |
| Single-MWM overlay | `StreetPixelsManager::m_countryId` | Overlay still loads **one** country `.pix` for the renderer. Weight path: overlay hit if overlay country matches the segment MWM and the overlay span is non-empty; else mmap `{mwmCountryName}.pix` (successful-mmap LRU of 4). Overlay country mismatch no longer forces 1.0 (SPD-045) |
| Options | `routing::StreetExplorationRoutingOptions` | `m_mode` Neither/Prefer/Avoid + `m_strength` 0–100 (default 50). Settings key `street_exploration_routing_mode` is source of truth; legacy `street_exploration_routing_enabled` dual-writes `"true"` iff Prefer. Avoid stored does not change weights until SP-057 |
| Adapter | `libs/map/street_exploration_routing_adapter.cpp` | Bridges IndexRouter → `StreetPixelsManager` from `RoutingManager` for every non-ruler router. Already passes segment MWM name; no adapter change in SP-056 |
| Walk/bike options UI | `WalkingOptionsFragment`, `CyclingOptionsFragment` | Prefer switch + strength seekbar via `include_street_exploration_prefer.xml`. Avoid **hidden** until SP-058 |
| Driving options UI | `DrivingOptionsFragment`, car `DrivingOptionsScreen` | Prefer toggle + strength seekbar (car screen: toggle only). Prefer ON ↔ Prefer, OFF ↔ Neither. No Avoid row |
| Options host | `RoutingOptionsFragment` | Three tabs (walk / cycle / drive); Prefer on all three. Rebuild detects `m_mode` + `m_strength` |
| No-route UX | `ResultCodesHelper` / `MwmActivity.onDrivingOptionsBuildError` | Generic `RouteNotFound` and “unable to calc — open settings”. **No** avoid-specific result code or fallback offer |
| Hard exclusion | — | **Not found** anywhere in `libs/`. Only continuous weight multiplication |
| Analytics | — | **Not found.** SP-003 explicitly deferred product-analytics events. No count sink for §32.2 |
| Feature flags | `explorer_pro::Capability` | GPX/track only. Prefer/avoid are free (§29.1); do not Pro-gate |
| Arithmetic tests | `street_pixels_tests` `ExplorationMultiplier_*` plus `ExplorationWeight_*` | Formula helpers unchanged. Manager tests cover Prefer 10.0, Avoid→1.0, imported=live, overlay-mismatch leaf `.pix`, missing pix → 1.0 |
| Graph / avoid tests | — | **Not found** |

**Difference from the technical audit (2026-07-20):** Phase 3 landed the
ever-live bit (SPD-015). Routing still uses `IsExplored()` only — the audit’s
“same explored bit” description remains true for the weight path. Walk/cycle
tabs now expose Prefer + seekbar (SP-056); Avoid chrome remains hidden until
SP-058, so the audit’s “driving-options surface only” snapshot is stale for
Prefer. The single-MWM overlay remains renderer-only; weight lookup consults
the segment MWM `.pix` when installed (SPD-045).

**Difference from the product spec:** Prefer is on walking and cycling
surfaces (§17.2, §34) after SP-056. Avoid-explored (§17.3, SPD-009) is still
absent as a selectable control (hidden until SP-058; weights 1.0 until
SP-057). Strength slider is not in the spec; **SPD-041** keeps it for V1.
**SPD-042** records two V1 divergences: Avoid excludes only fully explored
edges (`exploredRatio == 1`), and no-route fallback is Prefer+strength
rather than the §17.3 / §31 min-connection pair.

## Intended outcome

- Prefer-unexplored reachable from the walking and cycling route surfaces, not
  only from driving options, with the strength seekbar (SPD-041).
- Avoid-explored implemented: true exclusion of edges with
  `exploredRatio == 1`, with the spec's warning copy shown before use
  (SPD-042).
- When no route exists under that rule, the user sees a clear no-route
  result and a control that switches to Prefer with the strength seekbar.
  Never a silent downgrade (SPD-042; SPD-009 silent-degrade ban still
  holds).
- Behaviour defined for the case where the user explores a street that is part
  of the active avoid-explored route (SPD-043).

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
- Options model is `enabled` + `strength`, not Prefer / Avoid (SPD-041).
- Routing weights ignore non-loaded MWM `.pix` files.
- No product-analytics event pipeline (SP-003 out-of-scope leftover).
- Spike 7 measurement **recorded** (SP-054 desktop synthetic; city-scale /
  device residual → Phase 10).
- OQ-2 **closed by SPD-040**. Code already includes imported pixels via
  `IsExplored()`.

### Blocking unknowns (must not be guessed in coding items)

Product/architecture locks **R1–R12** are Accepted as **SPD-040–045**
(product-owner 2026-08-15). Coding SP-056+ may start: SP-054 recorded a Spike 7 desktop synthetic
outcome plus an explicit city-scale/device residual (SP-033 pattern).
SP-054 measured the locked algorithm; it does not re-open R5–R7.

| Ref | Question | Status | Lock |
| --- | --- | --- | --- |
| OQ-2 | Personal explored set including imported, or live-only? | **Closed** | **SPD-040** — `IsExplored()` including imported |
| R-UI | Walk/bike surface and mode names | **Closed** | **SPD-041** — Prefer / Avoid on walk and cycle tabs; neither = standard |
| R-strength | Keep 0–100 seekbar? | **Closed** | **SPD-041** — keep seekbar on Prefer; ETA/km cap is post-V1 |
| R-vehicle | Does Avoid apply to car? | **Closed** | **SPD-041** — Avoid pedestrian+bicycle only; car Prefer may remain |
| R-strict | Any explored pixel, or fully explored? | **Closed** | **SPD-042** — exclude iff `exploredRatio == 1` |
| R-fallback | Distinct no-route vs generic `RouteNotFound` | **Closed** | **SPD-042** — clear no-route; button to Prefer+seekbar; never auto-switch |
| R-min | Min explored segments / distance / pixels? | **Closed** | **SPD-042** — skip; fallback is Prefer with strength |
| R-warn | When is §17.3 warning shown? | **Closed** | **SPD-042** — before Avoid is applied |
| R-nav | Re-apply Avoid on every recompute, or freeze? | **Closed** | **SPD-043** — do not abandon followed path that turned green; off-route uses SPD-042 fallback |
| R-analytics | No event sink exists | **Closed** (upload residual) | **SPD-044** — count-only; avoid-fallback-prefer; Phase 10 upload if no sink |
| R-cross-leaf | Silent 1.0 on `m_countryId` mismatch | **Closed** | **SPD-045** — query the segment MWM’s `.pix` when installed |
| R-algo | Large finite penalty vs true exclusion | **Closed** | **SPD-042** — true exclusion of fully explored edges |
| R-copy | §17.3 vs §31 fallback wording | **Closed** (spec divergence) | **SPD-042** — Prefer+strength, not min-connection / return-to-normal |
| R-cost | Per-segment lookup on long country routes | **Open (measure)** | SP-054; residual slow paths → Phase 10 rather than dropping Avoid |

### Work-item breakdown

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-054](../work-items/SP-054-routing-spike.md) | Spike: exploration-aware routing measurement (**entry gate**) |
| 2 | [SP-055](../work-items/SP-055-routing-architecture-decisions.md) | Routing architecture decisions (**entry gate** for coding) |
| 3 | [SP-056](../work-items/SP-056-prefer-unexplored-walk-bike.md) | Prefer-unexplored on walking and cycling surfaces |
| 4 | [SP-057](../work-items/SP-057-avoid-explored-engine.md) | Avoid-explored engine (strict pass + distinct no-route) |
| 5 | [SP-058](../work-items/SP-058-avoid-fallback-and-warning.md) | Avoid warning, no-route UX, Prefer+strength fallback |
| 6 | [SP-059](../work-items/SP-059-mid-navigation-avoid-stability.md) | Mid-navigation stability when the route becomes explored |
| 7 | [SP-060](../work-items/SP-060-routing-mode-analytics.md) | Count-only routing-mode analytics |
| 8 | [SP-061](../work-items/SP-061-phase6-end-to-end-validation.md) | Phase 6 end-to-end validation (**exit gate**) |

Gate: SP-054 recorded outcome **met 2026-08-15** (desktop synthetic pass;
city-scale MWM+`.pix` and device → Phase 10 residual). SP-055 locks are
Accepted as SPD-040–045. SP-056+ product coding may proceed.

### Open questions

OQ-2 is closed (SPD-040). Remaining: SP-054 numbers for detour ratio,
no-route frequency under `exploredRatio == 1`, and lookup cost.

## Data and migration concerns

- Options already persist via `StreetExplorationRoutingOptions`. Reshaping to
  Prefer / Avoid / neither (SP-056, SPD-041) needs a backward-compatible
  settings migration: existing `m_enabled == true` → Prefer; `false` →
  neither. Strength stays persisted.
- No new exploration files. Avoid reuses the `.pix` explored bit.
- R1 is Accepted (SPD-040): adapter keeps `IsExplored()`.
- SPD-045 may mmap additional leaf `.pix` files during a route; do not load
  them into the renderer overlay.

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
- Avoid on a fixture graph where an unexplored (not fully explored) route
  exists: it is chosen; fully explored edges (`exploredRatio == 1`) are
  unused; mixed edges may be used.
- Avoid on a fixture graph where every path uses a fully explored edge: a
  **distinct** no-route state is signalled; not silent `RouteNotFound`.
- Prefer fallback is an explicit mode switch, not a min-connection search.
- Imported-only vs ever-live cells: routing follows SPD-040.
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
- Plan a route whose remaining paths are fully explored, with Avoid on, and
  confirm the no-route result and the control that switches to Prefer with
  the strength seekbar.
- Confirm Avoid remains selected until that control is used (no silent
  downgrade).
- Start navigation under avoid-explored, explore part of the route, and confirm
  the app does not thrash between recomputations.
- Confirm route computation time stays acceptable in both modes.
- Device residual (no handset in this environment) → Phase 10, same honesty
  pattern as SP-014 / SP-041.

## Entry criteria

- Phase 3 exit criteria met. **Met 2026-08-03.**
- OQ-2 has been answered and recorded as a decision: does prefer-unexplored use
  the personal explored set including imported pixels, or live-only?
  **Met 2026-08-15 — SPD-040.**
- A routing measurement exists comparing avoid-mode route length against normal
  routing on real data, including a forced disconnected case.
  **Met 2026-08-15 — SP-054** (desktop synthetic harness; city-scale MWM+`.pix`
  and device residual → Phase 10).

## Exit criteria

1. Prefer-unexplored is reachable and functional for walking and cycling.
2. Avoid-explored is implemented and, when a route exists that does not use
   fully explored edges (`exploredRatio == 1`), produces one.
3. When no such route exists, the user sees a clear no-route result and a
   control that switches to Prefer with the strength seekbar. The selected
   rule is never silently abandoned (SPD-042).
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
  prefer toggle (SPD-041); V1 targets walking and cycling.
- A min-connection second search (SPD-042 / R7).
- Replacing the strength seekbar with max ETA or km deviation (post-V1).
- Pro capability flags for prefer/avoid.
- A new analytics backend or Sentry route events.
- Country/world exploration percentages, competition, GPX (Phases 7–9).

## Known uncertainties

R1–R12 are locked (SPD-040–045). Remaining:

- SP-054 numbers for detour ratio, no-route frequency under
  `exploredRatio == 1`, and lookup cost.
- Whether SPD-045 full per-leaf `.pix` mmap is too heavy on a long
  multi-leaf walk — measure; if needed, cache recently used leaf maps, do
  not silently drop weights.
