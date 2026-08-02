# SP-021 — Denominator recalculation and §27.3 messaging

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

After a map update rematch, recalculate exploration denominators from the new
valid-pixel universe and surface the product's required framing when
percentages drop because there is more to explore — never as lost progress.

## Motivation

Spec §27.3 requires communicating reductions correctly. Country completion
today is `exploredCount / streetPixels.size()` from `.pix`. Rematch and 10 m
sampling change both numerator and denominator. Without messaging, users will
read a percentage drop as data loss — the failure mode SPD-013 calls out.

## In-scope behavior

- Ensure post-rematch fraction uses the new universe (verify; fix if any cache
  stalls).
- Detect previous completion vs new completion across an update when both are
  available (SP-015 map-data version helps).
- Android user-visible messaging aligned with §27.3 ("more to explore" /
  equivalent existing string patterns — prefer reuse).
- Hook to rematch progress/completion from SP-017.
- Tests for denominator change maths; UI string presence tests where the
  project already tests strings.

## Out-of-scope behavior

- Area-level denominators (Phase 5).
- Competition versioning UX (Phase 8).
- Rematch algorithm (SP-017).

## Relevant product requirements

- §27.3 Communicating reductions.
- §27.4 Previous completion.
- §3.6 Permanence framing.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.*` — `GetTotalExploredFraction`, rematch
  completion signals
- Android UI surfaces that show exploration percentage / update state
- `android/app/src/main/res/values/strings.xml`

## Implementation notes / constraints

- Depends on SP-017.
- Do not invent punitive copy; follow spec tone.
- If no percentage UI exists yet for country completion, provide the shared
  signal/API and a minimal Android notification or dialog — do not build Phase
  5 area UI.

## Acceptance criteria

1. After rematch, reported fraction matches explored ∩ new / |new|.
2. When percentage decreases due to added valid pixels, user-facing copy matches
   §27.3 intent.
3. Messaging never states that personal progress was deleted.
4. Automated coverage for fraction recalculation; manual check for copy.

## Required automated tests

- Synthetic rematch where denominator grows and numerator stays → lower
  fraction, explored count unchanged.
- Signal/API exposing previous vs new fraction for UI.

## Required manual validation

- Device: trigger update after exploration; confirm message and that greens
  remain.

## Failure and rollback considerations

- Missing UI surface is not an excuse to skip the shared signal; record gap if
  Android has nowhere to show it yet and add the smallest honest hook.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| String ids used | |
| Test output | |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
