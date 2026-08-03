# SP-029 — Settlement fallback and no-area state

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Implement SPD-007: where a settlement has no suitable subdivision, the whole
settlement is one exploration area; outside recognised settlements there is
no area, and exploration/routing continue without area completion.

## Motivation

Sparse admin data is common. Spec forbids inventing grid areas (§8.6). Fallback
and no-area must be explicit product states, not accidental nulls.

## In-scope behavior

- Detect settlement membership (per SP-024 geometry decision).
- If no valid subdivision candidate → assign settlement area.
- Outside settlements → no area id; pixels still collectable.
- Tests for settlement-only city, rural no-area, and subdivision-present city.

## Out-of-scope behavior

- Country/world aggregate percentages (post-V1).
- Competition eligibility for no-area pixels (Phase 8).

## Relevant product requirements

- §8.2, §8.5, §8.6; SPD-007.

## Acceptance criteria

1. Settlement without subdivisions → one area covering the settlement.
2. Rural / outside settlement → no area; exploration still works.
3. Subdivision present → SP-028 rules win over whole-settlement.

## Required automated tests

- Settlement-only, rural, subdivided fixtures.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
