# SP-028 — Deterministic pixel-to-area assignment

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 Accepted (SPD-021 locus), SP-025 (priority), SP-027
  (load API)

---

## Objective

Assign every valid street pixel to at most one **configured subdivision**
exploration area (or none), deterministically for a fixed (map-data version,
policy version) pair, using country-config priority, smallest-polygon rule, and
stable-identifier tie-break (§8.8).

V1 locus (SPD-021): **consume and verify** the generator-precomputed
subdivision assignment map — not a primary full-universe on-device PIP rematch.

Settlement fallback and rural no-area productization are **SP-029** (layered on
top of this item’s “none” outcomes).

## Motivation

Phase 5/7/8 need area identifiers. Spec forbids grids and invented place-node
polygons. Assignment must be reproducible and offline.

## In-scope behavior

- Consume/verify generator-precomputed **subdivision** assignments from SP-027
  (SPD-021); ensure §8.8 priority / smallest-polygon / stable-id semantics are
  what the generator encoded (fixture cross-check).
- Exactly one subdivision area per pixel when inside a valid candidate; **none**
  otherwise (SP-029 may then assign settlement or leave no-area).
- Integrate with rematch timing: apply new blob on map-data / policy_version
  change (SP-030), not full-universe client PIP.
- Unit tests for nested polygons, ties, and outside-all (against fixtures /
  generator goldens).

## Out-of-scope behavior

- Settlement-as-area fallback (SP-029).
- Persistence format (SP-030) beyond producing assignment results for a run.
- Area UI (Phase 5).
- Inventing grids when no candidate exists.
- Primary full-universe on-device PIP rematch (SPD-021).

## Relevant product requirements

- §8.8, §8.3; SPD-006, SPD-021.

## Acceptance criteria

1. Determinism: same fixture twice → identical assignments.
2. Nested polygons → smallest (verified vs generator output).
3. Equal-area tie → stable id, not iteration order.
4. No pixel assigned to two subdivision areas.
5. Outside all subdivision candidates → none (not a fabricated area).
6. Client consumption matches generator precompute for fixtures.

## Required automated tests

- Nested / tie-break / outside fixtures.
- Repeat-assignment identity.
- Fixture verifies client consumption matches generator output.

## Failure and rollback considerations

- Fail closed on unknown policy/map version pairing rather than inventing areas.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Decision ids (SP-024) | SPD-021 (primary); SPD-022 (feeds SP-030) |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
