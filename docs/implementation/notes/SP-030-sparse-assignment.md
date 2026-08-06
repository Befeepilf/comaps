# SP-030 — Sparse assignment persistence

Client-durable pixel→area state (SPD-022): sparse explored HEALPix→compact
area index on device; rematerialize from the dense uint16/uint32 sidecar map.

## On-disk format (`.spx`)

Path: `{writable}/{countryId}.spx` beside `.pix` (`SPX_FILE_EXTENSION`).

| Field | Type | Notes |
| --- | --- | --- |
| magic | uint32 | Little-endian fourcc `SPX1` (`kSpxMagic`) |
| format_version | uint32 | `kSpxFormatVersion` (= 1) |
| map_data_version | int64 | Pair key with policy |
| policy_version | uint32 | Pair key with map-data |
| entry_count | uint32 | Explored rows only |
| index_width | uint8 | 2 or 4 (same sentinel rules as `.spa`) |
| body | sorted rows | `int64` NEST id + `uint16`/`uint32` compact index |

No uint64 OSM id column. Writes use temp+rename. Corrupt or version-mismatched
stores rebuild from the sidecar; `.pix` exploration is never wiped.

## API

- `SparseAssignmentStore::Build` / `Rematerialize` — from
  `ExplorationAreaResolver` + ascending explored ids + sample centres
- `TryLoadSparseAssignmentStore` / `TryLoadAndVerifySparseAssignmentStore`
- `EnsureSparseAssignmentStore` — load if current, else rematerialize+save
- Path helpers: `SparseAssignmentPath`, `SparseAssignmentPathBesidePix`

Universe **U** and explored sets are caller-supplied (no
`street_pixels_areas` → `map` cycle). Manager/tests extract U from `.pix` via
`street_pixels_file::ScanUniverseAscending`.

## Size vs SP-023

SP-023 Uusimaa-scale proxy (**N ≈ 6.5×10⁶**, ~1.05× measured 6 844 831 cells):

| Representation | Budgeted size |
| --- | --- |
| Full uint16 dense map | ~13 MiB |
| Full uint32 dense map | ~26 MiB |
| Full uint64 OSM ids | ~52 MiB (rejected) |
| Sparse 1 % explored @ 12 B/row (int64 + uint32) | ~0.78 MiB |
| Rematerialize-only (no local sparse) | 0 |

V1 `.spx` stores only explored rows at **10 B** (uint16 index) or **12 B**
(uint32 index) per row. At 1 % explored with uint16: **~0.65 MiB** — under the
SP-023 sparse estimate and far below full-universe tables. Dense answers for
unexplored cells rematerialize from the downloadable `.spa` assign column
(SPD-021); settlement fallback still needs a sample centre (HEALPix cell
centre on the manager refresh path).

## Manager hooks (thin)

- After successful Phase 3 rematch / on load: best-effort `.spx` refresh when
  `.spa` is present and map-data matches
- Cleanup deletes `.spx`
- `RematerializeAssignmentsOnPolicyBump` — policy-only rebuild, `.pix` intact
- `TakePendingAssignmentRematch` — optional signal, no UI
