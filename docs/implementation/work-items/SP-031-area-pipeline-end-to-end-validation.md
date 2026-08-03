# SP-031 — Area-pipeline end-to-end validation

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-025–030 implemented (or explicitly residualled)

---

## Objective

Validate Phase 4 exit criteria with automated fixtures and device/manual
inspection for at least one full country (dense subdivisions, sparse, rural,
coastal/fragmented as available). Same gate pattern as SP-014 / SP-022.

## Motivation

Phase 4 is high-uncertainty. Exit needs evidence that polygons, config,
assignment, fallback, and size budget hold.

## In-scope behavior

- Validation plan + evidence log under `docs/implementation/validation/`.
- Automated assignment/determinism suite green; counts recorded.
- Manual: known city subdivision names look right; settlement-only city;
  rural no-area; **sidecar** (+ assignment blob) size vs SP-023 budget /
  SPD-020 acceptance (exit #7).
- Confirm no MWM country id shown as a neighbourhood.
- Exit criteria table (pass / fail / residual) for criteria 1–8.
- Device walks may residual to Phase 10 if OEM/device access blocks (same
  pattern as Phase 2/3).
- **Note (SPD-024):** exit #7 is sidecar/blob size acceptance — there is
  **no** V1 numeric client pixel/area floor to validate yet; do not invent one
  in the evidence log.

## Out-of-scope behavior

- Phase 5 UI polish.
- Fixing defects without routing to owning SP-023–030.
- Worldwide config completeness.
- Inventing or enforcing numeric suitability floors (SPD-024).

## Relevant product requirements

- Phase 4 exit criteria 1–8.
- SPD-004, SPD-006, SPD-007, SPD-020–025.

## Exit criteria checklist (fill in evidence log)

| # | Criterion | Result |
| --- | --- | --- |
| 1 | True closed polygons available for fixture country | |
| 2 | Versioned country config applied by priority | |
| 3 | Every valid street pixel ≤1 area; deterministic | |
| 4 | Smallest-polygon + stable-id tie-break tested | |
| 5 | Settlement fallback | |
| 6 | Outside settlements: exploration works, no area | |
| 7 | Sidecar/assignment-blob size measured and accepted (no client numeric floor yet — SPD-024) | |
| 8 | No MWM country id as neighbourhood | |

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed scenarios.
2. Each Phase 4 exit criterion has pass/fail/residual.
3. Maintainer accepts Phase 4 exit or records residuals (incl. Phase 10 walks).

## Required automated tests

- Full relevant unit targets (assignment, config, generator fixtures).

## Required manual validation

- At least one full-country inspection as in phase manual strategy.
- Device walks if available; else residual explicitly.

## Failure and rollback considerations

- Do not weaken tests to pass the gate; record conflicts and owning work item.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | |
| Evidence log | |
| Decision ids (SP-024) | SPD-020–025 |
| Test output | |
| Exit criteria table | |
| Residuals | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
