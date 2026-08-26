# SP-078 — Competition UI, card copy, and 30-pixel hint

**Phase:** 8 — Competition
**Status:** In progress
**Branch:** `cursor/sp-078-competition-ui-and-card-copy-f95c`
**Depends on:** SP-071 identity; SP-072 scores; SP-076 reads (stubs
  allowed with stale/offline labels); SPD-052 stub; SPD-066
**Unblocks:** SP-079

---

## Objective

Ship Android competition chrome: Explore vs Competition control, area
snapshot, ranking, overtaking hints, §22.10 completion-card lines, and
the §10 step 10 hint at 30 newly explored live pixels.

## Motivation

Phase 7 left `competitionLine` empty (SPD-052). Spec §10 step 10, §22.10,
and §23 are V1. Friends UI must stay hidden (SPD-061, SP-071).

## In-scope behavior

- Compact map control: **Explore** (default) vs **Competition**. Turning
  Competition on does not change red/green pixels into a territory skin
  (§23.1–§23.2).
- Area snapshot from SP-076: boss, contested, unclaimed, user score, gap
  to next relevant participant, personal completion (SPD-026, distinct
  from ownership). Offline/stale labelling (§26.2).
- Ranking snapshot: top three + current user, no duplicate (§23.3).
- Sparse-area copy when the server omits nicknames (§23.4). Never “someone
  is nearby”.
- Rate-limited overtaking hints from delayed aggregates (§23.5).
- Completion card: fill leading / not-leading sentences (§22.10). Card
  still works with no profile (SPD-052). Never imply personal 100% is
  invalid.
- Once-per-install hint after **30 newly explored live pixels**
  (SPD-066), non-blocking, no nearby-user language. Independent of the
  first-goal 10-pixel tracker.
- Competition settings: leave/delete actions from SP-077.

## Out-of-scope behavior

- Boss haptic (SPD-054).
- Friends (SPD-061).
- iOS.
- Drawing other users on the map.

## Relevant product requirements

- Spec §10 steps 10–12, §22.10, §23, §24 presentation, §26.2.
- SPD-052, SPD-054, SPD-061, SPD-066.

## Relevant source files or symbols

- `CompletionCardSource::m_competitionLine`, `ComposeCompletionCard`
- `FirstGoalTracker` (do not reuse threshold 10)
- Android map buttons / `MwmActivity` menu; area detail sheet from
  Phase 5
- `MyAccountDialogFragment` after SP-071

## Implementation notes / constraints

- Shared copy strings in C++ or Android resources; tests should lock
  §22.10 meaning, not only English pixels.
- Hints must not interrupt `IsRoutingFollowing` (same rule as SPD-050).
- Do not screenshot the map for competition chrome.

## Acceptance criteria

1. Explore remains default; competition overlay is readable on top of
   red/green pixels.
2. Card leading / not-leading copy; anonymous card still valid.
3. 30-pixel hint once; 10-pixel first-goal unchanged.
4. Sparse-area UI never shows other nicknames if the payload omitted
   them.
5. No nearby/live-location language in strings tests.

## Required automated tests

- Card copy: leading, not-leading, no-profile stub.
- Hint fires at 30 newly explored live pixels; not on import; does not
  reset first-goal.
- Ranking de-dup; stale flag presentation model.
- String deny-list: nearby, live location, exact coordinates.

## Required manual validation

- Device: opt-in walkthrough vs §20.2; card; hint; Explore/Competition
  toggle (SP-079).

## Failure and rollback considerations

- Prefer hiding competition chrome over showing nicknames from a sparse
  payload.
- Prefer empty `competitionLine` over copy that invalidates 100%.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
