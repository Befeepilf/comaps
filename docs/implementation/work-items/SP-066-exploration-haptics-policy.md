# SP-066 — Exploration haptics policy

**Phase:** 7 — Milestones and share cards
**Status:** Planned
**Branch:**
**Depends on:** SP-062 M9. Recording gate already in
  `OnLocationUpdate` (SP-007). Foreground signal:
  `OrganicMaps.nativeOnTransit` → `Framework::EnterForeground/Background`.
  First-goal / 50% / 100% **events** from SP-064 and SP-065 (waveforms may
  land after those items; the predicate and collection pulse do not wait).
**Notes:** Coding waits on OQ-17 (draft SPD-054).
**Unblocks:** SP-069 exit #7; first-goal / 50% / 100% stronger patterns

---

## Objective

Make exploration haptics match spec §28: foreground plus recording only,
one pulse per collecting update, stronger patterns for first-100 m / 50% /
100%, and a single **Exploration haptics** settings toggle (default on).

## Motivation

`StreetPixelsManager::TriggerCollectionVibration` already runs on the
collection path (so Phase 2's session gate stopped out-of-session
vibration as a side effect). It still: ignores foreground; vibrates once
per new pixel when `numNewlyExploredPixels > 1`; has no toggle; has no
milestone patterns.

## In-scope behavior

- Shared predicate: pulse iff
  1. recording session is `Recording` (not Idle/Paused/Finished/Discarded),
  2. application is foreground (not screen-off / background),
  3. exploration-haptics setting is on (default on).
- Collection pulse: one subtle pulse when an accepted foreground update
  collects at least one **new** pixel. Not one pulse per pixel. Not a
  pattern whose count equals `numNewlyExploredPixels`.
- Milestone patterns (stronger, still gated by the same predicate except
  as specified): consume first-100 m complete, 50% area, and 100% area
  **events** from SP-064/065. 25% has no extra pattern. Boss pattern out
  of scope (Phase 8). Spec §28.3 says “may”; this item still ships the
  three stronger patterns so exit #7 is not collection-pulse-only.
- Single settings toggle **Exploration haptics** on the Android settings
  surface (Interface or equivalent). Shared C++ owns the persisted flag
  (SPD-002). No extra strength sliders.
- Android supplies foreground to the shared core (existing transit
  already calls `EnterForeground` / `EnterBackground`).
- Tests as a **pure predicate** plus manager-level: recording+foreground+on
  → one pulse per update; background → none; not recording → none; toggle
  off → none including milestones; multi-pixel update → one collection
  pulse; first-goal / 50% / 100% events → stronger pattern once when
  foreground + toggle on.

## Out-of-scope behavior

- Multiple haptic strength settings (§28.4).
- Boss haptic (Phase 8).
- Using haptics for routing or unrelated UI.
- iOS `UIFeedbackGenerator` wiring.

## Relevant product requirements

- Spec §10 step 8, §28.1–§28.4, §34 Progress experience.
- SP-062 M9.

## Relevant source files or symbols

- `StreetPixelsManager::TriggerCollectionVibration`
- `platform::Vibrate` / `VibratePattern` (`libs/platform/vibration.*`)
- `Utils.vibrate` / `vibratePattern` (Android JNI)
- `OrganicMaps.nativeOnTransit`
- `android/app/src/main/res/xml/prefs_interface.xml` (candidate toggle)
- No “Exploration haptics” preference (2026-08-19)

## Implementation notes / constraints

- Keep the collection-path call site; change the policy, not a second
  vibrate from Java on every GPS tick.
- Screen off is background for this predicate even if the recording
  foreground-service is running.
- Do not log coordinates alongside vibrate.

## Acceptance criteria

1. Recording + foreground + toggle on + ≥1 new pixel → exactly one
   collection pulse per update.
2. Recording + background or screen-off → no exploration haptic.
3. Not recording (including paused) → no exploration haptic.
4. Toggle off suppresses collection and milestone patterns.
5. Multi-pixel updates do not produce per-pixel pulses.
6. Foreground + toggle on + first-goal complete / 50% / 100% event →
   stronger pattern once; 25% has no extra pattern.
7. Automated predicate / manager tests exist.

## Required automated tests

- Pure predicate matrix (recording × foreground × toggle × new-pixel
  count).
- Manager: `numNewlyExploredPixels` of 0, 1, and >1.
- Milestone pattern does not fire when toggle off or backgrounded.
- First-goal complete / 50% / 100% events fire the stronger pattern once
  when allowed; 25% does not.

## Required manual validation

- Record in foreground: feel one pulse on collect.
- Screen off / app backgrounded while recording: no pulse.
- Toggle off: no pulse.
  Device residual → SP-069 / Phase 10.

## Failure and rollback considerations

- Prefer no haptic over vibrating in the background (battery and spec).
- Do not re-introduce per-pixel patterns.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Manual validation | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
