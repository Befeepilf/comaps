# SP-041 — Phase 5 end-to-end validation

**Phase:** 5 — Area progress and map interaction
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-033–040 implemented (or explicitly residualled)
**Notes:** Exit gate. Device residual → Phase 10 pattern (same as SP-014 / SP-022 / SP-031).

---

## Objective

Validate Phase 5 exit criteria with automated fixtures and device/manual
inspection. Produce a validation plan + evidence log. Maintainer decides Phase 5
exit; agent does not mark Accepted unilaterally.

## Motivation

SP-033–040 each validate locally. Exit needs combined evidence: focus rules,
percentages, tap/detail, city rollup, completed/no-area states, and rendering
performance (or recorded residual).

## In-scope behavior

- Validation plan + evidence log under `docs/implementation/validation/`.
- Map each Phase 5 exit criterion (1–8) to pass / fail / residual with
  pointers.
- Re-run relevant automated suites; record counts.
- Manual: boundary walk, pan/recenter focus, tap areas, zoom street→city,
  completed chrome, no-area empty state.
- Rendering: cite SP-033 measurement; re-spot-check if SP-037 changed the
  renderer; device residual → Phase 10 if unavailable.
- Confirm no country/world percentage anywhere.

## Out-of-scope behavior

- New features beyond defect fixes routed to owning SP-033–040.
- Phase 7 milestones / Phase 8 competition.
- Marking Phase 5 **Accepted** or exit Met without maintainer decision.
- Weakening tests to pass the gate.

## Relevant product requirements

- Phase 5 exit criteria 1–8.
- Spec §7, §12, §18.6, §31, §34 Progress + Quality.

## Relevant source files or symbols

- All Phase 5 modules; validation docs are this item's primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-031.
- Device residual honesty: if mid-tier Android unavailable, residual walks and
  Spike 1 device re-measure to Phase 10; do not fabricate.
- Desktop suites remain mandatory.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed scenarios.
2. Each Phase 5 exit criterion has pass/fail/residual.
3. Automated suites green; counts recorded.
4. Maintainer accepts Phase 5 exit or records residuals (incl. Phase 10).

## Required automated tests

- Full relevant unit targets (completion, focus, hit-test, aggregation, etc.).

## Required manual validation

- Phase manual strategy on device if available; else explicit Phase 10 residual.

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
| | |
