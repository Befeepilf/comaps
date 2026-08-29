# SP-091 — Product analytics reconciliation

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H5 Accepted (SPD-081). Phase 10 implementation
  entry.
**Unblocks:** SP-097 exit #4 (analytics match §32; no location)

---

## Objective

Reconcile implemented counters with product spec §32, add any missing
**local** count-only events the lock requires, prove no upload path
can emit a location-shaped field, and either keep counters local
(recommended H5) or implement the locked privacy-safe sink.

---

## Motivation

Spec §32 lists activation, core engagement, competition, growth, and
monetisation measurements. Today only routing (SPD-044), completion
cards (SPD-055), and Explorer Pro (SPD-075) exist as local uint64s.
§32.1 and §32.3 are absent. SP-003 explicitly deferred adding the
§32 event set. Upload was residualled to Phase 10 from three phases.

Phase 10 exit #4 and §34 “Analytics contain no raw location data”
need a payload-shape proof, not a convention.

---

## In-scope behavior

- Inventory every increment site vs §32.1–§32.5. Table in completion
  evidence: specified event → implemented key / missing / out of V1.
- If H5 is **local-only** (recommended): implement missing §32
  counters as local uint64 settings in the existing pattern
  (`Explore.*` keys). Increment only for the real user action. No
  lat/lon, pixel id, OSM id, area name, track geometry, file name,
  screenshot, or view hierarchy.
- If H5 instead locks a sink: implement that sink only, with consent
  and deny-list as specified in the SPD. Never Sentry. Never mix
  into competition POST.
- Payload-shape test: every upload path in a **release-shaped**
  configuration is asserted not to contain location-shaped fields
  (GPS, track points, live lat/lon, home, route polyline). Include
  competition allow-list (spec §25.2) as the positive control.
- Confirm monetisation counters still increment only when the
  matching Pro capability is **Available** (SPD-075).
- Confirm growth counters have no area id (SPD-055).
- Debug/readout of counters is not required (SP-061 R5 recommended
  waive) unless H5 says otherwise.

## Out-of-scope behavior

- Purchase conversion metrics (spec §32.5 post-V1).
- Sending analytics through Sentry.
- Device proof that a Play build’s Sentry project is empty of
  screenshots (SP-097 / traffic capture).
- In-app public dashboard of stats.

## Relevant product requirements

- Spec §25.1, §32, §34 Release governance / Privacy.
- SPD-044, SPD-055, SPD-075, SP-003, draft SPD-081 (H5).

## Relevant source files or symbols

- `libs/routing/street_exploration_routing_analytics.*`
- `libs/map/completion_card_analytics.*`
- `libs/map/explorer_pro_analytics.*`
- `CompetitionUploadService` payload
- Sentry manifest meta-data
- Recording / permission / first-goal / milestone / consent sites
  (candidate increment points)

## Implementation notes / constraints

- Follow the existing uint64 settings helper; do not add a second
  analytics framework.
- Missing §32.1 events (permission granted, first recording, first
  pixel, first 10 pixels, first 100 m, first recording completed)
  map naturally onto SP-012 / SP-064 already-observable transitions.
- §32.2 “new pixels per active week” is an aggregate: implement as
  a local counter of newly explored live pixels with a week bucket,
  **not** as an upload of the pixel set.
- §32.3 competition events must not fire for imported pixels.

## Acceptance criteria

1. §32 inventory table is complete.
2. H5 is implemented (local-only **or** locked sink).
3. Payload-shape tests green on release-shaped config.
4. No location fields on any analytics increment.
5. Agent does not mark Accepted.

## Required automated tests

- New counter increment tests (once per event; no increment on
  import-only paths where spec says live).
- Deny-list test for analytics keys/payloads.
- Existing competition upload allow-list tests remain green.
- Explorer Pro counters still absent when capabilities are off.

## Required manual validation

- None beyond inspecting the inventory table. Traffic capture of
  uploads is SP-095 / SP-097.

## Failure and rollback considerations

- If adding §32.1 events requires a schema for “active week”, keep
  it local and crash-safe; do not block launch on a new backend.
- If H5 is local-only, do not treat missing cloud dashboards as a
  failed §32.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| §32 inventory | |
| H5 implementation | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
