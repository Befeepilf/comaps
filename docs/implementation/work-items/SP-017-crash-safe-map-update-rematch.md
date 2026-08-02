# SP-017 — Crash-safe rematch on map update

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Replace wipe-on-download with rematch: when a country MWM updates, derive the
new valid pixel universe, intersect with previously explored HEALPix ids
(preserving the ever-live bit), and commit only when the new state is durable.
Survive process death mid-migration.

## Motivation

SPD-013 and spec §27 forbid deleting personal exploration on map update.
`Framework::OnCountryFileDownloaded` still calls `CleanupStreetPixels`, which
deletes `.pix` / `.pixa` and `street_exploration` rows. `processed_tracks` is
**not** cleared, so track replay usually cannot rebuild what was wiped.

## In-scope behavior

- On successful country download/update: rematch instead of deleting explored
  state.
- Sequence: scan old `.pix` for explored entries (id + ever-live bit from
  SP-016) → derive new valid set → intersect → write new `.pix` (SP-015
  header + map-data version) → restore explored + ever-live for surviving
  ids → commit via temp + atomic replace.
- **Size/RAM (Uusimaa ~50 MB):** stream the old file; keep an explored-only
  working set (not a second full-universe mirror, not SQLite per cell). Avoid
  peak RAM ≈ 2–3× `.pix` if a simpler approach would do that. Background
  thread; map usable with updating/progress hook (copy in SP-021).
- Reconcile `processed_tracks` so rematch cannot strand users behind stale
  geometry hashes (clear-per-country, or equivalent policy recorded in
  evidence).
- Interrupted-migration recovery: relaunch finishes or rolls back to the last
  durable explored set — never empty progress without backup.
- Unit tests with synthetic old/new sets covering unchanged, removed, and
  added cells; plus a large-fixture or chunked test that fails designs which
  allocate O(universe) twice carelessly.

## Out-of-scope behavior

- Map delete / redownload permanence policy implementation beyond what rematch
  needs on the download path (SP-018 owns delete/deregister).
- Sampling distance change (SP-019) — coordinate release timing only.
- Eligibility tightening (SP-020).
- User-facing “more to explore” copy polish (SP-021); a progress flag/API is
  in scope if rematch needs it.
- Area reassignment (Phase 4).

## Relevant product requirements

- §3.6 Permanent personal progress.
- §27.1–§27.5 Updates, recalculation, communicating reductions.
- SPD-013.

## Relevant source files or symbols

- `libs/map/framework.cpp` — `OnCountryFileDownloaded`
- `libs/map/street_pixels_manager.{hpp,cpp}` — `CleanupStreetPixels`, derive,
  load/save
- `libs/map/street_stats_db.*` — `DeleteMwmData`, `processed_tracks`
  (reconcile with rematch; no source/ever-live table — SPD-015)
- `libs/map/street_pixels_tests/*`

## Implementation notes / constraints

- Depends on SP-015 (header + map-data version) and SP-016 (ever-live bit).
- Follow audit §14 rematch idea; ever-live comes from old `.pix` bits (SPD-015),
  not a side table. Do not invent wipe-and-replay as the permanence mechanism.
- `.pixf` may keep being deleted if present (SPD-018).
- Measure rematch wall time and approximate peak extra RAM on Uusimaa-class
  data; record in evidence (device completion in SP-022).

## Acceptance criteria

1. Updating a country rematches explored ids; surviving cells stay explored.
2. Ever-live bit for surviving cells is preserved (copied from old `.pix`).
3. Cells removed from the valid universe disappear; new cells appear
   unexplored; explored count equals intersection size.
4. Crash/interrupt at each documented stage loses no prior durable explored
   state (automated).
5. Download path no longer uses delete-first cleanup for exploration.
6. `processed_tracks` policy after rematch is implemented and tested.
7. Rematch does not require a second durable full-universe copy beyond the
   atomic temp+replace window; peak extra RAM strategy is documented.
8. Covered by `street_pixels_tests`.

## Required automated tests

- Unchanged / removed / added cell matrix.
- Ever-live bit persistence across rematch.
- Interrupted migration at each stage.
- `processed_tracks` does not block recovery after rematch.

## Required manual validation

- Explore a known area on device; apply a real country update (or staged
  replacement); confirm greens that still exist remain green.

## Failure and rollback considerations

- Never leave the user with zero exploration because a rematch failed; keep
  previous `.pix` + source until new files are durable.
- If rematch is too slow on large countries, report measurement; do not silently
  shrink scope.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| processed_tracks policy | |
| Rematch timing notes | |
| Test output | |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
