# SP-056 — Prefer-unexplored on walking and cycling surfaces

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Depends on:** SP-054 recorded outcome; SPD-040, SPD-041, SPD-045
**Unblocks:** SP-057–061 (mode model and walk/bike reachability)

---

## Objective

Make **Prefer unexplored streets** a first-class walking and cycling route
option, with persisted Prefer / Avoid / neither options and the existing
strength seekbar, using the existing soft multiplier for Prefer.

## Motivation

Spec §17.2 and §34 require prefer-unexplored for walking and cycling. Today
the control lives only on `DrivingOptionsFragment` / the car screen.
SPD-041 keeps the strength seekbar in V1.

## In-scope behavior

- Reshape `StreetExplorationRoutingOptions` to Prefer / Avoid / neither
  (SPD-041). Avoid may be stored but **must not** change weights until
  SP-057. Migrate `m_enabled == true` → Prefer. Keep `m_strength`.
- Expose Prefer and the strength seekbar on `WalkingOptionsFragment` and
  `CyclingOptionsFragment`. Selecting Prefer recomputes the active walk/bike
  route. Seekbar applies to Prefer (0–100, existing formula).
- Avoid control may be shown disabled, or hidden until SP-058 — pick one
  and test that Prefer / neither are unchanged.
- Weight query uses SPD-040 (`IsExplored()`). SPD-045: consult the segment
  MWM’s `.pix` even when overlay `m_countryId` differs.
- Leave the car driving-options prefer toggle and seekbar working. Do not
  add Avoid to car.
- Existing `ExplorationMultiplier_*` tests remain; add mode persist /
  migration tests.

## Out-of-scope behavior

- Avoid engine, no-route signal, fallback dialog (SP-057 / SP-058).
- Mid-navigation policy (SP-059).
- Analytics events (SP-060).
- Starting before SP-054 recorded outcome.
- Replacing the seekbar with max ETA / km deviation (post-V1, SPD-041).
- Pro capability flags.

## Relevant product requirements

- Spec §17.2, §29.1, §34 Routing.
- SPD-040, SPD-041, SPD-045.

## Relevant source files or symbols

- `libs/routing/routing_options.hpp` / `.cpp` `StreetExplorationRoutingOptions`
- `StreetPixelsManager::GetSegmentExplorationWeightMultiplier`
- `StreetExplorationRoutingAdapter`
- JNI `StreetExplorationRoutingOptions.java` / `.cpp`
- `WalkingOptionsFragment`, `CyclingOptionsFragment`, layouts
- `DrivingOptionsFragment` (do not break)
- `android/app/src/main/res/values/strings.xml` `prefer_unexplored_streets`

## Implementation notes / constraints

- Shared C++ owns mode persistence (SPD-002). Android only renders it.
- Prefer and Avoid are mutually exclusive (SPD-041).
- Do not apply Avoid to `VehicleType::Car`.
- Offline-only.

## Acceptance criteria

1. Prefer is reachable and functional from walking and cycling routing
   options, with the strength seekbar.
2. Settings migration: previously enabled prefer still prefers after
   upgrade; strength is preserved.
3. Car prefer toggle and seekbar still function.
4. Automated tests cover mode persist/migration and multiplier arithmetic.
5. SPD-040: imported-only cells affect Prefer the same as live explored
   cells.
6. SPD-045: segment MWM `.pix` is used when overlay country differs.

## Required automated tests

- Mode persist + `enabled → Prefer` migration; strength round-trip.
- `ExplorationMultiplier_*` still pass.
- Segment MWM `.pix` used when overlay country differs.

## Required manual validation

- Plan a walk and a bike route in a partly explored area; toggling Prefer
  and moving the seekbar visibly change the route; turning Prefer off
  restores ordinary routing. Device residual → SP-061 / Phase 10.

## Failure and rollback considerations

- Do not leave Prefer reachable only via the drive tab.
- Do not enable Avoid weights as a side effect.
- Do not drop the seekbar.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Manual validation | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
