# Street Pixels map pipeline

Numbered walkthrough for maintainers who generate Street Pixels map files
and serve them. One command on a build host produces the publish tree; an
8 GiB VPS only serves that tree.

This is the production path. Community CoMaps distributors (MWM-only
mirrors) and `prepare_spa_debug_root` (fetches CoMaps CDN countries) are
**not** this path.

Flag-level CLI notes live in
[`tools/python/street_pixels/README.md`](../tools/python/street_pixels/README.md).
Keygen detail:
[`implementation/notes/sp-101-map-identity.md`](implementation/notes/sp-101-map-identity.md).
VPS rsync and nginx/Caddy:
[`implementation/notes/sp-102-publish-and-serve-origin.md`](implementation/notes/sp-102-publish-and-serve-origin.md).
`maps_generator` internals:
[`tools/python/maps_generator/README.md`](../tools/python/maps_generator/README.md).

---

## What you produce

```text
{out}/meta/maps.json
{out}/maps/2026.06.28/{dataVersion}/countries.txt
{out}/maps/2026.06.28/{dataVersion}/countries.txt.sig
{out}/maps/2026.06.28/{dataVersion}/{leaf}.mwm
{out}/maps/2026.06.28/{dataVersion}/{leaf}.spa
```

The app requests `{base}/meta/maps.json` then
`{base}/maps/{MAP_SERIES}/{dataVersion}/{UrlEncode(file)}`. There is no
`/spa/` URL scheme. First grain is eight Finland leaves plus extract
`World.mwm`. `WorldCoasts` is omitted when coastline extract is skipped.

Default Finland leaves (`data/borders/Finland_*.poly`):

- `Finland_Eastern Finland_North`
- `Finland_Eastern Finland_South`
- `Finland_Northern Finland`
- `Finland_Southern Finland_Helsinki`
- `Finland_Southern Finland_Lappeenranta`
- `Finland_Southern Finland_West`
- `Finland_Western Finland_Jyvaskyla`
- `Finland_Western Finland_Tampere`

`--countries 'World,Finland_*'` expands to those eight plus `World`.

---

## Do not

- Fetch map bytes from CoMaps CDNs (`*.comaps.app`, `*.comaps.tech`, or
  community CoMaps mirrors). OSM input is Geofabrik, `planet.openstreetmap.org`,
  or a local `file://` PBF.
- Run `maps_generator` / `map_pipeline` on the 8 GiB VPS.
- Bake a LAN or Custom Maps URL into `DEFAULT_URLS_JSON`. Custom Maps stays
  a user Advanced override.
- Pass `--enable-debug-routes` on a public origin.
- Merge spa-bearing `data/countries.txt` into git until the stock origin
  already serves the blobs.
- Ship highway-proxy U. Production `.spa` uses `pix_derive_tool` U only.
- Commit `private.h`, production Ed25519 secret PEMs, PBF, `.mwm`, `.spa`,
  or a live hostname. Git uses `maps.example.invalid`.
- Use `prepare_spa_debug_root` as the production countries source.

---

## 1. Hardware

| Host | Role | RAM |
| --- | --- | --- |
| Builder | Generate MWMs, `.pix`, rings, `.spa`, assemble the tree, optional rsync | **≥32 GiB**. Cap threads (`THREADS_COUNT: 4`). `NODE_STORAGE: map`. Full-planet `mem` mode is out. |
| VPS | Serve the assembled tree (nginx or Caddy) | **8 GiB is enough to serve.** Do not generate here. Finland-scale artifacts are about 1 GiB. |

Builder disk: tens of GiB free (OSM extract, mapgen working set, `--out`).

Same git revision for `generator_tool`, `pix_derive_tool`, `spa_emit_tool`,
and the APK. The app does not load maps from a newer generator.

---

## 2. Clone and configure

```bash
git clone --recurse-submodules --shallow-submodules <your-fork-url>
cd comaps
SKIP_MAP_DOWNLOAD=1 ./configure.sh
```

`SKIP_MAP_DOWNLOAD=1` is required until you have a Street Pixels origin or a
local `World.mwm`. Without it, `configure.sh` fails closed rather than
fetching CoMaps World.

`configure.sh` copies
[`private.h.street-pixels.example`](../private.h.street-pixels.example) to
gitignored `private.h` when that file is missing. It does not overwrite an
existing `private.h`. Root `CMakeLists.txt` does the same copy at configure
time (Android Gradle).

`private.h` is gitignored **and untracked**. Clones never receive a live
header. Do not `git add private.h`.

---

## 3. Ed25519 keys (Channel A)

Stock builds verify `countries.txt` with Ed25519. Do not skip verification.

```bash
openssl genpkey -algorithm Ed25519 -out countries_ed25519_secret.pem
openssl pkey -in countries_ed25519_secret.pem -pubout -out countries_ed25519_public.pem
```

Keep the secret PEM on the **build host** only. Never commit it. Never copy
it to the VPS git checkout.

Public hex for `private.h`:

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels.map_identity public-hex \
  --public-key /path/to/countries_ed25519_public.pem
```

Paste the 64 hex characters:

```c
#define COUNTRIES_TXT_SIGNATURE_HEX "<64 hex chars>"
```

Leave `DEFAULT_URLS_JSON` as `https://maps.example.invalid/` until DNS and TLS
for the public origin exist (step 11). `METASERVER_URL` stays empty so the
client uses `DEFAULT_URLS_JSON`. `MAP_SERIES` stays `2026.06.28` unless a
compatibility bump is required.

---

## 4. Build the C++ tools

From the repo root, same revision as the APK:

```bash
./tools/unix/build_omim.sh -r generator_tool world_roads_builder_tool mwm_diff_tool pix_derive_tool spa_emit_tool
```

`build_omim.sh` writes binaries to `../omim-build-release` by default
(sibling of the repo). `-p` changes that directory. `map_pipeline` looks in
`--build-path`, then `{repo}/omim-build-release`, `{repo}/omim-build-debug`,
and the same names as siblings of the repo.

If binaries are not on `PATH` and not in those dirs:

```bash
# later, on map_pipeline:
--build-path /path/to/omim-build-release
```

Confirm the three Street Pixels tools exist and are executable:

```bash
ls -l ../omim-build-release/generator_tool \
      ../omim-build-release/pix_derive_tool \
      ../omim-build-release/spa_emit_tool
```

---

## 5. Python dependencies

```bash
source .venv/bin/activate
cd tools/python
pip install -r maps_generator/requirements_dev.txt
pip install osmium shapely
```

`osmium` and `shapely` are required for the rings stage
(`street_pixels_spike/extract_admin_place_polygons.py`). That script keeps
true closed rings only; it does not invent polygons around place nodes.

If you skipped the venv (`SKIP_PYTHON_VENV=1`), install into the Python you
will use with `PYTHONPATH=. python3 -m street_pixels`.

---

## 6. Get a Finland OSM extract

Allowed `--pbf` values: `file://` path, or HTTPS from Geofabrik /
`planet.openstreetmap.org`. Other HTTPS hosts are refused.

Download once (builder disk):

```bash
mkdir -p /var/sp-maps
curl -L -o /var/sp-maps/finland-latest.osm.pbf \
  https://download.geofabrik.de/europe/finland-latest.osm.pbf
curl -L -o /var/sp-maps/finland-latest.osm.pbf.md5 \
  https://download.geofabrik.de/europe/finland-latest.osm.pbf.md5
```

A local `file://` PBF without a `.md5` sidecar gets an MD5 written at mapgen
time (not during `--dry-run`).

Or pass the Geofabrik URL directly to `map_pipeline`; the generator downloads
it. Prefer `file://` when you will re-run stages.

---

## 7. Dry-run (no network, no generate)

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels map_pipeline --dry-run \
  --pbf file:///var/sp-maps/finland-latest.osm.pbf \
  --out /tmp/sp-out \
  --countries 'World,Finland_*' \
  --secret-key /path/to/countries_ed25519_secret.pem
```

Expect:

- Stages: mapgen → pix_derive → rings → spa_emit → assemble
- Expanded countries: eight `Finland_*` plus `World`; no `WorldCoasts`
- No CoMaps hosts in the printed plan or rendered ini
- `--out` is **not** created
- PBF is not downloaded

`--skip-coast` is an error when `World` is in the **expanded** country set.
Leave it off for this first grain (World is included). WorldCoasts is still
omitted when coasts are not produced.

---

## 8. Generate

On the ≥32 GiB builder, drop `--dry-run`:

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels map_pipeline \
  --pbf file:///var/sp-maps/finland-latest.osm.pbf \
  --out /var/sp-maps/publish \
  --countries 'World,Finland_*' \
  --secret-key /path/to/countries_ed25519_secret.pem
```

This runs:

1. `maps_generator` (MWMs + `countries.txt`, same revision as the APK)
2. `pix_derive_tool` (leaf `.pix` from those MWMs)
3. rings JSONL from the PBF (closed admin/place rings)
4. `spa_emit_tool --mode=production`
5. `assemble_spa_publish_tree` → `--out`

Scratch dir defaults to `{out}.work`. Override with `--work-dir`.

Default ini (`var/etc/map_pipeline.ini`): `NODE_STORAGE: map`,
`THREADS_COUNT: 4`. Override `--threads` if needed; do not set `0` (all
cores) on a 32 GiB host.

Extras (hotels, isolines, SRTM, subway, UGC, Wikipedia/descriptions) stay
**on** when you have an independent local path or URL. Empty feeds skip with
a warning. Do not complete them from CoMaps map hosts or
`cdn.organicmaps.app/subway.json`. Pass `--hotels-url`, `--srtm-path`,
`--subway-url`, `--enable-wikipedia`, and related flags only when you have
those sources.

Optional publish in the same command:

```bash
  --rsync-dest user@vps:/var/www/street-pixels/
```

`--rsync-dest` is `rsync -a --delete-delay` with a trailing slash on source
**and** dest. Placeholder dest only in git; substitute the real SSH target
on the builder.

---

## 9. Check the tree

```bash
OUT=/var/sp-maps/publish
python3 - <<'PY'
import json, os, sys
out = os.environ.get("OUT", "/var/sp-maps/publish")
maps = json.load(open(os.path.join(out, "meta", "maps.json")))
series = maps["map-series"]["2026.06.28"]
assert series["status"] == "active", maps
v = str(series["latest"])
leaf_dir = os.path.join(out, "maps", "2026.06.28", v)
need = [
    "countries.txt",
    "countries.txt.sig",
    "World.mwm",
    "Finland_Southern Finland_Helsinki.mwm",
    "Finland_Southern Finland_Helsinki.spa",
]
missing = [n for n in need if not os.path.isfile(os.path.join(leaf_dir, n))]
print("dataVersion", v)
print("missing", missing or "none")
sys.exit(1 if missing else 0)
PY
```

`countries.txt` for each Finland leaf must advertise `"spa"` and
`spa_sha1_base64`. World is not a leaf for `.spa`.

`--from-stage` skips earlier stages after a failure (for example spa emit
must not rebuild MWMs). `--from-stage rsync` publishes an already-assembled
`--out`.

```bash
PYTHONPATH=. python3 -m street_pixels map_pipeline \
  --pbf file:///var/sp-maps/finland-latest.osm.pbf \
  --out /var/sp-maps/publish \
  --from-stage spa_emit \
  --secret-key /path/to/countries_ed25519_secret.pem
```

---

## 10. Serve on the LAN (phone smoke)

Same Wi-Fi as the phone, or USB reverse:

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels serve_spa_publish_tree \
  --root /var/sp-maps/publish \
  --host 0.0.0.0 \
  --port 8080
```

Startup prints a pasteable Custom Maps URL. On the phone: Settings →
Advanced → Custom Maps server → that URL. Never a build default.

USB:

```bash
adb reverse tcp:8080 tcp:8080
# Custom Maps URL: http://127.0.0.1:8080/
```

Do **not** pass `--enable-debug-routes` here if you will copy the same habit
to a public VPS. Debug inventory is off by default.

This Python server is the supported **LAN** path only. The public origin is
nginx or Caddy (next step).

---

## 11. Publish and serve on the VPS

Generate stays on the builder. Rsync the `--out` tree to the VPS document
root (parent of `maps/` and `meta/`):

```bash
rsync -a --delete-delay /var/sp-maps/publish/ user@vps:/var/www/street-pixels/
```

`--delete-delay` makes dest match `--out`. If `--out` holds only the new
`maps/2026.06.28/{v}/` directory, previous version dirs on the VPS are
removed after the transfer. To keep N=2 old version dirs: copy them aside on
the VPS before rsync, then move them back under `maps/2026.06.28/` if
in-flight clients still need them. V1 has no automatic cleanup.

Copy one example config, substitute the **ops** hostname (not
`maps.example.invalid`), obtain TLS, reload:

- [`tools/python/street_pixels/var/etc/origin.nginx.conf`](../tools/python/street_pixels/var/etc/origin.nginx.conf)
  — `certbot --nginx` (or equivalent)
- [`tools/python/street_pixels/var/etc/origin.Caddyfile`](../tools/python/street_pixels/var/etc/origin.Caddyfile)
  — Caddy auto-HTTPS when DNS A/AAAA points at the VPS and ports 80/443 are
  open

Both: `gzip off` (Caddy: do not `encode gzip`) for `.mwm` / `.spa` / `.sig`
/ `.txt`. HTTP Range stays on (nginx default; Caddy `file_server`).
`inventory.json` returns 404. Do not run `serve_spa_publish_tree` as the
public origin.

Health (no `/health` on the public vhost):

```bash
curl -fsS https://maps.example.invalid/meta/maps.json
```

Expect HTTP 200 and `"status": "active"` for `2026.06.28`. Substitute the
ops hostname when you curl for real. Do not commit that hostname.

Then put the HTTPS origin in gitignored `private.h`:

```c
#define DEFAULT_URLS_JSON R"([ "https://maps.example.invalid/" ])"
```

Rebuild the APK. The in-tree example stays `https://maps.example.invalid/`.

---

## 12. Troubleshooting

| Symptom | What to do |
| --- | --- |
| `configure.sh` errors with no World | Set `SKIP_MAP_DOWNLOAD=1`, or `STREET_PIXELS_LOCAL_WORLD` / `STREET_PIXELS_WORLD_DIR`, or `STREET_PIXELS_MAPS_BASE_URL` HTTPS to **our** origin (not CoMaps, not `192.168`). |
| `pix_derive_tool` / `spa_emit_tool` / `generator_tool` not found | Build them (step 4); pass `--build-path`. Same git rev as the APK. |
| `--skip-coast` rejected | World is in the expanded country list. Leave `--skip-coast` off when generating World. |
| CoMaps host refused | Expected. Drop `--cdn-base` / `--allow-comaps-origin`. Fix `--pbf`. |
| Extra feed skipped with warning | Independent source missing. Supply `--hotels-url` (etc.) or accept the skip. Do not fetch CoMaps to fill it. |
| spa emit failed after long mapgen | `--from-stage spa_emit` (or `pix_derive` / `rings`). |
| Phone rejects `countries.txt` | Channel A: `COUNTRIES_TXT_SIGNATURE_HEX` must be the public key for `--secret-key`. Rebuild the APK after editing `private.h`. |
| SHA mismatch on download | Origin must not gzip `.mwm` / `.spa` / `.sig` / `.txt`. |
| Dest symlink replaced by rsync | Dest path must have a trailing slash (the CLI adds one if you omit it). |

`maps_generator` logs: `{work}/maps_build/generation.log` and
`{work}/maps_build/<build>/logs/`.

---

## 13. Residual (not claimed done by this doc)

A full eight-leaf Finland generate on a ≥32 GiB builder, Channel A sign,
and a VPS Range-GET of a Helsinki object are still maintainer evidence
(SP-103 / SP-104). This tutorial is the operator path; it does not mark
Phase 11 exit complete.

Option A (OSM collectors inside `generator/` / `StageMwm` emit) is out of
this pipeline.

---

## Related

| Doc | Role |
| --- | --- |
| [`tools/python/street_pixels/README.md`](../tools/python/street_pixels/README.md) | CLI flags, World bootstrap, LAN/VPS short form |
| [`DEPLOY_OWN_MAP_SERVER.md`](DEPLOY_OWN_MAP_SERVER.md) | Community MWM mirrors (not production) + Street Pixels reference sections |
| [`implementation/notes/sp-101-map-identity.md`](implementation/notes/sp-101-map-identity.md) | `private.h`, keygen, `configure-world` |
| [`implementation/notes/sp-102-publish-and-serve-origin.md`](implementation/notes/sp-102-publish-and-serve-origin.md) | rsync, keep N=2, nginx/Caddy |
| [`implementation/notes/spa-advertise-channels.md`](implementation/notes/spa-advertise-channels.md) | Channel A vs debug Channel B |
| [`tools/python/maps_generator/README.md`](../tools/python/maps_generator/README.md) | `generator_tool` / `maps_generator` setup |
