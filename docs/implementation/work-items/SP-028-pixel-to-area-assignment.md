# SP-028 — Deterministic pixel-to-area assignment

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 (locus), SP-025 (priority), SP-027 (load API)

---

## Objective

Assign every valid street pixel to at most one **configured subdivision**
exploration area (or none), deterministically for a fixed (map-data version,
policy version) pair, using country-config priority, smallest-polygon rule, and
stable-identifier tie-break (§8.8).

Settlement fallback and rural no-area productization are **SP-029** (layered on
top of this item’s “none” outcomes).

## Motivation

Phase 5/7/8 need area identifiers. Spec forbids grids and invented place-node
polygons. Assignment must be reproducible and offline.

## In-scope behavior

- Apply SP-025 priority to candidate **subdivision** polygons from SP-027 (or
  consume/verify generator-precomputed subdivision assignments per SP-024).
- Exactly one subdivision area per pixel when inside a valid candidate; **none**
  otherwise (SP-029 may then assign settlement or leave no-area).
- Smallest-polygon wins; equal area → stable id tie-break (not iteration order).
- Integrate with derivation / rematch timing per SP-024.
- Unit tests for nested polygons, ties, and outside-all.

## Out-of-scope behavior

- Settlement-as-area fallback (SP-029).
- Persistence format (SP-030) beyond producing assignment results for a run.
- Area UI (Phase 5).
- Inventing grids when no candidate exists.

## Relevant product requirements

- §8.8, §8.3; SPD-006.

## Acceptance criteria

1. Determinism: same fixture twice → identical assignments.
2. Nested polygons → smallest.
3. Equal-area tie → stable id, not iteration order.
4. No pixel assigned to two subdivision areas.
5. Outside all subdivision candidates → none (not a fabricated area).

## Required automated tests

- Nested / tie-break / outside fixtures.
- Repeat-assignment identity.
- If precomputed: fixture verifies client consumption matches generator output.

## Failure and rollback considerations

- Fail closed on unknown policy/map version pairing rather than inventing areas.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Decision ids (SP-024) | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
