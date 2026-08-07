# SP-043 — Freeze production `.spa` blob contract

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** In review
**Branch:** `cursor/sp-042-sidecar-shipping-fe62`
**Depends on:** SP-042 In review (SPD-032 freeze gate); SPD-017 (`nside`);
  SP-026 / SP-028 format notes
**Unblocks:** SP-044 (mapgen emit against frozen contract); SP-045–048 consume

---

## Objective

Freeze the production `.spa` header fields for HEALPix `nside`, universe order,
and `format_version` so generator and client cannot silently disagree before CDN
publish (SPD-032 → SPD-034).

## Motivation

SP-026 / SP-028 documented ascending NEST slot order informally; the header did
not encode `nside` or an ordering tag. Shipping assignment blobs without that
freeze risks irreversible mismatched assignment.

## In-scope behavior

- Bump `kSpaFormatVersion` **1 → 2**.
- Append to `SpaHeader` after `index_width` (little-endian): `uint32 nside`
  (**1048576**), `uint8 universe_order` (**1** = AscendingNest),
  `uint8 reserved[3]` (**0** on write; reject non-zero on read).
- Constants: `kSpaNside`, `kSpaUniverseOrderAscendingNest`.
- Writer always emits v2 with production values.
- Reader fail-closed rules: v2 validates fields; v1 + `assign_count==0`
  geometry-only dual-read; v1 + `assign_count>0` reject on production load.
- Append **SPD-034** to `DECISIONS.md`; update SP-026 / SP-028 notes
  (AscendingNest / `ScanUniverseAscending`; remove unsorted emit order as
  production contract).
- Create this work-item file; README / phase-04 index → **In review**.

## Out-of-scope behavior

- Wiring production mapgen collectors → `.spa` emit (**SP-044**).
- `countries.txt` / download / storage lifecycle (**SP-045–047**).
- Editing product spec or technical audit.
- Marking this work item Accepted.
- Inventing grids or changing `nside`.

## Relevant product requirements / decisions

- SPD-017, SPD-021, SPD-022, SPD-032, **SPD-034**.
- Product spec §8.8; notes SP-026 / SP-028.

## Relevant source files or symbols

- `libs/street_pixels_areas/areas_format.hpp` — `kSpaFormatVersion`, `kSpaNside`,
  `kSpaUniverseOrderAscendingNest`
- `libs/street_pixels_areas/areas_types.hpp` — `SpaHeader`
- `libs/street_pixels_areas/areas_serdes*.hpp/cpp` — Write/ReadSpaHeader
- `libs/street_pixels_areas/areas_writer.cpp` / `areas_reader.cpp`
- `libs/street_pixels_areas/exploration_sidecar.cpp` (load facade)
- `libs/street_pixels_areas/street_pixels_areas_tests/spa_serdes_tests.cpp`

## Acceptance criteria

1. Writer emits format_version 2 with `nside=1048576`, AscendingNest, reserved 0.
2. Reader rejects bad nside / universe_order / non-zero reserved (fail-closed).
3. Reader accepts v1 geometry-only (`assign_count=0`); rejects v1 with
   `assign_count>0` on production load paths.
4. SPD-034 recorded; SP-026 / SP-028 notes aligned; this file Status **In review**.
5. `street_pixels_areas_tests` green with new contract cases.
6. Maintainer decides acceptance; agent does not mark Accepted.

## Required automated tests

- Round-trip v2 header fields.
- Reject bad nside / universe_order / non-zero reserved.
- Accept v1 geometry-only; reject v1 with `assign_count>0`.
- Existing suite still passes.

## Required manual validation

- Maintainer review of SPD-034 against SPD-032 / D6.

## Failure and rollback considerations

- Do not weaken fail-closed rules to keep legacy assigning v1 blobs.
- Do not change `nside` or invent alternate grids.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `9fadc1be9` (code+tests); `da2c84209` (docs) |
| Decision ids | SPD-034 (implements SPD-032 freeze) |
| Format | `kSpaFormatVersion=2`; `nside=1048576`; `universe_order=AscendingNest(1)` |
| Test output | `./tools/unix/build_omim.sh -d -p /workspace street_pixels_areas_tests`; `./omim-build-debug/street_pixels_areas_tests` — **73/73 OK** (All tests passed.) |
| Docs touched | `DECISIONS.md`; notes SP-026 / SP-028; README; phase-04; this file |
| Implemented by | Agent |
| Accepted by | — |
| Accepted date | — |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Production leaf `.spa` emit (Option B offline batch; Option A residual) | [SP-044](SP-044-production-spa-emit.md) |
| Geometry-only v1 dual-read retained for fixtures / SP-032 harness | Retire when harness/emit always writes v2 (no blocker) |
