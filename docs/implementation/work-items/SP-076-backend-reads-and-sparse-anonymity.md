# SP-076 — Backend reads and sparse-area anonymity

**Phase:** 8 — Competition
**Status:** In progress
**Branch:** backend `cursor/sp-076-backend-reads-sparse-anonymity-f95c` (`Befeepilf/explorer`); client `cursor/sp-076-backend-reads-sparse-anonymity-f95c` (`Befeepilf/comaps`)
**Depends on:** SP-075 ingest + decay; SPD-058, SPD-060
**Unblocks:** SP-078 (area snapshot, weekly board, overtaking hints)
**Repository:** `comaps_backend`

---

## Objective

Serve area ranking snapshots and the weekly city board. Enforce
sparse-area anonymity **on the server**: fewer than three participants
→ no nicknames.

## Motivation

Spec §23.3–§23.4 and §24. Client-side hiding is not protection. Weekly
board excludes revisits and imports because clients only upload new-live
counts (SP-073 / SP-074); the server must not reintroduce those.

## In-scope behavior

- `GET` area snapshot: boss, contested (SPD-058), unclaimed, top three
  plus current user without duplicating them if already in top three,
  relative scores, stale flag when data is old.
- Fewer than **three** opted-in participants in the area: omit other
  nicknames; relative scores may remain; never imply someone is nearby
  (spec §23.4).
- `GET` weekly city board: rank by new-live-pixel counts for the SPD-060
  week; city OSM id; time remaining. Same sparse-area nickname rule.
- Contested computed from stored (possibly decayed) eligible scores.
- Map-data / score-calc version coexistence: accept supported older
  versions without strict normalisation (spec §27.5).
- Rate-limit reads.

## Out-of-scope behavior

- Client UI (SP-078).
- Nickname report (SP-077).
- Global/country boards.

## Relevant product requirements

- Spec §23.3–§23.5, §24, §25.4, §27.5.
- SPD-014, SPD-058, SPD-060, SPD-065.

## Relevant source files or symbols

- `competition` app from SP-075
- Client consumers added in SP-078

## Implementation notes / constraints

- Anonymity is a server filter, not a presentation hint.
- Do not return coordinates, last-seen, or presence.

## Acceptance criteria

1. Area snapshot matches ranking rules; contested uses 80% relative gap
   on decayed scores.
2. N < 3 → no other nicknames in JSON.
3. Weekly board uses new-live counts and SPD-060 week boundaries.
4. Responses contain no location/presence fields (deny-list tests).

## Required automated tests

- Top-three + current user de-duplication.
- 0, 1, 2, 3 participants nickname visibility.
- Contested true/false around 0.80 after decay.
- Weekly reset at Monday 00:00 tz / UTC fallback.
- Deny-list on response JSON.

## Required manual validation

- SP-079: N < 3 shows anonymous copy on device.

## Failure and rollback considerations

- Prefer omitting nicknames over leaking them for sparse areas.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch (backend) | `cursor/sp-076-backend-reads-sparse-anonymity-f95c` on `Befeepilf/explorer` at `06e5c067f8c506bc9c66e9473b495cf44d84fc34` |
| Branch (client) | `cursor/sp-076-backend-reads-sparse-anonymity-f95c` on `Befeepilf/comaps` at `ac4cd5ec4cb2a6d999c695f15ac9ccdec9a0884f` |
| Test output | Independent review added leak/ineligible/throttle tests. Explorer `uv run pytest -q` → `81 passed, 4 warnings in 0.87s`. Client C++ not changed; prior `street_pixels_tests --filter='BackendConfig_Competition'` still applies. Full log: `/opt/cursor/artifacts/sp076_review_explorer_pytest.log`. |
| Accepted by | |
| Accepted date | |

## Executed test output

Independent review re-run (`cd /home/ubuntu/explorer-src/explorer && uv run pytest -q`):

```
........................................................................ [ 88%]
.........                                                                [100%]
81 passed, 4 warnings in 0.87s
```

Client C++ was not modified in this review. Prior client filter run remains:

```
=== street_pixels_tests --filter='BackendConfig_Competition' ===
Running backend_config_tests.cpp::BackendConfig_CompetitionAggregatesUrlEmptyWhenUnconfigured
OK
...
Running backend_config_tests.cpp::BackendConfig_CompetitionAreaSnapshotUrlEmptyWhenUnconfigured
OK
Running backend_config_tests.cpp::BackendConfig_CompetitionAreaSnapshotUrlWhenConfigured
OK
Running backend_config_tests.cpp::BackendConfig_CompetitionWeeklyBoardUrlEmptyWhenUnconfigured
OK
Running backend_config_tests.cpp::BackendConfig_CompetitionWeeklyBoardUrlWhenConfigured
OK

All tests passed.
```

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `competition/city_timezones.py` starts as an empty dict; missing/invalid IANA names fall back to UTC (never Europe/Helsinki). Production city→IANA mapping is not loaded from sidecar/city records. | Keep UTC fallback (SPD-060). Persist/register city IANA zones when city records exist. |
| `ninja_extra.DynamicRateThrottle()` overwrote subclass `scope` with `None`, so register/nickname/ingest/read shared one cache key. SP-076 passes class `rate`/`scope` into `__init__`. | Keep. |
| Ingest still stores `week_start_unix` as UTC Monday of `last_update_unix`. Reads ignore that column and recompute membership from `last_update_unix` in the city zone. | Keep. |
| Independent review (2026-08-26): required hunt coverage for raw JSON nickname/`profile_id` leaks, ineligible runner-up contested, weekly N from current-week rows only, friends headers, `/stats/upload` alias, and distinct throttle scopes was missing. Tests added; implementation held (`81 passed, 4 warnings in 0.87s`). | Keep. |
