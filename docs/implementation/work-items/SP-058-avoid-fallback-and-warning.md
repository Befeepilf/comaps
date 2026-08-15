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

## Required manual validation

- OD with every path fully explored: no-route appears; Prefer button
  rebuilds under Prefer with seekbar. Walk and bike. Device residual →
  SP-061 / Phase 10.

## Failure and rollback considerations

- Do not catch `RouteNotFound` and retry Prefer automatically.
- Do not show only “Open settings”.
- Do not implement min-connection to “match the spec” against SPD-042.

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
