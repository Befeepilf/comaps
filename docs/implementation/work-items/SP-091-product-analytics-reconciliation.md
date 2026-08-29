# SP-091 — Product analytics reconciliation

**Phase:** 10 — Android release hardening
**Status:** Accepted
**Depends on:** SP-088 H5 Accepted (**SPD-081**). Phase 10 implementation
  entry.
**Unblocks:** SP-097 exit #4 (analytics match §32; no location)
**Notes:** Implement local §32 counters + payload-shape tests. **No
  upload sink** (SPD-081 closes the Phase 10 upload residual from
  SPD-044 / SPD-055 / SPD-075).

---

## Objective

Reconcile implemented counters with product spec §32, add any missing
**local** count-only events the lock requires, prove no upload path
can emit a location-shaped field, and keep counters local (**SPD-081**).
Do not implement a public analytics sink.

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
- H5 is **local-only** (**SPD-081**): implement missing §32
  counters as local uint64 settings in the existing pattern
  (`Explore.*` keys). Increment only for the real user action. No
  lat/lon, pixel id, OSM id, area name, track geometry, file name,
  screenshot, or view hierarchy. Do **not** build an upload sink.
- Payload-shape test: every upload path in a **release-shaped**
  configuration is asserted not to contain location-shaped fields
  (GPS, track points, live lat/lon, home, route polyline). Include
  competition allow-list (spec §25.2) as the positive control.
- Confirm monetisation counters still increment only when the
  matching Pro capability is **Available** (SPD-075).
- Confirm growth counters have no area id (SPD-055).
- Debug/readout of counters is not required (SP-061 R5 waived /
  Accept in SPD-083) unless a later SPD says otherwise.

## Out-of-scope behavior

- Purchase conversion metrics (spec §32.5 post-V1).
- Sending analytics through Sentry.
- Implementing a public analytics upload sink (SPD-081).
- Device proof that a Play build’s Sentry project is empty of
  screenshots (SP-097 / traffic capture — **residual**).
- In-app public dashboard of stats.

## Relevant product requirements

- Spec §25.1, §32, §34 Release governance / Privacy.
- SPD-044, SPD-055, SPD-075, SP-003, **SPD-081** (H5). SPD-081
  closes the Phase 10 upload residual from SPD-044 / SPD-055 /
  SPD-075.

## Relevant source files or symbols

- `libs/map/product_analytics.*` (new §32.1–§32.3 local counters)
- `libs/map/street_pixels_tests/product_analytics_tests.cpp`
- `android/sdk/src/main/java/app/organicmaps/sdk/ProductAnalytics.java`
- `libs/routing/street_exploration_routing_analytics.*` (SPD-044 reuse)
- `libs/map/completion_card_analytics.*` (SPD-055 reuse)
- `libs/map/explorer_pro_analytics.*` (SPD-075 reuse)
- `CompetitionUploadService` payload / `SerializeCompetitionUploadPayload`
- Recording / permission / first-goal / milestone / consent increment sites

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
2. H5 is implemented as local-only (**SPD-081**). No upload sink.
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
  uploads is SP-095 / SP-097 (**residual**; do not execute on a
  handset in this item).

## Failure and rollback considerations

- If adding §32.1 events requires a schema for “active week”, keep
  it local and crash-safe; do not block launch on a new backend.
- If H5 is local-only (Accepted SPD-081), do not treat missing cloud
  dashboards as a failed §32. Do not build a sink.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-091-product-analytics-6383` |
| §32 inventory | See §32 inventory below. 27 specified events: 20 new local `Explore.*` uint64 counters + 7 reused (SPD-044 / SPD-055 / SPD-075). Purchase conversion out of V1. |
| H5 implementation | **SPD-081 local-only.** `street_pixels::ProductAnalytics` stores count-only `uint64` settings. No public analytics upload sink. Counters are not sent through Sentry and are not attached to the competition POST (`ProductAnalytics_ReleaseUploadPayloadsHaveNoLocation` asserts `Explore.` absent from the JSON). |
| Test output | Independent review re-run (2026-08-29): `--filter=ProductAnalytics` → **20/20 OK**. Focused filter → **173/173 OK**. Android JVM ProductAnalyticsTest 1/1, ExplorerProAnalyticsTest 2/2. Review `4bbb78cbc`. |
| Accepted by | product owner (implement → review lock 2026-08-29) |
| Accepted date | 2026-08-29 |

### §32 inventory

| Spec event | Implemented key | Verdict |
| --- | --- | --- |
| §32.1 Location permission granted | `Explore.PositionPermissionGranted` | New (once). JNI on grant / Splash after native init |
| §32.1 Background recording permission granted | `Explore.NotifyPermissionGranted` | New (once). POST_NOTIFICATIONS grant **or** successful FGS start. ABL absent (SPD-082) |
| §32.1 First recording started | `Explore.FirstRecordingStarted` | New (once) on `RecordingSession::Start` |
| §32.1 First pixel collected | `Explore.FirstCollected` | New (once). Live newly explored only |
| §32.1 First 10 pixels collected | `Explore.FirstTenCollected` | New (once) at 10 live newly explored |
| §32.1 First 100 metres explored | `Explore.FirstGoalComplete` | New (once). SPD-047 = 10 newly explored live pixels / first-goal complete. Same threshold as first 10 |
| §32.1 First recording completed | `Explore.FirstRecordingCompleted` | New (once) on Finish; Discard does not increment |
| §32.2 Active recording sessions | `Explore.RecordingSessions` | New (count every Start) |
| §32.2 New pixels collected per active week | `Explore.NewCollectedThisWeek` | New. Local week bucket of newly explored live pixels (not pixel ids). Week id in `Explore.NewCollectedWeekId` (internal, not serialized) |
| §32.2 Areas with measurable progress | `Explore.PlacesWithProgress` | New. Count of live completion bumps; no OSM ids stored |
| §32.2 First 25% milestone | `Explore.FirstMilestone25` | New (once). Live crossings only |
| §32.2 First 50% milestone | `Explore.FirstMilestone50` | New (once). Live crossings only |
| §32.2 First area completed | `Explore.FirstComplete` | New (once). Live P100 only |
| §32.2 Prefer-unexplored routing usage | `street_exploration_routing_analytics_prefer_used` | Existing SPD-044. Reused |
| §32.2 Avoid-explored routing usage | `street_exploration_routing_analytics_avoid_used` | Existing SPD-044. Reused |
| §32.3 Competition prompt viewed | `Explore.CompetitionPromptViewed` | New (count each consent dialog show) |
| §32.3 Competition opt-in | `Explore.CompetitionOptIn` | New (count each `GrantCompetitionConsent`) |
| §32.3 Users qualifying for leadership | `Explore.LeadershipQualified` | New (once). Consented area-snapshot fetch; live ownership only; import does not fire |
| §32.3 Users becoming boss | `Explore.BecameBoss` | New (once). Consented area-snapshot fetch; live ownership only |
| §32.3 Areas becoming contested | `Explore.BecameContested` | New (once-ever from non-offline server snapshot flag; no OSM ids stored) |
| §32.3 Areas becoming unclaimed | `Explore.BecameUnclaimed` | New (once-ever from non-offline server snapshot flag; no OSM ids stored) |
| §32.3 Weekly city leaderboard usage | `Explore.WeeklyBoardUsed` | New (count each consented weekly-board fetch) |
| §32.4 Completion card generated | `Explore.CardGenerated` | Existing SPD-055. Confirmed no area id |
| §32.4 Share action initiated | `Explore.ShareInitiated` | Existing SPD-055. Confirmed no area id |
| §32.5 Explorer Pro information page viewed | `Explore.ProInfoViewed` | Existing SPD-075. Available gate |
| §32.5 GPX import usage | `Explore.GpxImportUsage` | Existing SPD-075. Available gate |
| §32.5 GPX export usage | `Explore.GpxExportUsage` | Existing SPD-075. Available gate |
| §32.5 Purchase conversion / restoration | — | **Out of V1** (spec §32.5 post-V1) |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Competition prompt viewed counts each first `onCreateDialog` (rotation uses `savedInstanceState != null` and does not increment) | Record. DialogFragment recreation after process death with a restored instance still does not increment; a fresh `maybeShow` does |
| Leadership / boss fire on consented `RequestCompetitionAreaSnapshot`, not on the GPS sample path | Independent review (2026-08-29). GPS-path `QueryCompetitionOwnership` per newly explored pixel was unbounded main-thread work until both flags were set; removed |
| Users who never open a consented area snapshot never increment leadership / boss | Record. Upload builder stays side-effect free; unique-area observation would need OSM ids |
| Contested / unclaimed are once-ever flags, not unique-area counts (storing OSM ids is forbidden) | Record. Unique-area counts would need a product lock that does not store location-adjacent ids |
| First 10 pixels and first 100 m fire at the same SPD-047 threshold (10 newly explored live pixels) | Expected under SPD-047. Spec lists both events; both counters increment once at that threshold |
| POST_NOTIFICATIONS is absent on API 32 and below; FGS start also increments the notify counter once | Honest mapping of “background recording permission” under SPD-082 (no ABL) |
| Device traffic capture that a Play build’s Sentry project is empty of screenshots / analytics | Residual SP-095 / SP-097. Not executed in this item |
| Purchase conversion and restoration metrics | Out of V1 per spec §32.5 |
| In-app debug readout of counters | Waived SPD-083 / SP-061 R5. Not added |
