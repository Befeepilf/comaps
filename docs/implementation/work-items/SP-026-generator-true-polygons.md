# SP-026 — Generator: emit true closed exploration polygons

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 (store), SP-025 (which levels/places to emit)

---

## Objective

Emit true closed administrative / place-boundary polygons required by the
country configuration into the store decided in SP-024 (in-MWM and/or sidecar),
without three-box approximation for exploration areas.

## Motivation

Today polygons are collected then boxified into `CityBoundary` for search.
Exploration needs real point-in-polygon geometry and stable identifiers
(SPD-006).

## In-scope behavior

- Retain closed rings for configured levels / place boundaries.
- Emit **only** true closed OSM ways/relations; never synthesise polygons around
  `place=*` nodes (§8.3).
- Classificator / mapcss changes only as required by SP-024 (document upstream
  divergence if levels 5/6/8 are added).
- Serialize into the chosen store with names, stable OSM ids, admin/place
  metadata, and map-data version association.
- If SP-024 chooses generator-precomputed assignment: emit or stage whatever
  assignment artifacts that decision requires (still no invented areas).
- Generator tests for retention on fixtures.

## Out-of-scope behavior

- Client polygon load API (SP-027).
- Client assignment algorithm (SP-028) unless SP-024 makes the generator the
  sole assignment producer — then this item emits precomputed tables and
  SP-028 verifies/consumes them.
- Drawing style for all new levels (may ship non-drawable useful types).
- Worldwide config coverage.
- Area UI (Phase 5).

## Relevant product requirements

- §3.5, §8.3–§8.4; SPD-006; SP-023/024/025.

## Relevant source files or symbols

- `generator/collector_routing_city_boundaries.cpp`, `place_processor.cpp`,
  `cities_boundaries_builder.cpp`
- Store writer path per SP-024

## Acceptance criteria

1. Fixture/test harness can deserialize true ring geometry for the fixture
   country from the chosen store (client production API may still be SP-027).
2. No three-box approximation used as the exploration-area geometry.
3. Size impact matches or is reconcilable with SP-023 budget / SP-024 decision.
4. Generator tests green.
5. No place-node-invented polygons in output.

## Required automated tests

- Generator fixture: expected polygons present with ring geometry.
- Optional golden size bound if SP-023 set one.

## Failure and rollback considerations

- If classificator changes disrupt upstream search/routing, isolate exploration
  types or document divergence; do not silently drop levels without SP-024.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Store format | |
| Size delta | |
| Decision ids (SP-024) | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
