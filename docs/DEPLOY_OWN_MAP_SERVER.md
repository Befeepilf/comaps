# Deploy your maps files server

This doc explains how to deploy your own instance of a Streifzug server with files from official CDNs (We are working to be able to download maps files without hardcoded countries.txt file embedded in the app)
We explain how to deploy with minimal config, but each tool has different options to change server port or choose maps files that you want to download.

## Deploy a server
Our community has developed different tools to deploy easily an instance of a Streifzug server:
- [comaps-map-distributor](https://codeberg.org/gedankenstuecke/comaps-map-distributor)
- [comaps-server](https://github.com/myanesp/comaps-server)

### Deploy comaps-map-distributor

Prerequisites
- [python3](https://www.python.org/downloads/) and [pip](https://pypi.org/project/pip/)
- Your server must be accessible from your network

- Launch your terminal
- Run `pip install comaps-map-distributor`
- Launch the tool with this command `comaps-map-distributor download-maps`
- Choose map files you want to download from official CDNs
- Run `comaps-map-distributor serve-maps`
- Go to your mobile device -> Streifzug -> settings -> Advanced -> Custom Maps server
- Edit URL with your URL server and enjoy

### Deploy comaps-server

Prerequisites
- Docker
- Your server must be accessible from your network

#### Docker

- Launch your terminal
- Run ``` docker run -d \
  --name comaps-server \
  --restart unless-stopped \
  -e MAPS=all \ 
  -e OUTPUT_DIR=/maps \
  -p "80:80" \
  ghcr.io/myanesp/comaps-server:latest```
- Go to your mobile device -> Streifzug -> settings -> Advanced -> Custom Maps server
- Edit URL with your URL server and enjoy   

#### Docker compose
- Launch your terminal
- Create a `compose.yml` file with this config and save it:

```services:
  maps-server:
    image: ghcr.io/myanesp/comaps-server
    container_name: comaps-server
    ports:
      - "80:80"
    environment:
      - MAPS=World,WorldCoasts,Spain
      - OUTPUT_DIR=/maps
    volumes:
      - ./maps:/maps
      - TZ=Europe/Madrid```

- Execute `docker compose up`	  
- Go to your mobile device -> Streifzug -> settings -> Advanced -> Custom Maps server
- Edit URL with your URL server and enjoy   

You can find more details in the [FAQ article](https://www.streifzug.app/support/how-can-i-host-a-custom-map-server-for-downloads/) to deploy your own HTTP maps server and find more details [here](https://www.streifzug.app/support/how-can-i-set-a-custom-map-server-for-downloads/) about restrictions.

## Street Pixels (`.spa` publish tree)

Community tools above mirror **MWMs only**. They are **not** the Street
Pixels production path (**SPD-087** / P1). Street Pixels exploration
sidecars (`.spa`) must sit beside matching `.mwm` under the CDN layout the
app already requests (`meta/maps.json` + `maps/{MAP_SERIES}/{version}/`).
**SPD-035** — no `/spa/` scheme.

**Start here:** numbered operator walkthrough
[`docs/STREET_PIXELS_MAP_PIPELINE.md`](STREET_PIXELS_MAP_PIPELINE.md)
(clone → keys → build tools → Finland PBF → dry-run → generate → LAN →
VPS). Flag notes:
[`tools/python/street_pixels/README.md`](../tools/python/street_pixels/README.md).
VPS rsync/nginx:
[`docs/implementation/notes/sp-102-publish-and-serve-origin.md`](implementation/notes/sp-102-publish-and-serve-origin.md).
**VPS generate is unsupported** (**SPD-088**): generate on the builder, **serve
here**.

### 1. Generate (production path) — `map_pipeline`

On a ≥32 GiB builder, one CLI runs `maps_generator` → `pix_derive_tool` →
rings extract → `spa_emit_tool --mode=production` → `assemble_spa_publish_tree`
and writes an SP-050 `--out` tree. OSM input is `file://` or Geofabrik /
planet OSM — not Streifzug map CDNs.

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels map_pipeline --help
PYTHONPATH=. python3 -m street_pixels map_pipeline --dry-run \
  --pbf file:///path/to/finland.osm.pbf \
  --out /tmp/sp100 \
  --countries 'World,Finland_*'
```

`--dry-run` prints the stage graph and paths and does not hit the network.
Optional `--rsync-dest` copies the tree with `rsync -a --delete-delay`
(trailing slash on source and dest). Placeholder dest:
`user@vps:/var/www/street-pixels/`. Do **not** use `prepare_spa_debug_root`
as the production countries source (**SPD-087**).

### 2. Debug prepare (Streifzug CDN countries + spa) — not production

Fetches **public Streifzug CDN** `meta/maps.json` latest for `MAP_SERIES`, downloads
that `countries.txt`, injects spa meta from your emit dir, and builds the
publish root (spa-only by default — phone already has MWMs from CDN).
**This is not the production path.**

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels prepare_spa_debug_root \
  --spa-dir /path/to/spa_emit_out \
  --out /tmp/spa_debug_root \
  --channel serve-only
```

Then serve (`--root /tmp/spa_debug_root`) as in §4. Channels A/B:
`--channel A --secret-key …` or `--channel B` (writes `{out}/_channel_b/countries.txt`
for a local APK only — do not merge). Override mirrors with repeated
`--cdn-base`; override catalog with `--countries` / `--data-version` if needed.

Do not invent placeholder spa meta. Full Channel A/B recipes:
`docs/implementation/notes/spa-advertise-channels.md` (SP-052).

### 3. Assemble manually (SP-050)

Build the CDN-identical tree from a local `countries.txt`, `{leaf}.spa` (from
`spa_emit_tool`), and matching MWMs. Prefer §1 (`map_pipeline`) for an
independent origin. Prefer §2 only for Streifzug-CDN debug. See
`docs/implementation/work-items/SP-050-spa-publish-tree-assemble.md`.

```bash
cd tools/python
PYTHONPATH=. python3 post_generation/assemble_spa_publish_tree.py \
  --countries /path/to/cdn_countries.txt \
  --spa-dir /path/to/spa_emit_out \
  --out /tmp/spa_publish \
  --map-series 2026.06.28 \
  --data-version 260803 \
  --spa-only
```

### 4. Serve on the LAN (SP-051)

Serve the `map_pipeline` / assemble `--out` root with the in-repo server (Range GETs for large
MWMs; `/health`; debug inventory opt-in only). This remains the supported
Street Pixels **LAN** path.

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels serve_spa_publish_tree \
  --root /tmp/spa_debug_root \
  --host 0.0.0.0 \
  --port 8080
```

Startup prints a pasteable Custom Maps URL (detected LAN IPv4). On the phone:
Settings → Advanced → Custom Maps server → that URL (never a build default).

Same Wi-Fi as the phone, or USB:

```bash
adb reverse tcp:8080 tcp:8080
# then Custom Maps URL: http://127.0.0.1:8080/
```

Optional: `--enable-debug-routes` exposes `GET /debug/inventory` (off by
default). **Do not** pass `--enable-debug-routes` on a public VPS.

### 5. Serve on the VPS (SP-102)

Generate on the builder (§1), then rsync the SP-050 `--out` tree to the
VPS document root (parent of `maps/` and `meta/`). Production HTTP is
nginx or Caddy in front of that root — **not** `serve_spa_publish_tree`.

```bash
rsync -a --delete-delay /tmp/sp100/ user@vps:/var/www/street-pixels/
```

Same argv as `map_pipeline --rsync-dest user@vps:/var/www/street-pixels/`.
`--rsync-dest` without a dest slash is normalized to one. `--delete-delay`
makes dest match `--out` (old version dirs not in `--out` are removed after
the transfer). Keep N=2 old `maps/{MAP_SERIES}/{v}/`
dirs by copying them aside before rsync if clients still need them. V1
has no automatic cleanup.

TLS: Let’s Encrypt (`certbot --nginx`) or Caddy auto-HTTPS. Example
configs (placeholder host `maps.example.invalid`, not the ops hostname):

- `tools/python/street_pixels/var/etc/origin.nginx.conf`
- `tools/python/street_pixels/var/etc/origin.Caddyfile`

`gzip off` (Caddy: do not `encode gzip`) for `.mwm` / `.spa` / `.sig` /
`.txt`. Range GETs stay enabled. External health:
`GET /meta/maps.json` → 200 and `"status": "active"`.

Finland-scale artifacts are about 1 GiB. 8 GiB RAM is enough to serve.
Do not run `maps_generator` on the 8 GiB VPS.

Stock APK `DEFAULT_URLS_JSON` is gitignored `private.h`, not this doc.
Community Streifzug distributors (sections above this Street Pixels heading)
stay MWM-only and are not production.

Full recipe: [`docs/implementation/notes/sp-102-publish-and-serve-origin.md`](implementation/notes/sp-102-publish-and-serve-origin.md).
