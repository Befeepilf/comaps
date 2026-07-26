# SP-004 — Network egress inventory and API base configuration

**Phase:** 1 — Baseline and guardrails
**Status:** In review
**Branch:** `street-pixels`

---

## Objective

Enumerate every network destination a Street Pixels build can reach, remove the
developer LAN address as a compiled-in default, and make the configured API
base explicit per build configuration.

## Motivation

`libs/map/backend_config.cpp` defines
`kDefaultApiBaseUrl = "http://192.168.178.89:8999/api"` — a private-range
address on someone's home network, over plain HTTP. It is overridable through
the `Explore.ApiBaseUrl` setting, and `android/sdk/build.gradle` does inject an
`EXPLORE_API_BASE_URL` BuildConfig value that release and beta builds set to
`https://api.comaps.app/api`, applied in `OrganicMaps.java` during platform
initialisation. But the C++ default remains the LAN address for any path that
resolves before the override is applied, and for any consumer that is not the
Android app.

Separately, `ExploreStatsService` posts to `{apiBase}/stats/upload`, an endpoint
that does not exist in `comaps_backend`. Its periodic check runs every minute.
So a build that starts uploading is contacting either a developer machine or a
production host that returns an error, once a minute, carrying a device
identifier.

Before Phase 8 designs an upload protocol, the current egress must be known and
under control. This is also the cheapest possible moment to fix it.

## In-scope behavior

- A written inventory of every outbound network destination reachable from a
  Street Pixels build: map downloads, explore statistics, friends, account,
  telemetry, and anything else found.
- Removing the private-range address as the compiled-in default. The default
  becomes either empty — meaning "not configured, do not call" — or a
  build-injected value, with no silent fallback to a developer host.
- Making the unconfigured state safe: no network call at all, rather than a call
  to a wrong host.
- Verifying the Android `EXPLORE_API_BASE_URL` injection covers every build
  type and flavor, including `fdroid` and `huawei`.
- Confirming that no plain-HTTP destination remains in a release build.

## Out-of-scope behavior

- Designing or implementing the competition upload protocol. Phase 8.
- Implementing `/stats/upload` on the backend. Phase 8.
- Changing what `ExploreStatsService` aggregates or when it aggregates. This
  work item may change *where* it would send and whether it sends when
  unconfigured; it does not change the payload.
- Removing the friends client. See OQ-6.
- Changing map-download infrastructure.
- Telemetry endpoints. SP-003.

## Relevant product requirements

- §3.2 Private by default.
- §25.1 Local-only information unless competition is enabled.
- §25.2 The closed list of uploaded fields.
- §34 "Privacy and competition": no raw GPS data is uploaded.
- §26.1 Offline core: the application must work fully with no network.

## Relevant source files or symbols

- `libs/map/backend_config.{hpp,cpp}`: `kApiBaseUrlKey`,
  `kDefaultApiBaseUrl`, `SetApiBaseUrl`, `GetApiBaseUrl`, `GetStatsUploadUrl`
- `libs/map/explore_stats_service.cpp`: `TryUpload`,
  `SchedulePeriodicUpload`, `m_syncEnabled`
- `libs/map/friends_manager.cpp`, for the other consumers of
  `backend::GetApiBaseUrl()`
- `android/sdk/build.gradle`, the `EXPLORE_API_BASE_URL` BuildConfig field
- `android/sdk/src/main/java/app/organicmaps/sdk/OrganicMaps.java`, where
  `nativeSetExploreApiBaseUrl` is called
- `android/sdk/src/main/java/app/organicmaps/sdk/Framework.java`, the explore
  natives
- `libs/platform/http_*`, for the HTTP client layer

## Dependencies

- SP-001, so a release-configured build can be produced and observed.

## Proposed implementation approach

1. Enumerate consumers of `backend::GetApiBaseUrl()` and every other outbound
   call in the Street Pixels surface. Record host, scheme, trigger, and
   frequency.
2. Decide the unconfigured-state behaviour. Recommended: `GetApiBaseUrl()`
   returns empty when neither a stored setting nor a build-injected value
   exists, and every caller treats empty as "do not attempt". Failing closed is
   safer than failing to a default host.
3. Remove `kDefaultApiBaseUrl`, or reduce it to empty.
4. Verify each caller handles the empty case without crashing, without retry
   storms, and without user-visible error noise, since offline is a normal
   state for this product.
5. Check every Android build type and flavor combination for the injected
   value. Record which combinations get which host.
6. Confirm no plain-HTTP destination survives in a release build.
7. Write the egress inventory into the work item evidence and, if it is
   generally useful, into the baseline document from SP-001.

## Acceptance criteria

1. No private-range or developer address is compiled into any build.
2. With no API base configured, no explore or friends network call is attempted.
3. With no API base configured, the application starts, shows a map, records,
   and routes normally.
4. Every Android build type and flavor has a known, recorded API base value.
5. No release build contacts a plain-HTTP endpoint.
6. A written egress inventory exists listing host, scheme, trigger, and
   frequency for every outbound call.
7. Offline behaviour is unchanged and no retry storm occurs when unconfigured
   or offline.

## Required automated tests

- `GetApiBaseUrl()` returns empty when no setting and no injected value exist.
- `SetApiBaseUrl` normalisation still strips a trailing slash, and setting an
  empty value clears the stored setting.
- URL builders such as `GetStatsUploadUrl` do not produce a call-worthy URL
  from an empty base.
- Upload and refresh entry points are no-ops with an empty base, asserted at the
  call decision rather than by mocking the network.

These belong in the SP-002 test target.

## Required manual validation

- Install a release-configured build with no stored API base. Capture network
  traffic for a full session including a recording. Confirm no explore or
  friends request is attempted and no private-range address appears.
- Confirm the app is fully functional offline: map, recording, routing.
- Install a debug build with a configured base and confirm it reaches the
  intended host.
- Leave the app running for at least 30 minutes unconfigured and confirm there
  is no repeating failed request.
- Record the traffic capture as evidence.

## Failure and rollback considerations

- Failing closed could disable a feature someone relies on during development.
  Mitigate by keeping the debug build's injected value working and documenting
  how to set `Explore.ApiBaseUrl` manually.
- A caller that does not handle the empty base could crash or log-spam. Review
  every caller; do not assume.
- Removing the default changes behaviour for any device that already has the
  service running against the LAN address. Since there is no public user base,
  the impact is limited to developer devices.
- Rollback is a revert. No persisted state changes; stored settings values are
  untouched.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | `7219de4be2`, `73e7df4088`, `c9eb7411ac`, `f1103c6650` |
| Egress inventory | [baseline.md §8](../baseline.md#8-network-egress-inventory-sp-004) |
| Unconfigured-state behaviour chosen | Empty `GetApiBaseUrl()`; `IsApiConfigured()`; callers no-op before HTTP |
| Build type and flavor to API base mapping | debug `""`; release/beta `https://api.comaps.app/api`; all flavors inherit SDK build type; optional `-PexploreApiBaseUrl` |
| Traffic capture, release build unconfigured | **Pending maintainer** — procedure: install `webBeta`, clear app data, PCAPdroid or `adb logcat` grep for 30+ min with recording; expect no explore/friends/LAN traffic |
| Traffic capture, debug build configured | **Pending maintainer** — `assembleWebDebug -PexploreApiBaseUrl=https://api.comaps.app/api`; confirm TLS to `api.comaps.app` |
| Offline session result | **Pending maintainer** — map, recording, routing with airplane mode |
| Automated tests | `ctest -L omim-test -R '^street_pixels_tests$'` — **Passed** 2026-07-26 (11 tests) |
| BuildConfig verification | `generateDebugBuildConfig` → `""`; release/beta → `https://api.comaps.app/api` |
| LAN string audit | No `192.168.178.89` in compiled sources (removed from `backend_config`, `build.gradle`, `network_security_config.xml`) |
| Implemented by | Cursor agent |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `ExploreStatsService` checks every minute vs spec §25.3 (15 min + jitter) | Phase 8 competition upload protocol |
| `/stats/upload` endpoint missing on backend | Phase 8 |
| Friends feature vs V1 non-goal | OQ-6; not resolved in SP-004 |
| `network_security_config.xml` still permits cleartext globally (`base-config cleartextTrafficPermitted="true"`) | Audit separately; SP-004 removed developer LAN domain only |
