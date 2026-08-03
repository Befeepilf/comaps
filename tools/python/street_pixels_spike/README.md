# Street Pixels spike tooling (desktop-only)

Non-shipping measurement scripts for Phase 4 spikes. Outputs belong under
`/tmp/sp023/` (or another local dir); do not commit PBF/MWM/GeoJSON artifacts.

## SP-023 — Finland admin / place polygon retention

Measures true closed-ring retention size and settlement-subdivision coverage
over full Finland OSM (Geofabrik PBF). Never invents polygons around
`place=*` nodes.

### Setup

```bash
python3 -m venv /tmp/sp023/venv
/tmp/sp023/venv/bin/pip install osmium shapely pyproj healpy
```

### Inputs (fetch once)

```bash
mkdir -p /tmp/sp023
curl -L -o /tmp/sp023/finland-latest.osm.pbf \
  https://download.geofabrik.de/europe/finland-latest.osm.pbf
curl -L -o /tmp/sp023/Finland_Southern_Finland_Helsinki.mwm \
  'https://mapgen-fi-1.comaps.app/maps/260728/Finland_Southern%20Finland_Helsinki.mwm'
curl -L -o /tmp/sp023/World.mwm \
  https://mapgen-fi-1.comaps.app/maps/260728/World.mwm
```

Borders: `data/borders/Finland_*.poly` in the repo.

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

Reports feed `docs/implementation/work-items/SP-023-admin-polygon-size-spike.md`
and optionally `docs/implementation/spikes/SP-023-finland-admin-polygons.md`.
