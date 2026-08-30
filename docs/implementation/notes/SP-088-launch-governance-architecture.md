# SP-088 — Phase 10 launch-governance architecture notes

**Date:** 2026-08-29
**Branch:** `cursor/phase-10-work-items-6383`
**Locks:** H1–H10 **Accepted** 2026-08-29 as **SPD-077–086**.
Brand-related writing and on-device test *execution* are residual
(not implemented in later Phase 10 coding items). The decisions
themselves are Accepted.

This note is the investigation companion to
[`SP-088-launch-governance-decisions.md`](../work-items/SP-088-launch-governance-decisions.md).
It is not a decision.

## Entry vs planning

Phase 10 **implementation** still requires Phases 1–9 at exit, no open
work item, and a release-configured build. Phase 11 is not a prerequisite.
in-progress work item, and a release-configured installable build
(`phases/phase-10-android-release-hardening.md` entry criteria).

Work-item **planning** on 2026-08-29 is a docs-only pass. SP-088 itself is
docs / `DECISIONS.md` and may run while Phases 5–8 await maintainer exit.
Product-owner lock 2026-08-29 Accepted H1–H10 as SPD-077–086 with brand
and on-device testing residualised. Those locks are recorded. Coding
SP-089+ waits on Phase 10 implementation entry criteria. Residual WIs
(SP-093, SP-095, SP-094 device, SP-097 device, brand listing) are not
coding items.

Open work items on the planning date (do not treat as current without
re-check): SP-041 awaiting acceptance; SP-050–053 In review; SP-055 In
review; SP-061 in progress; SP-063 In review; SP-069 In review; SP-071 in
progress; SP-079 in progress.

## Code snapshot (this working tree, 2026-08-29)

Re-verified against `street-pixels` at planning time. The 2026-07-25
phase-10 table is stale in several rows.

| Concern | Location | Observed state |
| --- | --- | --- |
| Android manifest | `android/app/src/main/AndroidManifest.xml` | `ACCESS_COARSE/FINE_LOCATION`, `ACCESS_LOCATION_EXTRA_COMMANDS`, `FOREGROUND_SERVICE` + `LOCATION` + `DATA_SYNC`, `POST_NOTIFICATIONS`, `VIBRATE`. **`ACCESS_BACKGROUND_LOCATION` still absent.** FGS types `location` (`NavigationService`, `TrackRecordingService`), `dataSync` (`DownloaderService`). **`streifzug://add-friend` and HTTPS `/add-friend` still registered** (SPD-061 hide not applied to intent-filters). |
| Store credentials | `docs/CREDENTIALS.md` | CI secrets for signed store builds (upstream Streifzug). |
| Release workflows | `.forgejo/workflows/android-release.yaml`, `android-beta.yaml`, `android-check-metadata.yaml`, `android-release-metadata.yaml` | Present; upstream Streifzug application identity. |
| Android lint | `.github/workflows/android-check.yaml` | `./gradlew -Pandroidauto=true lint`. |
| Flavors | `android/app/build.gradle` | `google`, `web`, `fdroid`, `huawei`; `debug`, `release`, `beta`. |
| Android tests | `android/app/src/test/`, `android/sdk/src/test/` | JVM tests now include Street Pixels gates (Explorer Pro, GPX, recording UI model, routing options). **Still no `androidTest` instrumented tests.** |
| Play listing | `android/app/src/google/play/listings/en-US/full-description.txt` | Upstream Streifzug copy. Advertises GPX import/export. Does not describe Street Pixels recording, competition, or session-only location. |
| Privacy / terms URLs | `HelpFragment` → `R.string.app_site_url` + `privacy/` / `terms/` | `app_site_url` is `https://streifzug.app/`. No Street Pixels policy text in this repository. |
| Privacy settings | `PrivacySettingsFragment` / `prefs_privacy.xml` | Search history + Google Play services location provider. **No** Street Pixels privacy information, terms, or competition-rules rows (spec §30). |
| Competition settings | `MyAccountDialogFragment`, `ExploreConsentDialogFragment`, `UsernameDialogFragment` | Opt-in, nickname, delete exist as dialogs. Friend-visibility / “username so friends can add you” strings still in `values/strings.xml`. |
| Location rationale | `track_recording_location_rationale` | Session-only; not bundled with competition (spec §10 step 3). Still branded “Streifzug”. |
| Product analytics | `StreetExplorationRoutingAnalytics`, `CompletionCardAnalytics`, `ExplorerProAnalytics` | Count-only local uint64. **No** spec §32.1 activation counters. **No** §32.2 core (except routing + milestones-as-UI). **No** §32.3 competition counters. **No upload sink** — Phase 10 upload residual from SPD-044 / SPD-055 / SPD-075 is **closed by SPD-081** (stay local; do not build a sink). |
| Sentry | AndroidManifest `io.sentry.*` | SP-003 private-by-default defaults (PII/screenshots off). Re-verify in SP-091 / SP-097. |
| Completed check | `area_overlay.cpp` `m_showCheck` | Style flag set for completed areas; **glyph not drawn** (SP-040 / SP-041 R3). |
| Share card date | `CompletionCardShare` / card layout | **SPD-056** always-include date; share-time checkbox residual (SP-069). |
| Weekly board JNI | SP-079 evidence | Weekly GET **not JNI-wired**. |
| Off-route Avoid | SP-061 R3 | `CheckLocationForRouting` `OnRemoveRoute` nullptr; Prefer+seekbar not shown. |

**Difference from the technical audit (2026-07-20) and from the 2026-07-25
phase-10 snapshot:** Phases 1–9 have landed session gating, rematch, areas,
routing, milestones, competition, and GPX gates. Android JVM tests are more
than three files. Instrumented tests are still absent. Friends deep links
and Streifzug store/privacy URLs are unchanged. ABL is still absent (SP-012
measured Pixel 3a without it). H1–H10 are **Accepted** 2026-08-29 as
SPD-077–086; brand writing and on-device testing remain residual.

## Carried residual inventory (planning date)

Classes used in H7 (**SPD-083**): **Fix** (SP-089), **Measure** (SP-094;
device execution residual), **Device-verify** (SP-095; execution
residual), **Ops** (SP-096), **Follow H5** (SP-091 local only,
**SPD-081**), **Accept/waive**, **Not Phase 10**. Brand writing is
not a Fix item.

| From | Residual | H7 class (SPD-083) |
| --- | --- | --- |
| Phase 2 / SP-014 | Aggressive-OEM screen-off / background sample continuity | Device-verify (**execution residual**) |
| Phase 3 / SP-022 | Pixel 3a / Uusimaa reconciliation UX; rematch timing on large `.pix` | Device-verify + Measure (**execution residual**) |
| Phase 4 / SP-031 | R3 Helsinki / rural / coastal walks; no MWM-id as neighbourhood name | Device-verify (**execution residual**) |
| Phase 4 / SP-042 | Option A mapgen collectors → `.spa` | **Not Phase 10** (SPD-033) |
| Phase 4 / SP-048 | Android incomplete-`.spa` toast/dialog | Fix |
| Phase 4 / SP-049–053 | LAN/CDN publish mirror; S2–S8 device download | Device enabler for Helsinki walks; **not** a Phase 10 feature |
| Phase 5 / SP-033 | Quantitative Spike 1 FPS p95 ≥30 / memory uplift &lt;150 MB | Measure (**execution residual**) |
| Phase 5 / SP-041 R1 | Helsinki badge/focus/tap/city zoom/completed chrome/§31 empty | Device-verify (needs `.spa` on device; **execution residual**) |
| Phase 5 / SP-041 R3 | Completed check glyph not drawn | Fix |
| Phase 5 / SP-041 R4 | Overlay neighbourhood-baked push retune | Accept/waive |
| Phase 6 / SP-054 | Spike 7 city-scale / device | Measure (**execution residual**) |
| Phase 6 / SP-061 R3 | GPS off-route Prefer dialog not shown | Fix |
| Phase 6 / SP-060 | Routing analytics upload | Follow H5 (**SPD-081**: stay local; no sink) |
| Phase 6 / SP-061 R5 | No in-app debug readout of counters | Accept/waive |
| Phase 7 / SP-069 | Device celebration, card image, share, haptics, nav | Device-verify (**execution residual**) |
| Phase 7 / SP-069 | Date checkbox vs SPD-056 | Fix |
| Phase 7 / SP-069 R4 | 4 s auto-ack vs share-target PNG lifetime | Fix |
| Phase 7 / SP-069 R5 | `onResume` rebind increments generated / resets checkbox | Fix |
| Phase 7 / SPD-055 | Growth-counter upload | Follow H5 (**SPD-081**: stay local; no sink) |
| Phase 8 / SP-079 | Device opt-in, traffic capture, opt-out, offline queue, N&lt;3, delete | Device-verify (**execution residual**) |
| Phase 8 / SP-079 | Weekly GET not JNI-wired | Fix |
| Phase 8 / SP-072 | Revoke does not delete `live_recency.db` rows | Fix (privacy) |
| Phase 8 / SP-075 | Postgres production deploy; exact EU region string | Ops |
| Phase 8 / SP-077 | Failed POST `/leave` no retry; HTTP 409 mapping; 7-day gates after admin reset | **Accept** (SPD-083; not SP-089 Fix, not SP-096 Ops) |
| Phase 9 / SP-087 | Device GPX, public APK dump, share-sheet VIEW, internal Pro walk | Device-verify (**execution residual**) |
| Phase 9 / SPD-075 | Monetisation analytics upload | Follow H5 (**SPD-081**: stay local; no sink) |
| Phase 9 / SP-087 | Qt ungated; `ReloadBookmarkRoutine` no `historicalTracks`; multi-cat KMZ; FromLatLon; system expat | Accept (not Android V1) |

## Accepted locks (2026-08-29)

Product-owner lock 2026-08-29 accepted every recommended H1–H10 position
as **SPD-077–086**, with brand writing and on-device testing
residualised. Full text and reject lists live in SP-088.

| Ref | Accepted position | Residual note |
| --- | --- | --- |
| H1 / SPD-077 | D1 Pixel-class + D2 one aggressive OEM. Optional D3 second API level. | Matrix *execution* residual. |
| H2 / SPD-078 | Rendering: Spike 1 bar unchanged. Battery: protocol lock now; numeric ceiling after SP-094 or explicit waiver. | Measurement *execution* residual. Docs may record the protocol. |
| H3 / SPD-079 | Google Play is the public V1 store gate. F-Droid may ship the same artefact. Huawei/web not a V1 gate. | Listing brand copy residual. |
| H4 / SPD-080 | Product-owned Street Pixels privacy policy + terms (intended). | Landing text/URLs residual. SP-093 residual. `streifzug.app` may stay for now. |
| H5 / SPD-081 | No new public analytics upload sink in V1. Local uint64 only. §33 hypotheses via closed-beta observation. Closes SPD-044/055/075 Phase 10 upload residual. | SP-091 implements local counters + payload-shape tests; no sink. |
| H6 / SPD-082 | Keep ABL absent unless a later D2 measurement proves FGS is insufficient. | D2 execution residual; Phase 10 coding keeps ABL absent. |
| H7 / SPD-083 | Disposition table above. | Device-verify *execution* residual. Fix list remains SP-089 (not brand). |
| H8 / SPD-084 | Reuse machinery; fork listing, applicationId, and signing conceptually. | Application name, listing copy, privacy/terms URLs residual. |
| H9 / SPD-085 | Operationalize SPD-061: hide friend UI and public add-friend filters. | **Implementable** in SP-092 (not brand). |
| H10 / SPD-086 | Recorded local `street_pixels_tests` + smoke + lint + clang-format as the V1 gate. Narrowing Forgejo `CTEST_EXCLUDE_REGEX` is not a launch blocker. | SP-097 automated suites in scope; device/manual hardware residual. |
