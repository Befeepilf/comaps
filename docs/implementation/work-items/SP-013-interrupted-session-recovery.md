# SP-013 — Interrupted-session detection and recovery

**Phase:** 2 — Recording and collection correctness
**Status:** Not started
**Branch:** `street-pixels/SP-013-interrupted-session-recovery`

---

## Objective

Detect that a recording session was interrupted by the operating system, tell
the user part of the session may be missing, resume from the next accepted
sample, and never fill the missing interval.

## Motivation

Product spec §11.5 states that if the operating system interrupts background
tracking, no line is interpolated across the missing period, recording may
resume from the next accepted sample, and the application informs the user that
part of the session may be missing. §31 repeats it as an error state: explain
that part of the track may be missing, and never fill the missing interval
automatically.

Nothing like this exists. There is no session concept to interrupt, no
persistence to detect an interruption after a process death, and no user-facing
message. The audit records that iOS session restoration for exploration was not
found, and the same is true on Android.

SP-012's measurement will establish how often this actually happens. On
aggressive OEM skins it may be common. When it does happen, the user should
learn that a gap exists rather than either losing the session silently or, far
worse, seeing a straight line of exploration across everything they did not
walk.

## In-scope behavior

- Detecting on startup that a session was active when the process last ended,
  using the breadcrumb from SP-006.
- Detecting mid-session that location updates have stopped arriving for
  significantly longer than expected, and classifying that as an interruption.
- Setting the interpolation barrier from SP-011 at every interruption boundary,
  so no segment spans the gap.
- Informing the user that part of the session may be missing, non-blockingly.
- Deciding and implementing what happens to the interrupted session: resume it,
  or finish it and let the user start a new one. Whichever is chosen, the
  already-collected pixels and the already-recorded track portion are preserved.
- Distinguishing an interruption from a user pause, since they have the same
  gap consequence but different user meaning and different messaging.

## Out-of-scope behavior

- Preventing interruptions. That is SP-012's foreground service work and, beyond
  that, outside the application's control.
- The poor-GPS "Waiting for accurate location" state, which is a different
  situation: samples are arriving but being rejected. SP-009 exposes the
  rejection reason; presenting it is separate UI work.
- Track repair or gap filling of any kind. Explicitly forbidden.
- Crash reporting for the interruption itself. Interruption is normal OS
  behaviour, not a crash.
- Retroactively reconstructing where the user went.
- iOS session restoration.

## Relevant product requirements

- §11.2 The session ends when the user finishes it, the user discards it, or the
  operating system terminates tracking and the application cannot recover.
- §11.5 Interrupted sessions: no line is interpolated across the missing period;
  recording may resume from the next accepted sample; the application informs
  the user that part of the session may be missing.
- §16.3 Interpolation requires that neither sample follows an interrupted
  session.
- §16.4 No connecting line when background tracking was interrupted.
- §31 "Interrupted recording": explain that part of the track may be missing;
  never fill the missing interval automatically.
- §34 "Recording": interrupted sessions do not create false connecting lines.
- §34 "Quality": no critical exploration-data loss exists.

## Relevant source files or symbols

- The session breadcrumb from SP-006
- The interpolation barrier from SP-011 — this work item must use the same
  mechanism, not add a second one
- The acceptance filter reference reset from SP-009
- `android/app/.../location/TrackRecordingService.java`, for service lifecycle
  callbacks that may indicate termination
- `android/sdk/.../location/LocationHelper.java`, for the expected update
  interval that "significantly longer than expected" is measured against
- `libs/map/gps_tracker.{hpp,cpp}`, for what happens to a partially recorded
  track
- `android/app/.../MwmActivity.java`, for where a non-blocking message would
  surface

## Dependencies

- SP-006 for the breadcrumb, SP-010 for the pause distinction, SP-011 for the
  barrier, SP-012 for the service lifecycle.

## Proposed implementation approach

1. Define what counts as an interruption. Two detectable cases: the process died
   while a session was active, discovered at startup via the breadcrumb; and
   updates stopped arriving for much longer than the configured interval while
   the session claims to be recording. Pick a multiple of the expected interval
   and justify it — long enough that a brief scheduling delay or a batched
   delivery is not misclassified.
2. Decide the recovery policy. Resuming the same session keeps the user's
   session intact but produces a session with an unexplained gap. Finishing it
   is simpler and arguably more honest. Whichever is chosen, no collected pixel
   and no recorded track portion is lost. Record the decision and reasoning.
3. Set the interpolation barrier at the interruption boundary using SP-011's
   mechanism.
4. Reset the acceptance filter reference point, so the first post-interruption
   sample is not rejected as an impossible jump.
5. Add the user-facing message. Non-blocking, per the product's general
   restraint, and worded so it does not read as an error the user caused.
6. Ensure the message distinguishes interruption from pause.
7. Clear the breadcrumb correctly on every normal termination path, so a clean
   finish is never reported as an interruption.

## Acceptance criteria

1. A process death during an active session is detected on next startup.
2. A prolonged absence of location updates during a claimed recording session is
   detected as an interruption.
3. No pixel is collected along the interrupted interval, and no track segment
   spans it.
4. The user is informed that part of the session may be missing, non-blockingly.
5. The recovery policy is implemented as decided and documented.
6. Pixels collected before the interruption are preserved.
7. The recorded track portion before the interruption is preserved.
8. The first accepted sample after an interruption is not rejected for implied
   speed.
9. A normal finish or discard is never reported as an interruption.
10. Interruption and pause are distinguishable to the user.

## Required automated tests

In the SP-002 target:

- Breadcrumb present at startup with no active session in memory yields
  interruption detected.
- Breadcrumb absent yields no interruption detected.
- Breadcrumb cleared on finish; a subsequent startup detects nothing.
- Breadcrumb cleared on discard; a subsequent startup detects nothing.
- A simulated update gap beyond the threshold sets the interruption barrier.
- A simulated update gap just under the threshold does not.
- After an interruption, no interpolated collection occurs to the next accepted
  sample, while that sample's own disc is collected.
- After an interruption, the next accepted sample is not rejected for implied
  speed.
- Pixels and track content from before the interruption are intact.
- A pause is not classified as an interruption.

## Required manual validation

- Start a session, walk, then force-stop the app from system settings. Reopen
  and confirm the interruption message appears, the pixels collected before the
  force-stop are still green, and no line appears across the gap.
- Start a session, walk, restart the device, reopen, and confirm the same.
- Start a session and let an aggressive-OEM device kill the app naturally by
  leaving it backgrounded for a long period. This is the realistic case and it
  needs a device where SP-012's measurement showed poor continuity.
- Enable airplane mode mid-session for several minutes; confirm this is handled
  as poor GPS or interruption per the implemented rule, and that no gap is
  filled either way.
- Finish a session normally, restart the app, and confirm no interruption
  message appears. This false-positive case is the one most likely to annoy
  users.
- Pause a session, background the app for a long time, resume; confirm the
  message distinguishes this from an interruption if a message appears at all.

## Failure and rollback considerations

- **False positives are the main usability risk.** Reporting an interruption
  after every clean finish would make the app feel broken. The breadcrumb must
  be cleared reliably on every normal exit path, including finish, discard, and
  ordinary app termination with no session running.
- **False negatives are the main correctness risk.** A missed interruption means
  the barrier is not set, and SP-011 will happily interpolate across a gap that
  might be kilometres long. This is the failure the whole phase exists to
  prevent.
- The gap threshold is a guess until measured. Too short and batched background
  delivery is misclassified — which would be a particularly bad interaction with
  SP-012's screen-off behaviour. Too long and a real interruption goes
  undetected. Test with the screen off.
- If the recovery policy resumes the session, the resulting track has an
  unexplained gap that later features must handle. If it finishes the session,
  the user may be surprised to find their session ended. Neither is free; pick
  one and document it.
- Rollback is a revert. A leftover breadcrumb value must be harmless to code
  that does not understand it.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Interruption definition and gap threshold, with justification | |
| Recovery policy and rationale | |
| Barrier mechanism, shared with SP-011 | |
| Test output | |
| Force-stop test result | |
| Device-restart test result | |
| Natural OEM kill test result, device model | |
| Airplane-mode test result | |
| Clean-finish false-positive check | |
| Screen-off batched-delivery false-positive check | |
| Test device models and OS versions | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
