# SP-010 — Pause and resume semantics

**Phase:** 2 — Recording and collection correctness
**Status:** Not started
**Branch:** `street-pixels`

---

## Objective

Implement what pause actually means: no pixel collection, no track segment
across the paused period, and a resume that starts fresh from the next valid
location rather than continuing from where the pause began.

## Motivation

SP-006 introduced `Paused` as a state and SP-007 made the collection gate honour
it. That covers the first of the spec's three pause requirements. The other two
are unaddressed.

Product spec §11.3 requires that while paused, location data is not used to
collect pixels, no competitive recency is refreshed, and **no track segment is
created across the paused period**, and that resuming begins from the new valid
location **without interpolating across the pause**. Spec §16.3 repeats the
constraint from the interpolation side: interpolation requires that neither
sample follows a pause.

Neither Android nor the shared core has any pause concept today —
`TrackRecorder` exposes start, stop, and save only. So the track path needs the
pause boundary too, not just the pixel path.

Getting this wrong produces the exact failure the product promises to avoid: a
user pauses, takes a bus across town, resumes, and a straight line of green
pixels appears across everything in between.

## In-scope behavior

- Pausing stops pixel collection. Already delivered by SP-007; verified here.
- Pausing creates a boundary in the recorded track, so no segment spans the
  paused period.
- Resuming resets the acceptance filter's previous-accepted-sample reference,
  so the first sample after resume cannot be paired with the last sample before
  the pause for implied-speed or interpolation purposes.
- Deciding and implementing whether the location subscription continues during
  pause or is suspended.
- Multiple pause and resume cycles within one session.
- Time spent paused is tracked, so session duration can distinguish recording
  time from elapsed time.

## Out-of-scope behavior

- Interpolation itself. SP-011. This work item establishes the barrier;
  SP-011 must respect it.
- Interruption, which is a different cause with similar consequences. SP-013.
- Android UI controls for pause and resume. SP-012.
- Competitive recency, which does not exist yet. Phase 8. The spec's "no
  competitive recency is refreshed" during pause is satisfied here by not
  collecting at all.
- Changing how tracks are stored.
- Auto-pause on detected stillness. Not specified and not a V1 feature.

## Relevant product requirements

- §11.3 Pause: while paused, location data is not used to collect pixels, no
  competitive recency is refreshed, and no track segment is created across the
  paused period. Resuming begins from the new valid location without
  interpolating across the pause.
- §11.2 The session continues in the background; pause is a user action, not a
  lifecycle event.
- §16.3 Interpolation requires that neither sample follows a pause.
- §16.4 No connecting line is created when recording was paused.
- §10 step 5 The recording control supports pause, resume, and finish.
- §34 "GPS integrity": pause and resume do not create connecting segments.

## Relevant source files or symbols

- The session module from SP-006
- The acceptance filter from SP-009, specifically its reference-point reset
- `libs/map/street_pixels_manager.cpp`, `OnLocationUpdate` and the SP-007 gate
- `libs/map/gps_tracker.{hpp,cpp}`, `libs/map/gps_track.{hpp,cpp}`, for the
  track segment boundary
- `libs/map/gps_track_collection.*` and `libs/map/map_tests/gps_track_*.cpp`,
  for existing track behaviour and its tests
- `android/sdk/.../location/LocationHelper.java`, for the subscription and its
  1000 ms track-recording interval, if pause suspends the subscription

## Dependencies

- SP-006, SP-007, SP-009.

## Proposed implementation approach

1. Decide the subscription question first, because it shapes everything else.
   Options: keep the location subscription running and discard samples, or
   suspend it. Suspending saves battery during a long pause but makes resume
   slower and depends on how quickly the platform reacquires a fix. Keeping it
   running is simpler and makes resume immediate. Record the decision and its
   reasoning; either is defensible, but it must be deliberate.
2. On pause, mark a boundary in the recorded track so the storage layer does
   not join across it. Reuse whatever segment or gap concept `GpsTrack` already
   has if one exists; if it does not, that is a real addition and should be
   noted.
3. On resume, reset the acceptance filter reference point.
4. Track accumulated paused time on the session.
5. Verify that repeated pause and resume cycles behave identically to a single
   one.
6. Verify that finishing from `Paused` produces a coherent stored track.

## Acceptance criteria

1. No pixel is collected while paused.
2. The recorded track contains no segment spanning a paused period.
3. After resume, the first accepted sample is not paired with any pre-pause
   sample for implied-speed evaluation.
4. Multiple pause and resume cycles in one session behave correctly.
5. Finishing while paused produces a valid stored track.
6. The subscription decision is implemented and documented.
7. Paused time is tracked separately from recording time.
8. Existing `gps_track_*` tests still pass.

## Required automated tests

In the SP-002 target, plus the existing map tests for regression:

- Pause, feed samples, resume: zero pixels collected from the paused samples.
- Pause, feed samples that would imply an impossible speed relative to the
  pre-pause sample, resume: the first post-resume sample is accepted rather
  than rejected for implied speed.
- Pause and resume three times in one session: correct collection in each
  recording interval and none in the paused ones.
- The stored track for a paused session has a boundary rather than a joining
  segment.
- Finish from `Paused` produces a valid track.
- Paused duration accumulates correctly across cycles.

## Required manual validation

The defining test is the bus test:

- Start a session, walk a block, pause. Travel a significant distance by
  vehicle or transit. Resume, walk another block, finish.
- Confirm no pixels along the travelled route.
- Confirm the stored track shows two separate walked portions with no line
  between them.
- Confirm the map shows no green trail across the gap.

Also:

- Pause and resume several times during one walk; confirm collection matches
  exactly the recording intervals.
- Pause with the screen off, resume later; confirm behaviour is the same.
- Pause for an extended period, then resume; confirm resume acquires a fix
  promptly under the chosen subscription behaviour.
- If the subscription is suspended during pause, measure the resume latency.

## Failure and rollback considerations

- **The worst failure is a track that joins across the pause.** It produces a
  visible false line and, once pixels are collected along it, permanent false
  exploration. Test it explicitly on a real vehicle journey, not synthetically
  only.
- Forgetting to reset the acceptance reference point causes the opposite
  failure: the first post-resume sample is rejected as an impossible jump, so
  the user resumes and nothing collects for a while. Less damaging but very
  confusing.
- If the subscription is suspended, a slow reacquisition means the user resumes
  and walks a stretch before anything collects. Measure it.
- If the track storage layer has no gap concept, adding one touches shared
  recording code used by upstream features. Keep that change minimal and
  explicitly reviewed; it is the most likely place for regression.
- Rollback is a revert. Any tracks recorded with pause boundaries must still be
  readable by the reverted code; check this before merging if the storage
  format changes.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Subscription decision and rationale | |
| Track boundary mechanism | |
| Test output | |
| Bus test: distance travelled while paused | |
| Bus test: pixels collected while paused | |
| Bus test: stored track inspection | |
| Multiple pause cycles result | |
| Resume latency, if subscription suspended | |
| `gps_track_*` regression result | |
| Test device model and OS version | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
