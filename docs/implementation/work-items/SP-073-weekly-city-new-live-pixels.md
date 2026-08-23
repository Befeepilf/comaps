# SP-073 — Weekly city new-live-pixel counting

**Phase:** 8 — Competition
**Status:** Not started
**Branch:**
**Depends on:** SP-070 (SPD-060); SP-072 recency/live writes; Phase 4
  settlement OSM ids
**Unblocks:** SP-074 (weekly field in upload), SP-076 (city board)

---

## Objective

Count unique **new** live pixels per city per week for the weekly
leaderboard, using Monday 00:00 in the city IANA timezone, else UTC.

## Motivation

Spec §24.1–§24.2. Current `ExploreStatsService` buckets weekly
`regionId` pixel deltas that are not city-scoped, not live-only, and not
unique-new. SPD-064 discards that file; this item is a new counter.

## In-scope behavior

- Per settlement OSM id (city), count unique pixels whose first
  **live** exploration in that city occurs in the current week.
- Revisits of already-live pixels do not count. Imports do not count.
- Week boundary: Monday 00:00 in the city IANA zone when known; else UTC
  (SPD-060). Never device local time.
- If IANA tz is not yet on the city record, both client and server use
  UTC for that city (fail closed). Optional: persist tz from centroid
  lookup onto the city record as follow-up inside this item — not a
  Phase 4 reopen.
- Query: current week id, remaining time, local new-live count for a
  city.

## Out-of-scope behavior

- Server ranking (SP-076).
- Global / country boards (spec §6).
- Upload (SP-074).

## Relevant product requirements

- Spec §24; SPD-060; SPD-007.

## Relevant source files or symbols

- `CityCompletionCache`, settlement-role areas, `StableOsmId`
- `ExploreStatsService` / `explore_stats.json` — do not reuse as the
  V1 weekly store (SPD-064)

## Implementation notes / constraints

- Week id must be reproducible given (city id, timestamp, tz or UTC).
- Tests must not depend on the developer’s local zone.

## Acceptance criteria

1. New live pixels increment; revisits and imports do not.
2. Week rolls at Monday 00:00 in the fixture tz; unknown tz uses UTC.
3. Device zone does not change the bucket.
4. City key is settlement OSM id.

## Required automated tests

- First live visit in city counts; second visit same cell does not.
- Import-only write does not increment.
- Fixed-tz Monday boundary; UTC fallback; device TZ ignored.
- Two cities independently.

## Required manual validation

- Device residual → SP-079 / Phase 10.

## Failure and rollback considerations

- Prefer UTC fallback over guessing Europe/Helsinki.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
