# SP-026 — Exploration sidecar (`.spa`) format and emission

Exploration-area geometry ships as a **downloadable sidecar** (SPD-020), not as
a mandatory in-MWM section. File grain is **one `.spa` per MWM leaf**
(`{mwmLeafId}.spa`); a country install may contain many leaf sidecars. SPD-020
“per-country” means the artifact is part of the country download package and is
not World three-box / in-MWM exploration storage.

True closed rings and a dense compact assignment map (SPD-021/022). Client load
API is SP-027.

## File name

`{mwmLeafId}.spa` — extension `SPA_FILE_EXTENSION` in `defines.hpp`.

## Container

`FilesContainer` sections:

| Tag | Macro | Contents |
| --- | --- | --- |
| `hdr` | `SPA_HEADER_FILE_TAG` | `SpaHeader` |
| `areas` | `SPA_AREAS_FILE_TAG` | area table with true rings |
| `assign` | `SPA_ASSIGN_FILE_TAG` | dense compact area-index column |

## Header fields

magic (`SPA1` / `kSpaMagic`), `format_version` (**2** production; **1**
geometry-only dual-read), `map_data_version`, `policy_version`, ISO 3166-1
alpha-2, MWM leaf id, `area_count`, `assign_count`, `index_width` (2 or 4),
then (format_version **2** only, little-endian after `index_width`):

| Field | Type | Production value |
| --- | --- | --- |
| `nside` | `uint32` | `1048576` (`kSpaNside`) |
| `universe_order` | `uint8` | `1` = AscendingNest (`kSpaUniverseOrderAscendingNest`) |
| `reserved` | `uint8[3]` | must be `0` on write; non-zero → fail-closed on read |

Frozen by **SPD-034** / **SP-043**. Writers always emit format_version 2 with
those values. Readers accept v2 only when `nside` / `universe_order` /
`reserved` match; accept v1 only when `assign_count == 0` (geometry-only
fixtures / offline harness); reject v1 with `assign_count > 0`.

Assignment determinism is keyed by `(map_data_version, policy_version)`.

### Assign column semantics (universe order)

`assign[i]` is the compact area index for **slot `i`** of the HEALPix NEST
exploration universe **U** for that MWM leaf: **U is strictly ascending** by
NEST id at `nside = 1048576`, matching `ScanUniverseAscending` / the SP-028
contract (`docs/implementation/notes/SP-028-universe-order.md`). Slot `i` ↔
`U[i]` ↔ `assign[i]`. `assign_count == 0` remains valid for geometry-only
fixtures.

## Areas

Each row: OSM id, OSM object type, role (subdivision / settlement /
place-boundary), admin level, place type, name, mercator area (host-endian
IEEE754 `double`), ring count, then each outer ring via `serial::SaveOuterPath`
(shipping geometry codec). Outer rings only; holes are not stored (PIP may
over-accept inside holes until a later format revision).

Settlement rings are stored for SPD-007 / SPD-025 fallback but are **not**
valid targets of the assignment column.

Callers must run `FilterExplorationCandidate` before write. Geometry source is
not persisted; three-box and place-node invention are rejected at filter time
(§8.3, SPD-020/025).

## Assign

Dense `uint16` or `uint32` compact indices into the areas table. Sentinel
`0xFFFF` / `0xFFFFFFFF` = no subdivision. Never stores full-universe uint64
OSM ids. Never points at settlement rows. Built by the §8.8 assigner:
configured admin priority order, then smallest mercator area, then lower OSM
id; place-boundary rings rank after all configured admin subdivision levels.

## Emission

Library: `libs/street_pixels_areas/` — filter (`street_pixels_config` policy,
named true rings only; reject three-box and place-node invention), §8.8
subdivision assigner, writer/reader.

`generator/` links the library for the future mapgen hook; **production
`PlaceProcessor` / three-box paths are not modified**. `libs/map` does not link
this library until SP-027.

CI path: synthetic fixtures in `street_pixels_areas_tests` (no Finland
mapgen). Offline emit tool (`tools/spa_emit_tool/`):

- **Production (SP-044 Option B, default `--mode=production`):** rings JSONL +
  country policy + leaf `.poly` borders + leaf `.pix` (ascending NEST **U**) →
  dense `{mwmLeafId}.spa` with `format_version` 2, `assign_count == |U|`.
  Sample centres via chealpix (`sample_centres`); settlements admitted but never
  assign targets. Full in-pipeline mapgen collectors remain **Option A**
  residual.
- **Geometry-only (fixtures / SP-032 debug):** `--mode=geometry_only` writes
  `assign_count=0` (not the production default).

```bash
# Production dense leaf emit (outputs under /tmp — not committed):
./omim-build-debug/spa_emit_tool \
  --mode=production \
  --rings=/tmp/sp044/finland_admin_place_rings.jsonl \
  --policy=data/street_pixels/country_policies.json --iso=FI \
  --borders_dir=data/borders \
  --pix_dir=/tmp/sp044/fi_pix \
  --out_dir=/tmp/sp044/publish

# Geometry-only fixtures/debug (SP-032 path):
./omim-build-debug/spa_emit_tool \
  --mode=geometry_only \
  --rings=/tmp/sp023/finland_admin_place_rings.jsonl \
  --policy=data/street_pixels/country_policies.json --iso=FI \
  --out_dir=/tmp/sp032 \
  --helsinki_poly="data/borders/Finland_Southern Finland_Helsinki.poly"
```

`FilterExplorationCandidate` → `WriteExplorationSidecar` (not PlaceProcessor /
three-box). Full generator mapgen hook remains Option A follow-up; SP-026 shipped
the format + library + tests; SP-032 shipped geometry-only offline emit; SP-044
ships production dense Option B.

## Size vs SP-023

SP-023 Finland budget baseline: country-concat zlib(coded_delta) ≈ **2.06 MiB**;
Helsinki MWM slice zlib_coded ≈ **0.52 MiB**. Shipping rings use
`SaveOuterPath` (MWM geometry codec). **Re-measured under SP-032** (exit #7):
FI country-concat `.spa` **2 019 268 B (~1.93 MiB)**; Helsinki leaf **456 484 B
(~0.44 MiB)**; assign section 0 (geometry-only). Policy filter on the SP-023
JSONL (FI): 2618 admitted / 64 unnamed / 69 policy_mismatch. See
[`validation/SP-031-evidence-log.md`](../validation/SP-031-evidence-log.md).
