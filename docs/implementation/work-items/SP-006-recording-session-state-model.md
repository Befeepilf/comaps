# SP-006 — Shared recording-session state model

**Phase:** 2 — Recording and collection correctness
**Status:** Not started
**Branch:** `street-pixels`

---

## Objective

Introduce a recording-session concept in shared C++ with an explicit state
machine — `Idle`, `Recording`, `Paused`, `Finished`, `Discarded` — that platform
code drives and that later work items read. No behaviour changes in this work
item.

## Motivation

There is no session concept anywhere in the shared core. `StreetPixelsManager`
and `StreetStatsDB` contain no session identifier, no session state, and no
lifecycle. On Android, `TrackRecorder` exposes start, stop, save, and
`isTrackRecordingEnabled`, with no pause or resume.

Product spec §3.3 and §11.1 make the session the entity that authorises
exploration collection, and §11.3, §11.4, and §11.5 define pause, completion,
and interruption semantics. Every remaining Phase 2 work item needs something
to ask "is a session recording right now".

Separating the model from the gate keeps SP-007's diff small enough that its
entire content is the behavioural change, which is exactly what a reviewer
should be looking at for a fix to a confirmed privacy violation.

Building it in `libs/` rather than in Android satisfies SPD-002: iOS should
adopt the same semantics later without reimplementing them.

## In-scope behavior

- A session state machine in shared code with the five states above.
- Transitions: `Idle → Recording` on start; `Recording ↔ Paused` on pause and
  resume; `Recording → Finished` and `Paused → Finished` on finish;
  `Recording → Discarded` and `Paused → Discarded` on discard;
  `Finished → Idle` and `Discarded → Idle` on reset.
- Rejection of every other transition, observably rather than silently.
- A session identifier and start timestamp, so later work items can attribute
  samples to a session.
- An observer or callback mechanism so interested components learn about state
  changes.
- A query the collection path will use in SP-007.
- Enough persisted breadcrumb to let SP-013 detect that a session was active
  when the process died. Whether the full session is persisted is decided in
  this work item and recorded.

## Out-of-scope behavior

- Gating pixel collection. SP-007.
- Pause semantics beyond the state transition itself — suppressing collection
  and preventing interpolation across the pause are SP-010 and SP-011.
- Any Android UI or service wiring. SP-012.
- Interruption detection logic. SP-013.
- Changing `GpsTracker` or the existing track recording behaviour.
- Track storage or session-to-track association.
- Any change to what pixels are collected.

## Relevant product requirements

- §3.3 Explicit recording: exploration is collected only during an explicit
  session started by the user.
- §11.1 Explicit start; opening the application does not begin recording.
- §11.2 The session ends only when the user finishes it, the user discards it,
  or the operating system terminates tracking unrecoverably.
- §11.3 Pause.
- §11.4 Session completion.
- §11.5 Interrupted sessions.
- §10 step 5: the recording control supports pause, resume, and finish.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.{hpp,cpp}`, the eventual consumer
- `libs/map/framework.{hpp,cpp}`, likely owner of the session object
- `libs/map/gps_tracker.{hpp,cpp}`, `libs/map/gps_track.*`, existing recording
  lifecycle to align with but not modify
- `libs/map/CMakeLists.txt`, to register new sources
- `android/sdk/.../location/TrackRecorder.java` and its JNI surface, read for
  context on the existing lifecycle
- `libs/platform/settings.hpp`, if a breadcrumb is persisted

## Dependencies

- SP-002, for the test target.

## Proposed implementation approach

1. Add a small session module in `libs/map/`. Keep it dependency-light so it can
   be tested in the SP-002 target without Qt or a test server.
2. Model states as an enumeration and transitions as an explicit function that
   returns success or rejection. Do not model transitions implicitly through
   flags; a reviewer should be able to read the legal transitions in one place.
3. Include a session identifier and a start timestamp. Do not add fields that
   nothing consumes yet.
4. Decide persistence. The minimum SP-013 needs is a durable marker meaning "a
   session was active", cleared on finish and discard. Record the decision and
   its reasoning in the evidence table.
5. Expose the state query the collection path will call, and an observer
   mechanism following whatever pattern the surrounding code already uses.
6. Wire nothing. Nothing calls the state machine to change behaviour in this
   work item.

## Acceptance criteria

1. A session state machine exists in `libs/` with the five specified states.
2. Every legal transition is implemented and every illegal transition is
   rejected observably.
3. A session has an identifier and a start timestamp.
4. Interested components can observe state changes.
5. The persistence decision is implemented and documented.
6. No pixel collection, track recording, routing, or UI behaviour changed. This
   is verifiable by walking a route with the build and observing identical
   behaviour to the parent commit.
7. The module is testable in the SP-002 target.

## Required automated tests

- Each legal transition succeeds and produces the expected resulting state.
- Each illegal transition is rejected and leaves the state unchanged. Cover at
  minimum: resume from `Idle`, pause from `Idle`, finish from `Idle`, start from
  `Recording`, start from `Paused`, resume from `Finished`.
- A new session receives a distinct identifier.
- Observers are notified on every state change and not on rejected transitions.
- If a breadcrumb is persisted: it is set on start, cleared on finish, cleared
  on discard, and survives a simulated restart while recording.

## Required manual validation

Light, because nothing user-visible changes.

- Build and run; confirm the app behaves identically to the parent commit.
- Walk a short route with the build installed and confirm pixel collection is
  unchanged, which is the correct behaviour at this point even though the
  ungated collection is the bug SP-007 fixes.

## Failure and rollback considerations

- The main risk is scope creep into SP-007. If the reviewer sees any change to
  collection behaviour, the work item is wrong.
- A second risk is over-modelling. Do not add session metadata that no work
  item consumes; unused fields become wrong fields.
- If the persistence choice turns out to be inadequate for SP-013, that is a
  small follow-up change, not a redesign — provided the state machine itself is
  not entangled with storage.
- Rollback is a revert with no user-visible effect and no data migration,
  assuming the breadcrumb is a single value that can be safely ignored if left
  behind.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Module path | |
| States and transitions implemented | |
| Persistence decision and rationale | |
| Test output | |
| Confirmation that no behaviour changed | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
