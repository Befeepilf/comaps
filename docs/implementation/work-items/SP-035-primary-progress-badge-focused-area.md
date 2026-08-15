# SP-035 — Primary progress badge bound to focused area

**Phase:** 5 — Area progress and map interaction
**Status:** Accepted 2026-08-07
**Branch:** `cursor/sp-035-focused-area-badge-191e`
**Depends on:** Phase 4 Accepted; SP-033 Accepted (partial); SP-034 Accepted
**Notes:** SP-037+ additionally note SP-033 LOD outcome; this item needs
  focused-area name + % only. Full §12.5 focus engine is SP-036 — temporary
  map-centre stub used until then.

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
| Branch | `cursor/sp-035-focused-area-badge-191e` |
| Test output | `street_pixels_tests` 196/196 (6 FocusedAreaBadge_* / LookupExplorationAreaAtPoint_*); `street_pixels_areas_tests` 50/50 |
| Manual validation | Desktop interactive harness `focused_area_badge_desktop_demo` on DISPLAY=:1 — all 8 UI steps + scripted pass PASS (2026-08-07). Recording: `/opt/cursor/artifacts/sp035-focused-area-badge-desktop-demo.mp4`. Note: CoMaps Qt has no Street Pixels badge UI; harness exercises the same manager APIs Android binds. Helsinki device badge residual → SP-041 / Phase 10 |
| Accepted by | Maintainer (accept 2026-08-07) |
| Accepted date | 2026-08-07 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Temporary map-centre `TryFocusAtPoint` / JNI refresh until §12.5 | Replace in SP-036 |
| JNI spa path must use `ExplorationSidecarPathBesideMwm` for versioned installs | Fixed in review follow-up |
| R8 stripped JNI-only `FocusedAreaProgress` ctor → `mid == null` SIGABRT on test/release | Fixed with `@Keep` |
| Badge hides when no focus / blank name (no-area stub) | SP-040 empty-state polish |
| CoMaps Qt desktop has no Street Pixels badge chrome | Validated via `tools/focused_area_badge_desktop_demo`; product UI remains Android |
| Helsinki on-device badge spot-check | SP-041 / Phase 10 |
