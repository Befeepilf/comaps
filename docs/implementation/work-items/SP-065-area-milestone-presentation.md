# SP-065 — Area milestone presentation (25 / 50 / 100)

**Phase:** 7 — Milestones and share cards
**Status:** In review
**Branch:** `cursor/sp-065-area-milestone-presentation-c417`
**Depends on:** SP-062 M5 (queue / non-interrupt) and M7 (competition stub);
  SP-063 fired-state; SP-034/035 focused-area badge. SP-067 compositor may
  land after: 100% must offer a card surface, which SP-067 fills.
**Notes:** OQ-13 / OQ-15 are SPD-050 / SPD-052 Accepted (2026-08-19 via SP-062). Presentation queue is this item; SP-063 records fires only.
**Unblocks:** SP-067 card surface; SP-069 exit #1, #2, #5

---

## Objective

Present 25%, 50%, and 100% area celebrations once per area per threshold:
small non-blocking acknowledgments, never interrupting active routing, never
requiring immediate interaction.

## Motivation

Phase 5 shipped focused-area % and completed chrome (§18.6). Nothing fires
when a threshold is crossed. Spec §18.2–§18.4 is the restrained reward
layer.

## In-scope behavior

- When SP-063 records a new 25% fire: non-blocking copy
  “25% of {DisplayName} explored” and a brief animation around the
  progress badge. No extra haptic pattern (spec §28.3 lists 50% / 100% /
  first-100 m, not 25%).
- 50%: “Half of {DisplayName} explored”; slightly more visible animation.
  Emit a 50% haptic **event** for SP-066. Do **not** call
  `TriggerCollectionVibration`.
- 100%: “{DisplayName} fully explored”; completed visual already from
  SP-040; add the celebration (outline glow/fill pulse allowed on top of
  existing completed chrome); show the completion-card surface with Share
  **chrome** (the tap → system sheet is SP-068). Image body: until SP-067,
  **copy-only or non-map card chrome**. **Never** capture `MapView`, Drape,
  the position marker, the recorded track, or explored pixels. Real image
  is SP-067.
- Emit a 100% haptic event for SP-066 (same rule: no
  `TriggerCollectionVibration`).
- Never interrupt `RoutingManager::IsRoutingFollowing` / navigation:
  no modal that steals the nav UI; no auto-pause; no forced stop of
  guidance. Queue until it is safe to show, or show as a non-blocking
  overlay that does not cover manoeuvres.
- Multi-area / multi-threshold in one session: follow SP-062 M5 (queue;
  100% > 50% > 25%; one at a time).
- §27.4: if an area was previously 100% and is now below, the detail /
  badge path may state it was previously completed. No achievement-history
  screen.
- Anonymous first-person 100% copy when competition is off (§19.2 /
  §22.10). Leading / not-leading competition sentences are a stub until
  Phase 8; do not imply completion was invalid.
- `DisplayName` never falls back to MWM id (Phase 4 / SP-035 rule).
- City-summary rollup must not fire these celebrations or a city card.

## Out-of-scope behavior

- Fired-state persistence (SP-063).
- First-100 m badge (SP-064).
- Haptic waveforms (SP-066).
- Pixel-accurate card image and deny-list compositor (SP-067).
- Opening the OS share sheet (SP-068). Auto-open is forbidden. “Card
  generated” counter is SP-068.
- Achievement list / trophy cabinet.
- Boss / contested chrome (Phase 8).
- City-summary 25/50/100 celebrations or a city completion card.

## Relevant product requirements

- Spec §18.2–§18.6, §19.2–§19.3, §22.10, §27.4, §34 Progress experience.
- SP-062 M5, M7.

## Relevant source files or symbols

- `FocusedAreaProgress`, `MapButtonsController.mExplorationBadge`
- `FocusedAreaDetailBottomSheet`
- `street_pixels::DisplayName`
- `Framework.nativeIsRoutingActive` / `RoutingManager::IsRoutingFollowing`
- SP-040 completed overlay style
- No milestone presentation symbol (2026-08-19)

## Implementation notes / constraints

- Shared C++ owns *what* fired and queue order; Android renders.
- Offline. Copy uses the local display name only.
- Share split: this item = surface + Share chrome + no auto-open.
  SP-067 = deny-list model + raster. SP-068 = tap → `ACTION_SEND` image
  only, plus “card generated” / “share initiated” counts.
- Do not generate a map-derived placeholder PNG.

## Acceptance criteria

1. 25 / 50 / 100 each show the specified acknowledgment once per area.
2. Celebrations are non-blocking and do not interrupt active following
   navigation.
3. Three fires in one update present without dropping 100%.
4. No achievement-history screen is introduced.
5. 100% offers a card surface and Share action without auto-opening the
   system share sheet.
6. Competition-off copy is first-person; no “invalid completion” wording.

## Required automated tests

- Presentation dispatcher: 25/50/100 mapping; queue order; skip when
  already shown this crossing (depends on SP-063).
- Following-navigation: celebration does not call into route-stop /
  follow-disable APIs.
- 100% fire / animation end does not invoke share.
- DisplayName: no MWM id.
- City-summary updates do not enqueue area celebrations.

## Required manual validation

- Complete a small fixture/test area while not navigating; confirm 100%
  celebration and that the map remains usable.
- Repeat with navigation following on; confirm guidance is not dismissed.
  Device residual → SP-069 / Phase 10.

## Failure and rollback considerations

- Prefer a quieter toast over a modal that breaks navigation.
- Do not auto-open share.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-065-area-milestone-presentation-c417` |
| Test output | `street_pixels_tests --filter=AreaMilestonePresentation` **14/14**; `street_pixels_areas_tests --filter=AreaMilestone` **8/8**; `street_pixels_tests --filter=AreaMilestone` **18/18**; `street_pixels_tests --filter=FocusedArea` **6/6** |
| Manual validation | Device residual → SP-069 / Phase 10 |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Area-milestone haptic is a C++ handler stub (`SetAreaMilestoneHapticHandler`). | SP-066 implements waveforms and the foreground/recording/toggle predicate. |
| 100% card is copy-only (title + body + Share chrome). | SP-067 fills the deny-list image. |
| Share button is enabled with an empty click listener. | SP-068 opens `ACTION_SEND` and counts. |
