# SP-026 — Exploration sidecar (`.spa`) format and emission

Per-MWM downloadable exploration sidecar (SPD-020) carrying true closed rings
and a dense compact assignment map (SPD-021/022). Client load API is SP-027.

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

## Areas

Each row: OSM id, OSM object type, role (subdivision / settlement /
place-boundary), admin level, place type, name, mercator area, ring count,
then each outer ring via `serial::SaveOuterPath` (shipping geometry codec).

Settlement rings are stored for SPD-007 / SPD-025 fallback but are **not**
valid targets of the assignment column.

## Assign

Dense `uint16` or `uint32` compact indices into the areas table. Sentinel
`0xFFFF` / `0xFFFFFFFF` = no subdivision. Never stores full-universe uint64
OSM ids. Never points at settlement rows.

## Emission

Library: `libs/street_pixels_areas/` — filter (`street_pixels_config` policy,
named true rings only; reject three-box and place-node invention), §8.8
subdivision assigner, writer/reader.

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
