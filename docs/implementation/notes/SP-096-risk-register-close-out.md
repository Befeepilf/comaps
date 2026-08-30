# SP-096 — Risk-register close-out and release pipeline

**Date:** 2026-08-29
**Branch:** `cursor/sp-096-risk-register-6383`
**Parent SHA (`street-pixels`):** `30eb240904d3a61aa5b36f4ddfa62e459d92352d`
  (`Merge branch 'cursor/sp-095-device-matrix-residual-6383'`)
**Audit date:** 2026-07-20 (`docs/street-pixels-technical-audit.md`). That
  document is a dated baseline, not current code. This note is the
  close-out. The audit file is not edited.
**Agent does not mark Accepted.**

Positions used below: `mitigated` / `accepted` / `realised` /
`n/a Android V1` / `residual`. **Realised** means the failure happened
(for example an OEM kill on D2). D2 execution is residual
(**SPD-077** / **SPD-083**). No OEM, battery, or renderer result is
invented. No signed-APK hash is invented. Play/F-Droid listing brand
copy and the application name are not rewritten (**SPD-084**).

---

## Method

Re-verified against this working tree on 2026-08-29. Audit §22 has
**19** rows; each has a code pointer or a measurement gap. “Confirmed
now” in the 2026-07-20 audit is not copied blindly.

Explorer checkout present at `/agent/repos/explorer` (`main`,
`e13a124`): friends-only historically. No `competition/` app. Do not
fake a competition schema.

---

## §22 Risk register (audit table, current tree)

| Audit risk | 2026-07-20 audit | Current evidence (this tree) | Position | Owner |
| --- | --- | --- | --- | --- |
| Renderer performance at city scale | Medium / High; circle-per-cell | `StreetPixelRenderer` still circle packs; LOD buckets at zoom 15, hidden below zoom 9 (`libs/drape_frontend/street_pixel_renderer.cpp` `kBucketZoomLevel` / `kMinVisibleZoomLevel`). Spike 1 bar locked (**SPD-078**). **No FPS/memory numbers in this slice.** Protocol: [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md). Evidence log empty. | **residual** (SP-094 Measure; **not realised**) | Device-verify / Measure (SP-094) |
| Battery during recording | High / High; continuous GPS + FGS | Session-gated collection (`StreetPixelsManager::OnLocationUpdate`). Location FGS: `TrackRecordingService` / `NavigationService`. Protocol documented; **no %/hour or mAh**. No ceiling invented (**SPD-078**). | **residual** (SP-094; **not realised**) | Device-verify / Measure (SP-094) |
| Android OEM background kills | High / High; industry-wide; FGS present | FGS present. `ACCESS_BACKGROUND_LOCATION` absent (`tools:node="remove"` in `android/app/src/main/AndroidManifest.xml`; **SPD-082**). SP-095 roster exists; **no D1/D2 handset run**. Pixel 3a SP-014 is prior D1-class citation only; it does not close Phase 10 D2. | **residual** (SP-095; **not realised**) | Device-verify (SP-095) |
| iOS Always permission / review | High / High | iOS is post-V1 (**SPD-002**). `NSLocationAlwaysUsageDescription` and `UIBackgroundModes` still exist in `iphone/` (Streifzug track-recording copy). Not a V1 launch risk. | **n/a Android V1** | Product (SPD-002) |
| False GPS exploration | High / Critical; no live filter; 20 m radius | 25 m radius (`kExploreRadiusMeters` in `libs/map/street_pixels_manager.cpp`). Live filter wired: `LiveSampleAcceptanceFilter` (accuracy 25 m, age 120 s, implied speed 50 km/h, jump 200 m) in `libs/map/live_sample_acceptance_filter.hpp`; `OnLocationUpdate` rejects then marks interpolation barrier. SP-009. | **mitigated** | Client (SP-009) |
| Ungated collection without recording | Confirmed now / Critical; `Framework::OnLocationUpdate` | `Framework::OnLocationUpdate` still forwards GPS to the manager. Gate is inside `StreetPixelsManager::OnLocationUpdate`: returns unless `m_recordingSession != nullptr && m_recordingSession->IsRecording()` (`libs/map/street_pixels_manager.cpp`). Tests: `libs/map/street_pixels_tests/collection_gate_tests.cpp`. SP-007. | **mitigated** | Client (SP-007) |
| Map update wipes progress | Confirmed now / Critical; `CleanupStreetPixels` | **Symbol still present**, not a wipe-on-update path. Download/update calls `RematchStreetPixelsOnMapUpdate` (`libs/map/framework.cpp`). Delete/deregister calls `CleanupStreetPixels`, which archives explored/ever-live into `.pixr` then removes `.pix` (SP-017 / SP-018). Rematch abort leaves previous pixels intact. | **mitigated** | Client (SP-017) |
| Admin boundary inconsistency | High / Critical; admin 7–11 deprecated | Phase 4 pipeline: `data/street_pixels/country_policies.json`, `.spa` format, client assignment, settlement fallback (SPD-020–025). Exit recorded SP-031. Production mapgen Option A residual (**SPD-033**, not Phase 10). LAN/CDN leaf shipping SP-049–053. Not city-allowlist. | **mitigated** (client/pipeline); production `.spa` shipping **residual** packaging | Client (Phase 4); Ops (CDN) |
| Database / file growth | Low–Medium / Medium; 8 B × N cells | `.pix` still packed `int64` entries: pixel id + explored + ever-live bits (`kExploredBit` / `kEverLiveBit` in `libs/map/street_pixels_file.hpp`). Sparse `.spx` beside `.pix`. No per-pixel SQL rows for the overlay universe. | **mitigated** | Client |
| Avoid-explored routing complexity | High / High; only soft multiplier | Hard avoid exists: `IStreetExplorationWeights::IsAvoidExclusionActive` / `IsSegmentExcluded` (`libs/routing/street_exploration_for_routing.hpp`). Prefer multiplier remains. Device walks residual (SP-061 → SP-095). | **mitigated** (code); device **residual** | Client (Phase 6); Device-verify |
| Client competition cheating | High / Medium; device auth only | Device-id auth (`IdentityStore::GetOrCreateDeviceId`). Client cadence 15 min + jitter (`CompetitionUploadService`). Server clamps were specified in SP-075; **this explorer checkout has no competition app to re-verify**. V1 accepts residual cheating (spec §6; audit). No new anti-cheat. | **accepted** | Product / audit |
| Sparse-area privacy leaks | Medium / High; spec N&lt;3 | Boss-line chrome withholds other nicknames when `participantCount < 3` (`ComposeSparseBossLine` in `libs/street_pixels_areas/competition_presentation.cpp`). Ranking rows are **not** nickname-stripped (`DedupeRankingRows`). Spec requires **server-side** enforcement (SP-076). Explorer `main` here: `Explorer` + `Friendship` only; no `competition/` app; no live N&lt;3 API to call. Client hide is not protection. | **residual** (Ops; unverified server) | Ops (SP-076 deploy) |
| Sentry PII / screenshots | Confirmed / High; Manifest meta-data | `io.sentry.send-default-pii=false`, `attach-screenshot=false`, `attach-view-hierarchy=false` in `android/app/src/main/AndroidManifest.xml`. Guard: `tools/unix/check_sentry_privacy.sh` (`.github/workflows/android-check.yaml` `sentry-privacy` job). SP-003. | **mitigated** | Client (SP-003) |
| Friends vs V1 non-goals | Confirmed / Medium; backend friends API | Public UI hidden (`FriendSettingsVisibility.friendsCapabilityEnabled()` returns false). Dedicated `add-friend` intent-filters removed; leftover URIs swallowed (`ExploreDeepLink.shouldPresentAddFriendOnboarding`). Code may stay in-tree (**SPD-085** / SP-092). Explorer still has friends endpoints. Device eyeball residual (SP-095). | **mitigated** (public APK surface) | Client (SP-092) |
| Upstream Streifzug divergence | High / High; deep forks | Street Pixels modules sit beside map/routing/UI. Release machinery still Streifzug-shaped (**SPD-084**). Maintenance cost remains. Not a coding close in this item. | **accepted** | Maintainers |
| Licensing / attribution | Low / Medium; Apache-2.0 | `LICENSE` Apache-2.0. `NOTICE` lists Streifzug / Organic Maps / My.com and `3party`. Fork OK. | **mitigated** | Maintainers |
| Cross-platform parity | High / Critical; iOS UI missing | Android V1 only (**SPD-001** / **SPD-002**). Shared C++ remains in `libs/`. iOS Street Pixels UI is not a V1 gate. | **n/a Android V1** | Product (SPD-002) |
| Map pipeline maintenance | Medium / High; eligibility may need generator | Client eligibility (SP-020). On-device derive from MWM plus `.spa` sidecar. Generator precompute optional; Option A mapgen residual (**SPD-033**). | **mitigated** (client-first) | Client (SP-020); Ops (mapgen) |
| Spec formula gaps | Confirmed / Medium; empty LaTeX §7/22 | Spec file **not edited**. V1 implements **SPD-026** (personal completion), **SPD-057** (ownership score), **SPD-058** (contested 80%) in `libs/street_pixels_areas/ownership_scoring.hpp`. | **mitigated** (V1 via SPDs) | Product (SPD-026/057/058) |

### Related close-outs (not separate §22 rows; Phase 10 locks)

| Topic | Evidence | Position | Owner |
| --- | --- | --- | --- |
| Product-analytics upload | Local uint64 only (`libs/map/product_analytics.cpp`, routing / card / Pro helpers). No new sink (**SPD-081** / SP-091). `ExploreStatsService::ShouldAttemptStatsUpload()` returns `false`. Competition POST is spec §25.2 aggregates, not analytics. | **mitigated** (stay local) | Client (SPD-081) |
| `ACCESS_BACKGROUND_LOCATION` | Absent; `tools:node="remove"` (**SPD-082** / SP-092). D2 exception path cannot fire until device execution exists. | **mitigated** (keep absent) | Client (SPD-082) |
| Application name / Play listing brand | `project.ext.appId = 'app.comaps'`; `project.ext.appName = 'Streifzug'`. Listing title `Streifzug - Navigate with Privacy`. GPX advertised in `android/app/src/google/play/listings/en-US/full-description.txt`. **Not rewritten** (**SPD-084**). | **residual** (brand) | Product (SPD-084) |

No §22 row is **realised**.

---

## §26 Go/no-go mapping (seven launch-blocking conditions)

Audit §26 “Go with major conditions.” Mapped 2026-08-29 against this
tree. Any **still-open** row blocks SP-097 exit for that condition.

| # | Audit condition | Current evidence | Position | Blocks SP-097? |
| --- | --- | --- | --- | --- |
| 1 | Pixel collection must require an explicit recording session | Gate inside `StreetPixelsManager::OnLocationUpdate` (SP-007). Pause is not `IsRecording()`. | **closed (mitigated)** | No |
| 2 | Map updates must rematch explored cells instead of deleting progress | `Framework` download path → `RematchStreetPixelsOnMapUpdate` (SP-017). `CleanupStreetPixels` is delete-archive, not update-wipe. | **closed (mitigated)** | No |
| 3 | Fund admin-polygon pipeline **or** narrow competition geography | Phase 4 pipeline in-tree (policies, `.spa`, assignment, settlement fallback). Worldwide product; not city-only. Production CDN `.spa` shipping remains packaging residual (SP-049–053), not a missing pipeline. | **closed (mitigated)** at client/pipeline; CDN **residual** | CDN not a §26 architecture miss; packaging still Ops |
| 4 | Live GPS validation must approach spec defaults | `LiveSampleAcceptanceFilter` + 25 m radius (SP-008 / SP-009). | **closed (mitigated)** | No |
| 5 | Competition backend must be built (friends API is not a substitute); `/stats/upload` gap closed with the correct schema | **Client:** `GetCompetitionAggregatesUrl()` → `{apiBase}/v1/competition/aggregates` (`libs/map/backend_config.cpp`). Tests assert this is not `/stats/upload`. `ExploreStatsService` upload attempt is hard-off. **Explorer checkout (`/agent/repos/explorer`, `main` `e13a124`):** `core.Explorer` / `Friendship` only; `apis/api.py` account + friends; `INSTALLED_APPS` has `core`, no `competition`; settings default `sqlite:///…/db.sqlite3`; **no `prod.py`**. SP-075/SP-076 were Accepted on other explorer SHAs; **this checkout does not contain that app.** Do not invent a schema. | **still-open (residual Ops)** | **Yes** — competition backend not verifiable here |
| 6 | iOS parity plan if iOS is in V1 scope | iOS is not in Android V1 (**SPD-002**). | **n/a Android V1** | No |
| 7 | Privacy telemetry (Sentry PII/screenshots) in line with private-by-default | Manifest defaults + `check_sentry_privacy.sh` (SP-003). | **closed (mitigated)** | No |

**SP-097 blocker from this item:** condition **5** (competition backend
not present in the explorer tree this environment can inspect; no
production deploy verified). Signed APK (exit #11) is a separate
residual below.

---

## Release pipeline (H3 / **SPD-079**, H8 / **SPD-084**)

Reuse Streifzug machinery. Do not rewrite application name or Play/F-Droid
listing brand copy.

### Identities as configured (this tree)

| Item | Value | Source |
| --- | --- | --- |
| Base `applicationId` | `app.comaps` | `android/app/build.gradle` `project.ext.appId` |
| Application name | `Streifzug` | `project.ext.appName`; release `resValue app_name` |
| V1 store gate flavor | `google` | **SPD-079** |
| `google` applicationId | `app.comaps.google` (`applicationIdSuffix '.google'`) | same file |
| Other flavors | `web` (no suffix), `fdroid` → `.fdroid`, `huawei` → `.huawei` | same; Huawei/web **not** V1 gates |
| Build types | `debug` (`.debug` + debug keystore), `release`, `beta` (`.test`) | same |
| Debug signing | `android/app/comaps-debug.keystore`, alias `Streifzug Debug` | `signingConfigs.debug` |
| Release signing | `secure.properties.release` → `signingConfigs.release` | Gradle |
| Beta/test signing | `secure.properties.test` → `signingConfigs.test` | Gradle |
| Play publish task | `./gradlew bundleGoogleRelease publishGoogleReleaseBundle` | `.forgejo/workflows/android-release.yaml` |
| Secret names | `PRIVATE_H`, `RELEASE_KEYSTORE`, `SECURE_PROPERTIES`, `GOOGLE_PLAY_JSON`, … | `docs/CREDENTIALS.md` |
| Release API base (Android release/beta) | `https://api.streifzug.app/api` via `BuildConfig.EXPLORE_API_BASE_URL` | `android/sdk/build.gradle`; applied in `OrganicMaps.java` |
| Unconfigured C++ default | empty; fail-closed (`backend::GetApiBaseUrl()`) | SP-004; **SPD-062** |

Forgejo `android-release.yaml` matrix includes `google` and `web`
(`huawei` commented). **SPD-079:** Google Play `google` is the V1 gate;
F-Droid may ship the same artefact; Huawei/web are not gates.

### Signed APK residual

This environment **does not** contain:

- `android/app/release.keystore`
- `android/app/secure.properties`
- `android/app/secure.properties.release`
- `android/app/secure.properties.test`
- `android/app/google-play.json`

`private.h` is gitignored (workspace root may have a local copy; it is
not committed). Forgejo production secrets are not available here.

**Position:** signed installable `google` release/beta artefact is
**residual Ops**. Reason: signing secrets and Play JSON absent. An
unsigned or debug-signed APK is **not** exit #11. No artefact hash is
recorded.

### Workflow vs Gradle filename mismatch (contradiction)

| Step | Path |
| --- | --- |
| Forgejo restore | writes `android/app/secure.properties` and `android/app/release.keystore` |
| `docs/CREDENTIALS.md` | documents those same paths for `gh secret set` |
| `android/app/build.gradle` | applies `secure.properties.release` / `secure.properties.test` only |

If CI secrets were restored exactly as the workflow is written, Gradle
would still print `NO RELEASE signing keys found` and fall back to the
**debug** keystore unless ops also materialises `secure.properties.release`
(and points `secretReleaseStoreFile` at the restored keystore). That is
an ops/workflow residual, not fixed in this docs item.

Root `.gitignore` lists the old `android/release.keystore` /
`android/secure.properties` paths (comment: transition).
`android/app/.gitignore` **does** list `secure.properties.release`,
`secure.properties.test`, `google-play.json`, and
`comaps-release.keystore`. It does **not** list the Forgejo restore
filename `release.keystore` (only `comaps-release.keystore`). Do not
add secret files to git.

Listing title and full description remain upstream Streifzug (advertises
GPX import/export; no Street Pixels session/competition copy). **SPD-084**
residual. Factual data-safety answers: `docs/implementation/play-data-safety.md`
(SP-092); not listing brand.

---

## Backend ops checklist

| Check | Evidence in this environment | Position |
| --- | --- | --- |
| Production settings not SQLite | Explorer `comaps/settings/base.py` `DATABASES` default `sqlite:///{BASE_DIR}/db.sqlite3`. `.env.example` comments Postgres. **No `comaps/settings/prod.py`.** `db.sqlite3` present in the checkout. | **residual Ops** |
| EU region string (**SPD-062**) | Decision: hosting region **EU**; exact provider/region string is ops, not a code constant. Not verifiable from client or this explorer tree. | **residual Ops** |
| Competition endpoints reachable from the signed app’s API base | Client fail-closed until `EXPLORE_API_BASE_URL` inject (`https://api.streifzug.app/api` on Android release/beta). Competition paths `{apiBase}/v1/competition/…`. **No competition app in explorer `main` to reach.** Signed APK not produced, so the installed artefact’s base is not hashed. Debug C++ default is empty (not a developer LAN host) — SP-004 still holds in code. | **residual Ops** (backend missing here); client fail-closed **mitigated** |
| Sparse-area N&lt;3 against a direct API call | Cannot call a competition read API that does not exist in this explorer tree. | **residual Ops** |

Friends API on explorer `main` is **not** the competition backend
(audit §26 condition 5).

---

## H10 / **SPD-086** — recorded local V1 gate commands

Forgejo `linux-check.yaml` `CTEST_EXCLUDE_REGEX` still excludes most C++
suites (including `map_tests`, `routing_tests`, …). **Not** a Phase 10
coding task (**SPD-086**). The V1 gate is the recorded local run.

Commands (README §8.1):

```
./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke
./tools/unix/build_omim.sh -d street_pixels_tests
# then: from the build dir, street_pixels_tests with --data_path / --user_resource_path
cd android && ./gradlew -Pandroidauto=true lint
./tools/unix/clang-format.sh
```

`street_pixels_tests` is **not** in the smoke suite list in
`tools/unix/run_tests.sh` (`base_tests` … `search_tests`). It is a
separate local binary.

### Tests actually run in this slice

`data/classificator.txt` and `data/types.txt` are **gitignored
generated files** (root `.gitignore` `data/classificator.txt*` /
`data/types.txt*`), not committed sources.

**Original SP-096 run (2026-08-29 14:18 UTC)** against
`omim-build-debug/street_pixels_tests` with
`--data_path=/workspace/data --user_resource_path=/workspace/data`,
**before** those files existed. Full suite aborted. Last completed
tests were in `competition_ownership_tests.cpp`. Then
`eligibility_tests.cpp::Eligibility_IncludesCommonHighways` threw
`FileAbsentException` (`classificator.txt` missing). Corroborated log
`/tmp/street_pixels_tests_full.log`: **393** `Running`, **392** `OK`,
**1** `FAILED` (then process abort). That abort was an environment
gap, not a product Fail.

**Independent review re-run (same day, after generated files
appeared locally at 14:20 UTC):** same binary and data paths.
`--filter` is `regex_search` (`libs/testing/testingmain.cpp`), so
`--filter=SampleAcceptance` matches both `SampleAcceptanceManager_*`
(**5**) and `LiveSampleAcceptance_*` (**15**) = **20**, not 20 extra
tests beside the live filter.

| Run | Running | OK | FAILED | Result |
| --- | --- | --- | --- | --- |
| Review unique `--filter` (`CollectionGate_`, `SampleAcceptanceManager_`, `Rematch_`, `BackendConfig_`, `LiveSampleAcceptance_`, `CompetitionUpload_`) | 93 | 93 | 0 | `All tests passed.` |
| Review `--filter=SampleAcceptance` (overlaps live filter) | 20 | 20 | 0 | `All tests passed.` |
| Review **full** `street_pixels_tests` | **499** | **499** | 0 | `All tests passed.` |

A clean checkout that has not generated `classificator.txt` /
`types.txt` still cannot run `Eligibility_*`. Smoke
(`run_tests.sh -s smoke`) was **not run** (includes `map_tests` /
`indexer_tests`; left to SP-097). Tests were not weakened.

**Android lint:** original SP-096 configure printed
`secure.properties.release doesn't exist` and
`secure.properties.test doesn't exist`. Those configure lines are
**not** the release-task `NO RELEASE signing keys found` message
(that prints only when a `release` Gradle task is requested). Task
`:sdk:lintDebug` **FAILED** (`abortOnError = true`): **5 errors, 24
warnings** corroborated from
`android/sdk/build/intermediates/lint_intermediate_text_report/debug/lintReportDebug/lint-results-debug.txt`
(last line `5 errors, 24 warnings`). Errors: four `MissingPermission`
(`VIBRATE`) in `sdk/util/Utils.java` lines 371, 375, 406, 412; one
`WrongConstant` in `RecordingSessionDebug.java:56`. App-module lint
did not finish because sdk aborted. Not triaged as clean. Not fixed
in this docs item. Residual SP-097. **clang-format:** no C++ edit in
this item.

Tests are not weakened.

---

## Contradictions (spec / audit / code)

1. **Audit §22 “Confirmed now” ungated collection** vs current
   `StreetPixelsManager::OnLocationUpdate` session gate (SP-007).
   Code wins; this note records **mitigated**.
2. **Audit wipe-on-update / `CleanupStreetPixels`** vs current rematch
   on update and archive-on-delete. The **symbol remains**; the wipe-on-
   update behaviour does not. Work-item hint “`CleanupStreetPixels`
   absence” is therefore **not** literal absence.
3. **Audit empty formulas** vs **SPD-026 / 057 / 058** implemented in
   `ownership_scoring.hpp`. Spec LaTeX remains empty (do not edit spec).
4. **Audit `/stats/upload` gap** vs client competition aggregates URL.
   `GetStatsUploadUrl()` still exists but is not the competition path;
   `ShouldAttemptStatsUpload()` is false. Explorer `main` still has no
   competition ingest.
5. **README SP-075/SP-076 Accepted** on explorer branches that are
   **not** this explorer `main` checkout. Do not treat those SHAs as
   present here.
6. **Forgejo `SECURE_PROPERTIES` → `secure.properties`** vs Gradle
   `secure.properties.release`.
7. **Phase-10 “current code locations” (planning)** previously said
   add-friend filters were registered. SP-092 removed them. The
   snapshot paragraph that still said “Friends deep links … unchanged”
   is corrected in this review; the manifest table already recorded
   removal.
8. **Play listing** still advertises GPX as a free feature while public
   V1 gates GPX (**SPD-084** residual; not rewritten here).

---

## H3/H8 posture (short)

- **H3 / SPD-079:** Google Play `google` release is the public V1 store
  gate. Flavor and `applicationId` are configured. Listing brand copy
  residual. Signed `google` artefact **not** produced in this
  environment.
- **H8 / SPD-084:** Gradle flavors + Forgejo `android-release.yaml` +
  `docs/CREDENTIALS.md` secret *names* reused. Application name, listing
  copy, privacy/terms URLs residual. Signing *execution* residual
  (secrets absent; filename mismatch).

---

## Independent review (2026-08-29)

Reviewer did not write the original close-out. Re-checked audit §22
(**19/19** table rows have a position), §26, explorer `main`
`e13a124`, signing files, Sentry/ABL/friends/session-gate/rematch/
filter/avoid-explored/analytics, and test/lint numbers against
executed logs. Spec and audit files were not edited. Phase 10 exit
not marked met. Accepted-by left empty.

Corrections in this pass: sparse-area chrome wording (boss line only);
`--filter=SampleAcceptance` overlap; gitignore paths; lint “NO
RELEASE keys” vs configure `doesn't exist`; full-suite **499/499**
after generated classificator files appeared; phase-10 friends
snapshot sentence. Positions in the §22/§26 tables were otherwise
held against the tree.
