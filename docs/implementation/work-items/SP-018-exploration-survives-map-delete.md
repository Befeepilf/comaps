# SP-018 — Explored state survives map delete and redownload

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Planned
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
| Branch | |
| Commits | |
| Decision id | |
| Test output | |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
