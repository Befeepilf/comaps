# SP-060 — Count-only routing-mode analytics

**Phase:** 6 — Exploration-aware routing
**Status:** In progress
**Branch:** `cursor/sp-060-routing-mode-analytics-35cf`
**Depends on:** SPD-044; SP-056/058 mode changes exist to hook
**Unblocks:** SP-061 exit #6

---

## Objective

Record prefer-unexplored and avoid-explored **usage counts** with no
location data, matching spec §32.2 and phase-06 exit #6.

## Motivation

SP-003 deliberately did not add product-analytics events. Exit #6 still
requires mode-usage measurement. There is no privacy-safe upload sink in
tree. R10: implement a shared counter API now; residual **upload** to
Phase 10 rather than sending routes to Sentry.

## In-scope behavior

- Shared counter API (names, not coordinates) for at least:
  - prefer used (route built under Prefer)
  - avoid used (route built under Avoid strict pass)
  - avoid-fallback-prefer (user took the SPD-042 switch)
- Increment from the routing success / fallback-choice paths (not from
  every GPS tick).
- Persistence optional; in-memory + process-lifetime is enough for V1 if
  documented. If persisted, store integers only.
- Tests: increment on the corresponding fake events; assert payloads
  contain no lat/lon, geometry, pixel id, or area id.
- If an existing private-by-default aggregate sink is found, wire counts
  only after confirming it cannot attach screenshots, PII, or view
  hierarchy (SP-003 posture). Otherwise local-only + Phase 10 residual.

## Out-of-scope behavior

- A new analytics vendor or backend.
- Sentry `captureMessage` / breadcrumbs that include routes.
- Competition or milestone events (Phases 7–8).
- Monetisation analytics (SPD-010).

## Relevant product requirements

- Spec §32.2, §32 (no raw GPS), §25.1, §34 analytics.
- SP-003 (events were deferred); SPD-044.

## Relevant source files or symbols

- No existing product-event helper found at Phase 6 entry. Add a small
  shared module under `libs/` (e.g. beside `explorer_pro` / settings).
- Android call sites in routing plan / SP-058 dialog callbacks.

## Implementation notes / constraints

- Counts only. No OD, no polyline, no country id, no pixel id.
- Public builds must not gain a new network endpoint for this item.
- Do not log coordinates alongside the increment.

## Acceptance criteria

1. The three counters exist and increment on the specified actions.
2. Tests prove the recorded event has no location fields.
3. Upload path is either privacy-safe and documented, or explicitly
   residualled to Phase 10.
4. Sentry is not used as the routing-analytics sink.

## Required automated tests

- Increment + payload/schema assertions (no location keys).

## Required manual validation

- Optional: debug read-out of counters after planning a Prefer route.
  Device residual → SP-061 / Phase 10.

## Failure and rollback considerations

- Do not block Phase 6 on an analytics vendor. Local API + residual meets
  R10 if upload is absent.
- Do not weaken exit #6 to “n/a” without recording the residual.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-060-routing-mode-analytics-35cf` |
| Test output | `routing_tests` **306/306** pass (2026-08-17; was 296/296 before this item). Filtered 10/10: 9 `StreetExplorationRoutingAnalytics_*` + `TestAssignRouteIncrementsExplorationAnalytics`. `cd android && ./gradlew :app:compileFdroidDebugJavaWithJavac` → BUILD SUCCESSFUL (native CMake skipped). |
| Sink (local / upload / residual) | local settings integers (`street_exploration_routing_analytics_*`); upload residual Phase 10. Not Sentry. |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| No privacy-safe upload sink | Phase 10 residual (do not send through Sentry) |
| No in-app debug readout of counters | Optional residual → SP-061 / Phase 10 |
| Android Auto toast path is not the SPD-042 switch | Out of scope; do not count as avoid-fallback-prefer |
