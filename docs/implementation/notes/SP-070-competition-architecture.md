# SP-070 — Phase 8 competition architecture notes

**Date:** 2026-08-23
**Branch:** `cursor/phase-08-work-items-c990`
**Locks:** product-owner 2026-08-23 (accept all SP-070 recommendations
except nicknames: **unique**)

This note is the investigation companion to
[`SP-070-competition-architecture-decisions.md`](../work-items/SP-070-competition-architecture-decisions.md).
It is not a decision.

## Code snapshot (this working tree)

| Concern | Location | Observed state |
| --- | --- | --- |
| Identity | `libs/map/identity_store.{hpp,cpp}` | Device id in `SecureStorage` (`Explore.DeviceId`, 24 random bytes base64url). Nickname in settings (`Explore.Username`) with ASCII `[a-z0-9_]{3,20}` after lowercasing. Consent boolean only (`Explore.ConsentGiven`). No policy version, no timestamp. |
| Stats upload | `libs/map/explore_stats_service.cpp` | Weekly `{regionId, weekStartSec, exploredPixels, version}` plus device id and optional username. Poll every **1 minute**. Gated by `m_syncEnabled`. File `explore_stats.json`. |
| Upload URL | `libs/map/backend_config.cpp` `GetStatsUploadUrl` | `{apiBase}/stats/upload`. Release/beta inject `https://api.comaps.app/api` (SP-004). Debug empty, fail-closed. |
| Friends | `libs/map/friends_manager.cpp`; Android `MyAccountDialogFragment`, `Friends.java` | Full friends client. Account UI is friends-first. Consent dialog is a boolean gate into that UI. |
| Recency / ownership | — | Not found. Ever-live bit exists in `.pix` (`IsEverLive()`, SPD-015). |
| Area / city ids | `street_pixels::ExplorationArea::m_osmId`, `StableOsmId` | OSM ids exist in the sidecar. Compact index is not the wire identity. |
| Card competition line | `CompletionCardSource::m_competitionLine` | Stub field; Phase 7 copy is first-person / empty (SPD-052). |
| First-goal counter | `kFirstGoalLivePixelThreshold = 10` | Newly explored live pixels (SPD-047). Competition hint at 30 is not implemented. |
| Backend (`comaps_backend`) | Not in this checkout | Phase-08 2026-07-25 snapshot: Django, `Explorer` + `Friendship` only, unique `username`, no `/stats/upload`, no competition models, SQLite default, no `prod.py`. Re-verify in that repo under SP-075. |

## Product-owner locks (2026-08-23)

Recorded as SPD-057–066. Nickname uniqueness **overrides** spec §20.4
(divergence recorded; spec not edited). Everything else matches the
recommended positions from the Phase 8 entry investigation.

## Backend repository

Phase 8 spans `comaps` (this repo) and `comaps_backend`. SP-075–077 are
specified here so the roadmap stays in one index; implementation of those
items lands in the backend checkout when it is available. Client items
must not pretend the missing `/stats/upload` endpoint exists.
