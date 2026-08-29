# SP-088 — Phase 10 launch-governance architecture notes

**Date:** 2026-08-29
**Branch:** `cursor/phase-10-work-items-6383`
**Locks:** recommended H1–H10 → draft **SPD-077–086**. Not Accepted.
This note is the investigation companion to
[`SP-088-launch-governance-decisions.md`](../work-items/SP-088-launch-governance-decisions.md).
It is not a decision.

## Entry vs planning

Phase 10 **implementation** still requires every other phase at exit, no open
in-progress work item, and a release-configured installable build
(`phases/phase-10-android-release-hardening.md` entry criteria).

Work-item **planning** on 2026-08-29 is a docs-only pass. SP-088 itself is
docs / `DECISIONS.md` and may run while Phases 5–8 await maintainer exit.
Coding SP-089+ waits on H1–H10 locks **and** those entry criteria.

Open work items on the planning date (do not treat as current without
re-check): SP-041 awaiting acceptance; SP-050–053 In review; SP-055 In
review; SP-061 in progress; SP-063 In review; SP-069 In review; SP-071 in
progress; SP-079 in progress.

## Code snapshot (this working tree, 2026-08-29)

Re-verified against `street-pixels` at planning time. The 2026-07-25
phase-10 table is stale in several rows.

| Concern | Location | Observed state |
| --- | --- | --- |
| Android manifest | `android/app/src/main/AndroidManifest.xml` | `ACCESS_COARSE/FINE_LOCATION`, `ACCESS_LOCATION_EXTRA_COMMANDS`, `FOREGROUND_SERVICE` + `LOCATION` + `DATA_SYNC`, `POST_NOTIFICATIONS`, `VIBRATE`. **`ACCESS_BACKGROUND_LOCATION` still absent.** FGS types `location` (`NavigationService`, `TrackRecordingService`), `dataSync` (`DownloaderService`). **`comaps://add-friend` and HTTPS `/add-friend` still registered** (SPD-061 hide not applied to intent-filters). |
| Store credentials | `docs/CREDENTIALS.md` | CI secrets for signed store builds (upstream CoMaps). |
| Release workflows | `.forgejo/workflows/android-release.yaml`, `android-beta.yaml`, `android-check-metadata.yaml`, `android-release-metadata.yaml` | Present; upstream CoMaps application identity. |
| Android lint | `.github/workflows/android-check.yaml` | `./gradlew -Pandroidauto=true lint`. |
| Flavors | `android/app/build.gradle` | `google`, `web`, `fdroid`, `huawei`; `debug`, `release`, `beta`. |
| Android tests | `android/app/src/test/`, `android/sdk/src/test/` | JVM tests now include Street Pixels gates (Explorer Pro, GPX, recording UI model, routing options). **Still no `androidTest` instrumented tests.** |
| Play listing | `android/app/src/google/play/listings/en-US/full-description.txt` | Upstream CoMaps copy. Advertises GPX import/export. Does not describe Street Pixels recording, competition, or session-only location. |
| Privacy / terms URLs | `HelpFragment` → `R.string.app_site_url` + `privacy/` / `terms/` | `app_site_url` is `https://comaps.app/`. No Street Pixels policy text in this repository. |
| Privacy settings | `PrivacySettingsFragment` / `prefs_privacy.xml` | Search history + Google Play services location provider. **No** Street Pixels privacy information, terms, or competition-rules rows (spec §30). |
| Competition settings | `MyAccountDialogFragment`, `ExploreConsentDialogFragment`, `UsernameDialogFragment` | Opt-in, nickname, delete exist as dialogs. Friend-visibility / “username so friends can add you” strings still in `values/strings.xml`. |
| Location rationale | `track_recording_location_rationale` | Session-only; not bundled with competition (spec §10 step 3). Still branded “CoMaps”. |
| Product analytics | `StreetExplorationRoutingAnalytics`, `CompletionCardAnalytics`, `ExplorerProAnalytics` | Count-only local uint64. **No** spec §32.1 activation counters. **No** §32.2 core (except routing + milestones-as-UI). **No** §32.3 competition counters. **No upload sink** (SPD-044 / SPD-055 / SPD-075 residual). |
| Sentry | AndroidManifest `io.sentry.*` | SP-003 private-by-default defaults (PII/screenshots off). Re-verify in SP-091 / SP-097. |
| Completed check | `area_overlay.cpp` `m_showCheck` | Style flag set for completed areas; **glyph not drawn** (SP-040 / SP-041 R3). |
| Share card date | `CompletionCardShare` / card layout | **SPD-056** always-include date; share-time checkbox residual (SP-069). |
| Weekly board JNI | SP-079 evidence | Weekly GET **not JNI-wired**. |
| Off-route Avoid | SP-061 R3 | `CheckLocationForRouting` `OnRemoveRoute` nullptr; Prefer+seekbar not shown. |

**Difference from the technical audit (2026-07-20) and from the 2026-07-25
phase-10 snapshot:** Phases 1–9 have landed session gating, rematch, areas,
routing, milestones, competition, and GPX gates. Android JVM tests are more
than three files. Instrumented tests are still absent. Friends deep links
and CoMaps store/privacy URLs are unchanged. ABL is still absent (SP-012
measured Pixel 3a without it).

## Carried residual inventory (planning date)

Classes used in H7: **Fix** (SP-089), **Measure** (SP-094), **Device-verify**
(SP-095), **Ops** (SP-096), **Follow H5** (SP-091), **Accept/waive**,
**Not Phase 10**.

| From | Residual | Recommended class |
| --- | --- | --- |
| Phase 2 / SP-014 | Aggressive-OEM screen-off / background sample continuity | Device-verify |
| Phase 3 / SP-022 | Pixel 3a / Uusimaa reconciliation UX; rematch timing on large `.pix` | Device-verify + Measure |
| Phase 4 / SP-031 | R3 Helsinki / rural / coastal walks; no MWM-id as neighbourhood name | Device-verify |
| Phase 4 / SP-042 | Option A mapgen collectors → `.spa` | **Not Phase 10** (SPD-033) |
| Phase 4 / SP-048 | Android incomplete-`.spa` toast/dialog | Fix |
| Phase 4 / SP-049–053 | LAN/CDN publish mirror; S2–S8 device download | Device enabler for Helsinki walks; **not** a Phase 10 feature |
| Phase 5 / SP-033 | Quantitative Spike 1 FPS p95 ≥30 / memory uplift &lt;150 MB | Measure |
| Phase 5 / SP-041 R1 | Helsinki badge/focus/tap/city zoom/completed chrome/§31 empty | Device-verify (needs `.spa` on device) |
| Phase 5 / SP-041 R3 | Completed check glyph not drawn | Fix |
| Phase 5 / SP-041 R4 | Overlay neighbourhood-baked push retune | Accept/waive |
| Phase 6 / SP-054 | Spike 7 city-scale / device | Measure |
| Phase 6 / SP-061 R3 | GPS off-route Prefer dialog not shown | Fix |
| Phase 6 / SP-060 | Routing analytics upload | Follow H5 |
| Phase 6 / SP-061 R5 | No in-app debug readout of counters | Accept/waive |
| Phase 7 / SP-069 | Device celebration, card image, share, haptics, nav | Device-verify |
| Phase 7 / SP-069 | Date checkbox vs SPD-056 | Fix |
| Phase 7 / SP-069 R4 | 4 s auto-ack vs share-target PNG lifetime | Fix |
| Phase 7 / SP-069 R5 | `onResume` rebind increments generated / resets checkbox | Fix |
| Phase 7 / SPD-055 | Growth-counter upload | Follow H5 |
| Phase 8 / SP-079 | Device opt-in, traffic capture, opt-out, offline queue, N&lt;3, delete | Device-verify |
| Phase 8 / SP-079 | Weekly GET not JNI-wired | Fix |
| Phase 8 / SP-072 | Revoke does not delete `live_recency.db` rows | Fix (privacy) |
| Phase 8 / SP-075 | Postgres production deploy; exact EU region string | Ops |
| Phase 8 / SP-077 | Failed POST `/leave` no retry; HTTP 409 mapping; 7-day gates after admin reset | Mix: small Fix vs Accept — lock in H7 |
| Phase 9 / SP-087 | Device GPX, public APK dump, share-sheet VIEW, internal Pro walk | Device-verify |
| Phase 9 / SPD-075 | Monetisation analytics upload | Follow H5 |
| Phase 9 / SP-087 | Qt ungated; `ReloadBookmarkRoutine` no `historicalTracks`; multi-cat KMZ; FromLatLon; system expat | Accept (not Android V1) |

## Recommended locks (not Accepted)

Full text and reject lists live in SP-088. Short form:

| Ref | Recommended position |
| --- | --- |
| H1 | D1 Pixel-class + D2 one aggressive OEM. Optional D3 second API level. |
| H2 | Rendering: Spike 1 bar unchanged. Battery: protocol lock now; numeric ceiling after SP-094 or explicit waiver. |
| H3 | Google Play is the public V1 store gate. F-Droid may ship the same artefact. Huawei/web not a V1 gate. |
| H4 | Product-owned Street Pixels privacy policy + terms; in-app links must not stay on unmodified CoMaps pages. |
| H5 | No new public analytics upload sink in V1. Local uint64 only. §33 hypotheses via closed-beta observation. |
| H6 | Keep ABL absent unless D2 measurement proves FGS is insufficient. |
| H7 | Disposition table above. |
| H8 | Fork listing, applicationId, signing, and metadata; do not ship unmodified CoMaps Play copy. |
| H9 | Operationalize SPD-061: hide friend UI and public add-friend filters. |
| H10 | Recorded local `street_pixels_tests` + smoke + lint + clang-format as the V1 gate. Narrowing Forgejo `CTEST_EXCLUDE_REGEX` is not a launch blocker. |
