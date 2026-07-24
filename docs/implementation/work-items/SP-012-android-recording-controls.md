# SP-012 — Android recording controls and foreground-service integration

**Phase:** 2 — Recording and collection correctness
**Status:** Not started
**Branch:** `street-pixels/SP-012-android-recording-controls`

---

## Objective

Give the user working start, pause, resume, finish, and discard controls on
Android, wire them to the shared session state machine and the foreground
service, and make the recording state clearly visible including in the
notification.

## Motivation

The shared session model from SP-006 has no user interface. Android exposes
start, stop, save, and `isTrackRecordingEnabled` through `TrackRecorder`, with
no pause or resume anywhere in the Android sources. Until this lands, sessions
can only be driven from tests or a developer affordance, which means the whole
of Phase 2 is unusable by an actual person.

Product spec §10 step 5 requires the recording control to support pause,
resume, and finish, and requires a persistent operating-system indicator or
notification where the platform requires one. §11.2 requires the session to
continue when the app is backgrounded, the screen is off, or the user switches
apps. §34 requires that session state is clearly visible.

This is also where the background-recording reliability question gets answered.
`ACCESS_BACKGROUND_LOCATION` is absent from the manifest, and the foreground
service is typed `location`. Whether that combination keeps GPS flowing with
the screen off across OEM skins cannot be determined by reading code.

## In-scope behavior

- UI controls for start, pause, resume, finish, and discard, on the map surface.
- Wiring those controls to the shared session state machine rather than
  duplicating state in Android.
- Foreground service lifecycle following the session: running while `Recording`
  and while `Paused`, stopped on finish and discard.
- Notification content reflecting the current state, with actions appropriate to
  it.
- Clear in-app recording status.
- Permission flow: requesting location permission when the user starts
  recording, with a rationale that connects the request to session recording and
  does not bundle competition consent.
- Deciding whether `ACCESS_BACKGROUND_LOCATION` is needed, based on measurement,
  and adding it only if measurement says so.
- Handling the background-location-denied case: foreground recording remains
  available, and the user is told recording will pause when the app is no longer
  active.
- Measuring screen-off sample continuity across a device matrix.

## Out-of-scope behavior

- Interruption detection and recovery. SP-013.
- Session history, track inspection, and deletion UI. Those are spec §11.4 and
  §30 requirements but belong to a later UI work item; note as follow-up.
- Onboarding cards and the first-launch journey. Phase 5 or later.
- Milestone badges and haptics policy. Phase 7.
- Competition consent. Phase 8. This work item must specifically avoid coupling
  location permission to it.
- Restyling the map controls beyond adding what is needed.
- iOS.

## Relevant product requirements

- §10 step 3 Location permission requested when the user taps to start, with an
  explanation tied to the feature, not bundled with competition consent.
- §10 step 4 Background-recording explanation before requesting any background
  capability, stating that recording continues while the screen is off and stops
  when the session ends, and not claiming continuous tracking.
- §10 step 5 Recording state clearly visible; pause, resume, and finish
  supported; persistent OS indicator or notification where the platform requires
  it.
- §11.2 Background continuation, and the three ways a session ends.
- §11.4 On finish, the recorded track is stored locally and the user may inspect
  or delete it.
- §31 "Location denied" and "Background location denied" states.
- §34 "Recording": sessions can be started, paused, resumed, and finished;
  recording continues in the background and with the screen off; session state
  is clearly visible.
- §33 success indicators 2 and 3.

## Relevant source files or symbols

- `android/app/src/main/java/app/organicmaps/location/TrackRecordingService.java`
  — the foreground service, typed `location`, and
  `TRACK_REC_NOTIFICATION_ID`
- `android/sdk/src/main/java/app/organicmaps/sdk/location/TrackRecorder.java` —
  `nativeStartTrackRecording`, `nativeStopTrackRecording`,
  `nativeSaveTrackRecordingWithName`, `nativeIsTrackRecordingEmpty`,
  `nativeIsTrackRecordingEnabled`
- `android/sdk/src/main/java/app/organicmaps/sdk/location/LocationHelper.java` —
  500 ms normal and 1000 ms track-recording intervals
- `android/app/src/main/java/app/organicmaps/MwmActivity.java` — map buttons and
  the existing `onStreetPixelsStateChanged` hook
- `android/app/src/main/AndroidManifest.xml` — location permissions, absence of
  `ACCESS_BACKGROUND_LOCATION`, foreground service types
- `android/sdk/src/main/java/app/organicmaps/sdk/Framework.java` — the JNI
  surface to extend for session control
- `docs/ANDROID_LOCATION_TEST.md` — existing manual location test cases

## Dependencies

- SP-006 for the session model, SP-010 for pause semantics.

## Proposed implementation approach

1. Extend the JNI surface with session start, pause, resume, finish, and
   discard, and a state query plus a state-change callback. Keep the state in
   shared code; Android reflects it and never owns it.
2. Add the controls to the map surface. Follow the existing map-button pattern
   rather than introducing a new UI paradigm.
3. Make the foreground service lifecycle follow session state, including staying
   alive while paused so that resume is immediate and the OS does not reclaim
   the process.
4. Update the notification to show state and offer the appropriate action.
5. Implement the permission flow with the spec's rationale copy, requested at
   the moment the user starts recording.
6. Run the screen-off measurement **without** `ACCESS_BACKGROUND_LOCATION`
   first, on the device matrix. Only if measurement shows the permission is
   required should it be added, and adding it is then recorded as a
   release-governance item for Phase 10 because it changes the Play Store data
   safety declaration.
7. Implement the background-location-denied path.

## Acceptance criteria

1. The user can start, pause, resume, finish, and discard a session from the UI.
2. Session state is visible in the app and in the notification.
3. The foreground service runs while recording and while paused, and stops on
   finish and discard.
4. Recording continues when the app is backgrounded, when the user switches
   apps, and with the screen off.
5. Location permission is requested at start, with a rationale tied to session
   recording, and is not bundled with competition consent.
6. If background location is denied, foreground recording works and the user is
   told recording will pause when the app is no longer active.
7. Android holds no session state of its own; it reflects the shared state.
8. Screen-off sample continuity is measured on at least two devices, one of them
   an aggressive-OEM device, and the results are recorded.
9. `ACCESS_BACKGROUND_LOCATION` is added only if measurement showed it is
   required, and its addition is flagged for Phase 10 release governance.

## Required automated tests

Android has three JVM unit-test files and no instrumented tests, so automated
coverage here is limited by existing infrastructure.

- Shared-side session transitions are already covered by SP-006.
- Add JVM unit tests for any pure Android-side logic introduced, such as
  notification content selection by state or control enablement by state.
- Do not add an instrumented test framework as part of this work item; if one is
  wanted, record it as follow-up.

## Required manual validation

This work item is validated mostly by hand, and it carries the phase's most
important reliability measurement.

For each device in the matrix:

- Start a session; confirm the notification appears and shows the recording
  state.
- Background the app; confirm recording continues.
- Turn the screen off for at least 30 minutes while walking; confirm samples
  continue and count how many were received against how many were expected at
  the configured interval.
- Pause from the notification and from the app; confirm both work and both are
  reflected in the other.
- Resume, finish, and confirm the track is stored.
- Discard and confirm nothing is stored.
- Switch to another app for an extended period; confirm recording continues.
- Deny location permission and confirm the app still shows the map and explains
  what is unavailable.
- Deny background location, if the permission is requested, and confirm the
  documented degraded behaviour.
- Reboot the device mid-session and observe what happens; the recovery
  behaviour itself is SP-013, but record the observation.

Device matrix: at least a Pixel-class device and at least one aggressive-OEM
device such as Xiaomi, Samsung, or Huawei. Record model, OS version, and
whether any battery-optimisation exemption was granted.

## Failure and rollback considerations

- **The likeliest failure is OEM background killing.** If an aggressive OEM
  terminates the service, recording stops silently and the user loses part of a
  session. The honest response is SP-013's interruption reporting plus accurate
  product copy — not a claim that recording always continues. If measurement
  shows poor continuity on common devices, that is a finding for the product
  owner, not something to engineer around silently.
- Adding `ACCESS_BACKGROUND_LOCATION` has consequences beyond code: a Play
  Console declaration, a justification, possibly a review video. Do not add it
  speculatively.
- Duplicating session state in Android would create two sources of truth that
  drift, most visibly between the notification and the map. Review for this
  specifically.
- Keeping the foreground service alive during pause consumes battery for no
  collection. If pause suspends the subscription per SP-010, ensure the service
  is not doing pointless work.
- A notification action that races the UI can produce an invalid transition. The
  shared state machine rejects illegal transitions, so the failure mode is a
  no-op rather than corruption, but confirm the UI does not get stuck.
- Rollback is a revert. If the manifest changed, confirm the reverted build
  still installs over the new one on a test device.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| JNI surface added | |
| Notification behaviour by state | |
| `ACCESS_BACKGROUND_LOCATION` added? Evidence for the decision | |
| Device 1: model, OS, screen-off samples received versus expected | |
| Device 2: model, OS, screen-off samples received versus expected | |
| Battery-optimisation exemptions granted, if any | |
| Pause and resume from notification result | |
| Permission-denied path result | |
| Background-location-denied path result | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
