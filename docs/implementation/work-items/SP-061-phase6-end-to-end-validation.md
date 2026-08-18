# SP-061 — Phase 6 end-to-end validation

**Phase:** 6 — Exploration-aware routing
**Status:** In progress
**Branch:** `cursor/sp-061-phase6-validation-35cf`
**Depends on:** SP-054–060 implemented or explicitly residualled
**Notes:** Exit gate. Device residual → Phase 10 pattern (SP-014 / SP-041).

---

## Objective

Validate Phase 6 exit criteria with automated fixtures and documented
manual/device inspection. Produce a validation plan + evidence log.
Maintainer decides Phase 6 exit; agent does not mark Accepted
unilaterally.

## Motivation

SP-054–060 each validate locally. Exit needs combined evidence: Prefer on
walk/bike, Avoid strict pass, warning + Prefer+strength fallback, mid-nav
stability, count-only analytics, and routing regression suites.

## In-scope behavior

- Validation plan + evidence log under `docs/implementation/validation/`
  (`SP-061-validation-plan.md`, `SP-061-evidence-log.md`).
- Map each Phase 6 exit criterion (1–7) to pass / fail / residual with
  pointers.
- Re-run relevant automated suites; record counts. Minimum:
  `street_pixels_tests` (including multiplier / avoid fixtures added in
  this phase), `routing_tests`, `routing_common_tests`.
- Manual: walk and bike Prefer (including seekbar); Avoid with a possible
  route that skips fully explored edges; Avoid with an impossible route and
  the Prefer+seekbar control; mid-nav walk along an Avoid route; confirm no
  location in analytics payloads.
- Device residual honesty if no handset: Phase 10; do not fabricate.
- Confirm car did not gain Avoid (R2) and §17.4 modes were not added.

## Out-of-scope behavior

- New features beyond defect fixes routed to owning SP-054–060.
- Phases 7–9.
- Marking Phase 6 Accepted or exit Met without maintainer decision.
- Weakening tests to pass the gate.
- Requiring `routing_integration_tests` (world dataset) unless already
  runnable in this environment.

## Relevant product requirements

- Phase 6 exit criteria 1–7.
- Spec §17, §31, §32.2, §34 Routing.
- SPD-009; SPD-040–045.

## Relevant source files or symbols

- All Phase 6 routing/UI/analytics modules; validation docs are this
  item’s primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-031 / SP-041.
- Desktop suites remain mandatory.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed
   scenarios.
2. Each Phase 6 exit criterion has pass/fail/residual.
3. Automated suites green; counts recorded.
4. Maintainer accepts Phase 6 exit or records residuals (incl. Phase 10).

## Required automated tests

- Full relevant unit targets listed above.

## Required manual validation

- Phase-06 manual strategy on device if available; else explicit Phase 10
  residual.

## Failure and rollback considerations

- Failed exit criterion blocks phase completion; report owning WI; do not
  weaken tests.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | |
| Evidence log | |
| Test output | |
| Device roster / residual | |
| Exit criteria table | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during validation) | |
