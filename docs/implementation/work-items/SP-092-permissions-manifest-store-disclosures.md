# SP-092 — Permissions, manifest, and store disclosures

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H3, H6, H8, H9 Accepted (**SPD-079**,
  **SPD-082**, **SPD-084**, **SPD-085**). Phase 10 implementation
  entry.
**Unblocks:** SP-096 signing/pipeline; SP-097 exit #5 / #11
**Notes:** Hide friends (H9 / SPD-085); keep ABL absent (H6 /
  SPD-082). Do **not** rewrite Play listing brand copy, application
  name, or privacy/terms URLs (residual, SPD-079 / SPD-080 /
  SPD-084).

---

## Objective

Make the Android manifest, permission rationales, and factual
data-safety answers match what the software actually does: session-only
location, no purchase action, no friends surface in public V1,
background location declared only if ABL is present (**SPD-082**:
ABL stays absent). Play/F-Droid listing copy rewrite that is
marketing/brand is residual.

---

## Motivation

The google Play `en-US` full description is still generic CoMaps and
lists GPX import/export. Help and listing still imply a tracker-free
OSM app. `add-friend` intent-filters are registered. Data-safety
questionnaires are not in this repository as Street Pixels answers.

Spec §34 Release governance requires accurate Android store
permissions and background-location disclosures. A mismatch is a
launch blocker even if the binary is correct.

---

## In-scope behavior

- Re-inventory permissions vs behaviour (location, FGS types,
  notifications, vibrate, internet). Document why each is required.
- Apply H6 / **SPD-082**: keep ABL absent. Do not add it. Play Console
  background-location declaration is out of scope unless a later SPD
  adds ABL after D2 measurement (D2 execution is residual).
- Apply H9 / **SPD-085**: public flavor must not register add-friend
  deep links and must not present friend onboarding. **This is
  implementable** (not brand).
- Apply H3 / **SPD-079** and H8 / **SPD-084** for non-brand gate
  behaviour: Google Play `google` is the V1 gate; no purchase; no
  ungated GPX claim in *factual* data-safety answers. Do **not**
  rewrite Play listing brand copy, application name, Help title, or
  location rationale that says “CoMaps” (residual).
- Data-safety answers as a checked-in document under
  `docs/implementation/` (or Play metadata path if one exists) that
  a human can paste into Play Console: location (not shared except
  competition aggregates when opted in), no ads, no sale of data.
  Factual questionnaire only; marketing listing identity residual.
- Permission rationale strings: session recording; not bundled with
  competition (already `track_recording_location_rationale`). Do **not**
  drop leftover “CoMaps” product name in this item (brand residual).
- Confirm public builds present no purchase/restore (SPD-010).
- Confirm `FOREGROUND_SERVICE_LOCATION` matches `TrackRecordingService`
  / `NavigationService` types.

## Out-of-scope behavior

- Writing the full privacy policy (SP-093 **residual**).
- Rewriting Play/F-Droid listing copy that is marketing/brand
  (SPD-079 / SPD-084 residual).
- Actually clicking Publish in Play Console (ops; SP-096 records
  that the pipeline produced an artefact; signed APK/ops may residual).
- Huawei/F-Droid listings if H3 excludes them from the V1 gate
  (**SPD-079**: Huawei/web not a gate; F-Droid listing brand copy
  residual).
- Adding instrumented tests.

## Relevant product requirements

- Spec §10 steps 3–4, §25.1, §29.3, §34 Explorer Pro / Release
  governance.
- SPD-010, SPD-011, SPD-061, **SPD-079**, **SPD-082**, **SPD-084**,
  **SPD-085**.

## Relevant source files or symbols

- `android/app/src/main/AndroidManifest.xml`
- `docs/CREDENTIALS.md`
- Rationale strings in `values/strings.xml`
- Play listing paths (`android/app/src/google/play/listings/**`,
  `android/app/src/fdroid/play/**`): **read-only** in this item; brand
  copy is residual (SPD-079 / SPD-084). Do not rewrite them.

## Implementation notes / constraints

- Listing brand copy is residual; this item does not invent marketing
  slogans and does not rewrite listing identity.
- Do not add ABL “just in case” (**SPD-082**).
- Friends code may remain compiled; public *surface* must not
  (**SPD-085**).

## Acceptance criteria

1. Permission inventory table: permission → code path → store
   disclosure line.
2. Public manifest matches H6 (**SPD-082**, ABL absent) and H9
   (**SPD-085**, friends hidden).
3. Gated flavor listing brand copy is **not** rewritten in this item
   (residual). Factual data-safety must not advertise ungated GPX or
   friends.
4. Data-safety document exists and matches behaviour.
5. Agent does not mark Accepted.

## Required automated tests

- JVM or lint-level assertion that public BuildConfig has Pro
  capabilities false (already SP-005/083; re-run).
- If add-friend filters are flavor-gated, a unit/manifest test or
  documented aapt dump of the public APK (device dump may wait for
  SP-095).

## Required manual validation

- Manifest / flavor checks in this environment (unit test or documented
  aapt dump of a locally built public APK). Listing brand text is
  residual (not rewritten here). Device install / device dump are
  SP-095 (**residual**); do not execute a hardware walk in this item.

## Failure and rollback considerations

- Play review of ABL is outside the team (phase-10 known
  uncertainty). H6 / **SPD-082** keeps ABL absent; do not add it
  inside this item.

## Permission inventory

Permission → code path → store disclosure line. Source manifests:
`android/app/src/main/AndroidManifest.xml` (merged with
`android/sdk/src/main/AndroidManifest.xml`). Flavor overlays
`debug` / `beta` do not add permissions or add-friend filters.
`google` / `web` / `fdroid` / `huawei` have no extra manifest.

| Permission | Code path | Store disclosure line |
| --- | --- | --- |
| `ACCESS_FINE_LOCATION` | `TrackRecordingService`, `NavigationService`, `LocationHelper`, first-run recording rationale `track_recording_location_rationale` | Precise location is used on-device for an active recording session and for turn-by-turn navigation. Raw GPS is never uploaded. |
| `ACCESS_COARSE_LOCATION` | Same location stack (Android location APIs) | Approximate location on-device; off-device only as competition area/city aggregates when the user opts in. |
| `ACCESS_LOCATION_EXTRA_COMMANDS` | `LocationHelper` A-GPS `sendExtraCommand` (`force_xtra_injection`, `force_time_injection`) | Not a Play data type. Speeds GPS time-to-fix. No extra upload. |
| `ACCESS_BACKGROUND_LOCATION` | **Absent** (`PublicManifestAssertionsTest`) | Do not declare Play background location (**SPD-082**). |
| `FOREGROUND_SERVICE` | `TrackRecordingService`, `NavigationService`, `DownloaderService` | Required to run the matching FGS types below. |
| `FOREGROUND_SERVICE_LOCATION` | `TrackRecordingService` and `NavigationService` (`foregroundServiceType="location"`; `ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION`) | Location FGS while a recording session or navigation is active. Matches the permission. |
| `FOREGROUND_SERVICE_DATA_SYNC` | `DownloaderService` (`foregroundServiceType="dataSync"`) | Map-download FGS. Not location. |
| `POST_NOTIFICATIONS` | `TrackRecordingService`, `NavigationService`, `DownloaderService` / `DownloaderNotifier`, `MwmActivity` request | Ongoing recording / navigation / download notifications. Not a Play user-data type. |
| `INTERNET` | Map downloads, competition HTTPS (opt-in aggregates), OSM editor upload, Sentry | Enables off-device collection listed in `docs/implementation/play-data-safety.md`. |
| `ACCESS_NETWORK_STATE` | Downloader / connectivity checks | Not a Play user-data type. |
| `WAKE_LOCK` | WorkManager OSM upload / pre-O JobIntentService comment in manifest | Keeps short work alive. Not a Play user-data type. |
| `VIBRATE` | `Utils` vibrator helpers; exploration haptics | On-device haptic feedback. Not collected. |
| `READ_EXTERNAL_STORAGE` (`maxSdkVersion=22`) | Legacy file open for bookmarks/tracks on API 22 and below | On-device files. Not a cloud GPX feature. |
| `androidx.car.app.NAVIGATION_TEMPLATES` | `CarAppService` | Android Auto navigation templates. |
| `androidx.car.app.ACCESS_SURFACE` | `CarAppService` | Android Auto surface. |
| `${applicationId}.permission.READ_NAVIGATION_DATA` (defined) | `NavigationContentProvider` | Custom permission other apps must hold. Not Play collection. |

Friends: dedicated `comaps://add-friend` and HTTPS `comaps.app/add-friend` intent-filters **removed** from the main manifest (**SPD-085**). `FriendSettingsVisibility.showAddFriendOnboarding` is false in public V1. `MyAccountDialogFragment.showWithAddFriend` no-ops. `MwmActivity.processIntent` still swallows leftover `add-friend` URIs that can match the generic `comaps://` scheme or `https://comaps.at/…`.

Purchase: no `BillingClient` / Play Billing in `android/app` or `android/sdk` Java (**SPD-010**). Re-run `ExplorerProGateTest` (capabilities closed when native is not ready / public flags off).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-092-permissions-manifest-6383` |
| Permission inventory | See table above. |
| Listing brand copy | Residual (not rewritten). `android/app/src/google/play/listings/**` and F-Droid listing paths untouched. |
| Data-safety doc | `docs/implementation/play-data-safety.md` |
| Test output | `./gradlew :app:testGoogleDebugUnitTest --tests 'app.organicmaps.settings.PublicManifestAssertionsTest' --tests 'app.organicmaps.settings.FriendSettingsVisibilityTest' --tests 'app.organicmaps.settings.PublicSettingsVisibilityTest'` → **BUILD SUCCESSFUL**. JUnit XML: PublicManifestAssertionsTest **5/5**, FriendSettingsVisibilityTest **7/7**, PublicSettingsVisibilityTest **1/1** (0 failures). `:sdk:testDebugUnitTest --tests 'app.organicmaps.sdk.ExplorerProGateTest' --tests 'app.organicmaps.sdk.ExplorerProAnalyticsTest' --rerun-tasks` → **BUILD SUCCESSFUL**. ExplorerProGateTest **11/11**, ExplorerProAnalyticsTest **2/2**. Merged `googleDebug` manifest: no `ACCESS_BACKGROUND_LOCATION`, no `add-friend`; `TrackRecordingService` / `NavigationService` `foregroundServiceType="location"`. |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Generic `comaps://` scheme and host-wide `https://comaps.at` filters can still deliver `…/add-friend` URIs to SplashActivity | Record. Dedicated filters removed. Code swallows add-friend URIs and hides onboarding. Splitting ge0-style `comaps://` hosts would break map links. |
| Help / listing still CoMaps; location rationale still allowed to say CoMaps | Residual **SPD-079 / SPD-084 / SPD-080**. Not rewritten. |
| Play Data safety form still needs a privacy-policy URL | SP-093 residual. Questionnaire checked in without inventing the URL. |
| Sentry installation IDs / interaction breadcrumbs | Declared as service-provider collection in the questionnaire. Device capture that a Play build’s Sentry project has no screenshots remains SP-095 / SP-097. |
| `https://comaps.app/add-friend` App Link `autoVerify` filter removed; `comaps.at` autoVerify map links remain | Expected. Do not re-add add-friend App Links. |
| Phase 10 / SP-088 architecture snapshots still say add-friend filters are registered | Dated planning notes. Code is source of truth after this item. |
| ABL Play background-location declaration | Out of scope unless a later SPD adds ABL after D2 (**SPD-082**). |
