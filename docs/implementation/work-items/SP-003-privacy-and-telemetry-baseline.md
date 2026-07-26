# SP-003 — Privacy and telemetry baseline

**Phase:** 1 — Baseline and guardrails
**Status:** In progress
**Branch:** `street-pixels`

---

## Objective

Bring telemetry and logging defaults into line with the product's
private-by-default principle: no personally identifying data, no screenshots,
no view hierarchies, no location values, and deliberate sampling rates.

## Motivation

`android/app/src/main/AndroidManifest.xml` currently configures Sentry with
`io.sentry.send-default-pii=true`, `io.sentry.attach-screenshot=true`,
`io.sentry.attach-view-hierarchy=true`, `io.sentry.traces.sample-rate=1.0`,
profiling session sample rate 1.0, profiling starting on app start, and
`io.sentry.logs.enabled=true`.

For this product specifically, a screenshot is location data. A screenshot
taken during a recording session shows the user's position on a map, and the
view hierarchy of the map screen can carry the same information in text form.
Product spec §32 states that raw GPS coordinates and tracks are never sent to
product analytics, and §34 requires that analytics contain no raw location
data. The technical audit records this tension directly: CoMaps markets privacy
while the Android build ships PII and screenshot capture.

The audit also notes GPS filter debug logging that includes latitude and
longitude, which is a risk if verbose logging survives into release builds.

Fixing this later is not equivalent. Any telemetry captured before the fix is
already captured.

## In-scope behavior

- Setting `io.sentry.send-default-pii` to `false`.
- Disabling screenshot attachment and view-hierarchy attachment.
- Choosing deliberate, justified trace and profiling sample rates rather than
  leaving them at 1.0, and documenting the choice.
- Reviewing whether profiling should start on app start.
- Auditing log statements on the location and street-pixel paths for coordinate
  values, and confirming what survives into a release build.
- Documenting, in the work item's evidence, exactly what telemetry a release
  build now emits.

## Out-of-scope behavior

- Removing Sentry. Crash reporting is legitimate and useful; the problem is its
  configuration.
- Adding new analytics events. Product spec §32 defines the eventual event set;
  implementing it is later work.
- Changing backend telemetry.
- Changing the `libs/tracking/reporter.hpp` legacy component, which the audit
  reports as currently unused. Confirm and record; do not delete here.
- Privacy policy text. That is Phase 10.
- The `/stats/upload` path and explore statistics, which are SP-004 and
  Phase 8.

## Relevant product requirements

- §3.2 Private by default.
- §32 Product analytics: aggregate and privacy-conscious; raw GPS coordinates
  and tracks are never sent to product analytics.
- §34 "Release governance": analytics contain no raw location data.
- §25.1 Local-only information, which includes raw GPS samples and tracks.

## Relevant source files or symbols

- `android/app/src/main/AndroidManifest.xml`, the `io.sentry.*` meta-data block
- `android/app/build.gradle`, for the Sentry Gradle plugin configuration
- `libs/map/gps_track_filter.cpp`, for coordinate logging
- `libs/map/street_pixels_manager.cpp`, for logging on the collection path
- `libs/tracking/reporter.hpp`, to confirm it is unused
- `libs/base/logging.hpp`, for how log levels behave per build type

## Dependencies

- SP-001, so that a release-configured build can actually be produced and
  inspected.

## Proposed implementation approach

1. Enumerate every `io.sentry.*` meta-data entry with its current value, which
   is already captured in the phase file, and confirm it against the tree at
   implementation time.
2. Determine which entries came from upstream CoMaps and which were added for
   this fork. Upstream defaults still need to satisfy this product's promise,
   but knowing the origin matters for merge conflicts later.
3. Change PII, screenshot, and view-hierarchy settings to off.
4. Choose sample rates. A trace sample rate of 1.0 is a volume and privacy
   concern in a location app; pick a defensible number and write down why.
5. Grep the location and pixel paths for logging that includes latitude,
   longitude, or a `GpsInfo`. Verify at which log level each is emitted and
   whether that level is compiled or filtered out in release builds.
6. Build a release-configured APK, exercise the app including a location
   session, and inspect what actually arrives in the telemetry backend.
7. Record the resulting telemetry inventory in the evidence table.

## Acceptance criteria

1. `io.sentry.send-default-pii` is `false`.
2. Screenshot attachment is disabled.
3. View-hierarchy attachment is disabled.
4. Trace and profiling sample rates are deliberate and documented, not 1.0 by
   default.
5. No log statement reachable in a release build emits latitude, longitude, or
   a full `GpsInfo`.
6. A release-configured build was exercised and the telemetry received contains
   no screenshot, no view hierarchy, no PII, and no location value.
7. Crash reporting still works: a deliberately triggered crash appears in the
   telemetry backend.
8. No exploration, recording, or routing behaviour changed.

## Required automated tests

Telemetry configuration is manifest data and is not directly unit-testable.

- If a static check is cheap, add one asserting that the manifest does not set
  `send-default-pii`, `attach-screenshot`, or `attach-view-hierarchy` to
  `true`. A small script run by the existing Android lint or check job is
  sufficient.
- Otherwise, rely on the documented manual inspection and record why an
  automated check was not added.

## Required manual validation

- Install a release-configured build on a physical device.
- Grant location permission and use the map, including movement, so that any
  location-bearing telemetry would be produced.
- Trigger a deliberate crash and confirm it arrives with no screenshot, no view
  hierarchy, and no PII fields.
- Inspect the telemetry backend for the session and record what was received.
- Capture the device log during a session and confirm no coordinates appear at
  release log level.

## Failure and rollback considerations

- Disabling screenshots and view hierarchies reduces crash-diagnosis quality.
  That is an accepted trade-off; if a specific crash needs richer context, it
  is gathered in an internal build, not in public builds.
- Reducing the trace sample rate reduces performance visibility. Record the
  chosen rate so it can be revisited with evidence.
- Risk of a silent regression: a future dependency upgrade could reintroduce a
  default. This is why an automated manifest check is preferred over a manual
  one.
- Rollback is a straightforward revert. There is no data migration and no user
  state involved.
- Telemetry already collected before this change is not addressed by this work
  item. If any exists in the backend, deleting it is a separate operational
  task and should be raised as discovered follow-up.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | `c550b53c78` build blockers; `4703e6c266` Sentry defaults; `673f89805a` log scrub; `e368541738` manifest guard; `71cc5a8a22` docs |
| Sentry settings before | `send-default-pii=true`, `attach-screenshot=true`, `attach-view-hierarchy=true`, `traces.sample-rate=1.0`, profiling session-sample-rate `1.0`, `start-on-app-start=true`, `logs.enabled=true`, user-interaction `true` |
| Sentry settings after | `send-default-pii=false`, `attach-screenshot=false`, `attach-view-hierarchy=false`, `traces.sample-rate=0.1`, profiling session-sample-rate `0.1`, `start-on-app-start=false`, `logs.enabled=false`, user-interaction `true` (unchanged) |
| Chosen sample rates and rationale | Trace and profiling session sample rates `0.1` (10%): enough crash-adjacent performance signal without 100% volume in a location app. Profiling remains coupled to sampled traces (`lifecycle=trace`). App-start profiling off to avoid always-on launch capture and known ART risk. |
| Log statements changed | `gps_track_filter.cpp` LDEBUG: dropped lat/lon. `mwm_url.cpp`, `geo_url_parser.cpp`, `serdes_gpx.cpp`: LWARNING/LERROR no longer print coordinate values or raw geo strings. |
| Release build telemetry inventory | Packaged `web`+`beta` APK `CoMaps-26072602-web-beta.apk` (`assembleWebBeta -Parm64`). `aapt dump xmltree` confirms PII/screenshot/view-hierarchy/logs/start-on-app-start are false (`0x0`); sample rates are float `0.1` (`0x3dcccccd`); user-interaction remains true. Sentry Gradle plugin left unchanged (source/native symbol upload is org tooling, not end-user PII). `tracking::Reporter` confirmed unused outside `tracking_tests`. |
| Deliberate-crash report link | **Pending human device validation** — no physical device attached; Pixel_9a emulator hung offline (no adb port). Trigger with search `?emulateJavaCrash` on installed webBeta, then link the comaps-dl/android event. |
| Test device model and OS version | **Pending** — intended Pixel 3a / LineageOS 22.2 per SP-001 baseline, or successor. Emulator attempt: Pixel_9a AVD (android-36) failed to become adb-online. |
| Implemented by | Cursor agent, 2026-07-26 |
| Independent reviewer | — |
| Manual validation performed by and date | Partial: APK build + packaged-manifest inspection + `./tools/unix/check_sentry_privacy.sh` (OK), 2026-07-26. Full location session, logcat, and deliberate-crash Sentry inspection **not executed** (device unavailable). |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `webBeta` build was broken: missing `#include "geometry/distance_on_sphere.hpp"` in `Framework.cpp`; duplicate `routing_options_title` string; stale `DrivingOptionsActivity` import; missing `StreetExplorationRoutingOptions` import / wrong field name in `RoutingOptionsFragment`. | Fixed on `street-pixels` as build-blocker commits under SP-003 so release-configured validation could run. Broader routing-options WIP still needs a dedicated work item if incomplete. |
| Pre-fix Sentry events may contain PII/screenshots. | Operational purge in Sentry org; not code. |
| Device crash + logcat acceptance criteria still open. | Maintainer installs `CoMaps-26072602-web-beta.apk` (or rebuild), runs location session + `?emulateJavaCrash`, records Sentry event URL and logcat result in this table. |
| Sentry Logs and user-interaction tracing residual policy. | Logs disabled; user-interaction left on. Revisit if UI surfaces coordinates. |
| Gradle sentry plugin still uploads source/native symbols when auth is present. | Leave as-is unless a later privacy decision forbids org source upload. |
