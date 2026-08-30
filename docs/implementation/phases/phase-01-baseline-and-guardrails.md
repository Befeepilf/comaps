# Phase 1 — Baseline and guardrails

**Status:** Not started
**Depends on:** nothing
**Blocks:** every other phase

---

## Objective

Establish a reproducible build, an executable test path, and the guardrails
that keep later phases honest: telemetry that matches the product's
private-by-default promise, known and controlled network egress, and a
feature-flag plus entitlement abstraction that lets incomplete surfaces ship
disabled.

This phase changes as little product behaviour as possible. Its output is the
ability to trust the output of every phase that follows.

## Product-spec references

- §3.2 Private by default.
- §29, §29.2 Explorer Pro architecture, feature availability versus entitlement.
- §32 Product analytics — aggregate and privacy-conscious; raw GPS coordinates
  and tracks are never sent to analytics.
- §34 "Release governance": analytics contain no raw location data.
- §34 "Explorer Pro and monetization": public builds do not present a
  non-functional purchase action.

## Technical-audit references

- §17 Privacy, analytics, and security — Sentry enabled with
  `send-default-pii`, screenshots, view hierarchy, and a 1.0 trace sample rate;
  hardcoded LAN API URL; GPS filter debug logs containing coordinates.
- §20 Build and testing status — tooling present; no Street Pixels unit tests;
  full builds not completed during the audit pass.
- §16 Monetization — no entitlement abstraction, no Android billing.
- §25 item 1 — correctness foundations, including "disable/fix privacy-hostile
  telemetry defaults for the fork".

## Current code locations

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Build entry | `configure.sh`, `docs/INSTALL.md`, `docs/INSTALL_DESKTOP.md` | Documented; Android builds use CMake from the Android SDK |
| Desktop build/test scripts | `tools/unix/build_omim.sh`, `tools/unix/run_tests.sh` | Present; smoke suite listed in `run_tests.sh` lines 7–17 |
| Test registration | `cmake/OmimTesting.cmake` `omim_add_test`, `omim_add_test_subdirectory` | Options: `REQUIRE_QT`, `REQUIRE_SERVER`, `NO_PLATFORM_INIT`, `BOOST_TEST` |
| Map test target | `libs/map/map_tests/CMakeLists.txt` | Declared `REQUIRE_QT REQUIRE_SERVER`; no street-pixel tests among its sources |
| CI (GitHub) | `.github/workflows/` | Only `android-check.yaml`, `ios-check.yaml`, `code-style-check.yaml`; no C++ test job |
| CI (Forgejo) | `.forgejo/workflows/linux-check.yaml` | Runs `run_tests.sh`, but `CTEST_EXCLUDE_REGEX` excludes `map_tests`, `routing_tests`, `indexer_tests`, and most other suites |
| Telemetry | `android/app/src/main/AndroidManifest.xml` | `io.sentry.send-default-pii=true`, `attach-screenshot=true`, `attach-view-hierarchy=true`, `traces.sample-rate=1.0`, profiling session sample rate 1.0, `logs.enabled=true` |
| API base URL | `libs/map/backend_config.cpp` | `kDefaultApiBaseUrl = "http://192.168.178.89:8999/api"`, overridable via the `Explore.ApiBaseUrl` setting |
| API base URL (Android) | `android/sdk/build.gradle`, `OrganicMaps.java` | `EXPLORE_API_BASE_URL` BuildConfig field; debug points at the LAN address, release and beta at `https://api.streifzug.app/api` |
| Existing flag mechanisms | `android/app/build.gradle`, `android/sdk/build.gradle`, `gradle.properties` | Product flavors `google`/`web`/`fdroid`/`huawei`; build types `debug`/`release`/`beta`; BuildConfig fields; `-P` gradle properties |
| Entitlement | — | Not found anywhere in the tree |

**Difference from the technical audit:** the audit describes the LAN URL as the
effective production default. On Android that is no longer strictly true — the
SDK Gradle build injects `EXPLORE_API_BASE_URL` per build type, so release and
beta builds point at `https://api.streifzug.app/api`. The compiled-in C++ default
is still the LAN address, so any non-Android consumer, and any path that reads
the default before the Android override is applied, still resolves to a
developer machine.

## Intended outcome

- A recorded, repeatable command sequence that produces a debug Android APK and
  a desktop debug build with tests on the maintainer's machine.
- A Street Pixels test target that builds quickly, contains at least one real
  assertion, and passes locally on the maintainer machine.
- Telemetry defaults that do not capture personally identifying data,
  screenshots, view hierarchies, or location values.
- A written inventory of every network destination a release build can reach,
  with no developer endpoint among them.
- A feature-flag and entitlement abstraction with all Explorer Pro flags off
  and no purchase surface exposed.

## Dependencies

None. This phase can start immediately.

## Proposed work-item breakdown

| ID | Title | Notes |
| --- | --- | --- |
| SP-001 | Reproducible Android and desktop build baseline | **Accepted** 2026-07-25 |
| SP-002 | Street Pixels test harness | **Accepted** 2026-07-26 — lean `street_pixels_tests`; local gate only for V1 |
| SP-003 | Privacy and telemetry baseline | **Accepted** 2026-07-26 |
| SP-004 | Network egress inventory and API base configuration | **Accepted** 2026-07-26 — fail-closed API base, egress inventory; build and `street_pixels_tests` green |
| SP-005 | Feature-flag and entitlement foundation | **Accepted** 2026-07-27 — capability + entitlement stub, Pro flags off, matrix tests green |

SP-002 is an addition to the originally suggested breakdown. It is justified by
two repository facts: no street-pixel test exists, and the one plausible host
target (`map_tests`) is both heavy (`REQUIRE_QT REQUIRE_SERVER`) and excluded
from the CI test run. Without SP-002, "run focused tests" in the validation
policy has nothing to run.

SP-004 is also an addition, justified by the compiled-in LAN default in
`backend_config.cpp`.

## Data and migration concerns

Minimal by design.

- SP-004 may change how the `Explore.ApiBaseUrl` setting is resolved. Existing
  installs that already stored a value must keep working; the change is to the
  default, not to stored settings.
- SP-005 introduces entitlement state. In V1 the entitlement source is a stub
  that reports "not entitled", so no user-visible state is created and no
  migration is needed. Choose a storage location that will not need renaming
  when a real source is added.
- No pixel, track, or statistics data is touched in this phase.

## Privacy and security implications

This is the phase where the product's privacy posture is either established or
permanently compromised.

- Sentry currently sends default PII, screenshots, and view hierarchies, and
  samples 100% of traces and profiling sessions. A screenshot of the map during
  a recording session is location data.
- The audit notes GPS filter debug logging that includes latitude and
  longitude. Verify the current log level in release builds.
- A compiled-in developer endpoint is both a privacy risk and a reliability
  risk. Egress must be enumerated, not assumed.
- Nothing in this phase may add a new upload path.

## Automated testing strategy

- SP-002 delivers the harness itself. Acceptance is local build and pass on the
  maintainer machine; C++ test CI is deferred post-V1.
- SP-005 adds unit tests for the flag and entitlement resolution matrix: flag
  off plus no entitlement, flag off plus entitlement, flag on plus no
  entitlement, flag on plus entitlement. Only the last combination may enable a
  Pro capability.
- SP-003 and SP-004 are primarily configuration; assert what can be asserted
  (for example that no build variant resolves to a private-range address) and
  rely on documented manual inspection for the rest.

## Manual validation strategy

- Execute the documented build commands from a clean checkout and confirm they
  succeed as written. If a command in `docs/INSTALL.md` does not work, record
  the working command rather than assuming.
- Install a release-configured build, exercise the app, and confirm through the
  telemetry backend that no screenshot, view-hierarchy, or PII payload arrives.
- Capture network traffic from a release-configured build during a normal
  session and confirm every destination appears in the SP-004 inventory.
- Confirm no purchase or Explorer Pro action is reachable in the UI.

## Entry criteria

- The product spec and the technical audit have been read.
- A build has been attempted and its outcome — success or the exact failure —
  is recorded.

## Exit criteria

1. Android debug APK and desktop debug-with-tests builds are reproducible from
   documented commands, and the commands are verified to work.
2. A Street Pixels test target exists, contains real assertions, and passes
   locally via the standard desktop build and `ctest`.
3. Telemetry in a release-configured build captures no PII, no screenshots, no
   view hierarchy, and no location values; sampling rates are deliberate rather
   than left at 1.0.
4. No release-configured build can reach a developer or private-network
   endpoint; the egress inventory is written down.
5. A feature-flag and entitlement abstraction exists, all Explorer Pro flags are
   off in public configuration, and no purchase action is reachable.
6. No pixel, recording, routing, or competition behaviour changed in this phase.

## Explicit non-goals

- Fixing the recording gate. That is Phase 2, deliberately, so that the gate
  lands on a branch with a working test harness.
- Changing collection radius, sampling distance, or any exploration constant.
- Removing the friends feature. Whether public builds expose it is OQ-6 in
  `DECISIONS.md`; hiding it behind a flag is acceptable, deleting it is not part
  of this phase.
- Building real billing or a real entitlement source.
- Adding new analytics events.
- Refactoring `StreetPixelsManager`.

## Known uncertainties

- Whether the maintainer's environment builds the Android target without
  changes. The audit did not complete a build.
- Whether a lean street-pixel test target can avoid `REQUIRE_QT` and
  `REQUIRE_SERVER` given `StreetPixelsManager`'s dependencies. If it cannot,
  the pure logic being tested (session state, sample acceptance) should be
  extracted into a dependency-light unit before Phase 2 adds tests for it.
- Whether CI runners are available to the maintainer at all, and whether the
  Forgejo or the GitHub workflow set is the live one. The `CTEST_EXCLUDE_REGEX`
  exclusions suggest the C++ suites are currently red.
- Which Sentry settings are required by the upstream Streifzug project versus
  which were added for this fork.
- Whether release builds already suppress verbose GPS logging.
