# SP-090 — Settings, empty-state, and first-run audit

**Phase:** 10 — Android release hardening
**Status:** Accepted
**Depends on:** SP-088 H4/H9 Accepted (**SPD-080**, **SPD-085**).
  Phase 10 implementation entry.
**Unblocks:** SP-095 / SP-097 observe the surfaces (device execution
  residual)
**Notes:** Implement spec §30/§31/§10 **except** privacy-policy/terms
  URL rows and app-name string rebrand (**residual**, SPD-080 /
  SPD-084). Hide friends settings (SPD-085). SP-093 is residual.

---

## Objective

Make Android settings match product spec §30, implement every spec §31
error and empty state that is missing or wrong, and walk spec §10
first-run so a new user can record without a tutorial — without
exposing radius or internal parameters.

---

## Motivation

Phase 10 exit criteria 2 and 3 are §31 and §30. `prefs_privacy.xml`
today is search history and Play services. Spec §30 also requires
competition enable/disable, nickname, delete competition profile,
map-data management, local recording management, and competition-rules
copy. Privacy-policy / terms URL rows and app-name rebrand are residual
(SPD-080 / SPD-084); this item does not land them. Friends copy still
describes “username so friends can add you” (SPD-061).

§31 states are specified as UX, not as “log a warning”. Several were
residualled from Phase 5 (no-area) and Phase 6 (Avoid impossible) as
device observations; the **copy and actions** still need an audit
against the spec sentences.

---

## In-scope behavior

- Audit current settings IA against spec §30. Add missing Street
  Pixels rows **except** privacy-policy / terms URL rows (residual,
  SPD-080) and app-name string rebrand (residual, SPD-084). Remove
  or hide public-V1 friend rows (H9 / **SPD-085** / SPD-061);
  confirm haptics toggle remains the single exploration-haptics
  control (SPD-054).
- Confirm the 25 m radius and HEALPix / GPS / decay / scoring
  internals are **not** ordinary settings.
- GPX rows only when capability+entitlement allow (SPD-070 / SPD-072
  / SP-084). Public builds add nothing.
- Audit each §31 state: location denied; background location denied
  (foreground recording remains); no downloaded map; poor GPS
  accuracy (no interpolation); interrupted recording; no selected
  exploration area; no local competitors (no empty leaderboard);
  no competition connectivity (queue); Avoid impossible (Prefer
  offer — SPD-042, may already be SP-089).
- Implement missing copy/actions. Reuse existing strings when they
  match the spec; do not paraphrase privacy guarantees.
- Spec §10 first-run: permission rationale is session-only and not
  bundled with competition; first recording; first pixels; no full
  tutorial required. Record gaps; fix copy/flow holes that are not
  owned by another Phase 10 item.
- English strings in this item; translations follow existing
  Streifzug process, not a launch blocker unless a user-visible English
  string is missing.

## Out-of-scope behavior

- Purchase / restore / pricing settings (SPD-010).
- Drawing the check glyph, share-card defects, weekly JNI (SP-089).
- Privacy *policy text*, hosting, and in-app URLs (SP-093
  **residual**; SPD-080 landing). Do not retarget Help to a new
  Street Pixels policy URL in this item. `https://streifzug.app/privacy/`
  may stay for now.
- App-name / Streifzug product branding in user-visible strings
  (residual, SPD-084).
- Device execution of the §31 matrix (SP-095 / SP-097 **residual**).
- New achievement screens (spec §18.5).

## Relevant product requirements

- Spec §10, §30, §31, §34 Progress / Recording / Routing / Privacy.
- SPD-010, SPD-011, SPD-042, SPD-054, SPD-061, SPD-070, SPD-074,
  SPD-080, SPD-082, SPD-084, SPD-085.

## Relevant source files or symbols

- `prefs_main.xml`, `prefs_interface.xml`, `prefs_privacy.xml`,
  `DataManagementSettingsFragment`, `GpxSettingsFragment`,
  `MyAccountDialogFragment`, `ExploreConsentDialogFragment`
- Location permission rationale in `MwmActivity`
- Area empty / no-area overlay (SP-040)
- Recording interruption UI (SP-013)
- Avoid no-route UI (SP-058)

## Implementation notes / constraints

- Settings audit first (table in completion evidence): required row
  → present / missing / wrong. Then implement missing/wrong only.
- Do not add a dedicated “Street Pixels debug” screen in public
  builds.
- Background-location denied copy must not imply ABL is requested:
  H6 / **SPD-082** keeps ABL absent. Explain FGS / screen-off limits
  honestly.

## Acceptance criteria

1. Evidence table maps every §30 bullet to a surface.
2. Evidence table maps every §31 state to copy + action, implemented
   or explicitly residualled with owner.
3. No radius or internal-parameter setting in public builds.
4. Friend settings hidden in public builds (H9 / **SPD-085**).
   Privacy-policy/terms URL rows and app-name rebrand remain residual.
5. Focused JVM / C++ tests for any new gating. Agent does not mark
   Accepted.

## Required automated tests

- Settings visibility: public (all Pro capabilities false) has no
  GPX rows and no friend rows.
- §31 Avoid / no-area behaviours that are already unit-tested remain
  green; add tests only for new branches.

## Required manual validation

- First-run click-through on D1 is SP-095 (**residual**). This item
  records the intended script; do not execute a hardware walk.

## Failure and rollback considerations

- If a §31 state needs ABL (H6 still absent), write copy for FGS
  death rather than adding the permission here.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-090-settings-empty-states-6383` |
| §30 table | See §30 audit below. |
| §31 table | See §31 audit below. |
| Test output | `./gradlew :app:testGoogleDebugUnitTest --tests 'app.organicmaps.settings.*' --tests 'app.organicmaps.location.GpsWaitingStateTest' --tests 'app.organicmaps.location.RecordingSessionUiModelTest' :sdk:testDebugUnitTest --tests 'app.organicmaps.sdk.routing.StreetExplorationRoutingOptionsTest'` → **BUILD SUCCESSFUL**. JUnit XML: **51 tests, 0 failures, 0 errors, 0 skipped**. Independent-review re-run after review fixes (`c9adff9b5`). |
| Accepted by | product owner (implement → review lock 2026-08-29) |
| Accepted date | 2026-08-29 |

### §30 settings audit

| Spec §30 bullet | Surface | Verdict |
| --- | --- | --- |
| Exploration haptics on or off | `prefs_main.xml` `StreetPixels.ExplorationHaptics`; `SettingsPrefsFragment`. Duplicate Interface row removed. | Present (SPD-054 single toggle) |
| Competition enabled or disabled | Privacy `pref_competition_enabled` only. My Account has no second sync switch | Present |
| Public nickname | Privacy `pref_public_nickname` → `MyAccountDialogFragment`. Summary: rankings sentence, not friends | Present |
| Delete competition profile | Privacy delete row + My Account delete | Present |
| Map-data management | `prefs_data_management.xml` storage / autodownload / incomplete SPA | Present |
| Local recording management | `pref_local_recordings` → `BookmarkCategoriesActivity` | Present |
| Privacy information | In-app dialog reuses `location_privacy_info` + `explore_consent_message` (no paraphrase, no URL) | Present |
| Terms and competition rules | In-app rules dialog from existing consent / leave / delete strings. Help still uses `streifzug.app` `privacy/` and `terms/` | Copy present; **URL rows residual** (SPD-080 / SP-093) |
| App name in Help / listing | Unchanged Streifzug product name | **Residual** SPD-084 / SP-093 |
| GPX import/export | `GpxSettingsVisibility` capability+entitlement only (SP-084) | Present; public build adds nothing |
| Purchase / restore / pricing | None added | Out of scope SPD-010 |
| Friend settings | `prefs_privacy.xml` friend-visibility row is inflated then removed when `FriendSettingsVisibility.friendsCapabilityEnabled()` is false. Nickname copy is rankings-only | Hidden SPD-085. Manifest add-friend filters are SP-092 |
| 25 m radius | Not in any `prefs_*.xml`. C++ `kExploreRadiusMeters = 25.0` | Confirmed not a setting |
| HEALPix / GPS / decay / scoring internals | Not in prefs XML or Advanced settings | Confirmed not ordinary settings |

### §31 empty-state audit

| Spec §31 state | Copy + action | Verdict |
| --- | --- | --- |
| Location denied | Map stays up. `street_pixels_location_denied`. Open app settings + `continue_browsing` | Implemented |
| Background location denied | No ABL. `track_recording_background_explanation` (FGS / notification / pause / foreground remains). Shown after first recording start, does not block recording | Implemented (SPD-082) |
| No downloaded map | `downloader_no_downloaded_maps_message` and `offline_explanation_text` use the spec download sentence | Implemented |
| Poor GPS accuracy | `GpsWaitingState` (>25 m or no fix while recording, not paused) → `gps_waiting_badge`; no interpolation | Implemented |
| Interrupted recording | Existing `track_recording_interrupted_text` matches spec | Present (unchanged) |
| No selected exploration area | `street_pixels_no_exploration_area_message` includes competition unavailable | Implemented |
| No local competitors | `CompetitionEmptyState.showRankingRows`; weekly board hidden when empty | Implemented |
| No competition connectivity | `competition_status_offline` queue copy | Implemented |
| Avoid explored impossible | Prefer (SP-089) + `dialog_routing_avoid_explored_normal_button` → `normalFallback` (`MODE_NEITHER`) | Implemented |

### §10 first-run (script for SP-095; not executed)

1. App opens to the map. Splash does not request location (`FirstRunFlow.requestLocationOnAppOpen()` is false).
2. Spec card: heading / body / **Start exploring**. Close marks the card seen.
3. **Start exploring** starts recording, which shows the session-only location rationale (not bundled with competition).
4. After permission, recording starts. One-shot FGS / screen-off explanation. No ABL request.
5. Recording control + first-100 m badge already exist. No full tutorial.

Device click-through remains SP-095 / SP-097.

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Help privacy/terms still `streifzug.app` `privacy/` and `terms/`; app-name Streifzug in Help | SP-093 / SPD-080 / SPD-084 residual. Do not retarget in this item |
| Manifest add-friend filters still present | SP-092 |
| Device execution of the §31 matrix and §10 click-through | SP-095 / SP-097 residual |
| First-run / empty-state English strings only | Translations follow Streifzug process |
| `applyCompetitionChrome` weekly board used out-of-scope `osmId`/`manager` locals (pre-existing compile hole) | Fixed in this item so weekly empty-hide compiles |
| Spec §10 first-run body says routes/history stay on device *unless* the user joins rankings. Spec §3.2 / §25 say tracks never upload; competition is aggregates only | Not rewritten in this item (copy matches spec §10). Residual for SP-093 / privacy copy |
