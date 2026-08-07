# SP-031 — Area-pipeline end-to-end validation

**Phase:** 4 — Administrative-area pipeline
**Status:** In review
**Branch:** `cursor/sp-031-area-pipeline-validation-191e` (lands on `street-pixels`)
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
| 1 | True closed polygons available for fixture country | Residual (fixtures/library green; mapgen emit / no shipping FI `.spa` — full-country bar unmet) |
| 2 | Versioned country config applied by priority | Pass |
| 3 | Every valid street pixel ≤1 area; deterministic | Pass |
| 4 | Smallest-polygon + stable-id tie-break tested | Pass |
| 5 | Settlement fallback | Pass |
| 6 | Outside settlements: exploration works, no area | Pass (automated) + Residual (device) |
| 7 | Sidecar/assignment-blob size measured and accepted (no client numeric floor yet — SPD-024) | Residual (shipping encoder unmeasured; SPD-024 — no floor invented) |
| 8 | No MWM country id as neighbourhood | Pass (automated) + Residual (device UI) |

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
| Validation plan | [SP-031-validation-plan.md](../validation/SP-031-validation-plan.md) |
| Evidence log | [SP-031-evidence-log.md](../validation/SP-031-evidence-log.md) |
| Decision ids (SP-024) | SPD-020–025 |
| Test output | Rebuild SHA `e10111c537`: `street_pixels_areas_tests` **44/44**; `street_pixels_tests --filter=Rematch` **18/18**; `--filter=AssignmentPersist` **3/3**; `--filter=CountryConfig` **11/11**; full `street_pixels_tests` **185/185** (PauseResume flake absent). |
| Exit criteria table | See evidence log — 1 Residual; 2–5 Pass; 6/8 Pass+Residual; 7 Residual |
| Residuals | R1 mapgen emit; R2 shipping size; R3 device walks; R4 `/tmp/sp023` Helsinki spot-check absent |
| Implemented by | Agent |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Production mapgen emit (collectors → `.spa`) still not wired | Owning follow-up from SP-026; required before claiming exit #1 Pass for shipping FI |
| Shipping-encoder FI size not measured this run (`/tmp/sp023` absent) | Exit #7 residual; re-measure under emit/offline harness; no SPD-024 floor |
| Device / Helsinki UI walks not executed | Phase 10 residual (SP-014/022 pattern) |
| Phase 4 “Current code locations” table still says area id / assignment Not found (dated 2026-08-03) | Docs hygiene; refresh after Phase 4 exit decision — not required to record SP-031 suite evidence |
