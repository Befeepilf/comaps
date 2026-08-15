# SP-056 — Prefer-unexplored on walking and cycling surfaces

**Phase:** 6 — Exploration-aware routing
**Status:** In review
**Branch:** `cursor/sp-056-prefer-walk-bike-35cf`
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
| Branch | `cursor/sp-056-prefer-walk-bike-35cf` |
| Test output | See below. Not Accepted. |
| Manual validation | Device residual → SP-061 / Phase 10. No handset in this environment. |
| Accepted by | |
| Accepted date | |

### Automated tests (executed 2026-08-15)

Build:

```
./tools/unix/build_omim.sh -d -p /workspace routing_tests
./tools/unix/build_omim.sh -d -p /workspace street_pixels_tests
```

Both succeeded (street_pixels_tests required host `autoconf`/`libomp-dev` to configure libsharp; not a product change).

Filtered:

```
./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "ExplorationMultiplier_|StreetExplorationRoutingOptions_|ExplorationWeight_"
```

Result: **2 / 2 passed** (`street_pixels_tests` + `routing_tests`).

- `ExplorationMultiplier_*`: 4/4 OK (unchanged formula helpers)
- `ExplorationWeight_*`: 8/8 OK (overlay Prefer 10.0 / unexplored 1.0 / Neither 1.0 / Avoid 1.0 / imported=live 10.0 / SPD-045 leaf `.pix` 10.0 / missing pix 1.0 / half-explored mid-strength)
- `StreetExplorationRoutingOptions_*`: 10/10 OK (default Neither, enabled→Prefer migration, dual-write enabled false for Avoid, mode key wins, invalid mode, strength clamp)

Full suites:

```
./omim-build-debug/routing_tests --data_path=data --user_resource_path=data
./omim-build-debug/street_pixels_tests --data_path=data --user_resource_path=data
```

- `routing_tests`: **285/285 OK**, All tests passed
- `street_pixels_tests`: **217/217 OK**, All tests passed

Android compile residual: `./gradlew -Pandroidauto=true :app:compileDebugJavaWithJavac` failed — plugin `com.android.application:8.13.2` unresolved and SDK has no `platforms`/`build-tools`. No instrumented tests added.

### Manual validation residual

Device, not this environment (same honesty as SP-014 / SP-041):

1. Plan a walk in a partly explored area. Walk tab: Prefer on → route changes; seekbar → route changes; Prefer off → ordinary routing; seekbar hidden when off.
2. Repeat for bike.
3. Car tab: Prefer toggle + seekbar still work. No Avoid row.
4. Confirm Avoid is not visible on walk/bike.
5. Offline: Prefer still recomputes from local `.pix`.

Full device matrix → **SP-061 / Phase 10**.

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `RoutingPlanController` still only inflates road-type chips, not Prefer. When only Prefer is on, `hasAnyOptions` is true but the banner can open with an empty chip list | Pre-existing gap; Prefer banner chips in a later item, not SP-056 |
| Leaf mmap LRU holds up to 4 successful `{country}.pix` mappings; city-scale RSS on multi-leaf walks is unmeasured | Phase 10 residual (same class as SP-054 lookup cost) |
| Avoid is stored and dual-written (`enabled=false`) but hidden in UI | By design until SP-058; weights stay 1.0 until SP-057 |
