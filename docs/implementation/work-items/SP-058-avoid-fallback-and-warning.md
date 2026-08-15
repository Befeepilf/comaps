# SP-058 — Avoid warning, no-route fallback offer, min-connection retry

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Depends on:** SP-055 R6–R8, R7 Accepted; SP-056 walk/bike surface; SP-057
  distinct no-route signal
**Unblocks:** SP-059 (follows an Avoid route); SP-061 exit #3–4

---

## Objective

Let the user turn Avoid on only after the spec warning, and when the strict
pass is impossible, explicitly offer min-connection versus return-to-normal.
The selected rule is never silently abandoned.

## Motivation

Spec §17.3 and §31 define the warning and the two options. Generic
`onDrivingOptionsBuildError` (“unable to calc — open settings”) is not that
contract. Min-connection is a **second search** after an explicit choice
(R7), not an automatic retry.

## In-scope behavior

- Walk/bike control to select Avoid (R3), with §17.3 warning **before** the
  mode is applied (R8). Warning copy: “This can produce very long routes or
  no available route.”
- On SP-057’s distinct no-route result, show explanation that no fully
  unexplored route is available, and offer:
  1. Allow the minimum necessary explored connection (§17.3 label).
  2. Return to normal routing.
- Choice 1: second search minimising explored **distance** (R7) while
  keeping ordinary weights on unexplored edges. Success shows a route;
  Avoid remains the selected rule (user allowed a bounded exception for
  **this** route). If even this pass fails, explain; do not silently switch
  mode.
- Choice 2: set mode to Standard, recompute, and make it obvious Avoid is
  off.
- Never auto-run choice 1 or 2.
- English strings in `values/strings.xml` (and `values-en` if that is the
  existing pattern). Do not machine-translate all locales in this item;
  follow repo translation practice.
- Tests: UI-state / controller logic if it lives in shared C++; otherwise
  C++ tests for the min-connection cost on a fixture graph plus a documented
  Android manual script.

## Out-of-scope behavior

- Strict exclusion engine (SP-057).
- Mid-navigation thrash policy (SP-059).
- Analytics counters (SP-060) — may call a stub if SP-060 lands first.
- Car Avoid.
- §17.4 deferred routing modes.

## Relevant product requirements

- Spec §17.3, §31, §34 Routing; SPD-009.
- SP-055 R3, R6, R7, R8.

## Relevant source files or symbols

- `WalkingOptionsFragment`, `CyclingOptionsFragment`, layouts
- `MwmActivity.onCommonBuildError` / `onDrivingOptionsBuildError` /
  `RoutingErrorDialogFragment` / `ResultCodesHelper`
- JNI result-code mirror
- Shared min-connection weight hook from SP-057 or added here
- `android/app/src/main/res/values/strings.xml`

## Implementation notes / constraints

- Reuse the existing routing-error dialog pattern; do not invent a second
  router. Distinguish Avoid-impossible from missing maps / GPS.
- Min-connection must not be implemented as “turn on Prefer at 100”.
- Returning to normal clears Avoid (R8). Do not leave Avoid enabled with
  standard weights.
- Offline-only.

## Acceptance criteria

1. Avoid cannot be applied without the §17.3 warning having been shown.
2. Impossible strict pass shows both spec options; no generic-only dialog.
3. Min-connection produces a fixture route with least explored metres when
   an explored bridge exists.
4. Return-to-normal clears Avoid and recomputes standard.
5. No code path auto-retries without the user choosing.

## Required automated tests

- Min-connection cost on a fixture (explored bridge vs long unexplored
  detour that does not exist — expect the bridge; vs a fully unexplored
  alternative — that case is SP-057 strict pass, not this retry).
- Result-code mapping does not treat Avoid-impossible as missing-maps
  download.

## Required manual validation

- Fully explored OD: fallback appears; both choices behave as specified.
  Walk and bike. Device residual → SP-061 / Phase 10.

## Failure and rollback considerations

- Do not catch `RouteNotFound` and retry Prefer.
- Do not show only “Open settings”.

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
