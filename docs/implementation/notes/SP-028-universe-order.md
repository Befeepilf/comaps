# SP-028 — Universe order for dense subdivision assignments

Contract for consuming the generator-precomputed `assign` column (SPD-021).

## Slot ↔ HEALPix NEST ↔ assign[i]

For a valid-street-pixel universe **U**:

1. **U** is the ordered list of HEALPix **NEST** cell ids that form the
   exploration universe for that MWM leaf (same NEST scheme / `nside` as live
   street pixels: `nside = 1048576`).
2. **U is strictly ascending** by NEST id (same order as
   `street_pixels_file::ScanUniverseAscending`).
3. Dense column entry **`assign[i]`** is the compact area index (or
   no-subdivision sentinel) for **`U[i]`**.
4. Therefore: **slot `i` ↔ `U[i]` ↔ `assign[i]`**.

Client lookup:

- `LookupSubdivisionBySlot(file, i)` → assignable area for slot `i`, or none
  (sentinel / OOB / non-assignable target → none; never invents grids or
  settlements).
- `LookupSubdivisionByHealpix(file, U, nestId)` → fails closed if `U` size
  mismatches `assign`, or `U` is not strictly ascending (including duplicates);
  otherwise binary search on `U`, then slot lookup; unknown id → none.
- `SubdivisionAssignmentTable::TryLoad` binds a version-verified `.spa` to a
  caller-supplied `U` and **fails closed** on map/policy mismatch, size
  mismatch, or non-ascending `U`. Table healpix lookup then assumes the bound
  `U` (no per-call ascending re-scan).

## Header freeze (SPD-034 / SP-043)

Production `.spa` **`format_version = 2`** encodes `nside = 1048576` and
`universe_order = 1` (AscendingNest) after `index_width`. Writers always emit
those values; readers reject mismatched `nside` / `universe_order` / non-zero
`reserved`, and reject legacy `format_version = 1` when `assign_count > 0`.
Geometry-only v1 (`assign_count == 0`) remains dual-readable for fixtures /
offline harness blobs.

## What this is not

- Not a primary full-universe on-device PIP rematch (SPD-021).
- Settlement fallback / no-area productization is **SP-029** (see
  `SP-029-settlement-fallback.md`).
- Sparse explored persistence + rematerialize is **SP-030** (SPD-022).
- Production mapgen collectors → `.spa` emit remains **SP-044**.

## Verification

`VerifyDenseAssignments` recomputes §8.8 (`BuildDenseAssignments`) from area
rings and sample centres in slot order and compares to the stored column. Used
by fixtures to prove client consumption matches generator precompute.
