# SP-022 — Exploration storage end-to-end validation

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Accepted
**Branch:** `street-pixels`

---

## Objective

Validate Phase 3 exit criteria on real hardware and with the automated
regression suite. Evidence only; no production behaviour changes on the
validation branch except defect fixes routed to owning SP-015–021 items.

## Motivation

Phase 2 used SP-014 as the exit gate. Phase 3's permanence and rematch promises
are user-data-critical and need the same treatment: plan, device matrix notes,
evidence log, and a human exit decision.

## In-scope behavior

- Validation plan and evidence log under `docs/implementation/validation/`.
- Re-run `street_pixels_tests` (full) and record counts.
- Device scenarios from the phase manual strategy:
  - explore → real country update → greens retained; percentage explainable
  - kill during migration → reopen → no exploration loss
  - update while viewing map → UI stays usable / updating state
  - delete country → redownload → behaviour matches SP-018 decision
- Confirm exit criteria 1–8 with pointers to evidence.
- Record residuals for Phase 10 if any (timing on large countries, OEM quirks).

## Out-of-scope behavior

- New features.
- Phase 4 area work.
- Fixing defects inside this item's commits without filing them against the
  owning SP (prefer fix on owner item; tiny validation-blocking fixes allowed
  with explicit note).

## Relevant product requirements

- Phase 3 exit criteria 1–8.
- Spec §27, §15.2–§15.3, §14, §13, §3.6.

## Relevant source files or symbols

- All Phase 3 modules; validation docs only for this item's primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014.
- Prefer Uusimaa (or equally large ~50 MB `.pix` region) for rematch timing,
  RAM behaviour, delete/redownload archive size, and confirmation that 15 m
  sampling (SPD-019) did not densify `.pix`.
- Pixel 3a may remain the handset; region choice matters more than OEM here.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed
   scenarios.
2. Automated `street_pixels_tests` green; count recorded.
3. Each Phase 3 exit criterion has a pass/fail/residual entry.
4. Maintainer accepts Phase 3 exit or records residuals explicitly.

## Required automated tests

- Full `street_pixels_tests` run (no new tests required unless gaps found).

## Required manual validation

- Entire phase manual strategy on at least one device; delete/redownload if
  SP-018 accepted survival.

## Failure and rollback considerations

- Failed exit criterion blocks phase completion; do not weaken tests.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | [SP-022-validation-plan.md](../validation/SP-022-validation-plan.md) |
| Evidence log | [SP-022-evidence-log.md](../validation/SP-022-evidence-log.md) |
| Test output | Plan baseline 2026-08-03: **171/171** All tests passed (`949e04621e`). Phase 3 Accepted on automated exit criteria 1–8 coverage via SP-015–021 + suite; Pixel 3a / Uusimaa device walks remain residual. |
| Device roster | D1 Pixel 3a proposed; walks **deferred** to Phase 10 residual (same posture as SP-014 OEM residual) |
| Exit criteria table | Automated: Met (see owning SP-015–021). Device permanence/toast/Uusimaa timing: residual → Phase 10 |
| Implemented by | Agent |
| Accepted by | Maintainer |
| Accepted date | 2026-08-03 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Device walks (Blocks A–E, Uusimaa S1–S8) not executed at Phase 3 Accept | Phase 10 residual — execute from SP-022 validation plan when building S1 correctness / release hardening |
| `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` intermittent flake | Pre-existing; not Phase 3 — re-run once if sole failure |
