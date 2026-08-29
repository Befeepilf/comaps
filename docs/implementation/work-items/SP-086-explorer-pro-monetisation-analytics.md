# SP-086 — Explorer Pro monetisation analytics

**Phase:** 9 — GPX and feature gating
**Status:** Accepted
**Branch:** `cursor/sp-086-pro-monetisation-analytics-db9d`
**Depends on:** SP-080 G8, G9 (draft SPD-074, SPD-075); SP-083/084
  surfaces that fire the events
**Unblocks:** SP-087 exit 8

---

## Objective

Record count-only monetisation events — information page viewed, GPX
import usage, GPX export usage — only when Explorer Pro is enabled in
the build. Public builds with Pro off must not emit these events.

## Motivation

Spec §32.5. SP-003 left product analytics unset. Phase 6/7 already
shipped local uint64 counters with no location payload and upload
residual to Phase 10 (SPD-044, SPD-055). Purchase conversion metrics are
post-V1 (SPD-010).

## In-scope behavior

- Shared counters (names, not files or coordinates):
  - info page viewed
  - GPX import usage (successful historical import, not every picker
    open — pick one and test it)
  - GPX export usage (successful export)
- Increment only when the matching **capability is available** in the
  build (G9), even if a test fakes entitlement. When all capabilities
  are off, increments are no-ops and stored values stay zero.
- Persistence: uint64 in settings, same pattern as
  `CompletionCardAnalytics`.
- Tests: increment; assert no lat/lon, path, track id, area id, pixel
  id, or filename in the stored representation.
- Upload residual → Phase 10. Do not send to Sentry.

## Out-of-scope behavior

- Purchase conversion, restoration, funnel (SPD-010).
- A new analytics vendor or endpoint.
- Wiring upload.

## Relevant product requirements

- Spec §32.5, §32, §25.1, §34 analytics.
- SPD-010, SPD-044, SPD-055, draft SPD-075.

## Relevant source files or symbols

- `street_pixels::CompletionCardAnalytics`
- `routing::StreetExplorationRoutingAnalytics`
- SP-083/084 info page and import/export call sites
- `libs/platform/settings.hpp`

## Implementation notes / constraints

- Counts only. Fail closed: missing capability → no increment.
- Do not log the GPX path beside the increment.
- Public binaries may contain the counter symbols; they must not fire.

## Acceptance criteria

1. Three counters exist and increment on the specified actions when Pro
   is available in the build.
2. Capabilities off → no increment.
3. Tests prove no location-shaped fields.
4. Upload is explicitly residualled to Phase 10.
5. Sentry is not the sink.

## Required automated tests

- Increment when available; zero when unavailable.
- Payload/schema assertions (no location keys).

## Required manual validation

- Optional debug read-out after one import in an internal Pro build.
  Device residual → SP-087 / Phase 10.

## Failure and rollback considerations

- Prefer no events over events that include file names or coordinates.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-086-pro-monetisation-analytics-db9d` |
| Test output | `ExplorerProAnalytics` **13/13**; `IsolationHistoricalImport` **16/16**; `ExplorerPro_` **12/12**; JVM `ExplorerProAnalyticsTest` **2/2** (after fail-closed getters). |
| Upload residual | Phase 10 |
| Accepted by | Product owner |
| Accepted date | 2026-08-28 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `ReloadBookmarkRoutine` does not pass `historicalTracks`, so a `.gpx` reload neither paints nor counts | **Accepted residual** 2026-08-28 |
| Multi-category “GPX” export is actually KMZ (`GetFileForSharing` size>1) | **Accepted residual** 2026-08-28: do not count as GPX usage |
| Phase-09 “current code locations” still describes ungated GPX / no analytics | Stale vs SP-083–085; SP-087/docs refresh, not this coding item. |
| G1–G10 still Open | **Closed** 2026-08-28: SPD-067–076 |
| Upload / debug device readout | Phase 10 / SP-087. |
