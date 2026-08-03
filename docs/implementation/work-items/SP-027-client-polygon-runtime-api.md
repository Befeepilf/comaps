# SP-027 — Client runtime polygon API

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 (store), SP-026 (emitted polygons available for fixture)

---

## Objective

Provide an offline client API to load exploration-area polygons (and, if
SP-024 chooses precompute, to load precomputed assignment artifacts) needed by
assignment (SP-028).

## Motivation

`CitiesBoundariesTable` is World-search oriented and only supports three-box
`HasPoint`. Assignment needs true geometry, area ids, names, and containment
relative to settlements — or a verified precomputed table — without network.

## In-scope behavior

- Load polygons (and/or precomputed assignment data) from the SP-024 store for
  an installed country.
- If on-device locus: point-in-polygon; iterate candidate areas; expose stable
  id + display name + policy/map-data version stamps as required.
- If precomputed locus: load/verify assignment table API; polygon PIP may be
  thinner or test-only — follow SP-024.
- Offline-only (no network boundary lookup).
- Android V1 / native C++ core; iOS not required for V1.
- Unit tests with synthetic polygons / tables.

## Out-of-scope behavior

- Full assignment policy (priority / smallest / tie-break) — SP-028.
- Settlement fallback — SP-029.
- Area progress UI (Phase 5).
- Competition upload payloads (Phase 8).

## Relevant product requirements

- §8.8; SPD-006; offline-first invariant.

## Relevant source files or symbols

- Likely near `libs/indexer/` / `libs/map/` street-pixels; exact symbols after
  SP-024 store choice.
- Do not treat MWM country id as a neighbourhood name.

## Acceptance criteria

1. API loads fixture store data without network.
2. On-device path: point-in-polygon matches expected insides/outsides.
3. Missing store → empty / no-area-safe behaviour (no crash, no invented areas).
4. Does not present MWM country id as a neighbourhood.

## Required automated tests

- Synthetic polygon hit-tests (on-device path) or table round-trip (precompute).
- Missing store → empty / no-area safe behaviour.

## Failure and rollback considerations

- Corrupt/missing store must not wipe exploration pixels; assignment may be
  absent until rematch (SP-030).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| API symbols | |
| Decision ids (SP-024) | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
