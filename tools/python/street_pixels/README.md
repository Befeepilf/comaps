# Street Pixels operator tools

Build-host generate, assemble, and LAN serve for Street Pixels map data.
**VPS generate is unsupported** (SPD-088): run this CLI on a ≥32 GiB builder
(`NODE_STORAGE: map`, capped threads). The 8 GiB VPS only serves an SP-050
tree: rsync + nginx or Caddy (SP-102).

Full-planet `NODE_STORAGE: mem` is out of this phase.

## Production generate (`map_pipeline`) — SP-100

One entrypoint, given an OSM extract and a country/leaf selector:

1. `maps_generator` (same git revision as the APK)
2. `pix_derive_tool` (SP-099)
3. rings JSONL (`street_pixels_spike/extract_admin_place_polygons.py`)
4. `spa_emit_tool --mode=production`
5. `assemble_spa_publish_tree` (SP-050)

Optional last step: `--rsync-dest` (`rsync -a --delete-delay`, trailing
slash on source and dest). Placeholder: `user@vps:/var/www/street-pixels/`.

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels map_pipeline --help
PYTHONPATH=. python3 -m street_pixels map_pipeline --dry-run \
  --pbf file:///tmp/finland.osm.pbf \
  --out /tmp/sp100 \
  --countries 'World,Finland_*'
```

`--pbf` must be `file://` or Geofabrik / planet OSM HTTPS. Other HTTPS
hosts and CoMaps map hosts (`*.comaps.app`, `*.comaps.tech`, community
mirrors) plus `--cdn-base` are refused unless `--allow-comaps-origin`
(default **off**). Default countries: `World,Finland_*` (eight Finland
leaves + extract World; `WorldCoasts` omitted). `--skip-coast` is an error
if World is in the **expanded** country set; without World it is allowed
and omits WorldCoasts (missing ocean fill). Do not default
`prepare_spa_debug_root`. A local `file://` PBF without a `.md5` sidecar
gets one written at mapgen time (not during `--dry-run`).

Extras (hotels, isolines, SRTM, subway, UGC, Wikipedia/descriptions) are
**on** when an independent local path or URL exists. Empty feeds skip with a
warning. Do not fetch CoMaps map hosts or `cdn.organicmaps.app/subway.json`
to complete them. Pass `--hotels-url`, `--srtm-path`, `--subway-url`,
`--enable-wikipedia`, … when you have sources.

`--from-stage` skips earlier pipeline stages so a failed spa emit does not
rebuild MWMs. `--from-stage rsync` publishes an already-assembled `--out`.

Default ini fragment: `var/etc/map_pipeline.ini` (`NODE_STORAGE: map`,
`THREADS_COUNT: 4`).

Finland full-data evidence is SP-103. Do not run full FI mapgen as a unit test.

## World bootstrap (SP-101)

`configure.sh` does not fetch CoMaps World. It sets `PYTHONPATH` to
`tools/python`, then calls `ensure-private-h` (copy example → `private.h`
when missing) and `configure-world`.

- `SKIP_MAP_DOWNLOAD=1` — skip (already used by `build_omim.sh` for targeted builds)
- `STREET_PIXELS_LOCAL_WORLD` / `STREET_PIXELS_WORLD_DIR` — copy operator World.mwm
- `STREET_PIXELS_MAPS_BASE_URL` — HTTPS Street Pixels origin (not CoMaps; public host is ops, documented as `maps.example.invalid` in git)

`private.h` is gitignored and untracked. Clones get the header by copying
[private.h.street-pixels.example](../../../private.h.street-pixels.example).
Keygen, `COUNTRIES_TXT_SIGNATURE_HEX`, and the template:
[docs/implementation/notes/sp-101-map-identity.md](../../../docs/implementation/notes/sp-101-map-identity.md).

## Debug prepare — not production (SPD-087)

`prepare_spa_debug_root` fetches CoMaps CDN `countries.txt`. It is a LAN
debug helper only. Stock / production origin must not use it.

## Serve LAN (SP-051)

```bash
PYTHONPATH=. python3 -m street_pixels serve_spa_publish_tree \
  --root /tmp/sp100 --host 0.0.0.0 --port 8080
```

Range GETs; `/health`; `--enable-debug-routes` off by default. Do not enable
debug routes on a public VPS.

## Serve public origin (SP-102)

Rsync the SP-050 `--out` tree to the VPS. nginx or Caddy in front of
`/var/www/street-pixels` (parent of `maps/` and `meta/`). TLS via Let’s
Encrypt / Caddy auto-HTTPS. `gzip off` for `.mwm` / `.spa` / `.sig` /
`.txt`. Range on. Health: `GET /meta/maps.json` 200 + `status: active`.

```bash
rsync -a --delete-delay /tmp/sp100/ user@vps:/var/www/street-pixels/
```

Example configs (placeholder `maps.example.invalid`, not the ops hostname):

- `var/etc/origin.nginx.conf`
- `var/etc/origin.Caddyfile`

Recipe: [docs/implementation/notes/sp-102-publish-and-serve-origin.md](../../../docs/implementation/notes/sp-102-publish-and-serve-origin.md).
Do not run `maps_generator` on the 8 GiB VPS.

