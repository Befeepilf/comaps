# Phase 2 — Recording and collection correctness

**Status:** In progress
**Depends on:** Phase 1
**Blocks:** Phase 3, and through it Phases 4, 6, 8, 9

---

## Objective

Make exploration mean what the product says it means: a pixel turns green only
because the user deliberately started a recording session and physically
travelled there, with location data good enough to believe.

This phase closes the single most serious behavioural gap in the codebase and
establishes the sample-acceptance pipeline everything downstream trusts.

## Product-spec references

- §3.3 Explicit recording — collection only during a user-started session;
  sessions continue in background and with the screen off.
- §11.1–§11.5 Recording sessions: explicit start, background continuation,
  pause semantics, session completion, interrupted sessions.
- §15.1 Exploration radius fixed at 25 metres, not user-configurable.
- §15.2 Permanent exploration state and first-explored source.
- §15.5 Movement types — walking and cycling treated equally.
- §16.1–§16.5 GPS validation and interpolation, including the accepted-sample
  defaults, the interpolation caps, and the rejected-gap rules.
- §28.1–§28.2 Haptics only while recording and in the foreground; one pulse per
  accepted update that collects at least one pixel.
- §31 Error states: background location denied, poor GPS accuracy, interrupted
  recording.
- §34 "Recording" and "GPS integrity" launch requirements.

## Technical-audit references

- §8 Recording and location feasibility, including the recommended state
  machine `Idle → Recording → Paused → Finished | Discarded`.
- §9 GPS validation and interpolation, including the proposed acceptance
  pipeline and the spec-versus-code numeric mismatch table.
- §22 Risk register: "Ungated collection without recording" and "False GPS
  exploration".
- Spikes 3 and 5.

## Current code locations

Verified 2026-08-02 against the working tree (post SP-012).

| Concern | Location | Observed state |
| --- | --- | --- |
| GPS entry point | `libs/map/framework.cpp` `Framework::OnLocationUpdate` | Calls `m_streetPixelsManager->OnLocationUpdate(rInfo)` unconditionally; collection gated inside the manager. |
| Collection | `libs/map/street_pixels_manager.cpp` `StreetPixelsManager::OnLocationUpdate` | Returns immediately unless `RecordingSession::IsRecording()`. Rejected samples collect nothing. |
| Sample acceptance | `libs/map/live_sample_acceptance_filter.{hpp,cpp}`, `StreetPixelsManager::OnLocationUpdate` | Spec §16.2 defaults: accuracy ≤ 25 m, staleness ≤ 120 s, OS-invalid rejected, implied speed ≤ 50 km/h, teleport > 200 m rejected. Reference resets on session change and via `ResetSampleAcceptanceReference()`. Rejection reason exposed through `GetLastSampleRejectReason()`. |
| Collection radius | `libs/map/street_pixels_manager.cpp` `kExploreRadiusMeters` | `25.0` metres; `kRadiusRads` derived. Not user-configurable. |
| Track filter | `libs/map/gps_track_filter.cpp` | Exists for the track path only. Minimum horizontal accuracy 250 m, 10 m decimation, 2 m/s² acceleration limit, direction check, requires `HasSpeed()`. Not applied to pixel collection. |
| Interpolation | — | **No interpolation exists in the live pixel path.** `serdes_gpx.cpp` fills GPX timestamps and `extrapolator.cpp` extrapolates for display; neither feeds pixel collection. |
| Session concept | `libs/map/recording_session.{hpp,cpp}`, `Framework::GetRecordingSession()` | State machine `Idle` / `Recording` / `Paused` / `Finished` / `Discarded`; wired to collection gate via `SetRecordingSession`. |
| Android recording | `android/app/.../location/TrackRecordingService.java`, `android/sdk/.../location/RecordingSession.java`, `TrackRecorder.java` | FGS typed `location`; production `RecordingSession` JNI drives shared session; Pause/Resume in UI + notification; ABL not added (D3). Device matrix → SP-014. |
| Debug session control | `android/sdk/.../location/RecordingSessionDebug.java` | DEBUG-only JNI wrappers for `RecordingSession` start/pause/resume/finish/discard (SP-007 validation affordance). |
| Location provider | `android/sdk/.../location/LocationHelper.java` | 500 ms interval normally, 1000 ms while track recording |
| Permissions | `android/app/src/main/AndroidManifest.xml` | `ACCESS_COARSE_LOCATION`, `ACCESS_FINE_LOCATION`, `ACCESS_LOCATION_EXTRA_COMMANDS`. **`ACCESS_BACKGROUND_LOCATION` is absent.** |
| Haptics | `libs/map/street_pixels_manager.cpp` `OnLocationUpdate` | Vibration only after gated collection marks pixels (`TriggerCollectionVibration`) |

**Differences from the technical audit:** the acceptance-filter gap identified in the
audit is closed by SP-009. No live interpolation exists yet; the audit's phrasing
"no interpolation across pause" still understates the situation because there is
nothing to prevent.

## Intended outcome

- A shared session state machine in `libs/` that Android drives and iOS can
  later drive unchanged.
- `StreetPixelsManager` collects pixels only when a session is in the
  `Recording` state.
- A sample-acceptance filter matching the spec defaults, applied before
  collection.
- Interpolation between consecutive accepted samples within the spec caps, with
  hard barriers at pause, resume, interruption, and rejection boundaries.
- Android controls for pause, resume, finish, and discard, wired to the
  foreground service and its notification.
- Interrupted sessions detected and reported to the user, with no gap filling.

## Dependencies

- Phase 1 exit criteria, especially SP-002. Every work item in this phase needs
  runnable tests; several are untestable by inspection alone.

## Proposed work-item breakdown

| ID | Title | Depends on | Notes |
| --- | --- | --- | --- |
| SP-006 | Shared recording-session state model | SP-002 | **Accepted** 2026-07-27 — state machine + settings breadcrumb; no collection gate yet |
| SP-007 | Pixel-collection recording gate | SP-006 | **Accepted** 2026-07-27 — gate in `StreetPixelsManager::OnLocationUpdate`; track import ungated |
| SP-008 | Align collection radius with the specified 25 metres | SP-007 | **Accepted** 2026-07-27 — `kExploreRadiusMeters` 25 m; 4 `CollectionRadius_*` boundary tests |
| SP-009 | Live sample acceptance filter | SP-007 | **Accepted** 2026-08-02 — `LiveSampleAcceptanceFilter` in collection path; 15 filter + 5 manager tests |
| SP-010 | Pause and resume semantics | SP-006, SP-007, SP-009 | **Accepted** 2026-08-02 — append suspend + in-memory segment boundaries; filter reset on pause/resume; D2 live drape deferred |
| SP-011 | Segment interpolation with pause and interruption barriers | SP-009, SP-010 | **Accepted** 2026-08-02 — `LiveSegmentInterpolation` 10 m sampling + barriers; shared `ForEachMercatorSegmentSample`; 19 segment tests; 98/98 suite |
| SP-012 | Android recording controls and foreground-service integration | SP-010 | **Accepted** 2026-08-02 — one Record Track control; FGS while Recording/Paused; notification Pause/Resume/Stop; ABL deferred; device matrix → SP-014 |
| SP-013 | Interrupted-session detection and recovery | SP-010, SP-012 | |
| SP-014 | Recording end-to-end validation | all of the above | |

Adjustments to the originally suggested breakdown, and why:

- The suggested item "no interpolation across paused or interrupted periods"
  was split into SP-009 (acceptance filter) and SP-011 (interpolation with
  barriers). Interpolation must be built before its prohibitions are
  meaningful, and an acceptance filter must exist before interpolation has
  defined endpoints.
- SP-008 exists separately so that a change in collected pixel counts during
  SP-007 validation is unambiguously attributable to the gate rather than to a
  radius change. It is a very small work item.
- SP-006 and SP-007 stay separate: SP-006 introduces the state machine with no
  behavioural change, which makes SP-007 a small and reviewable diff whose
  entire content is the gate.

## Data and migration concerns

- Session records are new persisted state. Decide early whether a session is
  persisted at all in V1 or only held in memory with enough on-disk breadcrumbs
  to detect an interrupted session after a process death. SP-006 makes this
  choice explicitly.
- SP-008 changes which pixels a given GPS fix collects. It does not invalidate
  already-explored pixels; exploration is permanent.
- The spec's "first-explored source" (§15.2) needs a per-pixel source field
  that does not exist. That field is Phase 3 work. Phase 2 must not invent a
  parallel storage location for it. Where Phase 2 needs to distinguish live
  collection, it does so in memory within the session.
- Recorded tracks continue to use the existing `GpsTracker` and bookmark
  storage. Do not introduce a second track store.

## Privacy and security implications

- The gate in SP-007 is the fix for a confirmed violation of product principle
  §3.3. Until it lands, the application derives exploration from location
  whenever it receives a fix.
- Raw samples must not become a new upload surface. Retention of raw versus
  accepted samples is decided in SP-009; accepted samples for the user's own
  exportable track are fine, raw debug logs are not enabled in release builds.
- `ACCESS_BACKGROUND_LOCATION` is absent from the manifest. If SP-012 concludes
  it is required, adding it changes the Play Store data-safety disclosure and
  the permission rationale copy. That is a release-governance change, and it is
  tracked into Phase 10 rather than being made quietly.
- Permission rationale copy must tie the request to session recording and must
  not bundle competition consent, per spec §10 steps 3 and 4.

## Automated testing strategy

Primary vehicle is the Street Pixels test target from SP-002, driven by
synthetic `location::GpsInfo` sequences. No device required.

- **Session state machine:** every legal transition; every illegal transition
  rejected; state after finish and after discard.
- **Gate:** identical fix sequences produce zero collected pixels when idle or
  paused, and the expected pixels when recording.
- **Acceptance filter,** one test per rule: accuracy worse than 25 m rejected;
  stale timestamp rejected; OS-invalid sample rejected; implied speed above
  50 km/h rejected; teleport rejected. Include boundary values on each
  threshold.
- **Interpolation:** accepted within 30 s and 200 m; rejected beyond either;
  never emitted across a pause, a resume, an interruption, or a rejected
  sample; the sample after a rejection becomes a fresh interpolation origin.
- **Radius:** a fix at a known distance from a known pixel collects it at
  24.9 m and does not at 25.1 m.
- **Regression:** `gps_track_*` tests in `libs/map/map_tests/` must still pass,
  since the track path shares types with the collection path.

## Manual validation strategy

Device testing is mandatory here; the spec makes claims about behaviour that
cannot be observed in a simulator.

- Walk a known route with a session recording; confirm the collected pixels
  match the route and that nothing green appears where the user did not go.
- Walk the same route with **no** session active; confirm nothing is collected.
- Pause mid-route, travel a segment, resume; confirm no pixels along the paused
  segment and no line connecting pause to resume.
- Screen off for at least 30 minutes during a session on at least a Pixel-class
  device and one aggressive-OEM device; confirm samples continue.
- Force-stop the app mid-session, reopen; confirm interruption is reported and
  no gap is filled.
- Cycle a route including a tunnel or an urban canyon; confirm no false
  exploration on either side of the signal loss.
- Ride as a passenger in a vehicle; confirm the speed rule suppresses
  collection.
- Confirm one haptic pulse per accepted collecting update in the foreground,
  and none with the screen off.

Record device model, OS version, build type, route, and outcome for each.

## Entry criteria

- Phase 1 exit criteria met.
- SP-002's test target is executable locally (`street_pixels_tests`).
- At least two physical Android test devices are available.

## Exit criteria

1. No pixel is collected outside an active, non-paused recording session, and
   an automated test proves it.
2. A session supports start, pause, resume, finish, and discard, with the state
   machine implemented in shared code rather than in Android code.
3. Sample acceptance implements the spec §16.2 defaults, with a test per rule
   including boundary values.
4. Interpolation implements the spec §16.3 caps and never crosses a pause,
   interruption, or rejection.
5. Interrupted sessions are detected, the user is informed that part of the
   session may be missing, and no interval is filled automatically.
6. The collection radius is 25 metres and is not user-configurable.
7. Documented device validation exists for background and screen-off recording
   on at least two devices, including one aggressive-OEM device, with recorded
   sample-continuity results.
8. Existing `gps_track_*` tests still pass.

## Explicit non-goals

- Per-pixel source flags, per-pixel timestamps, and competitive recency. Those
  are Phase 3 and Phase 8.
- Map-update reconciliation. Phase 3.
- Area assignment or area-scoped progress. Phases 4 and 5.
- Changing the derivation sampling distance. Phase 3, because it changes the
  pixel universe and needs the reconciliation machinery.
- GPX import behaviour. Phase 9.
- Redesigning the recording UI beyond adding the controls the spec requires.
- Battery optimisation work beyond not making battery use worse.

## Known uncertainties

- Whether background sampling survives on aggressive OEM skins without
  `ACCESS_BACKGROUND_LOCATION` while the foreground service runs. Resolve by
  measurement in SP-012, not by reasoning.
- Whether the spec's default thresholds produce acceptable results for cycling.
  The audit's spike 5 pass criteria — under 1% false urban teleports and under
  5% missed legitimate bike segments — are the target. Retuning requires a
  decision entry.
- How "stale" is determined from `location::GpsInfo`, and whether the platform
  reliably marks samples invalid.
- Whether interpolation should emit discrete sample points or collect along a
  segment. The spec says pixels within 25 m of the interpolated segment, which
  argues for segment-based collection; confirm the cost against the existing
  HEALPix query path in SP-011.
- Whether pause should stop the location subscription entirely (saving battery,
  slowing resume) or keep it running and discard samples. This is a product-
  visible trade-off and is decided in SP-010.
- Whether a session should survive process death within V1, or only be detected
  as interrupted.
