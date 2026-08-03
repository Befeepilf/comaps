# SP-027 — Client runtime polygon API

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Provide an offline client API to load exploration-area polygons and answer
point-in-polygon / metadata queries needed by assignment (SP-028).

## Motivation

`CitiesBoundariesTable` is World-search oriented and only supports three-box
`HasPoint`. Assignment needs true geometry, area ids, names, and containment
relative to settlements.

## In-scope behavior

- Load polygons from the SP-024 store for an installed country.
- Point-in-polygon; iterate candidate areas; expose stable id + display name +
  policy/map-data version stamps as required.
- Offline-only (no network boundary lookup).
- Unit tests with synthetic polygons.

## Out-of-scope behavior

- Full assignment algorithm (SP-028).
- Area progress UI (Phase 5).
- Competition upload payloads (Phase 8).

## Relevant product requirements

- §8.8; SPD-006; offline-first invariant.

## Acceptance criteria

1. API loads fixture polygons without network.
2. Point-in-polygon matches expected insides/outsides.
3. Does not present MWM country id as a neighbourhood.

## Required automated tests

- Synthetic polygon hit-tests.
- Missing store → empty / no-area safe behaviour.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| API symbols | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
