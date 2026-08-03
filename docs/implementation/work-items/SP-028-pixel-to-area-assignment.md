# SP-028 — Deterministic pixel-to-area assignment

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Assign every valid street pixel to at most one exploration area,
deterministically for a fixed (map-data version, policy version) pair, using
country-config priority, smallest-polygon rule, and stable-identifier
tie-break (§8.8).

## Motivation

Phase 5/7/8 need area identifiers. Spec forbids grids and invented point
polygons. Assignment must be reproducible and offline.

## In-scope behavior

- Apply SP-025 priority to candidate polygons from SP-027.
- Exactly one area per pixel when inside a valid candidate; none otherwise.
- Smallest-polygon wins; equal area → stable id tie-break.
- Integrate with derivation / rematch timing per SP-024 (on-device or consume
  precomputed).
- Unit tests for nested polygons, ties, and outside-all.

## Out-of-scope behavior

- Settlement fallback productization details beyond calling into SP-029.
- Persistence format (SP-030) beyond producing assignment results.
- Area UI.

## Relevant product requirements

- §8.8, §8.3; SPD-006.

## Acceptance criteria

1. Determinism: same fixture twice → identical assignments.
2. Nested polygons → smallest.
3. Equal-area tie → stable id, not iteration order.
4. No pixel assigned to two areas.

## Required automated tests

- Nested / tie-break / outside fixtures.
- Repeat-assignment identity.

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
