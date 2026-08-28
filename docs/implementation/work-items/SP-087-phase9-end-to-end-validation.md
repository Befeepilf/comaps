# SP-087 — Phase 9 end-to-end validation

**Phase:** 9 — GPX and feature gating
**Status:** In progress
**Branch:** `cursor/sp-087-phase9-validation-db9d`
**Depends on:** SP-080–086 implemented or explicitly residualled
**Notes:** Exit gate. Device residual → Phase 10 pattern (SP-014 / SP-041
  / SP-061 / SP-069 / SP-079). Maintainer decides Phase 9 exit; agent
  does not mark Accepted.

---

## Objective

Validate Phase 9 exit criteria with automated fixtures and documented
manual/device inspection. Produce a validation plan + evidence log.
Maintainer decides Phase 9 exit.

## Motivation

SP-081–086 each validate locally. Exit needs combined evidence: imported
pixels and personal completion, competition isolation regardless of
gate, Pro gating, public builds with no GPX tooling and no purchase
action, large/malformed import behaviour, existing `gpx_tests`, and
monetisation counters only when Pro is on.

## In-scope behavior

- Validation plan + evidence log under
  `docs/implementation/validation/`
  (`SP-087-validation-plan.md`, `SP-087-evidence-log.md`).
- Map each Phase 9 exit criterion (1–8) to pass / fail / residual.
- Re-run relevant automated suites; record counts. Minimum:
  `street_pixels_tests` (import, ever-live, recency, weekly, upload
  pending, explorer_pro matrix, analytics) and `gpx_tests`.
- Manual (phase-09 strategy):
  - Real multi-hour GPX: green pixels, area % up
  - Competition on: no ownership change, no weekly movement
  - Import over already-live area: competitive position unchanged
  - Public-configured build: no GPX import/export/purchase, including
    share-sheet and VIEW intents
  - Pro-enabled internal build (G7): tools appear and work
  - Batch-import several files
- Device residual honesty if no handset: Phase 10; do not fabricate.

## Out-of-scope behavior

- New features beyond defect fixes routed to owning SP-081–086.
- Billing (SPD-010).
- Marking Phase 9 Accepted or exit Met without maintainer decision.
- Weakening tests to pass the gate.

## Relevant product requirements

- Phase 9 exit criteria 1–8.
- Spec §7, §15.3, §16.1, §22.2, §24.1, §29, §30, §32.5, §34 Explorer Pro.
- SPD-010, SPD-011, SPD-015, and SP-080 Accepted locks.

## Relevant source files or symbols

- All Phase 9 GPX / gate / analytics modules; validation docs are this
  item’s primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-041 / SP-061 /
  SP-069 / SP-079.
- Desktop suites remain mandatory.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed
   scenarios.
2. Each Phase 9 exit criterion has pass/fail/residual.
3. Automated suites green; counts recorded.
4. Maintainer accepts Phase 9 exit or records residuals (incl. Phase 10).

## Required automated tests

- Full relevant unit targets listed above.

## Required manual validation

- Phase-09 manual strategy. No handset → residual, not a fake pass.

## Failure and rollback considerations

- Incomplete evidence blocks exit. Do not mark exit on desktop-only if
  a criterion is device-only; residual it.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-087-phase9-validation-db9d` |
| Validation plan | `docs/implementation/validation/SP-087-validation-plan.md` |
| Evidence log | `docs/implementation/validation/SP-087-evidence-log.md` (template; filled after suites) |
| Accepted by | — |
| Accepted date | — |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
