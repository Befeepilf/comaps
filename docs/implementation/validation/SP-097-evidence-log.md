# SP-097 — Evidence log (Phase 10 / §34 verification pack)

**Plan:** [SP-097-validation-plan.md](SP-097-validation-plan.md)
**Branch:** `cursor/sp-097-launch-verification-6383`
**Parent SHA (`street-pixels`):** `c9336737a3e085275e7806317774c98ea2808542`
  (`Merge branch 'cursor/sp-096-risk-register-6383'`)
**Date:** 2026-08-29
**Status:** Automated mapping recorded. Device Residual. Brand Residual.
  **Not Accepted.** Phase 10 exit **not met.**

Agent does not mark Phase 10 exit met. Do not copy Pixel 3a SP-014 as
Phase 10 **D2** close.

Logs: `/opt/cursor/artifacts/sp097_street_pixels_tests_full.log`,
`sp097_payload_shape.log`, `sp097_gradle_lint.log`,
`sp097_lint-results-debug.txt`, `sp097_smoke.log`,
`sp097_smoke_build.log`, `sp097_smoke_build_rest.log`,
`sp097_routing_search_tests.log`, `sp097_clang_format.log`.

---

## Build / automated baseline (executed this SHA)

| Field | Value |
| --- | --- |
| Date | 2026-08-29 |
| Client git SHA | `c9336737a3e085275e7806317774c98ea2808542` |
| Branch | `cursor/sp-097-launch-verification-6383` |
| `street_pixels_tests` binary | `/workspace/omim-build-debug/street_pixels_tests` mtime 2026-08-29 13:00 UTC (236517552 B) |
| `data/classificator.txt` | **Present** (`-rw-r--r--` 35452 B, 2026-08-29 14:20 UTC). Gitignored generated. Not committed. |
| `data/types.txt` | Present (35869 B, same timestamp) |
| Explorer | `/agent/repos/explorer` `main` `e13a124afefa97650e3a6d798f3b43d23d53b01a` — `core.Explorer` / `Friendship` only. No `competition/` app. No backend tests run. |
| `adb` / handset | **Not used.** |

### `street_pixels_tests` full suite

Executed. Eligibility **not** suppressed.

```text
$ git -C /workspace rev-parse HEAD
c9336737a3e085275e7806317774c98ea2808542

$ ls -la /workspace/data/classificator.txt
-rw-r--r-- 1 ubuntu ubuntu 35452 Aug 29 14:20 /workspace/data/classificator.txt

$ /workspace/omim-build-debug/street_pixels_tests \
    --data_path=/workspace/data --user_resource_path=/workspace/data
# grep -c '^OK$' → 499
# grep -c '^Running ' → 499
# Eligibility_IncludesCommonHighways … Eligibility_ParkingAisleAndBuswayStillIncluded each OK
All tests passed.
exit=0
```

Log: `/opt/cursor/artifacts/sp097_street_pixels_tests_full.log` (ended 14:42:33 UTC).

### Payload-shape filter

```text
$ /workspace/omim-build-debug/street_pixels_tests \
    --data_path=/workspace/data --user_resource_path=/workspace/data \
    --filter='ProductAnalytics_ReleaseUploadPayloadsHaveNoLocation'
Running product_analytics_tests.cpp::ProductAnalytics_ReleaseUploadPayloadsHaveNoLocation
OK
All tests passed.
exit=0
```

**1/1.** Log: `/opt/cursor/artifacts/sp097_payload_shape.log`.

### Smoke (`run_tests.sh -s smoke`)

Official command executed:

```text
./tools/unix/run_tests.sh -b /workspace/omim-build-debug -s smoke
```

`platform_tests` was **missing** (ninja compile failed: glaze `std::expected` /
Clang 18 vs `3party/glaze/include/glaze/util/expected.hpp`). Other smoke
binaries were built except that one. Official script then:

| Binary | Result (executed) |
| --- | --- |
| `base_tests` | **237/237** `All tests passed.` |
| `coding_tests` | **198/198** `All tests passed.` |
| `generator_tests` | **291/291** `All tests passed.` (`grep '^OK$'` = 290 because `Source_To_Element_create_from_o5m_test` prints `]OK` on the same line as the o5m dump; `Test took 2 ms` then the next test. Not a failure.) |
| `indexer_tests` | **Aborted.** `CitiesBoundaries_Compression` `FAILED` `Reader::OpenException` `/workspace/data/World.mwm` missing, then `ASSERT FAILED` abort. |
| `map_tests` | **Aborted** during `Bookmarks_Sorting` — `World.mwm` / `WorldCoasts.mwm` missing. |
| `mwm_tests` | **Aborted** on `ForEachFeatureID_Test` — same `World.mwm` missing. First test `Threading_ForEachFeature` OK. |
| `platform_tests` | **Not run.** `Can't find test platform_tests` → script `die`, exit 1. |
| `routing_tests` | **Not reached** by official smoke. Separate run below. |
| `search_tests` | **Not reached** by official smoke. Separate run below. |

Official smoke **exit=1**. World maps were not invented. Eligibility was not
skipped. Log: `/opt/cursor/artifacts/sp097_smoke.log`.

Separate runs after smoke died (same SHA, same data path):

| Binary | Result |
| --- | --- |
| `routing_tests` | **307/307** `All tests passed.` `routing_exit=0` |
| `search_tests` | **Aborted** at `LocalityFinderTest_Smoke` — `World.mwm` missing. 40 OK then abort. `search_exit=134` |

Log: `/opt/cursor/artifacts/sp097_routing_search_tests.log`.

`platform_tests` compile (ninja): `error: no template named 'expected' in namespace 'std'` in glaze. Log: `/opt/cursor/artifacts/sp097_smoke_build.log`. Not fixed in this WI.

### Android lint

```text
cd /workspace/android && ./gradlew -Pandroidauto=true lint --no-daemon
```

Configure printed `secure.properties.release doesn't exist` /
`secure.properties.test doesn't exist` (same as SP-096).

`:sdk:lintDebug` **FAILED**. **5 errors, 24 warnings.** App-module lint did
not finish (`abortOnError`). **BUILD FAILED** in 15s. `exit=1`.

Errors (from `lint-results-debug.txt`, last line `5 errors, 24 warnings`):

1. `Utils.java:371` `MissingPermission` `VIBRATE`
2. `Utils.java:375` `MissingPermission` `VIBRATE`
3. `Utils.java:406` `MissingPermission` `VIBRATE`
4. `Utils.java:412` `MissingPermission` `VIBRATE`
5. `RecordingSessionDebug.java:56` `WrongConstant`

Pre-existing vs SP-096. Not fixed here. Fail for the H10 lint-clean bar.
Triaged list only.

### clang-format

```text
./tools/unix/clang-format.sh
clang-format: Ubuntu clang-format version 18.1.3 (1ubuntu1)
```

CI (`.github/workflows/code-style-check.yaml`) installs **clang-format-20**.
This environment’s 18.1.3 cannot parse `.clang-format`
`AlignEscapedNewlines: LeftWithLastLine` → `Error reading /workspace/.clang-format: Invalid argument` on every file. Script **exit=123** (xargs). No C++/Java source reformat applied (config rejected). `git checkout -- .` after the run restored any tracked noise. **Residual (environment / tooling)** — CI uses clang-format-20; do not churn sources to satisfy 18. Not a product §34 Fail. Re-run with clang-format-20.

### Backend tests

**Not run.** Explorer `INSTALLED_APPS` has `core` only. No `competition/`.
Friends pytest is not a substitute (audit §26 #5 / SP-096).

---

## Public-build code confirmations (this SHA; not a device walk)

| Check | Evidence | Result |
| --- | --- | --- |
| No purchase | No `BillingClient` / Play Billing in `android/app` or `android/sdk` Java. **SPD-010**. | **Pass** |
| No ungated GPX | `explorer_pro.cpp` capabilities default `false`; `android/sdk/.../OrganicMaps.java` applies BuildConfig then `nativeFreezeExplorerProConfiguration()`. `GpxSettingsVisibility` requires enabled flags. `PublicSettingsVisibilityTest` (SP-092). | **Pass** |
| No friends surface | `FriendSettingsVisibility.friendsCapabilityEnabled()` returns `false`. Add-friend intent-filters absent (`PublicManifestAssertionsTest`). Leftover URIs swallowed. **SPD-085** / SP-092. | **Pass** (public APK). Explorer still has `Friendship`. Device eyeball residual SP-095. |
| No city allowlist | No runtime city/pilot allowlist. `spa_jsonl` Helsinki ids are a spot-check comment, not a gate. `country_policies.json` is versioned country config (FI fixture), not a city allowlist. **SPD-004**. | **Pass** |
| No known *client* path for another user’s live/exact location | Competition POST allow-list (payload-shape 1/1). `CompetitionCopyDeniedTokens` includes `live location` / `nearby`. Friends UI hidden. `ShouldAttemptStatsUpload()` false. | **Pass** (client). Server Residual Ops. |

---

## Device roster

| Slot | Status |
| --- | --- |
| D1 Pixel-class | **Residual.** Roster: SP-095. Not executed. Do not copy Pixel 3a as Phase 10 close. |
| D2 aggressive OEM | **Residual.** **SPD-077**. Not executed. Pixel 3a is not D2. |
| D3 optional | Unused. |

---

## §34 Core map and exploration

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| C1 | Eligible OSM route filtering | **Pass** | This SHA `Eligibility_*` 9 tests OK inside 499/499. Prior SP-020. |
| C2 | Street pixels generated deterministically | **Pass** | `StreetPixel_MaximalHealpixIdRoundTrip`, `PathSampling_*` in 499/499. Prior SP-008/011/019. |
| C3 | Red and green persist locally | **Pass** | Ever-live / rematch / `.pix` tests in 499/499. Prior SP-015–022. Device permanence residual SP-095. |
| C4 | 25-metre collection radius | **Pass** | `kExploreRadiusMeters = 25.0`. SP-008. Accuracy filter 25 m. |
| C5 | Imported vs live distinguishable | **Pass** | `EverLive_*` + `IsolationHistoricalImport_*` in 499/499. SP-016/081/082. |
| C6 | Area percentages for installed map version | **Pass** | `AreaCompletionManager_*` in 499/499. Prior SP-034/041. Device Helsinki residual SP-095. |
| C7 | Versioned country-specific admin config | **Pass** | `CountryConfig_*` in 499/499. Prior SP-025/031. `policy_version` / `schema_version` in `data/street_pixels/country_policies.json`. |
| C8 | Core personal exploration wherever compatible data | **Pass** | No city/pilot runtime restriction. Worldwide product. Device residual SP-095. |
| C9 | No city allowlist / pilot-only runtime | **Pass** | Code inspection this SHA. |

## §34 Recording

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| R1 | Start, pause, resume, finish | **Pass** | `RecordingSession_*` / `PauseResume_*` / `CollectionGate_*` in 499/499. Device residual SP-095. |
| R2 | Background where permission permits | **Residual** | SP-095 / **SPD-082** (ABL absent). Do not cite Pixel 3a as D2. |
| R3 | Screen off | **Residual** | SP-095. SP-094 Session A is not OEM close. |
| R4 | Session state clearly visible | **Residual** | Implemented SP-012/090. Device eyeball SP-095. |
| R5 | Interrupted sessions no false connecting lines | **Pass** | `InterruptedSession_*`, `SegmentInterpolation_Barrier_*` in 499/499. |
| R6 | Tracks inspect and delete locally | **Residual** | Track *save* is automated (`PauseResume_TrackBoundary_*` in 499/499). Inspect-and-delete is device UI. Official `map_tests` aborted at `Bookmarks_Sorting` (`World.mwm`); no executed delete/inspect walk this SHA. SP-095. Do not cite Pixel 3a as Phase 10 close. |

## §34 GPS integrity

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| G1 | Poor-accuracy samples rejected | **Pass** | `LiveSampleAcceptance_Accuracy26_Rejected`. `kMaxHorizontalAccuracyMeters = 25`. |
| G2 | Implausible speed rejected | **Pass** | `LiveSampleAcceptance_ImpliedSpeed55Kmh_Rejected`. 50 km/h. |
| G3 | Long jumps rejected | **Pass** | `LiveSampleAcceptance_Teleport_Rejected`. 200 m. |
| G4 | Normal valid samples interpolated safely | **Pass** | `SegmentInterpolation_WithinCaps_*`, walking/cycling sequences. |
| G5 | Signal loss does not paint a straight line | **Pass** | Barriers after rejection / interruption. |
| G6 | Pause and resume do not create connecting segments | **Pass** | `SegmentInterpolation_Barrier_AfterPause_*`, `PauseResume_TrackBoundary_*`. |

## §34 Progress experience

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| P1 | First-use guidance | **Residual** | SP-090 §10 script. Device click-through SP-095. |
| P2 | Area focus predictable | **Pass** | Focus-selection tests in 499/499. Prior SP-036/041. Device residual SP-095. |
| P3 | 25/50/100% milestones | **Pass** | `AreaMilestoneManager_*` / `AreaMilestonePresentation_*` in 499/499. Prior SP-065/069. Device residual SP-095. |
| P4 | Completed areas clear visual | **Pass** | SP-089 Accepted check glyph (`m_showCheck`). `area_overlay_tests` live in `street_pixels_areas_tests` (not in the 499 binary; not re-run this SHA). Device eyeball residual SP-095. |
| P5 | City-level aggregate progress | **Pass** | `FocusEngine_Manager_CitySummaryUsesRollupFraction` in 499/499. Dedicated `CityCompletion_*` tests are in `street_pixels_areas_tests` (not executed this SHA). Prior SP-039/041. Device residual SP-095. |
| P6 | No achievement-history screen required | **Pass** | Spec §35; none added. |

## §34 Routing

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| T1 | Standard routing works | **Pass** | This SHA `routing_tests` **307/307** All tests passed. Device residual SP-095. |
| T2 | Prefer-unexplored works | **Pass** | Prefer tests in `routing_tests` + 499/499. Prior SP-056/061. Device residual SP-095. |
| T3 | Avoid-explored handles impossible routes; no silent ignore | **Pass** | Avoid no-route + SP-089 Prefer control. Prior SP-057/058/089. Device residual SP-095. |

## §34 Privacy and competition

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| K1 | Competition is opt-in | **Pass** | `IdentityStore_*` in 499/499. Device residual SP-095. |
| K2 | Consent separate from location permission | **Pass** | `ExploreConsentDialogFragment` vs `track_recording_location_rationale`. SP-090. |
| K3 | No raw GPS uploaded | **Pass** | Payload-shape **1/1**. Allow-list has no lat/lon. `ShouldAttemptStatsUpload()` false. |
| K4 | Aggregate uploads delayed and batched | **Pass** | `kCompetitionMinUploadIntervalSeconds = 900`, jitter 900. SP-074 tests in 499/499. |
| K5 | Exact-location sharing absent | **Pass** | No feature. Copy deny-list. Payload-shape. |
| K6 | Nearby-user discovery absent | **Pass** | Friends capability off. Dedicated add-friend filters gone. Explorer `Friendship` remains; not public UI. |
| K7 | Pseudonymous identity creation | **Pass** | `IdentityStore_*` in 499/499. |
| K8 | Nickname restrictions and reporting | **Residual** | Client SP-077 tests in 499/499. Server **Ops** — no competition app (SP-096 §26 #5). |
| K9 | Rename within stated limits | **Residual** | Client tests. Server Ops residual. |
| K10 | Users can delete competition data | **Residual** | Client delete/recency-clear (SP-077/089) in 499/499. Server Ops residual. |
| K11 | Ownership calculations | **Pass** | `CompetitionOwnership_*` in 499/499. Prior SP-072. |
| K12 | Server-side decay | **Residual** | Ops. Explorer `main` has no `competition/` (SP-096). |
| K13 | Areas can become unclaimed | **Residual** | Client snapshot flags exist. Server Ops residual. |
| K14 | Sparse ranking states preserve privacy | **Residual** | Client boss-line hide is not protection. Server N&lt;3 unverified (SP-096). |
| K15 | Weekly city rankings exclude imports | **Pass** | `WeeklyCityLive_ImportOnlyDoesNot*` / isolation tests in 499/499. |

## §34 Offline and map updates

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| O1 | Manual map updates work | **Pass** | Rematch tests in 499/499. Prior SP-017/022. Device residual SP-095. |
| O2 | Users warned percentages may change | **Pass** | §27.3 fraction-drop signal. Prior SP-021. Device residual SP-095. |
| O3 | Statistics recalculated after updates | **Pass** | Rematch denominator tests. |
| O4 | Competition uploads include map-data versions | **Pass** | Payload-shape allow-list `map_data_version`. |
| O5 | Offline competition updates queue then sync | **Residual** | Client queue tests in 499/499. Sync to missing backend is Ops residual. Device residual SP-095. |

## §34 Sharing

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| S1 | Cards without routes / exact / home / individual timestamps | **Pass** | `CompletionCard_DenyListFieldsAbsent` and share deny-list in 499/499. Device residual SP-095. |
| S2 | Cards without a competition profile | **Pass** | `CompletionCard_ComposeWithoutNicknameIncludesDate`. |
| S3 | Share-card at 100% area completion | **Pass** | `CompletionCardShare_PrepareFailsWithoutHundredPercent`. Device residual SP-095. |

## §34 Explorer Pro and monetization

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| E1 | Feature gates / entitlement abstraction | **Pass** | `ExplorerPro_*` / `GpxGate_*` in 499/499. Prior SP-005/080–087. |
| E2 | Public builds no non-functional purchase | **Pass** | No Billing. Capabilities frozen off. |
| E3 | Imported tracks cannot affect competition regardless of flags | **Pass** | Isolation matrix in 499/499. Prior SP-082/087. |
| E4 | No Play Billing / purchase / restore | **Pass** | Code inspection. **SPD-010**. |

## §34 Release governance

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| L1 | Privacy policy describes local vs uploaded | **Residual** | SP-093 / **SPD-080**. Help: `app_site_url` + `privacy/` → `https://streifzug.app/privacy/`. |
| L2 | Competition consent text matches behaviour | **Residual** | SP-093 landing. In-app `explore_consent_message` snapshot in SP-093 WI. |
| L3 | Terms cover nicknames and rankings | **Residual** | SP-093 / **SPD-080**. Help `terms/`. |
| L4 | Competition-profile deletion operational | **Residual** | Client Pass (K10 tests). Server Ops residual SP-096 §26 #5. |
| L5 | Nickname moderation and administrative reset operational | **Residual** | Client SP-077. Server Ops residual. |
| L6 | Android store permissions and background-location disclosures accurate | **Pass** | SP-092 Accepted permission inventory + `play-data-safety.md` (ABL absent **SPD-082**; do not declare background location). Listing *brand* copy (app name, GPX advertised) is **not** this Pass — **SPD-084** residual / Exit 5. |
| L7 | Analytics contain no raw location data | **Pass** | Payload-shape **1/1**. Local uint64 only **SPD-081**. No sink. |

## §34 Quality

| ID | Bullet | Status | Pointer |
| --- | --- | --- | --- |
| Q1 | Rendering performance acceptable | **Residual** | SP-094 Spike 1 not executed. No FPS invented. |
| Q2 | Battery during active recording acceptable | **Residual** | SP-094 / **SPD-078**. No %/hour invented. |
| Q3 | Foreground haptics can be disabled | **Pass** | `ExplorationHaptic_Predicate_ToggleOff_Denies` and manager ToggleOff tests in 499/499. Device residual SP-095. |
| Q4 | No critical exploration-data loss | **Residual** | Automated rematch/interrupt Pass. Device lifecycle L1–L9 residual SP-094. |
| Q5 | No known path reveals another user’s live or exact location | **Residual** | Client **Pass** (table above). Server Residual Ops (no competition app to call). |

### §34 tally (this log)

69 spec bullets. **Pass 48. Residual 21. Fail 0** on the §34 rows
themselves. Residual IDs: R2, R3, R4, R6, P1, K8, K9, K10, K12, K13,
K14, O5, L1, L2, L3, L4, L5, Q1, Q2, Q4, Q5. H10 lint is **Fail**
(5 errors). Official smoke is **Fail** (exit 1; missing World.mwm +
missing `platform_tests`). clang-format-18 is **Residual**
(environment; config requires clang-format-20).

---

## Phase 10 exit criteria 1–11

Do **not** treat these as Phase 10 exit met.

| # | Criterion | Status | One-liner |
| --- | --- | --- | --- |
| 1 | Every §34 line verified with recorded evidence | **Residual** | Mapping filled; **21 Residual** bullets (device / brand / Ops), **48 Pass**, **0 Fail**. Not exit met. |
| 2 | Every §31 error/empty state implemented and observed | **Residual** | SP-090 implements copy/actions. Device observation SP-095. |
| 3 | Settings match §30; no radius/internal exposure | **Residual** | SP-090 §30 table. Privacy/terms URL rows and app-name residual **SPD-080** / **SPD-084**. |
| 4 | Analytics match §32 and contain no location | **Pass** | SP-091 Accepted. Payload-shape **1/1** this SHA. No sink **SPD-081**. |
| 5 | Privacy policy, terms, consent, store disclosures match behaviour | **Residual** | SP-093 / **SPD-080** landing. SP-092 factual data-safety Pass. Listing brand residual **SPD-084**. |
| 6 | Battery during recording measured and accepted | **Residual** | SP-094 protocol documented; not executed **SPD-078**. |
| 7 | Rendering on release build meets recorded criteria | **Residual** | Spike 1 protocol SP-094; not executed. |
| 8 | No critical exploration-data-loss across lifecycle | **Residual** | SP-094 L1–L9 not executed. |
| 9 | No known path reveals another user’s live/exact location | **Residual** | Client Pass this SHA. Server unverified (SP-096 §26 #5). |
| 10 | Every audit risk has a stated final position | **Pass** | SP-096 close-out 19/19 §22 rows. Not Accepted. Positions documented. |
| 11 | Store signing works; pipeline produces installable artefact | **Residual** | Secrets absent. No signed `google` APK. Unsigned is not exit #11. SP-096. |

---

## Contradictions (spec / audit / code)

1. **Audit §22 “confirmed now” ungated collection / wipe-on-update / empty formulas / `/stats/upload`** vs current tree: session gate, rematch, SPD formulas, competition aggregates URL. Code wins; SP-096 recorded mitigated. Spec LaTeX still empty (do not edit spec).
2. **README / SP-075–076 Accepted** on explorer SHAs that are **not** this explorer `main` (`e13a124`, friends-only). Do not treat those SHAs as present here.
3. **Play listing** still advertises GPX while public V1 gates GPX (**SPD-084** residual; not rewritten).
4. **Forgejo `secure.properties` vs Gradle `secure.properties.release`** (SP-096). Unchanged.
5. **H10 smoke** needs `World.mwm` / `WorldCoasts.mwm` which are not in this checkout; `platform_tests` does not compile under Clang 18 + glaze `std::expected`. Environment Fail, not a Street Pixels product defect in this WI.
6. **clang-format-18 vs CI clang-format-20** / `LeftWithLastLine`. Gate not re-run with 20. **Residual env**, not a source-format Fail.
7. **Phase 10 file “Status: Not started”** vs SP-088–096 work recorded. Status is not flipped to exit met.

---

## Independent review (2026-08-29)

Reviewer did not write the original mapping. Re-counted spec §34
(69 bullets, all present). Re-counted artifacts: `street_pixels_tests`
499/499; payload-shape 1/1; smoke exit 1; `routing_tests` 307/307;
lint 5 errors / 24 warnings; clang-format exit 123. Spec and audit
files were not edited. Accepted-by remains empty. Phase 10 status
remains Not started / exit not met.

Corrections in this review:

- **R6** Pass → Residual (inspect/delete is device UI; `map_tests` aborted).
- clang-format **Fail** → **Residual** (clang-format-18 vs CI 20).
- Tally **49/20** → **48/21** (Exit 1 already said 21; now matches).
- `generator_tests` `grep '^OK$'` 290 is `]OK` glued to o5m dump, not a failed test (291/291 All tests passed).
- P4/P5 pointers no longer attribute `street_pixels_areas_tests` to the 499 binary.

---

## Defects found in this item

None fixed in product code. Lint 5 errors, smoke World.mwm abort,
`platform_tests` glaze compile, clang-format-18 config, missing
competition backend, unsigned APK — recorded as follow-up. Tests not
weakened. Mapping honesty fixes only.
