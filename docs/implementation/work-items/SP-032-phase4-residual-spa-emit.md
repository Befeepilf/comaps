# SP-032 — Phase 4 residual: offline `.spa` emit harness

**Phase:** 4 — Administrative-area pipeline
**Status:** In review
**Branch:** `cursor/sp-032-phase4-residual-emit-191e` (lands on `street-pixels`)
**Depends on:** SP-026 (format/library), SP-031 residuals R1/R2/R4 (partial)
**Unblocks:** SP-031 exit #1 / #7 Pass (device walks R3 stay Phase 10)

---

## Objective

Close SP-031 residuals that block Phase 4 exit criteria **#1** (true closed
polygons available for fixture country) and **#7** (sidecar size measured vs
SP-023 baseline under the shipping encoder). Provide an offline emit harness
that reads SP-023 JSONL rings + FI country policy, runs
`FilterExplorationCandidate` → `WriteExplorationSidecar` (geometry-only
`assign_count=0` OK), and emits Helsinki leaf + FI country-concat `.spa` under
`/tmp/sp032/` (not committed).

## Motivation

SP-031 recorded automated suite Pass for exits 2–5 and automated halves of 6/8,
but left exit #1 / #7 Residual because production mapgen emit is unwired and
shipping-encoder FI sizes were unmeasured (`/tmp/sp023` absent). Full collector
→ mapgen wiring remains a pre-production follow-up; an offline harness is
enough to demonstrate true closed rings + `SaveOuterPath` sizes for Finland.

## In-scope behavior

- Work item + evidence updates (SP-031 evidence log, README, phase-04, SP-026
  emission pointer).
- `tools/spa_emit_tool/` CLI: JSONL + policy → Helsinki leaf `.spa` + FI
  country-concat `.spa`; section size report; Helsinki known-id spot-check.
- Shared library helpers in `libs/street_pixels_areas/spa_jsonl.*` (parse /
  filter / geometry-only write / section sizes / known-id spot-check).
- Tiny CI fixture test: emit→load round-trip from a synthetic JSONL.
- Re-fetch/rebuild `/tmp/sp023` JSONL from Geofabrik FI PBF when absent.
- Measure `SaveOuterPath` sizes vs SP-023 zlib(coded_delta) baselines; document
  in SP-031 evidence (no numeric floor — SPD-024).

## Out-of-scope behavior

- Full generator mapgen / OSM collectors → `.spa` wiring (still SP-026
  follow-up / pre-production).
- Device walks (R3 → Phase 10).
- Inventing grids, numeric client floors, city allowlists.
- Committing PBF / JSONL / `.spa` blobs.
- Marking Phase 4 exit criteria Met or SP-031/032 Accepted unilaterally.

## Relevant product requirements / decisions

- SPD-020 (per-country sidecar), SPD-021 (precomputed assign; geometry-only OK
  for size/fixture), SPD-023 (FI policy), SPD-024 (no numeric floors).
- Phase 4 exit #1, #7.
- Spec §8.3 (no place-node invention), §8.8 (filter/assigner elsewhere).

## Acceptance criteria

1. Offline harness builds and emits Helsinki + FI country-concat `.spa` under
   `/tmp/sp032/` from SP-023 JSONL + shipped FI policy.
2. Section sizes reported; Helsinki known OSM ids spot-checked.
3. Tiny `street_pixels_areas_tests` emit→load fixture green.
4. SP-031 evidence updated: exit #1/#7 → Pass (with mapgen emit still noted as
   residual for shipping production path); R1 narrowed / R2+R4 closed; R3
   Phase 10.
5. README / phase-04 / SP-026 note point at the emit harness.
6. Maintainer decides acceptance; agent does not mark Accepted or Phase 4 Met.

## Required automated tests

- `street_pixels_areas_tests` including new `SpaJsonlEmit_*` cases.

## Required manual / offline validation

- Rebuild `/tmp/sp023` JSONL if needed; run `spa_emit_tool`; record sizes and
  spot-check in SP-031 evidence.

## Failure and rollback considerations

- Do not invent floors to “accept” size; report measured bytes vs SP-023.
- Do not weaken tests.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-032-phase4-residual-emit-191e` |
| Tool | `tools/spa_emit_tool/` → `spa_emit_tool` |
| Library | `libs/street_pixels_areas/spa_jsonl.*` |
| Emit outputs | `/tmp/sp032/Finland.spa`, `/tmp/sp032/Finland_Southern Finland_Helsinki.spa` (local only) |
| Size table | FI **2 019 268 B (~1.93 MiB)**; Helsinki **456 484 B (~0.44 MiB)**; assign 0; vs SP-023 ~2.06 / ~0.52 MiB zlib coded |
| Spot-check | Helsinki known OSM ids **11/11** found + name_match |
| Test output | `./tools/unix/build_omim.sh -d -p /workspace spa_emit_tool street_pixels_areas_tests`; `./omim-build-debug/street_pixels_areas_tests` — **46/46 OK** (`SpaJsonlEmit_*` 2/2); emit re-verify sizes unchanged |
| Decision ids | SPD-020, SPD-021, SPD-023, SPD-024 |
| Implemented by | Agent |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Full generator mapgen emission (collectors → `.spa`) still not wired | Pre-production emit item / SP-026 follow-up; offline harness satisfies exit #1 availability bar for fixture country |
| Device walks (Helsinki UX, rural/coastal, no MWM-id neighbourhood in UI) | Phase 10 (R3) |
| Dense assignment blob size not measured (geometry-only emit) | Optional when HEALPix universe samples available from mapgen emit job |
| | |
