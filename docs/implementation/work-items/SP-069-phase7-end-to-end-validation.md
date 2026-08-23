# SP-069 — Phase 7 end-to-end validation

**Phase:** 7 — Milestones and share cards
**Status:** Planned
**Depends on:** SP-062–068 implemented or explicitly residualled
**Notes:** Exit gate. Device residual → Phase 10 pattern (SP-014 / SP-041 /
  SP-061). Exit waits on Accepted locks (OQ-9–OQ-18 / draft SPD-046–055) or
  explicit deferrals.

---

## Objective

Validate Phase 7 exit criteria with automated fixtures and documented
manual/device inspection. Produce a validation plan + evidence log.
Maintainer decides Phase 7 exit; agent does not mark Accepted
unilaterally.

## Motivation

SP-062–068 each validate locally. Exit needs combined evidence: fire-once
milestones, first-100 m, non-interrupting 100% card, deny-list card,
explicit share, haptics predicate, §27.4 survival, and count-only growth
analytics.

## In-scope behavior

- Validation plan + evidence log under `docs/implementation/validation/`
  (`SP-069-validation-plan.md`, `SP-069-evidence-log.md`).
- Map each Phase 7 exit criterion (1–9) to pass / fail / residual with
  pointers.
- Re-run relevant automated suites; record counts. Minimum:
  `street_pixels_tests` (milestone, first-goal, haptics, card-model
  deny-list), plus any new target this phase added.
- Manual (phase-07 strategy): small-area 100% celebration; card image vs
  exclusion list; competition-off first-person copy; share sheet only on
  tap; haptics with screen off / background / toggle off; navigation not
  interrupted. Competition-on leading/not-leading copy (§22.10) is Phase 8
  residual if competition is not yet present — do not fail Phase 7 exit
  on live competition sentences.
- Device residual honesty if no handset: Phase 10; do not fabricate.

## Out-of-scope behavior

- New features beyond defect fixes routed to owning SP-062–068.
- Phase 8 competition chrome.
- Marking Phase 7 Accepted or exit Met without maintainer decision.
- Weakening tests to pass the gate.

## Relevant product requirements

- Phase 7 exit criteria 1–9.
- Spec §10 steps 6/8/9, §18, §19, §22.10, §27.4, §28, §32.4, §34
  Progress experience and Sharing.
- SPD-008; SP-062 locks.

## Relevant source files or symbols

- All Phase 7 milestone / haptic / card / share modules; validation docs
  are this item’s primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-031 / SP-041 / SP-061.
- Desktop suites remain mandatory.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed
   scenarios.
2. Each Phase 7 exit criterion has pass/fail/residual.
3. Automated suites green; counts recorded.
4. Maintainer accepts Phase 7 exit or records residuals (incl. Phase 10).

## Required automated tests

- Full relevant unit targets listed above.

## Required manual validation

- Phase-07 manual strategy on device if available; else explicit Phase 10
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
| Device review 2026-08-22 (`?achievements` on `app.comaps.test`): first-goal `7/10` copy is confusing; 100% card is unstyled with a blank-looking outline; date checkbox is unclear. | SP-064 / SP-067 / SP-068 discovered follow-ups. Date-always-on is **SPD-056** (checkbox removal residual). |
| Same session: process crash after the 100% card (`IllegalFormatConversionException` on the 25% toast). | Fixed in `street_pixels_area_milestone_25` (`25%%`). Re-run `?achievements` through 25% on device. |
