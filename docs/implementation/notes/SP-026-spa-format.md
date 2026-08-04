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

magic (`SPA1` / `kSpaMagic`), `format_version`, `map_data_version`,
`policy_version`, ISO 3166-1 alpha-2, MWM leaf id, `area_count`,
`assign_count`, `index_width` (2 or 4).

Assignment determinism is keyed by `(map_data_version, policy_version)`.

### Assign column semantics (universe order)

`assign[i]` is the compact area index for **sample slot `i`** supplied to
`WriteExplorationSidecar` (typically valid-street-pixel HEALPix cell centres in
generator emit order). The header does **not** yet encode HEALPix `nside` or an
explicit universe-ordering tag; that contract is owned by the generator emit job
and must be fixed before SP-027/028 consume production blobs (see SP-026
follow-ups). `assign_count == 0` is valid for geometry-only fixtures.

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
mapgen). Optional offline FI preview when
`/tmp/sp023/finland_admin_place_rings.jsonl` is present:

```bash
python3 tools/python/street_pixels_spike/filter_rings_for_spa.py \
  --rings /tmp/sp023/finland_admin_place_rings.jsonl \
  --policy data/street_pixels/country_policies.json --iso FI
```

Admitted candidates are then passed to `WriteExplorationSidecar` from a local
C++ harness (not PlaceProcessor / three-box). Full generator mapgen hook
remains a follow-up; this work item ships the format + library + tests.

## Size vs SP-023

SP-023 Finland budget baseline: country-concat zlib(coded_delta) ≈ **2.06 MiB**;
Helsinki MWM slice zlib_coded ≈ **0.52 MiB**. Shipping rings use
`SaveOuterPath` (MWM geometry codec), not the spike `coded_delta` encoder, so
byte sizes are expected to differ and must be re-measured under SP-031 exit #7
before treating AC size as Met. Policy filter on the SP-023 JSONL (FI): 2618
admitted / 64 unnamed / 69 policy_mismatch.
