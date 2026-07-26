# SP-007 — Pixel-collection recording gate

**Phase:** 2 — Recording and collection correctness
**Status:** Accepted
**Branch:** `street-pixels`

---

## Objective

Make pixel collection conditional on an active, non-paused recording session.
No session, no exploration.

## Motivation

This is the most serious behavioural defect in the codebase and the direct
contradiction of a stated product principle.

`Framework::OnLocationUpdate` in `libs/map/framework.cpp` calls
`m_streetPixelsManager->OnLocationUpdate(rInfo)` unconditionally — no session
check, no enabled check, no consent check. `StreetPixelsManager::OnLocationUpdate`
then marks every valid pixel within the radius as explored.

The practical effect: installing the app and granting location permission is
enough to start accumulating a permanent record of everywhere the user goes,
with no session, no indication, and no user action. Product spec §3.3 says
Street Pixels does not track the user simply because the application is
installed, and §11.1 says pixels are collected automatically only while a
user-started session is active.

Every downstream feature inherits this. Area percentages, milestones,
competition eligibility, and share cards would all be computed from
exploration the user never chose to record.

## In-scope behavior

- Pixel collection occurs only when the session state is `Recording`.
- No collection when the state is `Idle`, `Paused`, `Finished`, or `Discarded`.
- The gate lives in shared code, so it applies to every platform.
- Haptic feedback tied to collection is suppressed accordingly, since it fires
  from the collection path.
- The gate placement is explicit and reviewable in one place, not scattered
  across call sites.

## Out-of-scope behavior

- Sample quality filtering. SP-009.
- Radius change. SP-008.
- Pause semantics beyond honouring the `Paused` state. SP-010.
- Interpolation. SP-011.
- Android UI for starting and stopping a session. SP-012. Until SP-012 lands,
  the session may be started through an existing entry point or a temporary
  developer affordance; note which was used during validation.
- GPX and track-replay pixel updates, which are a separate path
  (`UpdateExploredPixels`) and belong to Phase 9. Confirm this work item does
  not accidentally gate them, because import is not a recording session and
  must still work.
- The foreground and background condition for haptics under spec §28.1. That is
  Phase 7. This work item only stops haptics from firing when nothing is being
  collected.

## Relevant product requirements

- §3.3 Explicit recording.
- §11.1 Explicit start.
- §11.3 While paused, location data is not used to collect pixels.
- §15.1 Pixels may be collected from an accepted live location — "accepted"
  presupposes a session.
- §25.1 Local-only information, which this protects by not generating it in the
  first place.
- §34 "Recording": session state is clearly visible; sessions can be started,
  paused, resumed, and finished.

## Relevant source files or symbols

- `libs/map/framework.cpp`, `Framework::OnLocationUpdate` — the unconditional
  call site
- `libs/map/street_pixels_manager.cpp`, `StreetPixelsManager::OnLocationUpdate`
  — the collection routine, including `AddPixelsInRadius`, `FindStreetPixel`,
  the explored-bit write, and the vibration trigger
- `libs/map/street_pixels_manager.cpp`, `UpdateExploredPixels` and
  `UpdateStreetStatsForTrack` — the import path that must **not** be gated
- The session module from SP-006
- `libs/map/explore_stats_service.cpp`, `OnExplorationDelta` — confirm what it
  receives changes consistently

## Dependencies

- SP-006, for the session state.
- SP-002, for the tests.

## Proposed implementation approach

1. Decide where the gate lives. Two candidates: at the `Framework` call site,
   or inside `StreetPixelsManager::OnLocationUpdate`. Prefer inside the manager,
   so that any future caller inherits the gate and cannot bypass it by calling
   the manager directly. Record the choice and reasoning.
2. Add the check as the first thing the collection routine does, returning
   early when the state is not `Recording`.
3. Confirm the early return also suppresses the haptic, since it fires from the
   same routine.
4. Confirm the GPX and track-replay path is unaffected. Read
   `UpdateExploredPixels` and `UpdateStreetStatsForTrack` and verify neither
   routes through the gated function.
5. Confirm the renderer behaves correctly when no pixels change — it should
   simply receive no updates.
6. Verify that statistics aggregation in `ExploreStatsService` now sees fewer
   deltas and that nothing depends on receiving a delta per location update.

## Acceptance criteria

1. With no session active, a sequence of location updates collects zero pixels.
2. With a session in `Paused`, a sequence of location updates collects zero
   pixels.
3. With a session in `Recording`, the same sequence collects the same pixels it
   collected before this change.
4. No haptic fires when no pixel is collected.
5. GPX and track-replay pixel updates still work, unaffected by the gate.
6. The gate exists in exactly one place and cannot be bypassed by calling the
   manager directly.
7. Statistics aggregation does not break when deltas stop arriving between
   sessions.
8. The diff is small and contains no unrelated change.

## Required automated tests

In the SP-002 target, using the synthetic GPS helpers:

- A fixed sequence of location updates with the session `Idle` produces zero
  explored pixels.
- The same sequence with the session `Paused` produces zero explored pixels.
- The same sequence with the session `Recording` produces the expected explored
  set.
- Starting a session partway through a sequence collects only from the start
  onward.
- Pausing partway through collects only up to the pause.
- A track-replay import marks pixels regardless of session state.
- No haptic callback is invoked when the gate rejects.

## Required manual validation

Two paired walks over the same short route, on a physical device:

1. With no session started, walk the route. Confirm no pixel turns green and
   no haptic fires. This is the core regression this work item exists to
   prevent.
2. Start a session, walk the same route, confirm the expected pixels turn green.

Additionally:

- Start a session, pause, walk a segment, resume, and confirm no pixels were
  collected during the paused segment.
- Confirm the app is otherwise unchanged: map, routing, and track recording all
  behave as before.

Record device model, OS version, build type, and route for each.

## Failure and rollback considerations

- **Highest-risk failure:** gating too much and breaking GPX import, which uses
  a different path but shares the manager. Explicitly tested above.
- **Second risk:** gating in the wrong place, leaving a bypass. If the gate is
  at the `Framework` call site, any other caller of the manager collects
  ungated. Prefer the manager.
- Until SP-012 lands there may be no convenient way to start a session in the
  UI, which makes manual validation awkward. Use whatever entry point exists,
  or a temporary developer affordance that is removed before review. Record
  what was used.
- Statistics aggregation may have assumed a steady stream of deltas. Check for
  timing assumptions in `ExploreStatsService`.
- Rollback is a revert. No data migration. Pixels explored before the change
  remain explored, which is correct: exploration is permanent, and this work
  item is about not creating new exploration incorrectly.
- Note for the record: exploration already collected by existing installs was
  gathered without a session. Whether to clear it is a product question, best
  handled once the source flag exists in Phase 3. Raise it as discovered
  follow-up rather than deciding it here.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | `ffd83450e4`, `05090ba101` on `street-pixels` |
| Gate location and rationale | `StreetPixelsManager::OnLocationUpdate` — first statement checks `m_recordingSession->IsRecording()`; gate inside the manager so no caller can bypass. `Framework` wires `SetRecordingSession(m_recordingSession.get())`; call site unchanged. |
| Test output | `street_pixels_tests`: **47 / 47** passed (10 new `CollectionGate_*` cases); `run_tests.sh -f "street_pixels_tests"` exit 0 |
| Walk 1, no session: pixels collected | Pending device paired-walk (debug affordance ready) |
| Walk 2, session recording: pixels collected | Pending device paired-walk (debug affordance ready) |
| Pause segment result | Pending device paired-walk (debug affordance ready) |
| GPX import regression result | Covered by `CollectionGate_TrackReplay_MarksRegardlessOfSession` (`MarkTrackPixelsForTesting` / `MarkExploredPixelIds` path); full `UpdateExploredPixels` bookmark fixture deferred to Phase 9 |
| Test device model and OS version | Desktop validation only (macOS arm64); device walks before SP-014 |
| Implemented by | Cursor agent |
| Independent reviewer | Maintainer |
| Manual validation performed by and date | Maintainer code review 2026-07-27; device paired-walks pending — use `RecordingSessionDebug` on debug builds |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Exploration accumulated on existing installs without a session | Product question for Phase 3 once per-pixel source exists; do not clear retroactively without a decision |
| Release builds collect no live pixels until SP-012 wires session UI | Expected; `RecordingSessionDebug` is debug-only for interim validation |
| Device paired-walk validation not yet executed | Required before SP-014; use `RecordingSessionDebug` on debug APK |
