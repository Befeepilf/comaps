# SP-031 — Area-pipeline end-to-end validation

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Validate Phase 4 exit criteria with automated fixtures and device/manual
inspection for at least one full country (dense subdivisions, sparse, rural,
coastal/fragmented as available).

## Motivation

Phase 4 is high-uncertainty. Exit needs evidence that polygons, config,
assignment, fallback, and size budget hold — same gate pattern as SP-014/022.

## In-scope behavior

- Validation plan + evidence log under `docs/implementation/validation/`.
- Automated assignment/determinism suite green; counts recorded.
- Manual: known city subdivision names look right; settlement-only city;
  rural no-area; MWM/sidecar size vs SP-023 budget.
- Confirm no MWM country id shown as a neighbourhood.

## Out-of-scope behavior

- Phase 5 UI polish.
- Fixing defects without routing to owning SP-023–030.

## Relevant product requirements

- Phase 4 exit criteria 1–8.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed scenarios.
2. Each Phase 4 exit criterion has pass/fail/residual.
3. Maintainer accepts Phase 4 exit or records residuals.

## Required automated tests

- Full relevant unit targets (assignment, config, generator fixtures).

## Required manual validation

- At least one full-country inspection as in phase manual strategy.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | |
| Evidence log | |
| Test output | |
| Exit criteria table | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
