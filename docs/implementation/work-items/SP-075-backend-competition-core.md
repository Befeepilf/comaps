# SP-075 — Backend competition core

**Phase:** 8 — Competition
**Status:** Not started
**Branch:**
**Depends on:** SP-070 (SPD-057–059, SPD-062, SPD-065); SP-074 payload
  contract
**Unblocks:** SP-076, SP-077
**Repository:** `comaps_backend` (not this checkout). Re-verify models
  and URLconf there before coding.

---

## Objective

Add a Django `competition` app: profile registration with **unique**
nicknames, aggregate ingest, server-side decay between uploads, clamping,
and a non-SQLite deployable settings module.

## Motivation

Backend today has `Explorer` + `Friendship` only. `/stats/upload` does
not exist. Phase 8 entry also asks for a production settings module and a
database that is not SQLite.

## In-scope behavior

- New app `competition/` (SPD-065). Do not bolt scores onto `Explorer` /
  `Friendship`.
- Register: device/profile id, unique nickname (§21.1 format + SPD-059),
  consent policy version + timestamp. No email/password.
- Unique nickname: 409 on collision; generated names retry client-side
  (SP-071).
- Ingest: closed schema matching spec §25.2. **Reject** any
  location-shaped extra field at schema level.
- Store last accepted aggregate score, observed time, decay version,
  map-data version, score-calc version (start at 1).
- Between uploads, apply the same 30-day half-life to the stored
  aggregate (spec §22.8). A newer valid upload replaces the decayed
  estimate.
- Clamp: non-finite rejected; score and live coverage clamped to
  `[0, 100]`; `eligible` forced false if coverage `< 2%` or score
  `< 0.5`. Do not require unique pixel counts on the wire (SPD-065).
- Identifiers: OSM ids, not compact indices.
- Throttles on register, nickname change, and upload.
- `prod` settings + Postgres (or equivalent non-SQLite). Tests may use
  the project’s test runner; do not require SQLite as the production
  default.
- pytest (or existing backend test tool) coverage; this repo currently
  has no backend tests — add a harness if missing.

## Out-of-scope behavior

- Read APIs and sparse-area nickname hiding (SP-076).
- Report / admin reset / profile deletion (SP-077).
- Friends endpoints (leave unused; SPD-061).
- Anti-cheat theatre (spec §6, §35).
- Strict map-version normalisation (spec §27.5).

## Relevant product requirements

- Spec §20.3–§20.5, §21.1, §22.8, §25.2, §26.3, §27.5.
- SPD-014, SPD-057, SPD-059, SPD-062, SPD-065.

## Relevant source files or symbols

- `comaps_backend`: `core/models.py` `Explorer` / `Friendship`,
  `apis/auth.py` `DeviceIdAuth`, `apis/api.py`, `apis/throttling.py`,
  `comaps/settings/`
- Client contract: SP-074 payload

## Implementation notes / constraints

- Re-verify the 2026-07-25 backend snapshot in the backend tree.
- Device-id auth is an accepted V1 weakness; rate-limit and clamp are
  the controls.
- Do not accept raw GPS “for accuracy”.

## Acceptance criteria

1. `competition` app exists; friends models unchanged.
2. Unique nickname enforced; 409 on conflict.
3. Schema rejects location-shaped fields.
4. Decay between uploads matches the 30-day half-life within test
   tolerance; newer upload replaces the decayed value.
5. Clamp / ineligible rules hold.
6. Production settings are not SQLite-default.

## Required automated tests

- Register + unique nickname collision.
- Ingest allow-list; extra lat/lon field rejected.
- Decay at 30/60/90 days on stored aggregate.
- Clamp score 101 → 100; eligible with 1% coverage → ineligible.
- Throttle smoke.

## Required manual validation

- Staging against SP-074 client when both exist (SP-079).

## Failure and rollback considerations

- Prefer rejecting a payload over storing GPS.
- Do not create a public `/stats/upload` compatibility alias that
  accepts the old schema.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch (backend) | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
