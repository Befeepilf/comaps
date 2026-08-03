# SP-027 — Client runtime polygon API

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 Accepted (SPD-020/021), SP-026 (sidecar + precompute
  artifacts available for fixture)

---

## Objective

Provide an offline client API to load exploration-area polygons and the
**precomputed assignment artifacts** from the per-country sidecar (SPD-020,
SPD-021) needed by assignment consumption (SP-028) and settlement fallback
(SP-029).

## Motivation

`CitiesBoundariesTable` is World-search oriented and only supports three-box
`HasPoint`. It is **not** assignment authority (SPD-025). Assignment needs
true geometry metadata, area ids, names, and a verified precomputed table —
without network.

## In-scope behavior

- Load polygons and precomputed dense assignment map from the SPD-020 sidecar
  for an installed country.
- Primary path: load/verify precomputed assignment table API (SPD-021).
  Full-universe on-device PIP is **not** the V1 rematch path; PIP may remain
  for tests or narrow diagnostics only.
- Expose stable id + display name + policy/map-data version stamps.
- True municipal ring load for settlement fallback (SPD-025) — do not use
  three-box `HasPoint` as the assignment containment API.
- Offline-only (no network boundary lookup).
- Android V1 / native C++ core; iOS not required for V1.
- Unit tests with synthetic polygons / tables.

## Out-of-scope behavior

- Full assignment policy (priority / smallest / tie-break) — SP-028 verifies
  against generator output.
- Settlement fallback productization — SP-029.
- Area progress UI (Phase 5).
- Competition upload payloads (Phase 8).
- Primary full-universe client PIP rematch (SPD-021).

## Relevant product requirements

- §8.8; SPD-006, SPD-020, SPD-021, SPD-025; offline-first invariant.

## Relevant source files or symbols

- Likely near `libs/indexer/` / `libs/map/` street-pixels; exact symbols after
  sidecar format lands in SP-026.
- Do not treat MWM country id as a neighbourhood name.
- Do not treat World `CitiesBoundariesTable` as exploration assignment API.

## Acceptance criteria

1. API loads fixture sidecar data without network.
2. Precomputed table round-trip / verify matches fixture generator output.
3. Missing store → empty / no-area-safe behaviour (no crash, no invented areas).
4. Does not present MWM country id as a neighbourhood.
5. Settlement rings used for assignment come from the exploration sidecar, not
   three-box World boundaries.

## Required automated tests

- Table round-trip (precompute path); optional synthetic PIP for diagnostics only.
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
| Decision ids (SP-024) | SPD-020, SPD-021, SPD-025 |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
