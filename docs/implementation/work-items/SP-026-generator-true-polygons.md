# SP-026 — Generator: emit true closed exploration polygons

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

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
- Classificator / mapcss changes only as required by SP-024 (document upstream
  divergence).
- Serialize into the chosen store with names, stable OSM ids, admin/place
  metadata, and map-data version association.
- Generator tests for retention on fixtures.

## Out-of-scope behavior

- Client assignment (SP-028).
- Drawing style for all new levels (may ship non-drawable useful types).
- Worldwide config coverage.

## Relevant product requirements

- §3.5, §8.3–§8.4; SPD-006; SP-023/024/025.

## Acceptance criteria

1. Client (or test harness) can load true polygons for the fixture country.
2. No three-box approximation used as the exploration-area geometry.
3. Size impact matches or is reconcilable with SP-023 budget / SP-024 decision.
4. Generator tests green.

## Required automated tests

- Generator fixture: expected polygons present with ring geometry.
- Optional golden size bound if SP-023 set one.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Store format | |
| Size delta | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
