# SP-074 — Competition upload queue

**Phase:** 8 — Competition
**Status:** Not started
**Branch:**
**Depends on:** SP-070 (SPD-014, SPD-062, SPD-064, SPD-065); SP-071
  consent; SP-072 scores; SP-073 weekly counts
**Unblocks:** SP-075 (ingest), SP-079 (cadence checks)

---

## Objective

Queue and send **only** spec §25.2 aggregates, at most once per 15 minutes
plus up to 15 minutes jitter, with offline drain. Replace the 1-minute
`ExploreStatsService` poll. Do not reuse `/stats/upload`.

## Motivation

Spec §25.3 and SPD-014. Current client posts a different schema every
minute to an endpoint the backend does not implement. Delayed batching
exists so competition cannot act as a live-location signal.

## In-scope behavior

- Upload **only** when competition consent is on (SPD-064 record).
- Payload allow-list: pseudonymous profile id, nickname, area OSM id,
  aggregate ownership score, live coverage %, eligibility, weekly
  new-live-pixel count by city OSM id, map-data version,
  score-calc version (1), last update time (SPD-065).
- Deny-list tests: no lat/lon, GPS, tracks, per-pixel timestamps, live
  movement, device advertising ids, friends ids.
- Cadence: ≤ 1 upload / 15 minutes, plus jitter in `[0, 15]` minutes.
  No “sync now” control.
- Offline: queue; flush after connectivity. No interpolation across
  pauses (existing collection rules unchanged).
- URL: `{apiBase}/v1/competition/…` (SPD-062). Empty apiBase → no HTTP
  (SP-004).
- Discard `explore_stats.json` as a source of truth (SPD-064). Stop the
  1-minute stats poll for competition (remove or leave dead behind the
  consent gate — pick one and test that it does not fire).
- No upload when opted out.

## Out-of-scope behavior

- Backend ingest (SP-075).
- Read APIs (SP-076).
- Account deletion HTTP (SP-077) may share the client, but deletion is
  that item.

## Relevant product requirements

- Spec §25.1–§25.6, §26.2.
- SPD-014, SPD-062, SPD-064, SPD-065.

## Relevant source files or symbols

- `libs/map/explore_stats_service.{hpp,cpp}`
- `libs/map/backend_config.{hpp,cpp}` `GetStatsUploadUrl`
- `IdentityStore`, `StreetPixelsManager`

## Implementation notes / constraints

- Schema rejection is a backend duty; the client still must not send
  extra fields.
- Tests freeze time for cadence and jitter bounds.
- Fail closed if API unconfigured.

## Acceptance criteria

1. Payload contains only the allow-list; deny-list fields never appear.
2. No upload more than once per 15 minutes; jitter in range; no upload
   when opted out or apiBase empty.
3. Offline queue flushes later; no sync-now affordance.
4. `/stats/upload` is not used for competition.
5. Imported-only progress does not produce competitive payload values.

## Required automated tests

- Allow-list / deny-list JSON fixtures.
- Cadence + jitter with a fake clock.
- Opt-out and empty apiBase → zero HTTP.
- Offline enqueue then flush.
- Consent boolean-only (pre-SP-071) must not be testable as sufficient
  once SP-071 landed; if this item lands first, gate on the new record
  type.

## Required manual validation

- Capture traffic on device (SP-079): no coordinates; cadence holds.

## Failure and rollback considerations

- Prefer no upload over sending location-shaped fields.
- Do not add a debug “upload now” in public builds.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Endpoint path | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
