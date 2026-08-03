# SP-029 — Settlement fallback and no-area state

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 (settlement geometry), SP-028 (subdivision-or-none)

---

## Objective

Implement SPD-007: where a settlement has no suitable subdivision, the whole
settlement is one exploration area; outside recognised settlements there is
no area, and exploration/routing continue without area completion.

## Motivation

Sparse admin data is common. Spec forbids inventing grid areas (§8.6). Fallback
and no-area must be explicit product states, not accidental nulls.

## In-scope behavior

- Detect settlement membership (per SP-024 geometry decision: three-box and/or
  true municipal polygons).
- Input: SP-028 result (subdivision id or none).
- If none and inside a settlement with no valid subdivision → assign settlement
  area.
- If none and outside settlements → no area id; pixels still collectable.
- If SP-028 already assigned a subdivision → keep it (do not replace with
  whole-settlement).
- Tests for settlement-only city, rural no-area, and subdivision-present city.

## Out-of-scope behavior

- Country/world aggregate percentages (post-V1).
- Competition eligibility for no-area pixels (Phase 8).
- Area progress UI (Phase 5).
- Grids or place-node polygons.

## Relevant product requirements

- §8.2, §8.5, §8.6; SPD-007.

## Acceptance criteria

1. Settlement without subdivisions → one area covering the settlement.
2. Rural / outside settlement → no area; exploration still works.
3. Subdivision present → SP-028 assignment wins over whole-settlement.
4. No invented grid / place-node areas.

## Required automated tests

- Settlement-only, rural, subdivided fixtures.

## Failure and rollback considerations

- Ambiguous settlement containment must not invent areas; prefer no-area over
  false settlement claim if undecidable (document choice in evidence).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Settlement geometry (SP-024) | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
