#!/usr/bin/env bash
set -euo pipefail

COUNTRIES='World,Germany_*'
PBF_DIR="$HOME/Downloads/sp-maps"
PBF="$PBF_DIR/germany-latest.osm.pbf"
OUT=/tmp/sp-out
WORK="${OUT}.work"
VERSION_FILE="$WORK/data_version"
RSYNC_DEST='mo@10.0.0.2:/srv/streifzug/maps/'
REPO="$(cd "$(dirname "$0")/../../.." && pwd)"
SECRET_KEY="$REPO/countries_ed25519_secret.pem"
BUILD_PATH="$(cd "$REPO/.." && pwd)/omim-build-release"

mkdir -p "$PBF_DIR" "$WORK"
[[ -f "$PBF" ]] || curl -L -o "$PBF" https://download.geofabrik.de/europe/germany-latest.osm.pbf
[[ -f "$PBF.md5" ]] || curl -L -o "$PBF.md5" https://download.geofabrik.de/europe/germany-latest.osm.pbf.md5

if [[ -f "$VERSION_FILE" ]]; then
  DATA_VERSION="$(tr -d '[:space:]' < "$VERSION_FILE")"
else
  DATA_VERSION="$(
    python3 - "$WORK" <<'PY'
import datetime
import glob
import json
import os
import sys

work = sys.argv[1]
hits = glob.glob(os.path.join(work, "mapgen", "*", "*", "countries.txt"))
hits.sort()
if hits:
    print(json.load(open(hits[0], encoding="utf-8"))["v"])
else:
    print(datetime.date.today().strftime("%y%m%d"))
PY
  )"
  printf '%s\n' "$DATA_VERSION" > "$VERSION_FILE"
fi

if [[ ! -f "$SECRET_KEY" ]]; then
  openssl genpkey -algorithm Ed25519 -out "$SECRET_KEY"
  openssl pkey -in "$SECRET_KEY" -pubout -out "$REPO/countries_ed25519_public.pem"
  (
    cd "$REPO/tools/python"
    PYTHONPATH=. python3 -m street_pixels.map_identity public-hex \
      --public-key "$REPO/countries_ed25519_public.pem"
  )
  echo "Set COUNTRIES_TXT_SIGNATURE_HEX in private.h and rebuild the APK, then re-run."
  exit 1
fi

if [[ ! -x "$BUILD_PATH/generator_tool" || ! -x "$BUILD_PATH/pix_derive_tool" || ! -x "$BUILD_PATH/spa_emit_tool" ]]; then
  (cd "$REPO" && ./tools/unix/build_omim.sh -r generator_tool world_roads_builder_tool mwm_diff_tool pix_derive_tool spa_emit_tool)
fi

source "$REPO/.venv/bin/activate"
cd "$REPO/tools/python"
pip install -r maps_generator/requirements_dev.txt
pip install osmium shapely

map_pipeline() {
  PYTHONPATH=. python3 -m street_pixels map_pipeline \
    --pbf "file://${PBF}" \
    --out "$OUT" \
    --countries "$COUNTRIES" \
    --data-version "$DATA_VERSION" \
    --secret-key "$SECRET_KEY" \
    --build-path "$BUILD_PATH" \
    "$@"
}

map_pipeline --dry-run
map_pipeline
python3 - "$OUT" <<'PY'
import json, os, sys
out = sys.argv[1]
maps = json.load(open(os.path.join(out, "meta", "maps.json")))
series = maps["map-series"]["2026.06.28"]
assert series["status"] == "active", maps
v = str(series["latest"])
leaf_dir = os.path.join(out, "maps", "2026.06.28", v)
need = [
    "countries.txt",
    "countries.txt.sig",
    "World.mwm",
    "Germany_Berlin.mwm",
    "Germany_Berlin.spa",
]
missing = [n for n in need if not os.path.isfile(os.path.join(leaf_dir, n))]
print("dataVersion", v)
print("missing", missing or "none")
sys.exit(1 if missing else 0)
PY
map_pipeline --from-stage rsync --rsync-dest "$RSYNC_DEST"
