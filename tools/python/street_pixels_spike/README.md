# Street Pixels spike tooling (desktop-only)

Non-shipping measurement scripts for Phase 4 spikes. Outputs belong under
`/tmp/sp023/` (or another local dir); do not commit PBF/MWM/GeoJSON artifacts.

Evidence write-up:
[`docs/implementation/spikes/SP-023-finland-admin-polygons.md`](../../../docs/implementation/spikes/SP-023-finland-admin-polygons.md)
· work item
[`SP-023`](../../../docs/implementation/work-items/SP-023-admin-polygon-size-spike.md).

## SP-023 — Finland admin / place polygon retention

Measures true closed-ring retention size and settlement-subdivision coverage
over full Finland OSM (Geofabrik PBF). Never invents polygons around
`place=*` nodes (spec §8.3). No city allowlists (SPD-004).

| Script | Role |
| --- | --- |
| `extract_admin_place_polygons.py` | Closed admin_5–11 + polygonal place suburb/quarter/neighbourhood → JSONL (+ optional Helsinki metro GeoJSON) |
| `measure_sizes.py` | raw_pod / coded_delta / zlib sizes; MWM border attribution; World/`packed_polygons` baselines |
| `coverage_and_assign.py` | Settlement subdivision coverage; highway→HEALPix universe proxy; PIP cost; table-size estimates; spot-check |
| `sp023_common.py` | Shared `.poly` parse, encodings, MWM section reader |

### Setup

```bash
python3 -m venv /tmp/sp023/venv
/tmp/sp023/venv/bin/pip install osmium shapely pyproj healpy
```

Approximate runtimes on a cloud x86_64 VM (Finland PBF ~700 MiB): extract ~few
minutes; measure seconds; coverage+PIP (second PBF pass + densify) ~tens of
minutes.

### Inputs (fetch once)

```bash
mkdir -p /tmp/sp023
curl -L -o /tmp/sp023/finland-latest.osm.pbf \
  https://download.geofabrik.de/europe/finland-latest.osm.pbf
# Optional: pin the snapshot used for recorded evidence (2026-08-02 Geofabrik
# finland-latest → 737 359 571 bytes; sha256
# a446647ff15a2fc334cc83be283cc637fd66ff560b166d589525793e5ffc2724).
# cp /tmp/sp023/finland-latest.osm.pbf /tmp/sp023/finland-260802.osm.pbf
curl -L -o /tmp/sp023/Finland_Southern_Finland_Helsinki.mwm \
  'https://mapgen-fi-1.streifzug.app/maps/260728/Finland_Southern%20Finland_Helsinki.mwm'
curl -L -o /tmp/sp023/World.mwm \
  https://mapgen-fi-1.streifzug.app/maps/260728/World.mwm
```

Borders: `data/borders/Finland_*.poly` in the repo.
Also uses `data/packed_polygons.bin` for size baseline.

### Run

```bash
OUT=/tmp/sp023
PY=/tmp/sp023/venv/bin/python
ROOT=tools/python/street_pixels_spike

$PY $ROOT/extract_admin_place_polygons.py \
  --pbf $OUT/finland-latest.osm.pbf \
  --out-jsonl $OUT/finland_admin_place_rings.jsonl \
  --out-geojson-helsinki $OUT/helsinki_subdivisions.geojson

$PY $ROOT/measure_sizes.py \
  --rings $OUT/finland_admin_place_rings.jsonl \
  --borders-dir data/borders \
  --world-mwm $OUT/World.mwm \
  --helsinki-mwm $OUT/Finland_Southern_Finland_Helsinki.mwm \
  --packed-polygons data/packed_polygons.bin \
  --out $OUT/size_report.json

$PY $ROOT/coverage_and_assign.py \
  --rings $OUT/finland_admin_place_rings.jsonl \
  --pbf $OUT/finland-latest.osm.pbf \
  --helsinki-poly "data/borders/Finland_Southern Finland_Helsinki.poly" \
  --out $OUT/coverage_report.json \
  --universe-mode highway_healpix_proxy
```

### Notes / limitations

- `coded_delta` is a spike approximation (1e7 scale + zigzag varint), not the
  shipping geometry codec — re-measure in SP-026.
- PIP assignment in `coverage_and_assign.py` is a coverage/cost proxy: smallest
  area among subdivision candidates, then settlement fallback. It does **not**
  apply country-config admin_level priority before smallest-area (full §8.8 is
  SP-028).
- No `{countryId}.pix` in the workspace; universe is highway densify @15 m →
  HEALPix `nside=1048576` (SPD-017 / SPD-019).
- Desktop timings are optimistic vs phone-class hardware.
