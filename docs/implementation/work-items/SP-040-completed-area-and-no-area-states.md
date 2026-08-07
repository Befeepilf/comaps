# SP-040 — Completed-area visual state and no-area empty state

**Phase:** 5 — Area progress and map interaction
**Status:** Accepted 2026-08-07
**Branch:** `cursor/sp-040-completed-no-area-191e`
**Depends on:** Phase 4 Accepted; SP-033 measurement recorded; SP-034
  completion; SP-037 shading/boundaries preferred; SP-035/038 for badge/detail
**Notes:** Spec §18.6 and §31. SP-037+ note SP-033.

---

## Objective

Ship a distinct completed-area visual state (§18.6) that survives zoom changes,
and a designed "no selected exploration area" / no-area-here empty state (§31)
for badge and detail surfaces.

## Motivation

Users need clear feedback when an area is 100% and when they are outside any
exploration area. Spec forbids inventing grid areas; empty state must not lie.

## In-scope behavior

- Distinct completed visual (e.g. green completion outline / check per §18.6)
  visible across relevant zooms.
- No-area empty state for badge and detail (§31): honest copy; no fake area;
  no MWM country id as a substitute name.
- Coordination with SP-034 100% detection and SP-036 no-focus signal.

## Out-of-scope behavior

- Milestone celebrations and share cards (Phase 7) — may store completion date
  locally if introduced here (§18.5); otherwise defer date persistence.
- Competition contested/unclaimed chrome (Phase 8).
- Invented fallback areas (SPD-006).

## Relevant product requirements

- Spec §18.6 Completed visual state; §31 empty state; §8.6 outside settlements.

## Relevant source files or symbols

- SP-037 overlay; SP-035 badge; SP-038 detail
- Phase 4 no-area assignment path

## Implementation notes / constraints

- Do not start coding until SP-033 measurement is recorded.
- Completed chrome must not destroy Spike 1 performance (reuse SP-037 LOD).
- Optional local 100% completion date: if added, document as new persisted
  state; otherwise leave to Phase 7.

## Acceptance criteria

1. A 100% fixture area shows the distinct completed visual at street and
   neighbourhood (and city if applicable) zooms.
2. Outside any area, badge/detail show the no-area empty state — no invented
   area, no MWM id as name.
3. Automated coverage for 100% vs in-progress vs no-area signals.

## Required automated tests

- Completion-state and no-area state unit tests on shared signals.

## Required manual validation

- Complete a small fixture/test area; confirm chrome; leave settlement; confirm
  empty state.

## Failure and rollback considerations

- Prefer missing chrome over implying an area exists when assignment is none.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-040-completed-no-area-191e` |
| Test output | `street_pixels_areas_tests` 67/67 (`AreaOverlay_StyleCompletedDistinctFromInProgress` + existing); `street_pixels_tests` 205/205 (`FocusedAreaBadge_NoAreaSignalNeverUsesMwmId`; 100% → `m_areaCompleted`; clear → `m_noExplorationArea`) |
| Manual validation | Device completed chrome + leave-settlement empty walk → SP-041 / Phase 10 |
| Completion-date persistence (yes/no) | No — deferred to Phase 7 |
| Accepted by | Maintainer (accept 2026-08-07) |
| Accepted date | 2026-08-07 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `m_showCheck` set for City/Neighbourhood completed styles; drape has no check glyph path yet | Residual — outline+restrained fill ships §18.6 core; check marker → polish / SP-041 |
| Overlay push still bakes Neighbourhood band colors (SP-037 stub) | Completed style still distinct at 100%; zoom-band retune optional |
| Device Helsinki completed / empty walk | SP-041 / Phase 10 |
