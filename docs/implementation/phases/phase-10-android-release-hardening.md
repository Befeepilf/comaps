# Phase 10 — Android release hardening

**Status:** Not started (work-item planning 2026-08-29; implementation
blocked on other phases’ exit)
**Depends on:** every other phase
**Blocks:** public release

---

## Objective

Turn a feature-complete build into a shippable one. Verify every launch
requirement in product spec §34 with recorded evidence, make the store
disclosures and legal text match actual behaviour, and confirm quality
characteristics that only show up on real devices over real time.

This phase adds no features. Everything it touches is verification, disclosure,
or a defect found during verification.

## Product-spec references

- §33 V1 success indicators.
- §34 Public V1 launch requirements — the complete checklist, in all of its
  sections: core map and exploration, recording, GPS integrity, progress
  experience, routing, privacy and competition, offline and map updates,
  sharing, Explorer Pro and monetisation, release governance, quality.
- §30 Settings.
- §31 Error and empty states.
- §32 Product analytics.

## Technical-audit references

- §20 Build and testing status.
- §22 Risk register, in full. Every risk needs a final position: mitigated,
  accepted, or realised.
- §26 Go/no-go assessment and its seven launch-blocking conditions.
- §17 Privacy, analytics, and security.

## Current code locations

Verified 2026-07-25 against the working tree. **Re-verified 2026-08-29**
during work-item planning. Extra detail in
[`notes/SP-088-launch-governance-architecture.md`](../notes/SP-088-launch-governance-architecture.md).

| Concern | Location | Observed state |
| --- | --- | --- |
| Android manifest | `android/app/src/main/AndroidManifest.xml` | Location permissions present; `ACCESS_BACKGROUND_LOCATION` still absent; FGS types `location` (`NavigationService`, `TrackRecordingService`), `dataSync` (`DownloaderService`); **`comaps://add-friend` and HTTPS `/add-friend` still registered** (SPD-061 not applied to intent-filters) |
| Store credentials | `docs/CREDENTIALS.md` | Documents the CI secrets required for signed store builds |
| Release workflows | `.forgejo/workflows/android-release.yaml`, `android-beta.yaml`, `android-check-metadata.yaml`, `android-release-metadata.yaml` | Present; upstream CoMaps release machinery and listing identity |
| Android lint | `.github/workflows/android-check.yaml` | `./gradlew -Pandroidauto=true lint` |
| Flavors | `android/app/build.gradle` | `google`, `web`, `fdroid`, `huawei`; build types `debug`, `release`, `beta` |
| Android tests | `android/app/src/test/`, `android/sdk/src/test/` | JVM tests now include Street Pixels gates (Explorer Pro, GPX, recording UI, routing options). **Still no `androidTest` instrumented tests.** |
| Play listing | `android/app/src/google/play/listings/en-US/full-description.txt` | Upstream CoMaps copy; advertises GPX import/export; no Street Pixels session/competition disclosure |
| Privacy policy and terms | `HelpFragment` → `https://comaps.app/privacy/` and `terms/` | No Street Pixels policy text in this repository |
| Privacy settings | `PrivacySettingsFragment` / `prefs_privacy.xml` | Search history + Play services. Spec §30 Street Pixels rows not present |
| Product analytics | routing / card / Pro uint64 helpers | Local count-only. No §32.1 / §32.3. No upload sink |
| Error and empty states | across the Android app | Partial (no-area, interruption, Avoid no-route). Not yet audited against spec §31 |

**Difference from the technical audit (2026-07-20) and the 2026-07-25
snapshot:** Phases 1–9 have landed session gating, rematch, areas, routing,
milestones, competition, and GPX gates. Instrumented tests are still absent.
Friends deep links and CoMaps store/privacy URLs are unchanged. ABL is still
absent (SP-012 Pixel 3a without it). H1–H10 are **not** Accepted until SP-088.

## Intended outcome

- Every product spec §34 line item verified, with the evidence written down and
  attributable to a person, a device, a build, and a date.
- Store listing, data-safety disclosure, permission rationales, privacy policy,
  and competition consent text that match what the software actually does.
- Battery and rendering behaviour measured over realistic sessions.
- Every risk in the audit register closed out with a stated position.

## Dependencies

All other phases at their exit criteria before **implementation** (SP-089+).
Partial implementation entry produces false confidence.

SP-088 is docs / `DECISIONS.md` only and may run while Phases 5–8 await
maintainer exit, matching the SP-062 pattern.

## Carried residuals from earlier phases

These do not block earlier phase exits. Phase 10 must close them with recorded
device evidence, a Fix in SP-089, a measurement in SP-094, ops in SP-096, or
an explicit accepted waiver (H7 / draft SPD-083). Full inventory:
[`notes/SP-088-launch-governance-architecture.md`](../notes/SP-088-launch-governance-architecture.md).

| From | Residual | Source | Recommended H7 class |
| --- | --- | --- | --- |
| Phase 2 | Aggressive-OEM screen-off / background sample continuity (exit #7 partial; Pixel 3a done) | SP-014 | Device-verify (SP-095) |
| Phase 3 | Maintainer device walks (Pixel 3a / Uusimaa-scale reconciliation UX); rematch timing | SP-022 | Device-verify + Measure |
| Phase 4 | R3 device walks: Helsinki UX, rural/coastal, no MWM-id as neighbourhood name in UI | SP-031 | Device-verify (SP-095) |
| Phase 4 | R1 (narrowed): production mapgen collectors → `.spa` / CDN shipping | SP-042–048 **Accepted** 2026-08-08 (**SPD-033**); Option A remains residual | **Not Phase 10** |
| Phase 4 | LAN/CDN publish mirror; S2–S8 device download | SP-049–053 | Device *enabler* for Helsinki; not a Phase 10 feature |
| Phase 4 | Incomplete-`.spa` Android chrome | SP-048 | Fix (SP-089) |
| Phase 5 | Quantitative Spike 1 FPS/memory (Pixel 3a qualitative OK; numbers deferred) | SP-033 / SP-041 R2 | Measure (SP-094) |
| Phase 5 | Device Helsinki walks: badge/focus/tap/city zoom/completed chrome/§31 empty/no country-world UI | SP-041 R1 — needs `.spa` via download (SP-053) | Device-verify (SP-095) |
| Phase 5 | Completed check glyph not drawn (`m_showCheck` reserved; outline+fill shipped) | SP-040 / SP-041 R3 | Fix (SP-089) |
| Phase 6 | Spike 7 city-scale / device; all routing device walks | SP-054 / SP-061 | Measure + Device-verify |
| Phase 6 | GPS off-route Prefer dialog not shown (`nullptr` removeRouteCallback) | SP-061 R3 | Fix (SP-089) |
| Phase 6 | Routing analytics upload | SP-060 / SPD-044 | Follow H5 (SP-091) |
| Phase 7 | Device celebration, card, share, haptics, nav | SP-069 | Device-verify (SP-095) |
| Phase 7 | Date checkbox vs SPD-056; 4 s PNG lifetime; `onResume` counter | SP-069 | Fix (SP-089) |
| Phase 7 | Growth-counter upload | SPD-055 | Follow H5 (SP-091) |
| Phase 8 | Device opt-in, traffic capture, opt-out, queue, N&lt;3, delete | SP-079 | Device-verify (SP-095) |
| Phase 8 | Weekly GET not JNI-wired; recency rows survive revoke | SP-079 / SP-072 | Fix (SP-089) |
| Phase 8 | Postgres production deploy; exact EU region string | SP-075 / SPD-062 | Ops (SP-096) |
| Phase 9 | Device GPX / public APK / share-sheet / internal Pro | SP-087 | Device-verify (SP-095) |
| Phase 9 | Monetisation analytics upload | SPD-075 | Follow H5 (SP-091) |
| Phase 9 | Qt ungated; reload no-paint; multi-cat KMZ; FromLatLon; system expat | SP-087 | Accept (not Android V1) |

## Work-item breakdown

Work-item planning 2026-08-29. Locks H1–H10 live in
[`SP-088`](../work-items/SP-088-launch-governance-decisions.md) as recommended
positions (OQ-30–OQ-39 / draft **SPD-077–086**). Coding SP-089+ waits on those
locks **and** phase entry (other phases at exit). SP-088 itself is docs-only
and may run while Phases 5–8 await maintainer exit.

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-088](../work-items/SP-088-launch-governance-decisions.md) | Launch-governance decisions (**entry gate**) |
| 2 | [SP-089](../work-items/SP-089-residual-defect-close-out.md) | Locked residual defect close-out |
| 3 | [SP-090](../work-items/SP-090-settings-empty-states-first-run.md) | Settings, empty-state, and first-run audit |
| 4 | [SP-091](../work-items/SP-091-product-analytics-reconciliation.md) | Product analytics reconciliation |
| 5 | [SP-092](../work-items/SP-092-permissions-manifest-store-disclosures.md) | Permissions, manifest, and store disclosures |
| 6 | [SP-093](../work-items/SP-093-privacy-policy-terms-consent.md) | Privacy policy, terms, and consent alignment |
| 7 | [SP-094](../work-items/SP-094-battery-rendering-lifecycle.md) | Battery, rendering, and lifecycle measurement |
| 8 | [SP-095](../work-items/SP-095-device-matrix-residual-close-out.md) | Device-matrix residual close-out |
| 9 | [SP-096](../work-items/SP-096-risk-register-and-release-pipeline.md) | Risk-register close-out and release pipeline |
| 10 | [SP-097](../work-items/SP-097-phase10-launch-requirement-verification.md) | Phase 10 / §34 verification (**exit gate**) |

Gate: SP-088 must lock H1–H10 (or record maintainer deferrals) before SP-089+
coding. This phase still **adds no features** beyond defects H7 classifies as
Fix, disclosure text, and verification.

## Data and migration concerns

- This phase should introduce no schema change. If verification uncovers one,
  it is a defect in an earlier phase and is fixed there, on its own branch.
- Confirm no exploration data is lost across: app upgrade, map update, force
  stop, low-memory kill, device restart, and time-zone change.
- Confirm the app behaves correctly on a device where storage is nearly full
  during pixel derivation or migration.
- Confirm that deleting the competition profile leaves local exploration
  intact, and that clearing app data removes everything as the privacy policy
  claims.

## Privacy and security implications

The final privacy gate. What must be true before release:

- No raw GPS data is uploaded, from any code path, in any build configuration.
- Analytics contain no location values, no screenshots, and no view
  hierarchies.
- The privacy policy accurately describes what stays local and what is
  uploaded.
- The competition consent text matches actual upload behaviour item by item.
- Store permission declarations and background-location disclosure are
  accurate. ABL was **not** added in Phase 2 (SP-012). If H6 later adds it,
  the Play Console background-location declaration and its justification
  video must match the session-based behaviour.
- No known path reveals another user's live or exact location.
- Logs in release builds contain no coordinates.
- Sparse-area anonymity holds against direct API calls, not only in the UI.

## Automated testing strategy

- Full smoke suite green: `./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke`.
- The Street Pixels test target green.
- Android lint clean, or every remaining warning triaged.
- `./tools/unix/clang-format.sh` clean.
- A payload-shape test asserting that no upload path can emit a
  location-shaped field, run against the release configuration.
- Backend test suite green, if it exists by then.

Note the standing CI gap: `.forgejo/workflows/linux-check.yaml` excludes
`map_tests` and most other suites through `CTEST_EXCLUDE_REGEX`, and
`.github/workflows/` has no C++ test job. Recommended H10: the recorded local
run is the V1 gate; narrowing exclusions is not a Phase 10 coding task.

## Manual validation strategy

This phase is mostly manual, structured as an evidence log rather than a
walkthrough.

- Execute every spec §34 line item and record: who, which device, which OS
  version, which build, what was observed.
- Execute every spec §31 error and empty state deliberately, including denied
  location, denied background location, no downloaded map, poor GPS accuracy,
  interrupted recording, no exploration area, no local competitors, no
  connectivity, and an impossible avoid-explored route.
- Multi-hour recording session with battery measurement, compared against a
  control session with recording off.
- Screen-off recording on the full device matrix.
- Cold start with a large city loaded, measured to first interactive frame.
- Fresh-install first-run journey following spec §10 step by step.
- Upgrade from a prior build with existing exploration data.
- Offline-only usage for a full session including routing.

## Entry criteria

- Every other phase has met its exit criteria (required for SP-089+; not for
  SP-088).
- No open work item is in progress (required for SP-089+).
- A release-configured build exists and is installable (required for
  SP-094–097).

## Exit criteria

1. Every product spec §34 line item is verified with recorded evidence.
2. Every spec §31 error and empty state is implemented and observed.
3. Settings match spec §30, with no radius or internal-parameter exposure.
4. Analytics match spec §32 and contain no location data.
5. Privacy policy, terms, consent text, and store disclosures match actual
   behaviour.
6. Battery consumption during active recording is measured and accepted.
7. Rendering performance on the release build meets the recorded criteria.
8. No critical exploration-data-loss path exists across the tested lifecycle
   events.
9. No known path reveals another user's live or exact location.
10. Every audit risk has a stated final position.
11. Store build signing works and the release pipeline produces an installable
    artefact.

## Explicit non-goals

- Any new feature.
- iOS release preparation of any kind.
- Explorer Pro purchasing enablement.
- Post-V1 candidates from spec §35.
- Performance work beyond meeting the stated criteria.
- Refactoring.
- Marketing assets and campaign material, which are not code and are not gated
  here.

## Known uncertainties

Recorded as **H1–H10** in
[`SP-088`](../work-items/SP-088-launch-governance-decisions.md), proposed as
**OQ-30–OQ-39** (draft SPD-077–086) in `DECISIONS.md` §15.
Coding SP-089+ waits on those locks (Accepted SPDs or an explicit maintainer
deferral).

Recommended positions (not Accepted until SP-088 / maintainer):

| Ref | Question | Recommended lock |
| --- | --- | --- |
| H1 | Device matrix | D1 Pixel-class + D2 one aggressive OEM |
| H2 | Battery / rendering bars | Spike 1 unchanged; battery protocol now, numeric ceiling after SP-094 |
| H3 | Store flavors | Google Play is the V1 gate; F-Droid same artefact optional; Huawei/web not a gate |
| H4 | Privacy policy / terms | Product-owned Street Pixels text; not unmodified `comaps.app` pages |
| H5 | Analytics upload | No new public sink; local uint64 only |
| H6 | `ACCESS_BACKGROUND_LOCATION` | Keep absent unless D2 proves FGS insufficient |
| H7 | Residual disposition | Fix / Measure / Device-verify / Ops / Follow H5 / Accept table in the note |
| H8 | Release workflows | Reuse machinery; fork listing, applicationId, and signing |
| H9 | Friends in public APK | Operationalize SPD-061 (hide UI and add-friend filters) |
| H10 | C++ CI exclusions | Not a launch blocker; recorded local suites are the V1 gate |

Friends *presence* in V1 is already **SPD-061** (hidden). H9 is how far the
public APK strips the leftover surface.

Play Store review of an ABL justification (only if H6 adds ABL) remains
outside the team’s control.
