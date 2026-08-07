# Phase 10 — Android release hardening

**Status:** Not started
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

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Android manifest | `android/app/src/main/AndroidManifest.xml` | Location permissions present; `ACCESS_BACKGROUND_LOCATION` absent; foreground service types `location` for `NavigationService` and `TrackRecordingService`, `dataSync` for `DownloaderService`; add-friend deep links registered |
| Store credentials | `docs/CREDENTIALS.md` | Documents the CI secrets required for signed store builds |
| Release workflows | `.forgejo/workflows/android-release.yaml`, `android-beta.yaml`, `android-check-metadata.yaml`, `android-release-metadata.yaml` | Present; upstream CoMaps release machinery |
| Android lint | `.github/workflows/android-check.yaml` | `./gradlew -Pandroidauto=true lint` |
| Flavors | `android/app/build.gradle` | `google`, `web`, `fdroid`, `huawei`; build types `debug`, `release`, `beta` |
| Android tests | `android/app/src/test/`, `android/sdk/src/test/` | Three JVM unit-test files total. **No `androidTest` instrumented tests anywhere.** |
| Error and empty states | across the Android app | Not yet audited against spec §31 |
| Privacy policy and terms | — | Not located in this repository |

**Difference from the technical audit:** the audit did not note that there are
no instrumented Android tests at all, and that Android unit-test coverage is
three files. Any Android-side verification in this phase is manual unless
instrumentation is added.

## Intended outcome

- Every product spec §34 line item verified, with the evidence written down and
  attributable to a person, a device, a build, and a date.
- Store listing, data-safety disclosure, permission rationales, privacy policy,
  and competition consent text that match what the software actually does.
- Battery and rendering behaviour measured over realistic sessions.
- Every risk in the audit register closed out with a stated position.

## Dependencies

All other phases at their exit criteria. This phase cannot start early, and
partial entry produces false confidence.

## Carried residuals from earlier phases

These do not block earlier phase exits. Phase 10 must close them with recorded
device evidence (or an explicit accepted waiver).

| From | Residual | Source |
| --- | --- | --- |
| Phase 2 | Aggressive-OEM screen-off / background sample continuity (exit #7 partial; Pixel 3a done) | SP-014 |
| Phase 3 | Maintainer device walks (Pixel 3a / Uusimaa-scale reconciliation UX) | Phase 3 exit |
| Phase 4 | R3 device walks: Helsinki UX, rural/coastal, no MWM-id as neighbourhood name in UI | SP-031 |
| Phase 4 | R1 (narrowed): production mapgen collectors → `.spa` still unwired | Pre-production follow-up; not a Phase 10 device item |
| Phase 5 | Possible mid-tier Android Spike 1 / rendering device residual if SP-033 uses desktop secondary | SP-033 (when recorded) |

## Proposed work-item breakdown

Not yet decomposed. Likely shape:

1. Launch-requirement verification pass over spec §34, producing an evidence
   log.
2. Error and empty-state audit against spec §31.
3. Settings audit against spec §30.
4. Analytics audit: every event reviewed for location content, and the §32 list
   reconciled with what is implemented.
5. Permission, manifest, and store data-safety disclosure alignment.
6. Privacy policy, terms, and competition consent text alignment.
7. Battery measurement during active recording.
8. Rendering performance re-measurement on the release build.
9. Device matrix pass, including at least one aggressive-OEM device.
10. Data-loss and crash-recovery pass.
11. Risk-register close-out.

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
  accurate. If `ACCESS_BACKGROUND_LOCATION` was added in Phase 2, the Play
  Console background-location declaration and its justification video must
  match the session-based behaviour.
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
`.github/workflows/` has no C++ test job. Before release, either the exclusions
are narrowed so Street Pixels-relevant suites actually run, or the manual test
run is part of the release checklist with recorded output.

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

- Every other phase has met its exit criteria.
- No open work item is in progress.
- A release-configured build exists and is installable.

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

- Whether battery consumption during multi-hour recording is acceptable, and
  what "acceptable" is numerically. The spec does not set a threshold; one must
  be agreed before this phase can pass its own criterion.
- Whether Play Store review accepts the background-location justification. This
  is outside the team's control and may require copy or behaviour changes late.
- Which device matrix is sufficient. The audit suggests a Pixel-class device
  plus at least one aggressive OEM.
- Where the privacy policy and terms live and who owns their text.
- Whether the upstream CoMaps release workflows can be reused as-is for a fork
  with a different application identity and store listing.
- Whether the friends feature is present in the released build (OQ-6), which
  changes the store listing, the privacy policy, and the moderation surface.
- Whether the F-Droid, Huawei, and Google flavors are all in scope for the
  first release, or only one.
