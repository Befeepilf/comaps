# Phase 10 — Android release hardening

**Status:** Not started (work-item planning 2026-08-29; implementation
blocked on other phases’ exit)
**Depends on:** Phases 1–9 (implementation SP-089+). Phase 11 is not required.
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
| Android manifest | `android/app/src/main/AndroidManifest.xml` | Location permissions present; `ACCESS_BACKGROUND_LOCATION` still absent (`tools:node="remove"`; **SPD-082**); FGS types `location` (`NavigationService`, `TrackRecordingService`), `dataSync` (`DownloaderService`); dedicated `streifzug://add-friend` and HTTPS `/add-friend` filters **removed** (SP-092 / **SPD-085**; generic VIEW can still deliver leftover URIs, onboarding hidden) |
| Store credentials | `docs/CREDENTIALS.md` | Documents the CI secrets required for signed store builds |
| Release workflows | `.forgejo/workflows/android-release.yaml`, `android-beta.yaml`, `android-check-metadata.yaml`, `android-release-metadata.yaml` | Present; upstream Streifzug release machinery and listing identity |
| Android lint | `.github/workflows/android-check.yaml` | `./gradlew -Pandroidauto=true lint` |
| Flavors | `android/app/build.gradle` | `google`, `web`, `fdroid`, `huawei`; build types `debug`, `release`, `beta` |
| Android tests | `android/app/src/test/`, `android/sdk/src/test/` | JVM tests now include Street Pixels gates (Explorer Pro, GPX, recording UI, routing options). **Still no `androidTest` instrumented tests.** |
| Play listing | `android/app/src/google/play/listings/en-US/full-description.txt` | Upstream Streifzug copy; advertises GPX import/export; no Street Pixels session/competition disclosure |
| Privacy policy and terms | `HelpFragment` → `https://streifzug.app/privacy/` and `terms/` | No Street Pixels policy text in this repository |
| Privacy settings | `PrivacySettingsFragment` / `prefs_privacy.xml` | Search history + Play services. Spec §30 Street Pixels rows not present |
| Product analytics | routing / card / Pro uint64 helpers | Local count-only. No §32.1 / §32.3. No upload sink |
| Error and empty states | across the Android app | Partial (no-area, interruption, Avoid no-route). Not yet audited against spec §31 |

**Difference from the technical audit (2026-07-20) and the 2026-07-25
snapshot:** Phases 1–9 have landed session gating, rematch, areas, routing,
milestones, competition, and GPX gates. Instrumented tests are still absent.
Dedicated add-friend intent-filters were removed (SP-092 / **SPD-085**);
leftover URIs are swallowed. Streifzug store/privacy URLs are unchanged.
ABL is still absent (SP-012 Pixel 3a without it). H1–H10 are **Accepted**
2026-08-29 as **SPD-077–086**. Brand writing and on-device testing are
residual.

**SP-096 re-verification (2026-08-29):** friends dedicated add-friend
intent-filters are gone (SP-092 / **SPD-085**); ABL still absent
(**SPD-082**). §22/§26 close-out:
[`notes/SP-096-risk-register-close-out.md`](../notes/SP-096-risk-register-close-out.md)
(independent review same day: 19/19 §22 rows vs tree; full
`street_pixels_tests` 499/499 after generated classificator; not
Accepted). Signed APK residual (secrets absent; Gradle
`secure.properties.release` vs Forgejo `secure.properties`).
Competition backend not in the explorer checkout present here (§26 #5
residual Ops). Brand listing residual (**SPD-084**). Do **not** mark
Phase 10 exit met.

**SP-097 mapping recorded (2026-08-29):** every spec §34 bullet and
every Phase 10 exit criterion 1–11 is mapped to pass / fail /
residual in
[`validation/SP-097-evidence-log.md`](../validation/SP-097-evidence-log.md)
(plan:
[`validation/SP-097-validation-plan.md`](../validation/SP-097-validation-plan.md)).
H10 executed on SHA `c9336737a3e085275e7806317774c98ea2808542`:
`street_pixels_tests` 499/499; payload-shape 1/1; official smoke
exit 1 (missing `World.mwm` + missing `platform_tests`); lint 5
errors / 24 warnings; clang-format-18 cannot parse the repo style
file. Device Residual. Brand Residual. Not Accepted. Do **not** mark
Phase 10 exit met.

## Intended outcome

- Every product spec §34 line item verified, with the evidence written down and
  attributable to a person, a device, a build, and a date.
- Store listing, data-safety disclosure, permission rationales, privacy policy,
  and competition consent text that match what the software actually does.
- Battery and rendering behaviour measured over realistic sessions.
- Every risk in the audit register closed out with a stated position.

Product-owner lock 2026-08-29 residualised brand writing and on-device
execution, so several of these outcomes cannot close in the Phase 10
coding slice (see Exit criteria). Do **not** mark Phase 10 exit met.

## Dependencies

**Depends on:** Phases 1–9 at their exit criteria before **implementation**
(SP-089+). Phase 11 is **not** a prerequisite. Partial implementation
entry produces false confidence.

SP-088 is **Accepted** 2026-08-29 (SPD-077–086). Brand writing and
on-device testing are residual. Coding SP-089+ still waits on other
phases at exit.

## Carried residuals from earlier phases

These do not block earlier phase exits. H7 / **SPD-083** classifies each
row. Phase 10 coding closes Fix (SP-089), local Follow-H5 (SP-091), and
Ops docs (SP-096). Device-verify *execution*, H2 measurement execution,
and brand writing are residual (product-owner lock 2026-08-29). Full
inventory:
[`notes/SP-088-launch-governance-architecture.md`](../notes/SP-088-launch-governance-architecture.md).

| From | Residual | Source | H7 class (SPD-083) |
| --- | --- | --- | --- |
| Phase 2 | Aggressive-OEM screen-off / background sample continuity (exit #7 partial; Pixel 3a Phase 2 D1 only — does **not** close Phase 10 D2 or fill SP-095 D1 cells) | SP-014 | Device-verify (SP-095; **execution residual**) |
| Phase 3 | Maintainer device walks (Pixel 3a / Uusimaa-scale reconciliation UX); rematch timing | SP-022 | Device-verify (SP-095; **execution residual**) + Measure (Uusimaa timing on the same walks; **execution residual**) |
| Phase 4 | R3 device walks: Helsinki UX, rural/coastal, no MWM-id as neighbourhood name in UI | SP-031 | Device-verify (SP-095; **execution residual**) |
| Phase 4 | R1 (narrowed): production mapgen collectors → `.spa` / CDN shipping | SP-042–048 **Accepted** 2026-08-08 (**SPD-033**); Option A remains residual | **Not Phase 10** |
| Phase 4 | Independent MWM+`.spa` generate + own HTTP origin | Phase 11 SP-098 **Accepted** (**SPD-087–096**); SP-099–104 Planned | **Not Phase 10** (parallel ops) |
| Phase 4 | LAN/CDN publish mirror; S2–S8 device download | SP-049–053 | Device *enabler* for Helsinki; not a Phase 10 feature |
| Phase 4 | Incomplete-`.spa` Android chrome | SP-048 | Fix (SP-089) |
| Phase 5 | Quantitative Spike 1 FPS/memory (Pixel 3a qualitative OK; numbers deferred) | SP-033 / SP-041 R2 | Measure (SP-094; **protocol documented** 2026-08-29; **execution residual**) |
| Phase 5 | Device Helsinki walks: badge/focus/tap/city zoom/completed chrome/§31 empty/no country-world UI | SP-041 R1 — needs `.spa` via download (SP-053) | Device-verify (SP-095; **execution residual**) |
| Phase 5 | Completed check glyph not drawn (`m_showCheck` reserved; outline+fill shipped) | SP-040 / SP-041 R3 | Fix (SP-089) |
| Phase 5 | Overlay neighbourhood-baked push retune | SP-041 R4 | Accept/waive |
| Phase 6 | Spike 7 city-scale / device; all routing device walks | SP-054 / SP-061 | Measure (SP-054 Spike 7; H7 Measure, **not** SP-095 Device-verify; **execution residual**) + Device-verify (SP-061 I* → SP-095; **execution residual**) |
| Phase 6 | GPS off-route Prefer dialog not shown (`nullptr` removeRouteCallback) | SP-061 R3 | Fix (SP-089) |
| Phase 6 | Routing analytics upload | SP-060 / SPD-044 | Follow H5 (SP-091; **SPD-081** stay local, no sink) |
| Phase 6 | No in-app debug readout of counters | SP-061 R5 | Accept/waive |
| Phase 7 | Device celebration, card, share, haptics, nav | SP-069 | Device-verify (SP-095; **execution residual**) |
| Phase 7 | Date checkbox vs SPD-056; 4 s PNG lifetime; `onResume` counter | SP-069 | Fix (SP-089) |
| Phase 7 | Growth-counter upload | SPD-055 | Follow H5 (SP-091; **SPD-081** stay local, no sink) |
| Phase 8 | Device opt-in, traffic capture, opt-out, queue, N&lt;3, delete | SP-079 | Device-verify (SP-095; **execution residual**) |
| Phase 8 | Weekly GET not JNI-wired; recency rows survive revoke | SP-079 / SP-072 | Fix (SP-089) |
| Phase 8 | Postgres production deploy; exact EU region string | SP-075 / SPD-062 | Ops (SP-096 close-out recorded 2026-08-29; **residual** — explorer checkout is SQLite default, no `prod.py`; EU string unverified) |
| Phase 8 | Failed POST `/leave` no retry; HTTP 409 mapping; 7-day gates after admin reset | SP-077 | Accept (SPD-083; not SP-089 Fix, not SP-096 Ops) |
| Phase 9 | Device GPX / public APK / share-sheet / internal Pro | SP-087 | Device-verify (SP-095; **execution residual**) |
| Phase 9 | Monetisation analytics upload | SPD-075 | Follow H5 (SP-091; **SPD-081** stay local, no sink) |
| Phase 9 | Qt ungated; reload no-paint; multi-cat KMZ; FromLatLon; system expat | SP-087 | Accept (not Android V1) |

## Work-item breakdown

Work-item planning 2026-08-29. Locks H1–H10 live in
[`SP-088`](../work-items/SP-088-launch-governance-decisions.md) as
**Accepted SPD-077–086** (product-owner lock 2026-08-29; brand writing
and on-device testing residualised). Coding SP-089+ waits on other
phases at exit. Residual WIs are not coding items.

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-088](../work-items/SP-088-launch-governance-decisions.md) | Launch-governance decisions (**Accepted** 2026-08-29; SPD-077–086) |
| 2 | [SP-089](../work-items/SP-089-residual-defect-close-out.md) | Locked residual defect close-out (**SPD-083** Fix list; not brand, not device) |
| 3 | [SP-090](../work-items/SP-090-settings-empty-states-first-run.md) | Settings, empty-state, and first-run audit (§30/§31/§10 except privacy/terms URLs and app-name rebrand; hide public friend settings **SPD-085**) |
| 4 | [SP-091](../work-items/SP-091-product-analytics-reconciliation.md) | Product analytics reconciliation (local §32; **SPD-081** no sink) |
| 5 | [SP-092](../work-items/SP-092-permissions-manifest-store-disclosures.md) | Permissions, manifest, and store disclosures (hide friends **SPD-085**; ABL absent **SPD-082**; listing brand residual) |
| 6 | [SP-093](../work-items/SP-093-privacy-policy-terms-consent.md) | Privacy policy, terms, and consent alignment (**Residual** — slice close-out 2026-08-29; **SPD-080** landing remains open; not Accepted) |
| 7 | [SP-094](../work-items/SP-094-battery-rendering-lifecycle.md) | Battery, rendering, and lifecycle measurement (**protocol documented** 2026-08-29; **device execution Residual**; **SPD-078**; not Accepted) |
| 8 | [SP-095](../work-items/SP-095-device-matrix-residual-close-out.md) | Device-matrix residual close-out (**roster documented** 2026-08-29; **device execution Residual**; **SPD-077**/**SPD-083**; not Accepted) |
| 9 | [SP-096](../work-items/SP-096-risk-register-and-release-pipeline.md) | Risk-register close-out and release pipeline (**close-out recorded** 2026-08-29; [note](../notes/SP-096-risk-register-close-out.md); signed APK/ops residual; brand listing residual; §26 #5 residual Ops; not Accepted) |
| 10 | [SP-097](../work-items/SP-097-phase10-launch-requirement-verification.md) | Phase 10 / §34 verification (**mapping recorded** 2026-08-29; automated + evidence; device/manual Residual; **not Accepted**; exit **not met**) |

Gate: SP-088 has locked H1–H10. This phase still **adds no features** beyond
defects H7 classifies as Fix, disclosure text that is not brand, and
verification that is not on-device. Do **not** mark Phase 10 exit met.

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
- Executable lifecycle protocol (L1–L9):
  [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md)
  (documented 2026-08-29; **not executed** in this coding slice).

## Privacy and security implications

The final privacy gate. What must be true before release:

- No raw GPS data is uploaded, from any code path, in any build configuration.
- Analytics contain no location values, no screenshots, and no view
  hierarchies.
- The privacy policy accurately describes what stays local and what is
  uploaded. **Landing** that policy text is residual (**SPD-080**;
  SP-093 residual); `https://streifzug.app/privacy/` may stay for now.
- The competition consent text matches actual upload behaviour item by item.
- Store permission declarations and background-location disclosure are
  accurate. ABL was **not** added in Phase 2 (SP-012). **SPD-082** keeps it
  absent. D2 measurement execution is residual, so the exception path
  cannot fire in Phase 10 coding. If a later SPD adds ABL, the Play
  Console background-location declaration and its justification
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
`.github/workflows/` has no C++ test job. **SPD-086:** the recorded local
run is the V1 gate; narrowing exclusions is not a Phase 10 coding task.

## Manual validation strategy

This phase is mostly manual, structured as an evidence log rather than a
walkthrough. Product-owner lock 2026-08-29 residualised **on-device
execution**. The scripts remain defined; do **not** execute hardware walks
in SP-089–097.

On-device scripts (residual until a later WI executes them):

- Every spec §34 line item, recording: who, which device, which OS
  version, which build, what was observed.
- Every spec §31 error and empty state deliberately, including denied
  location, denied background location, no downloaded map, poor GPS accuracy,
  interrupted recording, no exploration area, no local competitors, no
  connectivity, and an impossible avoid-explored route.
- Multi-hour recording session with battery measurement, compared against a
  control session with recording off. Executable protocol:
  [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md)
  (documented 2026-08-29; **not executed** in this coding slice).
- Screen-off recording on the full device matrix (OEM *functional*
  continuity remains SP-095 residual). Device-verify roster:
  [`validation/SP-095-validation-plan.md`](../validation/SP-095-validation-plan.md)
  (documented 2026-08-29; **not executed** in this coding slice).
- Cold start with a large city loaded, measured to first interactive frame
  (SP-094 CS1; recorded, not gated).
- Fresh-install first-run journey following spec §10 step by step.
- Upgrade from a prior build with existing exploration data (SP-094 L1).
- Offline-only usage for a full session including routing.

## Entry criteria

- Every other **of Phases 1–9** has met its exit criteria (required for
  SP-089+; not for SP-088). Phase 11 is not required.
- No open work item is in progress (required for SP-089+).
- A release-configured build exists and is installable (required for
  SP-094–097 **device execution**. Not required to record the SP-094
  protocol while that execution remains residual).

## Exit criteria

These remain the phase exit bar. Product-owner lock 2026-08-29 residualised
brand writing and on-device execution, so several items cannot be closed in
this coding slice. **Do not mark Phase 10 exit met.**

1. Every product spec §34 line item is verified with recorded evidence.
   Device/manual hardware observations residual (SP-095 / SP-097 device).
2. Every spec §31 error and empty state is implemented and observed.
   Device observation residual; SP-090 implements copy/actions except
   privacy/terms URL rows and app-name rebrand.
3. Settings match spec §30, with no radius or internal-parameter exposure.
   Privacy-policy/terms URL rows and app-name rebrand residual.
4. Analytics match spec §32 and contain no location data.
   Local counters + payload-shape in SP-091; no upload sink (**SPD-081**).
5. Privacy policy, terms, consent text, and store disclosures match actual
   behaviour. Policy/terms landing and listing brand copy residual
   (**SPD-080**, **SPD-084**; SP-093 residual).
6. Battery consumption during active recording is measured and accepted.
   Protocol documented SP-094 / **SPD-078** (no %/hour ceiling);
   measurement execution residual.
7. Rendering performance on the release build meets the recorded criteria.
   Spike 1 bar locked; protocol documented SP-094; measurement execution
   residual.
8. No critical exploration-data-loss path exists across the tested lifecycle
   events. Device lifecycle walks residual (SP-094 protocol documented;
   not executed).
9. No known path reveals another user's live or exact location.
10. Every audit risk has a stated final position (SP-096 docs table).
11. Store build signing works and the release pipeline produces an installable
    artefact. Signed APK/ops may residual.

## Explicit non-goals

- Any new feature.
- iOS release preparation of any kind.
- Explorer Pro purchasing enablement.
- Post-V1 candidates from spec §35.
- Performance work beyond meeting the stated criteria.
- Refactoring.
- Marketing assets and campaign material, which are not code and are not gated
  here.
- Independent map generation and Streifzug-free map origin (Phase 11).

## Known uncertainties

H1–H10 are **locked** 2026-08-29 via
[`SP-088`](../work-items/SP-088-launch-governance-decisions.md) as
**SPD-077–086**. **OQ-30–OQ-39** are closed. Brand writing and on-device
testing remain residual (not later Phase 10 coding items).

| Ref | Question | Accepted lock | Residual |
| --- | --- | --- | --- |
| H1 | Device matrix | D1 Pixel-class + D2 one aggressive OEM (**SPD-077**) | Execution residual |
| H2 | Battery / rendering bars | Spike 1 unchanged; battery protocol now, numeric ceiling after measurement or waiver (**SPD-078**) | Protocol documented (SP-094); measurement execution residual |
| H3 | Store flavors | Google Play is the V1 gate; F-Droid same artefact optional; Huawei/web not a gate (**SPD-079**) | Listing brand copy residual |
| H4 | Privacy policy / terms | Product-owned Street Pixels text (**SPD-080**) | Landing text/URLs residual; SP-093 residual |
| H5 | Analytics upload | No new public sink; local uint64 only (**SPD-081**); closes SPD-044/055/075 upload residual | SP-091 local counters; no sink |
| H6 | `ACCESS_BACKGROUND_LOCATION` | Keep absent (**SPD-082**) | D2 exception path residual |
| H7 | Residual disposition | Fix / Measure / Device-verify / Ops / Follow H5 / Accept table in the note (**SPD-083**) | Device-verify execution residual |
| H8 | Release workflows | Reuse machinery (**SPD-084**) | Application name, listing copy, privacy/terms URLs residual |
| H9 | Friends in public APK | Operationalize SPD-061 (hide UI and add-friend filters) (**SPD-085**) | **Done in SP-092** (Accepted 2026-08-29); leftover URI swallow; device eyeball residual (SP-095) |
| H10 | C++ CI exclusions | Not a launch blocker; recorded local suites are the V1 gate (**SPD-086**) | Device/manual §34 residual |

Friends *presence* in V1 is already **SPD-061** (hidden). H9 is how far the
public APK strips the leftover surface.

Play Store review of an ABL justification (only if a later SPD adds ABL)
remains outside the team’s control.
