# SP-016 — Per-pixel ever-live bit in `.pix`

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** In progress
**Branch:** `street-pixels`

---

## Objective

Persist, for every explored pixel, whether it is **ever-live** (competition-
eligible) or **imported-only**, using a single spare bit in the existing
`.pix` `int64_t` entry (SPD-015). Wire the live collection path to set the
bit. Add **zero** bytes to on-disk or mmap size beyond the format version
already required by SP-015.

## Motivation

Spec §15.2–§15.3 require distinguishing live from imported for competition.
V1 consumers need ever-live behaviour: imported-then-later-live must count as
live. A separate `both` state is unnecessary. One bit encodes that.

A HEALPix-keyed side table was drafted then rejected after measuring Uusimaa
`.pix` ≈ 50 MB. Two in-entry source bits were drafted then simplified to one
ever-live bit by maintainer decision.

## In-scope behavior

- Bit layout: bit 63 = explored (unchanged); one further high bit = ever-live;
  low bits = HEALPix id only.
- Semantics when explored:
  - ever-live `0` → imported-only
  - ever-live `1` → live-eligible (including former imported-only after a live
    collect)
- `df::StreetPixel` accessors: `GetPixelId()` masks flag bits; `IsEverLive` /
  setter (names as fit surrounding style); renderer colour/density still
  depend only on explored + id.
- Format version bump when the ever-live bit is written (coordinate with
  SP-015 header). Legacy explored entries with clear ever-live migrate as
  imported-only (safe default — excludes unknown provenance from competition).
- Live collection sets ever-live on explore / upgrade.
- Import / `MarkImported` explores with ever-live clear and **must not clear**
  an already-set ever-live bit.
- Interim bookmark-track replay policy until Phase 9: must not clear ever-live
  for live-session pixels; record policy in evidence.
- `msync` stays per-entry; no second file for provenance.

## Out-of-scope behavior

- Storing a distinct `both` or archaeological first-source enum.
- Side tables for this flag.
- Competitive recency timestamps (Phase 8; sparse later — never in `.pix`).
- Rematch (SP-017) and delete archive (SP-018).
- Full GPX import UX (Phase 9).
- Changing `nside` (SPD-017).
- Renderer visual changes beyond correct id masking.

## Relevant product requirements

- §7 Live exploration pixel vs explored pixel.
- §15.2–§15.3 Live vs imported; later live visits become competition-eligible.
- SPD-015.

## Relevant source files or symbols

- `libs/drape_frontend/street_pixel.{hpp,cpp}` — bit accessors
- `libs/map/street_pixels_manager.{hpp,cpp}` — mark explored paths
- `libs/map/street_pixels_tests/*` including `MakeStreetPixel` helpers

## Implementation notes / constraints

- Uusimaa-class files: no O(cells) SQLite write on explore.
- Unexplored entries keep ever-live clear (ignored until explored).
- SPD-015: bit means ever-live / live-eligible, not immutable first-source.

## Acceptance criteria

1. Newly live-explored pixels have ever-live set.
2. First import explores with ever-live clear; a subsequent live collect sets
   it; import never clears a set bit.
3. Flag survives process restart via the mmapped file.
4. `GetPixelId()` never returns flag bits; renderer density tests still pass.
5. On-disk `.pix` size for a fixed universe is unchanged by adding the bit
   (header from SP-015 may add a few tens of bytes only).
6. Interim track-replay policy documented and tested.
7. Covered by `street_pixels_tests`.

## Required automated tests

- First live explore → ever-live set.
- First imported → explored, ever-live clear; then live → ever-live set.
- Live then imported mark → ever-live remains set.
- Round-trip load/save preserves the bit.
- `GetPixelId` mask / low-zoom parent bucketing unaffected.
- Live path never clears ever-live.

## Required manual validation

- Short recorded walk; greens remain correct; dump/log one entry ever-live =
  true.

## Failure and rollback considerations

- Wrong `GetPixelId` mask would corrupt rendering density — treat as P0 in
  review.
- Rollback: ignore ever-live bit; explored MSB alone matches pre-SP-016.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | *(not committed — pending human)* |
| Bit layout constants | bit 63 explored `0x8000000000000000`; bit 62 ever-live `0x4000000000000000`; `GetPixelId` mask `0x3FFFFFFFFFFFFFFF`; format v2 (`kFormatVersionV2`); v1 load = imported-only (bit 62 clear) |
| Interim track-replay policy | Bookmark-track replay (`UpdateExploredPixels` → `MarkExploredPixelIds` / `MarkTrackPixelsForTesting`) uses imported-only semantics until Phase 9: `SetExplored(true)` only, never sets ever-live, never clears a set ever-live bit. `MarkTrackPixelsForTesting` delegates to `MarkImportedPixelsForTesting`. Covered by `EverLive_TrackAloneLeavesClear` and `EverLive_TrackAfterLiveRemainsSet`. |
| Test output | `ninja street_pixels_tests` OK. `./street_pixels_tests --filter=EverLive` → All tests passed (EXIT=0). `--filter=StreetPixel` → All tests passed (EXIT=0). `--filter=StreetPixelsFile` → All tests passed (EXIT=0). Full `./street_pixels_tests` → **All tests passed.** (EXIT_ALL=0). Includes `EverLive_*` (upgrade no double-count), `StreetPixel_*`, `StreetPixelsFile_*`, `StreetPixelsManager_LiveEverLiveSurvivesReload`. |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Live `OnLocationUpdate` can set ever-live in-place on an already-open v1 mmap without rewriting the header to v2. Save/migrate/new writes stamp v2. | Accept for SP-016 (load accepts v1+v2; bit is readable either way). Optional header bump on first ever-live write can be a later follow-up if reviewers want stricter format/bit coupling. |
| Review fixed live skip condition: bare `IsEverLive()` alone skipped corrupt ever-live-without-explored entries forever. Now requires explored∧ever-live. | Fixed in this change set. |
