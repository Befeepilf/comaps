# SP-063 — Milestone state tracking

**Phase:** 7 — Milestones and share cards
**Status:** In review
**Branch:** `cursor/sp-063-milestone-state-aee9`
**Depends on:** SP-062 locks for store location, key (OSM id), re-fire
  policy, and 100% completion-date persistence. Phase 5 area completion
  cache (SP-034) as the percentage source.
**Notes:** SPD-048 / SPD-049 Accepted 2026-08-19 via SP-062.
**Unblocks:** SP-065–069 (fired-once and §27.4 survival). Not a
  prerequisite for SP-064 (install-scoped first-goal).

---

## Objective

Persist which area-milestone thresholds have fired, plus the original 100%
completion date, keyed stably across map updates, so celebrations fire once
per area per threshold and §27.4 previous-completion remains available.

## Motivation

`AreaCompletionCache` is a derived cache rebuilt from `.pix` + `.spa`. It
has no fired-state and is invalidated on collect/rematch. Without a separate
store, 25/50/100 would re-fire after every cache rebuild or map update.

## In-scope behavior

- Shared C++ store (SPD-002) for, per exploration area:
  - which of 25 / 50 / 100 have already fired
  - original 100% completion timestamp (local, once)
- Key by the identifier SP-062 locked (recommended: OSM id). Never MWM
  country id. Compact index may be a cache hint only if the stable key is
  also stored.
- Observe area-scoped fractions from `AreaCompletionCache` after invalidation
  / rebuild. Crossing a threshold that has not fired records the fire.
  Already-fired thresholds do not re-record (M4).
- Three thresholds crossed in one update all record as fired (presentation
  queue is SP-065).
- Survive simulated rematch, policy-version change, and `.spa` refetch
  without resetting fired-state or the original date (spec §27.4).
- Query API: given an area, return fired set + optional original 100% date.
  Query: areas that were previously 100% and are now below (for §27.4 copy
  in SP-065 / detail surface).
- Crash-safe write (same discipline as other local Street Pixels stores).

## Out-of-scope behavior

- UI / celebration chrome (SP-065).
- First-100 m onboarding state (SP-064) — may live in the same module if
  natural, but first-goal is install-scoped, not per-area.
- Card compositor and share (SP-067 / SP-068).
- Achievement history screen (forbidden, §18.5).
- City-summary rollup thresholds (`FocusedAreaProgress.m_citySummary` /
  SP-039). Spec §18.1 is per exploration area. A city badge hitting
  25/50/100 must not fire area celebrations or a city share card.
- Re-opening M4.

## Relevant product requirements

- Spec §18.1–§18.5, §27.4.
- SPD-008 (cards in V1); SP-062 M3/M4/M6.
- Phase 7 exit #1 and #8.

## Relevant source files or symbols

- `street_pixels::AreaCompletionCache` / `StreetPixelsManager::GetAreaCompletion`
- `ExplorationArea::m_osmId`, `m_compactIndex`
- `libs/map/street_stats_db.*` (do not overload feature-bitmask tables)
- `settings.ini` via `settings::Get` / `Set` (unbounded rows: likely wrong)
- Rematch / `InvalidateAreaCompletionCache` call sites

## Implementation notes / constraints

- Offline-only. No upload of fired-state or dates.
- Imported pixels count toward personal % (SPD-026) and therefore can
  cause a threshold to be crossed; that is correct for personal milestones.
  They must not be treated as competition (unchanged invariant).
- Do not reset on `InvalidateAreaCompletionCache`.
- Generated card images are not this store.

## Acceptance criteria

1. Crossing 25%, 50%, and 100% records each threshold exactly once per area.
2. Re-crossing after dropping below (including simulated map update) does
   not record a new fire.
3. Three thresholds crossed in one update all record.
4. Original 100% date is stored once and survives a drop below 100%.
5. Automated tests cover fire-once, no-refire, triple-cross, and rematch
   survival.
6. Keying matches SP-062 (OSM id unless that lock was revised).

## Required automated tests

- Fire-once per area per threshold; no re-fire; triple-cross in one update.
- Fired-state and original date survive simulated rematch / cache rebuild.
- Same OSM id with a changed compact index still matches (if M3 keys on
  OSM id).
- Imported pixels can cause a personal threshold to be crossed (SPD-026);
  that is not a competition event.
- Zero-total area does not fire (fraction 0).
- City-summary fraction does not write area fired-state.

## Required manual validation

- Device residual → SP-069 / Phase 10. Desktop fixtures are the gate.

## Failure and rollback considerations

- Prefer missing celebrations over re-firing or showing an achievement list.
- Do not drop fired-state on map update to “give the user the celebration
  again”.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-063-milestone-state-aee9` |
| Test output | `street_pixels_areas_tests` **111/111** (`AreaMilestone_*` 6/6); `street_pixels_tests --filter=AreaCompletion\|AreaMilestone` **15/15** |
| Store location / key | `area_milestones.db` (SQLite WAL); OSM id + `fired_mask` + `completed_100_at` |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
