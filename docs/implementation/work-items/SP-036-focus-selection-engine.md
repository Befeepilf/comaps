# SP-036 — Focus-selection engine (§12.5)

**Phase:** 5 — Area progress and map interaction
**Status:** Accepted 2026-08-07
**Branch:** `cursor/sp-036-focus-selection-engine-191e`
**Depends on:** Phase 4 Accepted; SP-033 measurement recorded; area membership
  from Phase 4; SP-034/035 for %/badge consumers preferred
**Notes:** SP-033 qualitative gate Met; quantitative Spike 1 → Phase 10.

---

## Objective

Implement the focused-area selection engine so behaviour matches all five rules
in product spec §12.5, each covered as a separate automated test case.

## Motivation

Focus drives the primary badge and detail surface. Incorrect focus makes
percentage changes look like lost progress. Spec §12.5 is the contract.

## In-scope behavior

- Shared focus-selection engine implementing **all five** §12.5 rules.
- Separate automated cases per rule (including recentre-returns-to-current-area).
- Inputs: user location during recording / idle, map centre, tap-selected area
  (tap wiring may complete in SP-038), zoom context as required by the rules.
- Predictable transitions when the user crosses area boundaries or pans.

## Out-of-scope behavior

- Badge chrome (SP-035) beyond consuming focus id.
- Polygon hit-test implementation details (SP-038) — engine must accept an
  explicit selected-area input.
- City zoom summary badge (SP-039).
- Competition focus / boss overlays (Phase 8).

## Relevant product requirements

- Spec §12.5 focus behaviour rules (all five).
- Spec §7 focused area.

## Relevant source files or symbols

- New shared focus module (preferred under `libs/`)
- `StreetPixelsManager` / Android map centre and location feeds
- Phase 4 `ExplorationAreaResolver` for point→area

## Implementation notes / constraints

- Do not start coding until SP-033 measurement is recorded.
- Rules 1 and 2 can both seem to apply when map centre ≠ user during
  recording — implement per-spec and escalate product conflict if observed;
  do not silently pick one without recording.
- Offline-only; no network for focus.

## Acceptance criteria

1. Each of the five §12.5 rules has a dedicated passing automated case.
2. Focus id updates drive badge consumers without inventing areas.
3. No-area locations yield a defined no-focus / no-area signal (SP-040 may
   polish empty UI).

## Required automated tests

- Five separate §12.5 rule cases (+ any fixture helpers).

## Required manual validation

- Walk across a boundary; pan away and recentre; confirm focus matches rules.

## Failure and rollback considerations

- Do not weaken or merge rule tests to hide conflicts; report spec ambiguity.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-036-focus-selection-engine-191e` |
| Test output (§12.5 cases) | `street_pixels_areas_tests` 57/57 incl. FocusSelection_Rule1–5 + Rule1OverRule2 + NoArea; `street_pixels_tests` 199/199 incl. FocusEngine_Manager_* + FocusedAreaBadge_* |
| Manual validation | Automated engine + manager coverage for all five rules. Device walk/pan/recentre residual → SP-041 / Phase 10 (same pattern as SP-035 Helsinki badge) |
| Accepted by | Maintainer (accept 2026-08-07) |
| Accepted date | 2026-08-07 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Rules 1 vs 2 during recording + pan: user area wins | Recorded; escalate only if product wants otherwise |
| `kCityScaleMaxDrawScale = 12` provisional | SP-039 polish city-zoom band + true city rollup |
| City-summary fraction currently settlement-area counts | SP-039 aggregate completion |
| Explicit tap API ready; polygon hit-test not wired | SP-038 |
| Helsinki device walk across boundary / recentre | SP-041 / Phase 10 |
