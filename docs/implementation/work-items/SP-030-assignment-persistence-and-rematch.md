# SP-030 — Persist area assignments and rematch hooks

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 (persistence strategy), SP-028/029 (assignment results),
Phase 3 rematch hooks (map-data version)

---

## Objective

Persist pixel→area assignments keyed by (map-data version, policy version),
and reconcile them on map update / policy change without wiping exploration
(Phase 3 rematch integration).

## Motivation

Assignments are new durable state. Spec §8.8 requires every **valid street
pixel** to have a deterministic assignment (or none). Map updates and config
changes can reassign pixels; percentages may move. Spec §27.4 allows keeping
prior completion dates locally when an area disappears.

## In-scope behavior

- On-disk format following SP-024 persistence strategy (full-universe map,
  sparse, or rematerialize — **not** casually “explored-only” unless SP-024
  accepted that and percentages can still be computed correctly).
- Size-conscious for Uusimaa-scale universes (~6.5×10⁶ cells); document
  measured or budgeted size.
- Rebuild/rematch on map-data version change; policy-version change path.
- Define behaviour when an area id disappears (keep local completion date if
  stored; no invented replacement area / no grid).
- Hook messages if percentages change due to reassignment (may reuse SP-021
  framing patterns; no Phase 5 UI required).
- Tests for rematch of assignments across synthetic version bumps.

## Out-of-scope behavior

- Area progress UI (Phase 5).
- Competition upload of area aggregates (Phase 8).
- Changing Phase 3 rematch of explored bits (integrate with it).

## Relevant product requirements

- §8.8, §27.4; Phase 3 rematch invariants; offline-first.

## Acceptance criteria

1. Assignments survive process restart per SP-024 strategy.
2. Map-data rematch refreshes assignments for surviving pixels; exploration
   bits are not wiped.
3. Missing area after update does not invent a grid replacement.
4. Size strategy documented against SP-023 estimates / SP-024 decision.
5. Policy-version bump triggers reassignment without requiring a map download
   if config shipped independently.

## Required automated tests

- Persist/reload round-trip.
- Version-bump reassignment fixture (map-data and policy).

## Failure and rollback considerations

- Corrupt assignment store: rebuild from polygons/policy rather than wiping
  `.pix` exploration.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Format | |
| Decision ids (SP-024) | |
| Size note | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
