# SP-030 — Persist area assignments and rematch hooks

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Persist pixel→area assignments keyed by (map-data version, policy version),
and reconcile them on map update / policy change without wiping exploration
(Phase 3 rematch integration).

## Motivation

Assignments are new durable state. Map updates and config changes can reassign
pixels; percentages may move. Spec §27.4 allows keeping prior completion dates
locally when an area disappears.

## In-scope behavior

- On-disk format for assignments (size-conscious; Uusimaa-scale explored sets).
- Rebuild/rematch on map-data version change; policy-version change path.
- Define behaviour when an area id disappears (keep local completion date if
  stored; no invented replacement area).
- Hook messages if percentages change due to reassignment (may reuse SP-021
  framing patterns; no Phase 5 UI required).
- Tests for rematch of assignments across synthetic version bumps.

## Out-of-scope behavior

- Area progress UI (Phase 5).
- Competition upload of area aggregates (Phase 8).

## Relevant product requirements

- §8.8, §27.4; Phase 3 rematch invariants.

## Acceptance criteria

1. Assignments survive process restart.
2. Map-data rematch refreshes assignments for surviving pixels.
3. Missing area after update does not invent a grid replacement.
4. Size strategy documented (no careless O(universe) duplicate stores).

## Required automated tests

- Persist/reload round-trip.
- Version-bump reassignment fixture.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Format | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
