# SP-064 — First-100-metres contextual goal

**Phase:** 7 — Milestones and share cards
**Status:** Planned
**Branch:**
**Depends on:** SP-062 M2 (pixel equivalent) and M8 (once per install).
  Recording session (SP-006) and live collection (SP-007/009/011).
**Unblocks:** SP-065 presentation consistency; SP-066 first-goal haptic;
  SP-069 exit #3

---

## Objective

Show a small non-blocking “Explore your first 100 m” badge on first
recording, with an incomplete progress indicator, and complete it after the
locked new-live-pixel equivalent of 100 metres.

## Motivation

Spec §10 steps 6 and 9 are the first-use reward. No first-goal surface exists.
This is contextual onboarding, not an achievement system (§18.5).

## In-scope behavior

- Shared C++ progress for the first-goal window. The **count rule is
  SP-062 M2 after maintainer lock**, not this item. Recommended (not
  decided): 10 new live pixels. Product must also lock (a) newly explored
  cells only vs (b) cells whose `IsEverLive` becomes set (imported→live
  flip). Import-only writes do not count. Do **not** reuse
  `numNewlyExploredPixels` unless M2 is locked to (a) — that counter
  ignores imported→live flips.
- Appear when the first recording session starts, if the goal is not yet
  complete. Incomplete progress indicator. Incomplete progress **persists
  across later recording sessions** until M2 (M8 is once per install, not
  once per session).
- Complete with a small animation when the threshold is reached. User stays
  on the map.
- Once complete (or if already complete on a later session), do not show
  again (M8).
- Persist completion so process death / reinstall-of-data (same files)
  does not revive the badge. Reinstall that wipes local files may restart
  onboarding; do not invent cloud restore.
- Android V1: bind near the top of the map, non-blocking, not a modal.
  Reuse or sit beside `mExplorationBadge` in `MapButtonsController` —
  do not replace the focused-area percentage badge with this onboarding
  chip for returning users.
- Emit a first-goal-complete haptic **event** for SP-066. Do **not** call
  `TriggerCollectionVibration` for that event (per-pixel, not
  foreground-gated). Until SP-066, the event may be a no-op sink.
- Tests: threshold exact per locked M2; imported-only pixels do not
  advance; paused session does not collect and does not advance;
  already-complete stays complete; incomplete survives a second session.

## Out-of-scope behavior

- Area 25/50/100 celebrations (SP-065).
- Haptic pattern implementation (SP-066) — may call a stub / existing
  vibrate until then.
- Competition hint at ~300 m / 30 pixels (Phase 8).
- Achievement history.
- iOS UI.

## Relevant product requirements

- Spec §10 steps 6, 8, 9; §18.5; §34 Progress experience (first-use
  guidance).
- SP-062 M2, M8.

## Relevant source files or symbols

- `RecordingSession::Start` / `IsRecording`
- `StreetPixelsManager::OnLocationUpdate` (`numNewlyExploredPixels`,
  ever-live bit)
- `android/app/.../MapButtonsController.java` `mExplorationBadge`
- No existing first-goal symbol (2026-08-19)

## Implementation notes / constraints

- Collection already gated on active non-paused recording (SP-007). Do not
  add a second collection path.
- Count rule waits on Accepted M2. Do not treat the recommended 10-pixel
  figure or the imported→live flip as decided in this item.
- Offline. No network. No analytics area id.

## Acceptance criteria

1. On first recording, the 100 m badge appears with incomplete progress.
2. It completes at the **locked** SP-062 M2 threshold of new live pixels
   and does not reappear.
3. Imported-only collection does not advance it.
4. Paused / idle session does not advance it.
5. Incomplete progress still shows on a later recording session until M2.
6. Automated tests cover threshold (per locked M2), import-only exclusion,
   pause, once-complete, and cross-session incomplete.

## Required automated tests

- Progress reaches complete at locked M2 count; not before.
- Imported-only pixels do not increment.
- Appears on first `Recording` start; resume after pause continues;
  second session while incomplete still shows progress.
- Second session after complete: badge suppressed.
- Pause: no increment.

## Required manual validation

- Start first recording on a device or emulator; walk / simulate until
  complete; confirm map stays visible and badge does not return on next
  recording. Device residual → SP-069 / Phase 10.

## Failure and rollback considerations

- Prefer not showing the badge over showing it on every session.
- Do not block recording start on the badge.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| M2 count used | |
| Manual validation | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
