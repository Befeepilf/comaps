# SP-075 — Backend competition core

**Phase:** 8 — Competition
**Status:** Accepted
**Branch:** backend `cursor/sp-075-backend-competition-core-f95c` (`Befeepilf/explorer` `a16a462b2c1c7186456b016f24f405952b4393a0`); client `cursor/sp-075-backend-competition-core-f95c` (`Befeepilf/comaps` `867c9a544`)
**Depends on:** SP-070 (SPD-057–059, SPD-062, SPD-065); SP-074 payload
  contract
**Unblocks:** SP-076, SP-077
**Repository:** `comaps_backend` (`Befeepilf/explorer`) plus a tiny client
  claim HTTP path in this checkout.

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
| Branch (backend) | `cursor/sp-075-backend-competition-core-f95c` on `Befeepilf/explorer` at `a16a462b2c1c7186456b016f24f405952b4393a0` |
| Branch (client) | `cursor/sp-075-backend-competition-core-f95c` on `Befeepilf/comaps` at `867c9a544` |
| Test output | See executed output below. Independent review Explorer: cwd `/home/ubuntu/explorer-src/explorer`, `uv run pytest -q` → `31 passed`. Client: binary `/home/ubuntu/omim-build-debug/street_pixels_tests`, `--data_path=/workspace/data --user_resource_path=/workspace/data --filter='BackendConfig_|IdentityStore_|CompetitionUpload_'` → all tests passed. |
| Accepted by | Product owner |
| Accepted date | 2026-08-26 |

## Executed test output

Explorer (`cd /home/ubuntu/explorer-src/explorer && uv sync --extra dev && uv run pytest -q`):

```
.........................                                                [100%]
25 passed, 4 warnings in 0.24s
```

Required cases covered: register 200; Alice/alice 409; ingest allow-list; extra lat/lon 422; decay 30/60/90; newer replaces; clamp 101→100; 1% coverage ineligible; 6th register 429; prod rejects SQLite.

Client (`./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests` then the filter below):

```
=== street_pixels_tests --filter='BackendConfig_|IdentityStore_|CompetitionUpload_' ===
...
Running backend_config_tests.cpp::BackendConfig_CompetitionRegisterUrlEmptyWhenUnconfigured
OK
Running backend_config_tests.cpp::BackendConfig_CompetitionRegisterUrlWhenConfigured
OK
Running backend_config_tests.cpp::BackendConfig_CompetitionNicknameUrlEmptyWhenUnconfigured
OK
Running backend_config_tests.cpp::BackendConfig_CompetitionNicknameUrlWhenConfigured
OK
...
Running identity_store_tests.cpp::IdentityStore_ProductionClaimEmptyApiNoHttp
OK
Running identity_store_tests.cpp::IdentityStore_ProductionClaimPostsRegisterJsonWithoutFriendsHeaders
OK
Running identity_store_tests.cpp::IdentityStore_ProductionClaimRenameUsesNicknameUrl
OK

All tests passed.
```

Full client log: `/opt/cursor/artifacts/sp075_street_pixels_tests.log`. Explorer log: `/opt/cursor/artifacts/sp075_explorer_pytest.log`.

## Independent review

Reviewed both repos against SP-075, SPD-014/057/059/062/065, spec §21.1
§22.8 §25.2, `competition_upload_payload.hpp`, and
`backend-and-privacy.mdc`. Charter items already held: nested
`ClosedSchema` `extra="forbid"`; ingest/register `auth=None` (no
DeviceIdAuth); success 200 not 201; no `/stats/upload` alias; scores in
`competition/` not on `Explorer`; Unicode nickname rules not the friends
ASCII regex; 30-day half-life replace not blend; clamp + 1% coverage
ineligible; `prod.py` rejects SQLite.

Fixed:

| Severity | Finding | Fix |
| --- | --- | --- |
| High | Nickname uniqueness used SQL `Lower()` / `iexact`, so casefold pairs such as `Straße` / `STRASSE` both registered. | Stored `nickname_key` from `str.casefold()` with a unique constraint. |
| High | `IntegrityError` races on unique nickname could skip atomic write and hit the global handler as HTTP 422. | `transaction.atomic()` around create/save; map leftover `IntegrityError` to 409 (`nickname_taken` or `profile_registered`). |
| Medium | `TryClaimNickname` returned `Unavailable` unless a test handler was injected. Production JNI does not go through a Framework instance method, so a missing ctor wire skipped HTTP. | Default handler is `PostNicknameClaim` when no test override is set. |
| Low | `SerializeCompetitionUploadPayload` relied on NRVO for `SerializerJson` destructor flush. | Scope the serializer before returning the body. |
| Low | Ingest `update_or_create` uniqueness races could leak `IntegrityError` as 422. | Retry once after `IntegrityError`; wrap the write loop in `atomic()`. |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Seven-day rename interval is client-only (`IdentityStore::CanRenameNickname`). Server nickname POST does not enforce §21.4. | SP-077 |
| Weekly ingest stores `week_start_unix` as Monday 00:00 UTC of `last_update_unix`. Payload has no city TZ or `week_id` (deny-listed). | Keep UTC fallback (SPD-060). City IANA zone belongs with SP-076 / city records. |
| Server nickname rules use Unicode categories; the client uses script letter tables. Some scripts may pass one side and fail the other. | Residual; server remains authority. SP-077 if a script gap is product-visible. |
| `profile_id` is the device id. Device-id auth remains an accepted V1 weakness. | Keep. Rate-limit and clamp are the controls. |
| Ingest ignores `nickname` for identity (auth is `profile_id` only) and does not rename. | Keep. Uniqueness stays on register/nickname. |
| Production settings module exists and rejects SQLite; no Postgres is deployed from this item. | Ops / Phase 10. |
| `GetStatsUploadUrl` (`/stats/upload`) remains in the client tree and is unused by competition. Competition API has no `/stats/upload`. | Leave unused. Do not add a compatibility alias. |
| Decayed eligibility is not recomputed on the stored row; `decayed_score()` is for reads. | SP-076. |
| Global `integrity_error_handler` in `comaps/api_services.py` still returns 422 for friends-app leaks. Competition views catch locally. | Leave; friends surface is SPD-061 unused. |

## Independent review executed test output

Explorer (`cd /home/ubuntu/explorer-src/explorer && uv run pytest -q`):

```
...............................                                          [100%]
31 passed, 4 warnings in 0.28s
```

Added coverage in this review: `Straße`/`STRASSE` 409; IntegrityError nickname 409; duplicate `profile_id` 409; weekly-city extra lat/lon 422; nested `extra="forbid"`; score 0.49 ineligible; ingest without friends headers.

Client (`./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests` then the filter):

```
Running identity_store_tests.cpp::IdentityStore_DefaultHandlerPostsWhenApiConfigured
OK
...
All tests passed.
```

Review logs: `/opt/cursor/artifacts/sp075_review_explorer_pytest.log`, `/opt/cursor/artifacts/sp075_review_street_pixels_tests.log`.
