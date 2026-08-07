# SP-035 — Primary progress badge bound to focused area

**Phase:** 5 — Area progress and map interaction
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** Phase 4 Accepted; SP-033 measurement recorded; SP-034
  completion API (or tightly sequenced same stack with SP-034 first)
**Notes:** SP-037+ additionally note SP-033 LOD outcome; this item needs
  focused-area name + % only.

---

## Objective

Bind the primary progress badge to the focused exploration area: show the area
**name** and personal completion **percentage**. Never present an MWM country
id as the area name.

## Motivation

Spec §7 / §12 require the focused area's name and percentage in the primary
badge. Today progress is MWM-scoped and ExploreStats regions use `countryId`.
Phase 4 `DisplayName` exists but is not wired to the badge.

## In-scope behavior

- Primary badge displays focused area `DisplayName` + area-scoped %.
- Require `StreetPixelsManager::IsAreaCompletionCacheValid()` before trusting
  `GetAreaCompletionFraction` (invalid cache fail-closes to 0 and must not be
  shown as a real 0% without rebuild).
- Empty / no-area handling may stub toward SP-040; must not show MWM id.
- Numeric/name updates when focus changes (focus engine may land in SP-036;
  this item supplies the binding surface).
- Android V1 UI wiring to shared completion + focus state.

## Out-of-scope behavior

- Implementing all five §12.5 focus rules (SP-036).
- Area boundary shading (SP-037).
- Detail surface / tap selection (SP-038).
- City summary badge (SP-039).
- Milestones (Phase 7).

## Relevant product requirements

- Spec §7 Focused area; §12.1; §34 Progress experience.
- Phase 4 exit #8 / `DisplayName` never MWM id.

## Relevant source files or symbols

- `android/app/.../MwmActivity.java` street-pixels attach / explore UI
- `StreetPixelsManager` state callbacks
- `street_pixels::DisplayName`
- SP-034 completion cache API

## Implementation notes / constraints

- Do not start coding until SP-033 measurement is recorded.
- Prefer shared core for name/%; Android presents it.
- Animation of name/number transitions may be minimal in this item; polish can
  follow if discovered.

## Acceptance criteria

1. With a focused area present, badge shows that area's display name and correct
   percentage from SP-034.
2. No path shows MWM country id as the neighbourhood / area name.
3. When focus changes (even if driven by a temporary stub), badge updates.

## Required automated tests

- Display-name / percentage binding unit or instrumentation coverage as
  practical; at minimum shared-core tests if logic lives in C++.

## Required manual validation

- Focus a known Helsinki (or fixture) area; confirm badge name and % match.

## Failure and rollback considerations

- Prefer blank / no-area empty treatment over showing country id.

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
| | |
