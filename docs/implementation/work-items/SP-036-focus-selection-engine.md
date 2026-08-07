# SP-036 — Focus-selection engine (§12.5)

**Phase:** 5 — Area progress and map interaction
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** Phase 4 Accepted; SP-033 measurement recorded; area membership
  from Phase 4; SP-034/035 for %/badge consumers preferred
**Notes:** Depends on SP-033 gate before coding (Phase 5 entry rule).

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
| Branch | |
| Test output (§12.5 cases) | |
| Manual validation | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
