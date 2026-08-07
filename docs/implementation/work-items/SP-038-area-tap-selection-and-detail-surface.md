# SP-038 — Area tap selection and focused-area detail surface

**Phase:** 5 — Area progress and map interaction
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** Phase 4 Accepted; SP-033 measurement recorded; SP-034
  completion; SP-036 focus engine (or sequenced to feed it); SP-037 boundaries
  helpful for affordance
**Notes:** Polygon hit-test, **not** street-pixel picking. SP-037+ note SP-033.

---

## Objective

Let the user tap an exploration area to focus it and open a focused-area detail
surface showing the exact personal completion percentage (and name). Selection
uses **polygon hit-testing**, not pixel picking.

## Motivation

Spec §12 / §7: tapping an area focuses it and reveals exact percentage. Audit
notes focused-area details as a new screen. Pixel picking would be wrong
geometrically and would miss unexplored cells.

## In-scope behavior

- Polygon hit-test against Phase 4 exploration rings (smallest / §8.8-consistent
  choice when nested — align with assignment rules).
- Tap sets focus via SP-036 input path.
- Focused-area detail surface: name, exact %, and any minimal V1 fields needed
  for understanding (no competition boss UI).
- No-area taps / empty handling coordinated with SP-040.

## Out-of-scope behavior

- Street-pixel hit testing.
- Milestones / share cards (Phase 7).
- Competition ownership panels (Phase 8).
- Country/world drill-down.

## Relevant product requirements

- Spec §7; §12 tap-to-focus; §31 empty state coordination.
- Phase 4 polygons + `DisplayName`.

## Relevant source files or symbols

- Android map tap / overlay picking hooks
- Phase 4 PIP / ring geometry
- SP-036 focus API; SP-034 completion API

## Implementation notes / constraints

- Do not start coding until SP-033 measurement is recorded.
- Hit-test must work offline on installed sidecar geometry.
- Prefer shared hit-test helper in C++ with Android UI shell.

## Acceptance criteria

1. Tapping inside an area focuses that area (nested case follows assignment
   smallest-polygon / stable rules).
2. Detail surface shows correct name and exact %.
3. No reliance on street-pixel picking.
4. Tap outside areas yields no-area / clear focus behaviour (with SP-040).

## Required automated tests

- Polygon hit-test fixtures (nested, adjacent, outside).

## Required manual validation

- Tap several known areas; confirm focus + detail %.

## Failure and rollback considerations

- Ambiguous taps: fail toward clear no-selection or documented nested rule —
  never invent an area.

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
