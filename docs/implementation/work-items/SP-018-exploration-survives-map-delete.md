# SP-018 — Explored state survives map delete and redownload

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** In progress
**Branch:** `street-pixels`

---

## Objective

When the user deletes a country map and later redownloads it, personal explored
HEALPix ids and the ever-live bit rematch onto the new derived universe instead of
being lost.

## Motivation

Spec §3.6 permanence is not limited to in-place updates. Today
`OnCountryFileDelete` and `OnMapDeregistered` call `CleanupStreetPixels`, which
destroys exploration artifacts. SPD-016 requires survival on delete +
redownload via a compact explored-only archive so users can free a ~50 MB
regional `.pix` without losing progress.

## In-scope behavior

- On map delete/deregister: remove the full valid-universe `.pix` (frees tens
  of MB) but write a **compact explored-only archive** (HEALPix id + packed
  explored/ever-live flags per explored cell) — SPD-016.
- On later redownload: rematch (SP-017) from that archive onto the new
  universe, then drop or replace the archive as appropriate.
- Decision already in `DECISIONS.md` (SPD-016).
- Tests for delete → redownload retaining intersection exploration; assert
  archive size ≪ full `.pix` for sparse exploration.

## Out-of-scope behavior

- Changing rematch algorithms beyond hooks needed for retained explored sets
  (SP-017).
- UI copy (SP-021).
- Cloud backup / cross-device sync (post-V1).

## Relevant product requirements

- §3.6 Permanent personal progress.
- §27 map-data updates (redownload is an update path).

## Relevant source files or symbols

- `libs/map/framework.cpp` — `OnCountryFileDelete`, `OnMapDeregistered`
- `libs/map/street_pixels_manager.*` — `CleanupStreetPixels`
- Ever-live bit from SP-016; rematch from SP-017; compact archive format

## Implementation notes / constraints

- Depends on SP-016 and SP-017.
- Never retain a full regional `.pix` (~50 MB class) merely to remember
  exploration after map delete.
- Depends on SP-016 ever-live bit layout and SP-017 rematch entry points.

## Acceptance criteria

1. Delete + redownload preserves explored ∩ new universe and the ever-live bit.
2. Post-delete retained archive is explored-only and much smaller than the full
   `.pix` for realistic exploration fractions.
3. Full `.pix` is not kept on disk solely because the map was deleted.

## Required automated tests

- Delete country artifacts → redownload/rematch → intersection explored
  preserved with ever-live bits (SPD-016).

## Required manual validation

- Delete a small country/region map on device, redownload, confirm prior greens
  that still exist remain green.

## Failure and rollback considerations

- Retained explored sets without a map must not crash country-switch UI.
- Storage growth of retained sets for many deleted countries should be noted in
  evidence if material.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | (uncommitted; proposed message below) |
| Decision id | SPD-016 |
| Test output | `ninja street_pixels_tests` OK. `./street_pixels_tests --filter=Archive` → All tests passed (EXIT_ARCHIVE=0): Roundtrip, SizeMuchSmallerThanFull, CleanupArchivesExplored, CleanupIdempotent, ScanFailKeepsPix, CorruptPixKeepsPixAndArchive, ProbeRejectsArchiveMagicAsLegacy, RematchFallsBackToPixrWhenPixUnreadable, RedownloadRematchFromPixrOnly, RematchFailKeepsArchive, OrphanCleanup, LoadPathNoBlankDerive. `./street_pixels_tests --filter=Rematch` → All tests passed (EXIT_REMATCH=0) including SP-017 rematch suite. Full `./street_pixels_tests` → **All tests passed.** (EXIT_ALL=0). |
| Manual validation | Deferred to SP-022 / device delete+redownload check |
| Implemented by | Agent |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `Archive_LoadPathNoBlankDerive` exercises rematch-from-archive seed (same entry as load recovery) rather than full `LoadStreetPixels` with a real MWM; load path still refuses blank `SaveUnexploredIds` when `.pixr` is present. | Accept for SP-018; optional MWM-backed load integration later. |
| Production `CleanupStreetPixels` remains background-async; sync `CleanupStreetPixelsForTesting` used by unit tests. | Accept; pre-existing async pattern. |
| Many deleted countries retain one `.pixr` each (explored-only); growth is O(explored) not O(universe). | Note for storage monitoring; no change. |
| Review fixed: `ScanExploredEverLive(Corrupt)` previously returned empty success (Cleanup deleted `.pixr`); archive magic probed as Legacy; unreadable `.pix`+`.pixr` did not fall back on rematch/load. | Fixed in this implementation; covered by CorruptPix / ProbeRejects / RematchFallsBack tests. |
