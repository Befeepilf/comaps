# SP-079 — Phase 8 end-to-end validation

**Phase:** 8 — Competition
**Status:** Not started
**Branch:**
**Depends on:** SP-070–078 implemented or explicitly residualled
**Notes:** Exit gate. Device residual → Phase 10 pattern (SP-014 / SP-041
  / SP-061 / SP-069). Maintainer decides Phase 8 exit; agent does not
  mark Accepted.

---

## Objective

Validate Phase 8 exit criteria with automated fixtures and documented
manual/device inspection. Produce a validation plan + evidence log.
Maintainer decides Phase 8 exit.

## Motivation

SP-071–078 each validate locally. Exit needs combined evidence: consent,
unique nicknames, aggregate-only delayed upload, ownership / eligibility /
decay / contested / unclaimed, weekly board without revisits/imports,
server sparse-area anonymity, nickname moderation, deletion, and no
presence leak.

## In-scope behavior

- Validation plan + evidence log under
  `docs/implementation/validation/`
  (`SP-079-validation-plan.md`, `SP-079-evidence-log.md`).
- Map each Phase 8 exit criterion (1–12) to pass / fail / residual.
- Re-run relevant automated suites; record counts. Minimum:
  `street_pixels_tests` (recency, ownership, eligibility, weekly,
  upload cadence, payload deny-list, card copy, 30-pixel hint) and
  backend competition tests when that checkout is present.
- Manual (phase-08 strategy): opt-in vs §20.2; traffic capture; opt-out
  zero upload; offline queue; N < 3 nicknames; decay without opening the
  app (server); delete profile / local exploration intact; no presence
  copy.
- Device residual honesty if no handset: Phase 10; do not fabricate.
- Backend-missing residual: record if `comaps_backend` is not in the
  environment; do not fake ingest tests.

## Out-of-scope behavior

- New features beyond defect fixes routed to owning SP-071–078.
- Marking Phase 8 Accepted or exit Met without maintainer decision.
- Weakening tests to pass the gate.
- Friends feature revival.

## Relevant product requirements

- Phase 8 exit criteria 1–12.
- Spec §20–§26, §22.10, §34 Privacy and competition.
- SPD-014, SPD-057–066.

## Relevant source files or symbols

- All Phase 8 competition modules; validation docs are this item’s
  primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-031 / SP-041 /
  SP-061 / SP-069.
- Desktop suites remain mandatory.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed
   scenarios.
2. Each Phase 8 exit criterion has pass/fail/residual.
3. Automated suites green; counts recorded.
4. Maintainer accepts Phase 8 exit or records residuals (incl. Phase 10
   / backend ops).

## Required automated tests

- Full relevant unit targets listed above.

## Required manual validation

- Phase-08 manual strategy on device if available; else explicit Phase 10
  residual. Traffic capture required for the upload deny-list if a device
  is available.

## Failure and rollback considerations

- Failed exit criterion blocks phase completion; report owning WI; do
  not weaken tests.

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
