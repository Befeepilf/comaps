# SP-080 — Phase 9 GPX and feature-gating architecture notes

**Date:** 2026-08-28
**Branch:** `cursor/phase-09-work-items-db9d`
**Locks:** product-owner 2026-08-28 → **SPD-067–076** (recommended G1–G10).
This note remains a dated investigation snapshot, not a decision.

This note is the investigation companion to
[`SP-080-gpx-feature-gating-architecture-decisions.md`](../work-items/SP-080-gpx-feature-gating-architecture-decisions.md).
It is not a decision.

## Entry criteria

Phase 9 may start: Phase 3 exit met (2026-08-03); SP-005 Accepted
(2026-07-27). Competition (Phase 8) is not a hard prerequisite; isolation
tests in SP-082 reuse Phase 8 stores when they are present.

## Code snapshot (this working tree, 2026-08-28)

| Concern | Location | Observed state |
| --- | --- | --- |
| GPX serdes | `libs/kml/serdes_gpx.*` | Import and export present. Timestamp repair is display/metadata only. No file-size cap. Parse failure may `ReadAsString` the whole file. |
| GPX tests | `libs/kml/kml_tests/gpx_tests.cpp` | Round-trip, colours, timestamps, fixtures. No OOM / oversized cases. |
| Android import | `Factory.KmzKmlProcessor`, `BookmarkManager.importBookmarksFiles`, Favorites Import button, manifest VIEW/SEND for `application/gpx` | Ungated. Multi-URI batch exists. GPX is re-saved as KML category with tracks. |
| Android export | `PlacePageView`, `BookmarksListFragment`, `BookmarkCategoriesFragment` `export_file_gpx` | Ungated GPX export on category and track share sheets. |
| Track-to-pixel | `StreetPixelsManager::UpdateExploredPixels` → `ComputeTrackPixels` → `MarkExploredPixelIds` | Every bookmark track is replayed. Sampling **15 m** (`kPathSamplingStepMeters`, SPD-019). `SetExplored(true)` only; never `SetEverLive`; never recency / weekly / upload. |
| Geometry flatten | `Track::GetGeometry` | Concatenates all segments into one line. Replay samples across the join (possible corridor across a GPX `trkseg` gap or a recording pause). |
| `processed_tracks` | `street_stats.db` | PK `(geometry_hash, country_id)`. Hash is mercator x,y only. Identical geometry skips; timestamp-only re-export skips; any point change reprocesses. Track delete does not drop the ledger row. Rematch clears per country. |
| Ever-live | `.pix` bit 62 (SPD-015 / SP-016) | Import-first → clear; live sets; import cannot clear (`SetEverLive(false)` is a no-op). |
| Recency | `LiveRecencyStore` / `live_recency.db` (SP-072) | Written only from live `OnLocationUpdate` and opt-in seed. |
| Weekly | `WeeklyCityLiveStore` (SP-073) | `RecordFirstLive` only on ever-live flip. |
| Upload | `CompetitionUploadService::MarkPending` on recording **Finished** only (SP-074) | Import does not enqueue. Snapshot drops areas with zero ever-live. |
| Personal % | `AreaCompletionCache` (SPD-026) | Import increments explored count without full invalidate. Can fire 25/50/100 with no haptic (not recording). |
| First-goal / hint | `FirstGoalTracker`, `CompetitionHintTracker` | Live `numNewlyExploredPixels` only. Import does not advance. |
| Routing | SPD-040 | `IsExplored()` includes imported. |
| Explorer Pro | `libs/map/explorer_pro.*` | `GpxImport` / `GpxExport` / `AdvancedTrackManagement`. `IsCapabilityEnabled` = available ∧ entitled. Stub entitlement always false. Android pushes three `EXPLORER_PRO_*` BuildConfig booleans (default false; `-PenableExplorerProCapabilities=true` sets all true). **No production call site.** Java has setter JNI only, no getter. |
| Settings GPX | `prefs_interface.xml` and other prefs | **None.** GPX lives in Favorites / share intents. |
| Purchase UI | Android | **None** (SPD-010). |
| Monetisation analytics | — | **Not found.** Count-only pattern exists (`CompletionCardAnalytics`, `StreetExplorationRoutingAnalytics`). |
| Entitlement storage | Documented `ExplorerPro.Entitled` in SecureStorage | Stub never reads or writes. No debug grant path. **Internal Pro-capability builds still cannot open the gate** because entitlement stays false. |

## Differences from phase-09 (verified 2026-07-25) and the 2026-07-20 audit

- Sampling is 15 m, not 10 m (SP-019).
- Imported marking exists as ever-live-clear; the audit/phase “Not found” row is stale.
- Competition isolation already holds on `MarkExploredPixelIds`. Phase 9 still lacks a GPX-file fixture and a dedicated importer.
- Pro foundation exists (SP-005); UX is still ungated.
- Import already creates a stored bookmark track (KML category).
- Spike 9 (large import vs competition queues) is partly answered for isolation; memory/chunking is not measured.

## Product invariants this phase must not break

- Raw GPS and recorded tracks are never uploaded.
- Pixel collection from live GPS happens only during an active, non-paused session.
- Imported exploration never affects recency, ownership, eligibility, or weekly ranking, regardless of flag or entitlement (data rule, not the Pro gate — SPD-011).
- No interpolated exploration across a pause, interruption, or rejected sample. Retiring bookmark-wide replay of live-saved tracks is part of that (G1 / G5).

## Recommended positions (not Accepted)

See SP-080 G1–G10. Coding SP-081+ must not guess these.
