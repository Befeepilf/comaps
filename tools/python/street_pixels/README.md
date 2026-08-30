# Street Pixels operator tools

Build-host generate, assemble, and LAN serve for Street Pixels map data.
**VPS generate is unsupported** (SPD-088): run this CLI on a ≥32 GiB builder
(`NODE_STORAGE: map`, capped threads). The 8 GiB VPS only serves an SP-050
tree (rsync + nginx/TLS is SP-102).

Full-planet `NODE_STORAGE: mem` is out of this phase.

## Production generate (`map_pipeline`) — SP-100

One entrypoint, given an OSM extract and a country/leaf selector:

1. `maps_generator` (same git revision as the APK)
2. `pix_derive_tool` (SP-099)
3. rings JSONL (`street_pixels_spike/extract_admin_place_polygons.py`)
4. `spa_emit_tool --mode=production`
5. `assemble_spa_publish_tree` (SP-050)

Optional last step: `--rsync-dest` (thin rsync of the `--out` tree).

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels map_pipeline --help
PYTHONPATH=. python3 -m street_pixels map_pipeline --dry-run \
  --pbf file:///tmp/finland.osm.pbf \
  --out /tmp/sp100 \
  --countries 'World,Finland_*'
```

`--pbf` must be `file://` or Geofabrik / planet OSM HTTPS. CoMaps map hosts
and `--cdn-base` are refused unless `--allow-comaps-origin` (default **off**).
Default countries: `World,Finland_*` (eight Finland leaves + extract World;
`WorldCoasts` omitted). `--skip-coast` is an error if World is in the
**expanded** country set; without World it is allowed and omits WorldCoasts
(missing ocean fill). Do not default `prepare_spa_debug_root`.

Extras (hotels, isolines, SRTM, subway, UGC, Wikipedia/descriptions) are
**on** when an independent local path or URL exists. Empty feeds skip with a
warning. Do not fetch CoMaps map hosts or `cdn.organicmaps.app/subway.json`
to complete them. Pass `--hotels-url`, `--srtm-path`, `--subway-url`,
`--enable-wikipedia`, … when you have sources.

`--from-stage` skips earlier pipeline stages so a failed spa emit does not
rebuild MWMs.

Default ini fragment: `var/etc/map_pipeline.ini` (`NODE_STORAGE: map`,
`THREADS_COUNT: 4`).

Finland full-data evidence is SP-103. Do not run full FI mapgen as a unit test.

## Debug prepare — not production (SPD-087)

`prepare_spa_debug_root` fetches CoMaps CDN `countries.txt`. It is a LAN
debug helper only. Stock / production origin must not use it.

## Serve (SP-051)

```bash
PYTHONPATH=. python3 -m street_pixels serve_spa_publish_tree \
  --root /tmp/sp100 --host 0.0.0.0 --port 8080
```
