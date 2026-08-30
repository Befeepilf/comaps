# SP-102 — Publish and serve origin (VPS)

**Status:** Recipe for maintainers. Live hostname is ops, not in git.
**Decisions:** SPD-035, SPD-087, SPD-088, SPD-096.
**Work item:** [SP-102-publish-and-serve-origin.md](../work-items/SP-102-publish-and-serve-origin.md).

Generate on a ≥32 GiB builder (`map_pipeline`, **SPD-088**). The 8 GiB VPS
**serves only**. `maps_generator` on that VPS is unsupported. Community
Streifzug distributors remain MWM-only and are **not** the production path
(**SPD-087**). URL layout stays **SPD-035** — no `/spa/` scheme.

LAN serve remains SP-051 (`serve_spa_publish_tree`). The public origin is
nginx or Caddy in front of the same SP-050 document root.

---

## Document root

Document root is the parent of `maps/` and `meta/` — the same tree
`assemble_spa_publish_tree` / `map_pipeline --out` writes:

```text
{root}/meta/maps.json
{root}/maps/{MAP_SERIES}/{dataVersion}/countries.txt
{root}/maps/{MAP_SERIES}/{dataVersion}/countries.txt.sig
{root}/maps/{MAP_SERIES}/{dataVersion}/{leaf}.mwm
{root}/maps/{MAP_SERIES}/{dataVersion}/{leaf}.spa
```

Placeholder document root in committed configs:
`/var/www/street-pixels/`. Placeholder origin host:
`maps.example.invalid` ([private.h.street-pixels.example](../../../private.h.street-pixels.example)).
Replace both on the VPS; do not commit the live hostname.

`inventory.json` at the tree root is operator-only. The example configs
return 404 for it. Do not enable
`serve_spa_publish_tree --enable-debug-routes` on the public VPS. SP-051
`GET /health` is LAN-only; production omits it.

---

## Publish (rsync)

`map_pipeline --rsync-dest` runs:

```text
rsync -a --delete-delay {out}/ user@vps:/var/www/street-pixels/
```

Trailing slash on the source copies **contents** of `--out` into dest.
`map_pipeline` also appends a trailing slash on dest if `--rsync-dest`
omits it, so dest is a directory (`--delete-delay` follows a dest symlink
instead of replacing it). `--delete-delay` waits until the transfer
finishes, then removes dest files that are not in the source. Equivalent
manual command (SSH keys already trusted):

```bash
rsync -a --delete-delay /path/to/sp100/ user@vps:/var/www/street-pixels/
```

Pipeline (after assemble on the builder):

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels map_pipeline \
  --pbf file:///path/to/finland.osm.pbf \
  --out /tmp/sp100 \
  --rsync-dest user@vps:/var/www/street-pixels/ \
  --secret-key /path/to/countries_ed25519_secret.pem
```

Tree already assembled:

```bash
PYTHONPATH=. python3 -m street_pixels map_pipeline \
  --pbf file:///path/to/finland.osm.pbf \
  --out /tmp/sp100 \
  --from-stage rsync \
  --rsync-dest user@vps:/var/www/street-pixels/
```

`--dry-run` prints `rsync argv` and does not contact the VPS. Ed25519
secret PEM stays on the **build host**, not the VPS git checkout.

### Old version directories

`--delete-delay` makes dest match `--out`. If `--out` holds only the new
`maps/{MAP_SERIES}/{v}/` directory, previous version dirs on the VPS are
removed after the transfer. V1 has **no** automatic cleanup script.

To keep N previous version dirs (recommended N=2):

1. On the VPS, copy them aside before rsync:
   `cp -a /var/www/street-pixels/maps/2026.06.28/<old_v> /var/tmp/keep-<old_v>`
2. Run rsync.
3. Move the kept dirs back under `maps/{MAP_SERIES}/` if they are still
   needed for clients that have not updated.

`meta/maps.json` `latest` still points at the new `v`. Old dirs are only
for in-flight downloads.

---

## Serve (production)

Committed examples (placeholders, not a live host):

| File | Role |
| --- | --- |
| [`tools/python/street_pixels/var/etc/origin.nginx.conf`](../../../tools/python/street_pixels/var/etc/origin.nginx.conf) | nginx + Let’s Encrypt paths |
| [`tools/python/street_pixels/var/etc/origin.Caddyfile`](../../../tools/python/street_pixels/var/etc/origin.Caddyfile) | Caddy auto-HTTPS |

Pick one. Both:

- Serve the document root above (SPD-035).
- Enable HTTP Range for static files (nginx default; Caddy `file_server`).
- Disable gzip of `.mwm` / `.spa` / `.sig` / `.txt` (gzip would break SHA).
- Do not expose debug inventory.

TLS: Let’s Encrypt via `certbot --nginx` (or equivalent) for nginx;
Caddy auto-HTTPS when DNS A/AAAA for the ops hostname points at the VPS
and ports 80/443 are open. Copy the example, substitute the ops hostname
and cert paths, reload.

Do **not** run `python3 -m street_pixels serve_spa_publish_tree` as the
public origin. That CLI is the supported **LAN** path (SP-051).

---

## Health

External check (no `/health` on the public vhost):

```bash
curl -fsS https://maps.example.invalid/meta/maps.json
```

Expect HTTP 200 and JSON with `"status": "active"` for the `MAP_SERIES`
entry. After SP-103, also Range-GET a Helsinki object from the VPS.
This environment does not curl a live origin.

---

## Disk and RAM

Finland-scale artifacts are about **1 GiB** (eight leaves + World +
`.spa` + countries). The 8 GiB VPS is enough to **serve**. Keep N=2 old
version dirs by the manual procedure above (~3 GiB worst case).

**Do not start `maps_generator` / `map_pipeline` on the 8 GiB VPS.**
Mapgen working set is tens of GiB (**SPD-088**). That host serves the
tree; it does not generate it (**SPD-096**).

---

## Stock APK

After DNS and TLS exist, put the HTTPS origin in gitignored `private.h`
`DEFAULT_URLS_JSON` (not a Custom Maps LAN URL; D12). The in-tree
template stays `https://maps.example.invalid/`. See
[sp-101-map-identity.md](sp-101-map-identity.md).
