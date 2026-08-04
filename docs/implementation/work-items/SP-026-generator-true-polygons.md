# SP-026 — Generator: emit true closed exploration polygons

**Phase:** 4 — Administrative-area pipeline
**Status:** In review
**Branch:** `cursor/sp-026-generator-exploration-sidecar-191e`
**Depends on:** SP-024 Accepted (SPD-020 store, SPD-021 precompute), SP-025
  (which levels/places to emit)

---

## Objective

Emit true closed administrative / place-boundary polygons required by the
country configuration into the **per-country downloadable sidecar** (SPD-020),
without three-box approximation for exploration areas. Also emit or stage the
**generator-precomputed** assignment artifact (SPD-021) — dense uint16/uint32
area-index map plus id table (SPD-022).

## Motivation

Today polygons are collected then boxified into `CityBoundary` for search.
Exploration needs real point-in-polygon geometry and stable identifiers
(SPD-006). SP-023 showed Finland rings fit a sidecar budget; World three-box
remains search-only (SPD-020, SPD-025).

## In-scope behavior

- Retain closed rings for configured levels / place boundaries (SPD-023
  Finland seed when emitting FI).
- Emit **only** true closed OSM ways/relations; never synthesise polygons around
  `place=*` nodes (§8.3).
- Include **true municipal (settlement) rings** in the same sidecar for
  SPD-007 fallback (SPD-025) — not three-box.
- Classificator / mapcss changes only as required; document upstream
  divergence if levels 5/6/8 are added (sidecar may avoid drawable-type
  pressure — still verify).
- Serialize into the sidecar with names, stable OSM ids, admin/place
  metadata, map-data version association, and the precomputed dense
  assignment map (compact area index — no full-universe uint64 OSM id column).
- No invented numeric size floors in emission filters (SPD-024).
- Generator tests for retention on fixtures.

## Out-of-scope behavior

- Client polygon load API (SP-027).
- Client primary full-universe PIP assignment (rejected by SPD-021) — SP-028
  verifies/consumes generator output.
- Drawing style for all new levels (may ship non-drawable useful types).
- Worldwide config coverage.
- Area UI (Phase 5).

## Relevant product requirements

- §3.5, §8.3–§8.4; SPD-006, SPD-020, SPD-021, SPD-022, SPD-025; SP-023/024/025.

## Relevant source files or symbols

- `libs/street_pixels_areas/` — `.spa` format, filter, §8.8 assigner, writer/reader
- `defines.hpp` — `SPA_FILE_EXTENSION`, section tags
- Format notes: `docs/implementation/notes/SP-026-spa-format.md`
- `generator/` links `street_pixels_areas` (full mapgen emission hook deferred)
- Does **not** extend `place_processor` / three-box for exploration

## Acceptance criteria

1. Fixture/test harness can deserialize true ring geometry for the fixture
   country from the sidecar (client production API may still be SP-027).
2. No three-box approximation used as the exploration-area geometry.
3. Size impact matches or is reconcilable with SP-023 budget / SPD-020.
4. Precomputed dense assignment artifact present and keyed for
   (map-data version, policy_version).
5. Generator tests green.
6. No place-node-invented polygons in output.

## Required automated tests

- Generator fixture: expected polygons present with ring geometry.
- Optional golden size bound if SP-023 set one.
- Precomputed map round-trip / determinism smoke.

## Failure and rollback considerations

- If classificator changes disrupt upstream search/routing, isolate exploration
  types or document divergence; do not silently drop levels without a new SPD.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-026-generator-exploration-sidecar-191e` |
| Commits | `5d0d0e79f` [generator][cmake] library+tests; `33c8dce03` [generator] DebugPrint/∞ fix; docs evidence |
| Store format | per-MWM `.spa` FilesContainer (hdr/areas/assign); true rings via `SaveOuterPath`; dense uint16/uint32 compact index + sentinel (SPD-020/021/022) |
| Size delta | Not re-measured with shipping encoder on full FI (follow-up / SP-031 exit #7); fixture path only in CI |
| Decision ids (SP-024) | SPD-020, SPD-021, SPD-022, SPD-025 (plus SPD-004/006/023/024 constraints) |
| Test output | `./tools/unix/run_tests.sh -b …/omim-build-debug -f "ExplorationFilter_\|SubdivisionAssigner_\|SpaSerdes_"` — 15/15 OK; `street_pixels_tests` links; `CountryConfig_` 11/11 OK |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Full generator mapgen emission from OSM collectors into `.spa` not wired | SP-026 intermediate ships format+library+fixture tests; wire emission in a follow-up commit/item before SP-031 |
| Shipping-encoder FI size not re-measured vs SP-023 coded_delta | Optional offline emit from `/tmp/sp023` JSONL + measure under SP-031 exit #7 |
| Classificator / mapcss still lack admin 5/6/8 drawable types | Sidecar avoids drawable pressure; document if/when mapcss needed (SP-024 follow-up) |
| Dense assign samples in writer are caller-supplied (HEALPix universe not generated here) | SP-028 / generator emit job supplies valid-universe sample centres |
| | |
