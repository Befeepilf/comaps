# SP-058 — Avoid warning, no-route UX, and Prefer+strength fallback

**Phase:** 6 — Exploration-aware routing
**Status:** In progress
**Branch:** `cursor/sp-058-avoid-fallback-ux-35cf`
**Depends on:** SPD-041, SPD-042; SP-056 walk/bike surface; SP-057 distinct
  no-route signal
**Unblocks:** SP-059 (follows an Avoid route); SP-061 exit #3–4

---

## Objective

Let the user turn Avoid on only after the spec warning, and when the strict
pass finds no route, show a clear no-route result with a simple control that
switches to Prefer with the strength seekbar. Never silently abandon Avoid.

## Motivation

SPD-042 replaces the spec §17.3 / §31 min-connection pair with Prefer plus
the strength seekbar. Generic `onDrivingOptionsBuildError` (“unable to calc
— open settings”) is not that contract. The switch is an explicit user
action, so SPD-009’s ban on silent degrade still holds.

## In-scope behavior

- Walk/bike control to select Avoid (SPD-041), mutually exclusive with
  Prefer, with §17.3 warning **before** the mode is applied (SPD-042).
  Warning copy: “This can produce very long routes or no available route.”
- On SP-057’s distinct no-route result, show a clear explanation that no
  route could be found under Avoid, and a simple button that switches to
  **Prefer** with the **strength seekbar** visible, then recomputes.
- Avoid remains selected until that button (or the user changing options)
  is used. No auto-switch.
- Do **not** implement a min-connection second search.
- English strings in `values/strings.xml` (and `values-en` if that is the
  existing pattern). Do not machine-translate all locales in this item;
  follow repo translation practice.
- Tests: controller / result-code mapping; documented Android manual
  script. No explored-distance cost fixtures (out of scope).

## Out-of-scope behavior

- Strict exclusion engine (SP-057).
- Mid-navigation thrash policy (SP-059).
- Analytics counters (SP-060) — may call a stub if SP-060 lands first.
- Car Avoid.
- §17.4 deferred routing modes.
- Min-connection / return-to-normal as specified in §17.3 / §31 (V1
  divergence recorded in SPD-042).

## Relevant product requirements

- Spec §17.3 warning copy; SPD-009 (no silent degrade); SPD-041; SPD-042.
- Phase 6 exit #3–4 as amended under SPD-042.

## Relevant source files or symbols

- `WalkingOptionsFragment`, `CyclingOptionsFragment`, layouts
- `MwmActivity.onCommonBuildError` / `onDrivingOptionsBuildError` /
  `RoutingErrorDialogFragment` / `ResultCodesHelper`
- JNI result-code mirror
- `android/app/src/main/res/values/strings.xml`

## Implementation notes / constraints

- Reuse the existing routing-error dialog pattern. Distinguish
  Avoid-impossible from missing maps / GPS.
- The Prefer switch must actually set Prefer (and show the seekbar), not
  leave Avoid enabled with Prefer weights.
- Offline-only.

## Acceptance criteria

1. Avoid cannot be applied without the §17.3 warning having been shown.
2. Impossible strict pass shows a clear no-route result and a Prefer+seekbar
   control; not a generic-only “open settings” dialog.
3. Using the control selects Prefer, shows the seekbar, and recomputes.
4. No code path auto-retries Prefer without the user choosing.
5. No min-connection search exists in this item.

## Required automated tests

- Result-code mapping does not treat Avoid-impossible as missing-maps
  download.
- Mode after the fallback action is Prefer (shared settings test if the
  action is reachable from C++/JNI; otherwise document the Java path).
- Production fallback path is `RoutingErrorDialogFragment` positive click →
  `preferFallback` → `SaveToSettings` (JNI) → `rebuildLastRoute()`. JUnit
  covers `preferFallback` only.

## Required manual validation

Device residual → SP-061 / Phase 10. Record device, OS, build type, router, and
outcome. This environment cannot complete the OD.

### Setup

1. Install a local map whose street graph you can saturate (small extract or a
   fully explored neighborhood). Personal `.pix` must mark every matched sample
   on every OD path as explored (`exploredRatio == 1` on all connecting edges).
2. Airplane mode / offline. Confirm a walk and a bike route still build with
   Prefer off (ordinary routing works).
3. Strength seekbar at a known value (e.g. 50). Leave it there unless a step
   says otherwise.

### Walk — warning before Avoid

4. Routing options → Walking tab. Prefer and Avoid both off. Seekbar hidden.
5. Turn Avoid on. Warning appears with title “Avoid explored streets” and
   body “This can produce very long routes or no available route.”
6. Cancel / dismiss. Avoid is off. Mode unchanged (Neither). Seekbar hidden.
   No rebuild under Avoid.
7. Turn Prefer on. Seekbar visible. Turn Avoid on. Warning again (every time,
   not once-ever). Cancel. Prefer still on, seekbar still visible, Avoid off.
8. Turn Avoid on, confirm OK. Prefer off, seekbar hidden, Avoid on.

### Walk — no-route and Prefer fallback

9. Plan an OD where every connecting path is fully explored. Build a walking
   route with Avoid on.
10. Result is the Avoid no-route dialog:
    title “No route under Avoid explored streets”
    message “No route could be found without using fully explored streets.”
    Positive: “Switch to Prefer unexplored streets”
    Negative: Cancel.
    This is not “Unable to calculate route” / Settings.
11. With ferry (or another walk avoid-road option) also on, rebuild under
    Avoid. Still the Avoid no-route dialog, not the driving-options settings
    dialog. (SP-057 steal bug.)
12. Tap Cancel on the no-route dialog. Avoid remains selected in Walking
    options. Seekbar still hidden.
13. Rebuild under Avoid; tap “Switch to Prefer unexplored streets”.
    Dialog closes. Route recomputes immediately under Prefer. Options screen
    does not have to open. Do not expect a seekbar inside the error dialog.
14. Open Walking options: Prefer is on, Avoid is off, seekbar is visible,
    strength still 50 (or whatever was set in step 3). Route is a Prefer
    route, not Avoid exclusion.

### Bike

15. Repeat steps 4–8 on the Cycling tab (warning before apply; cancel leaves
    mode unchanged; confirm clears Prefer and hides seekbar).
16. Repeat steps 9–14 for a cycling OD with every path fully explored.

### Driving / Android Auto (negative)

17. Driving tab: Prefer + seekbar only. No Avoid row.
18. Android Auto driving options: Prefer toggle only. No Avoid row. No Prefer
    fallback button required on the car screen.

### Must not happen

19. Avoid never applies without the warning having been shown that time.
20. No silent switch to Prefer, no automatic second search, no min-connection
    pass, no download prompt for Avoid-impossible.

## Failure and rollback considerations

- Do not catch `RouteNotFound` and retry Prefer automatically.
- Do not show only “Open settings”.
- Do not implement min-connection to “match the spec” against SPD-042.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-058-avoid-fallback-ux-35cf` |
| Test output | See below. |
| Manual validation | Device residual → SP-061 / Phase 10. This environment cannot complete the OD. |
| Accepted by | |
| Accepted date | |

### Automated tests (executed 2026-08-15)

```
cd android && ./gradlew :sdk:testDebugUnitTest \
  --tests app.organicmaps.sdk.routing.RoutingBuildErrorTest \
  --tests app.organicmaps.sdk.routing.StreetExplorationRoutingOptionsTest \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks
```

`BUILD SUCCESSFUL in 3s`

- `RoutingBuildErrorTest`: **11/11 OK**, 0 failures, 0 errors
- `StreetExplorationRoutingOptionsTest`: **5/5 OK**, 0 failures, 0 errors
- Total: **16/16 OK**, 100% successful

JUnit covers `preferFallback` and result-code mapping only. Production
fallback path is `RoutingErrorDialogFragment` positive click →
`preferFallback` → `SaveToSettings` (JNI) → `rebuildLastRoute()`.

App Java compile (F-Droid debug, native CMake skipped):

```
cd android && ./gradlew :app:compileFdroidDebugJavaWithJavac \
  -x externalNativeBuildDebug -x externalNativeBuildRelease \
  -x configureCMakeDebug -x buildCMakeDebug
```

`BUILD SUCCESSFUL in 18s` (deprecation warnings only; no errors).

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| SP-057 `isDrivingOptionsBuildError()` steal of Avoid no-route when ferry/toll/etc. are on | Fixed here: `RoutingBuildError.isDrivingOptionsBuildError` excludes `AVOID_EXPLORED_NO_ROUTE` |
