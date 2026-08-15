# SP-056 — Prefer-unexplored on walking and cycling surfaces

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Depends on:** SP-054 recorded outcome; SP-055 Group A Accepted (R1–R4, R11
  as it applies to the prefer weight path)
**Unblocks:** SP-057–061 (mode model and walk/bike reachability)

---

## Objective

Make **Prefer unexplored streets** a first-class walking and cycling route
option, with a persisted Standard / Prefer / Avoid mode model, using the
existing soft multiplier for Prefer.

## Motivation

Spec §17.2 and §34 require prefer-unexplored for walking and cycling. Today
the control lives only on `DrivingOptionsFragment` / the car screen. V1
does not treat driving-options as the exploration-routing surface (phase-06
non-goal: car routing changes).

## In-scope behavior

- Reshape `StreetExplorationRoutingOptions` to a mode enum
  `Standard | Prefer | Avoid` (Avoid may be stored but **must not** change
  weights until SP-057). Migrate `m_enabled == true` → Prefer.
- Expose Prefer on `WalkingOptionsFragment` and `CyclingOptionsFragment`
  (and strings). Selecting Prefer recomputes the active walk/bike route.
- Prefer continues to use `1 + strength * 9 * exploredRatio` with internal
  strength = `kDefaultStrength` (50) on walk/bike (R4). No walk/bike
  seekbar.
- Weight query uses R1 (`IsExplored()`). If R11 is Accepted, prefer weights
  consult the segment MWM’s `.pix` even when overlay `m_countryId` differs;
  otherwise record the mismatch as follow-up owned by SP-057/R11 — do not
  silently leave it if R11 is Accepted.
- Leave the car driving-options prefer toggle working (R2). Do not add
  Avoid to car in this item.
- Existing `ExplorationMultiplier_*` tests remain; add mode persist /
  migration tests.

## Out-of-scope behavior

- Avoid engine, no-route signal, fallback dialog (SP-057 / SP-058).
- Mid-navigation policy (SP-059).
- Analytics events (SP-060).
- Starting before SP-054 + SP-055 Group A gate.
- User-facing strength on walk/bike (R4).
- Pro capability flags.

## Relevant product requirements

- Spec §17.2, §29.1, §34 Routing.
- SP-055 R1, R2, R3, R4, R11.
- SPD-009 does not change Prefer; it constrains Avoid (later items).

## Relevant source files or symbols

- `libs/routing/routing_options.hpp` / `.cpp` `StreetExplorationRoutingOptions`
- `StreetPixelsManager::GetSegmentExplorationWeightMultiplier`
- `StreetExplorationRoutingAdapter`
- JNI `StreetExplorationRoutingOptions.java` / `.cpp`
- `WalkingOptionsFragment`, `CyclingOptionsFragment`, layouts
- `DrivingOptionsFragment` (do not break; optional: keep seekbar)
- `android/app/src/main/res/values/strings.xml` `prefer_unexplored_streets`

## Implementation notes / constraints

- Shared C++ owns mode persistence (SPD-002). Android only renders it.
- Avoid stored as a mode must be a no-op for weights until SP-057, **or**
  hide/disable Avoid in UI until SP-058 — pick one and test that Standard
  and Prefer behaviour is unchanged if the user cannot yet select Avoid.
- Do not apply Avoid to `VehicleType::Car`.
- Offline-only.

## Acceptance criteria

1. Prefer is reachable and functional from walking and cycling routing
   options.
2. Settings migration: previously enabled prefer still prefers after
   upgrade.
3. Walk/bike Prefer uses internal strength 50; no new seekbar on those
   tabs.
4. Car prefer toggle still functions (R2).
5. Automated tests cover mode persist/migration and multiplier arithmetic.
6. R1 explored-set: imported-only cells affect Prefer the same as live
   explored cells.

## Required automated tests

- Mode persist + `enabled → Prefer` migration.
- `ExplorationMultiplier_*` still pass.
- If R11 lands here: segment MWM `.pix` used when overlay country differs.

## Required manual validation

- Plan a walk and a bike route in a partly explored area; toggling Prefer
  visibly changes the route; Standard restores the previous practical
  route. Device residual → SP-061 / Phase 10.

## Failure and rollback considerations

- Do not leave Prefer reachable only via the drive tab.
- Do not enable Avoid weights as a side effect.

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
