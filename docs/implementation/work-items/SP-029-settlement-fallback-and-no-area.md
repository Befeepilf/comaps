# SP-029 — Settlement fallback and no-area state

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 Accepted (SPD-025 settlement geometry), SP-028
  (subdivision-or-none)

---

## Objective

Implement SPD-007: where a settlement has no suitable subdivision, the whole
settlement is one exploration area; outside recognised settlements there is
no area, and exploration/routing continue without area completion.

Settlement membership uses **true municipal rings from the exploration
sidecar** (SPD-025). World three-box `CityBoundary` is **not** assignment
authority.

## Motivation

Sparse admin data is common. Spec forbids inventing grid areas (§8.6). Fallback
and no-area must be explicit product states, not accidental nulls.

## In-scope behavior

- Detect settlement membership via true municipal rings loaded from the
  sidecar (SPD-025 / SP-027) — for Finland, admin_8 (SPD-023).
- Input: SP-028 result (subdivision id or none), possibly already encoded in
  the precomputed blob with a settlement layer — verify product layering:
  subdivision wins over whole-settlement.
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
- Using World three-box `HasPoint` as settlement assignment containment.

## Relevant product requirements

- §8.2, §8.5, §8.6; SPD-007, SPD-025.

## Acceptance criteria

1. Settlement without subdivisions → one area covering the settlement.
2. Rural / outside settlement → no area; exploration still works.
3. Subdivision present → SP-028 assignment wins over whole-settlement.
4. No invented grid / place-node areas.
5. Settlement containment for assignment uses sidecar true rings, not
   three-box World boundaries.

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
| Settlement geometry (SP-024) | True municipal rings from exploration sidecar (SPD-025); three-box not authority |
| Decision ids (SP-024) | SPD-025 (primary); SPD-020, SPD-023 |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
