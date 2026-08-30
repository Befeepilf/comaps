# SP-102 — Publish and serve origin

**Phase:** 11 — Independent map build and serve
**Status:** In review
**Depends on:** SP-098 lock (**SPD-088**, **SPD-096**); SP-050/051 tools
**Unblocks:** SP-103

---

## Objective

Document and, where useful, script **rsync of an SP-050 tree** to the 8 GiB
VPS and serve it with static HTTP that matches production: `meta/maps.json`,
`maps/{MAP_SERIES}/{v}/…`, Range GETs, no gzip of `.mwm`/`.spa`/`.sig`.

Reuse `serve_spa_publish_tree` for LAN; production VPS should be nginx/Caddy
(or equivalent) in front of the same document root.

---

## Motivation

SP-051 is the LAN server. A public origin needs TLS, a stable hostname for
`DEFAULT_URLS_JSON`, and a publish step from the Mac. The VPS must not run
`maps_generator`.

---

## In-scope behavior

- Operator doc: document root layout (parent of `maps/` and `meta/`), TLS
  (Let’s Encrypt or equivalent), `gzip off` for binary map types, `Range`
  enabled.
- `rsync -a --delete-delay` (or documented equivalent) from pipeline `--out`
  to the VPS. Optional `--rsync-dest` on SP-100 after this WI.
- Health: SP-051 `/health` is LAN-only; production may omit debug routes.
  External check = `GET /meta/maps.json` 200 + `status: active`.
- Disk budget: Finland-scale artifacts ~1 GiB; keep N old version dirs
  documented (not automatic cleanup in V1 unless trivial).
- Confirm 8 GiB RAM is enough to **serve** (yes); warn if someone starts
  mapgen there.
- Update `docs/DEPLOY_OWN_MAP_SERVER.md` Street Pixels section: generate on
  builder, serve here; community Streifzug distributors remain MWM-only and
  are **not** our production path (P1).
- Tests: none against the live VPS in CI. Reuse SP-051 unit tests.

## Out-of-scope behavior

- Changing URL layout (**SPD-035**).
- Metaserver implementation if `METASERVER_URL` is emptied (P1 may set
  empty and rely on `DEFAULT_URLS_JSON` only — match storage behavior).
- CDN (Cloudflare etc.) unless the maintainer adds it later; not required
  for exit.
- Exact hostname (ops; record in evidence, not as a compiled default in
  git beyond the example template).

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `tools/python/street_pixels/serve_spa_publish_tree.py` | LAN reference |
| `tools/python/post_generation/assemble_spa_publish_tree.py` | Tree to upload |
| `tools/python/street_pixels/map_pipeline.py` | `--rsync-dest` (`rsync -a --delete-delay`) |
| `tools/python/street_pixels/var/etc/origin.nginx.conf` | nginx example (placeholder host) |
| `tools/python/street_pixels/var/etc/origin.Caddyfile` | Caddy example (placeholder host) |
| `docs/DEPLOY_OWN_MAP_SERVER.md` | Generate on builder; serve here |
| `docs/implementation/notes/sp-102-publish-and-serve-origin.md` | VPS recipe |

## Acceptance criteria

1. Written VPS recipe a maintainer can follow without Streifzug.
2. Local SP-051 still the supported LAN path.
3. rsync/publish command documented from pipeline `--out`.
4. Maintainer decides acceptance.

## Required automated tests

- None new if SP-051 tests remain.

## Required manual validation

- After SP-103: `curl` maps.json and a Helsinki object from the VPS.

## Failure and rollback considerations

- Do not enable `--enable-debug-routes` on a public VPS.
- Do not gzip binaries (SHA mismatch).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-102-publish-serve-origin-b3d3` |
| Commits | `c1046ee3e` `[tools] Align map_pipeline rsync with delete-delay`; `a75a7b0d2` `[tools] Add nginx and Caddy origin example configs`; `f060e4513` `[docs] Document VPS publish and static origin serve`; `ed64dd046` `[docs] Record SP-102 in-review evidence`; `5dd65b684` `[tools] Force rsync dest trailing slash`; this `[docs]` commit |
| Recipe | [notes/sp-102-publish-and-serve-origin.md](../notes/sp-102-publish-and-serve-origin.md) — document root parent of `maps/` and `meta/`; TLS Let’s Encrypt / Caddy auto-HTTPS; `gzip off`; Range on; no `--enable-debug-routes`; health `GET /meta/maps.json` 200 + `status: active`; keep N=2 old version dirs **manual**; 8 GiB serves, mapgen on VPS unsupported |
| rsync | `rsync -a --delete-delay {out}/ user@vps:/var/www/street-pixels/` — same argv as `map_pipeline --rsync-dest`. Trailing slash on source and dest (`build_rsync_argv` appends dest `/` if omitted). Hostname in git is `maps.example.invalid` / `user@vps` only |
| nginx / Caddy | `tools/python/street_pixels/var/etc/origin.nginx.conf`; `tools/python/street_pixels/var/etc/origin.Caddyfile` |
| LAN | SP-051 `serve_spa_publish_tree` unchanged |
| Tests | `cd tools/python && PYTHONPATH=. python3 -m unittest street_pixels.tests.test_serve_spa_publish_tree street_pixels.tests.test_map_pipeline street_pixels.tests.test_origin_configs` — **61/61** OK (`test_serve_spa_publish_tree` 18; `test_map_pipeline` 40; `test_origin_configs` 3). No live VPS curl. Full FI mapgen **not** run (SP-103). |
| Implemented by | Cursor Agent (`cursoragent@cursor.com`) |
| Reviewed by | Independent review agent (fix `5dd65b684`; not Accepted) |
| Accepted by | — |

Phase 11 exit is **not** met.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Finland publish | SP-103 |
| `curl` maps.json and a Helsinki object from the VPS | SP-103 (this environment has no VPS) |
| `--rsync-dest` dest without trailing slash + `--delete-delay` can replace a dest symlink | Fixed in `5dd65b684` (`build_rsync_argv` appends dest `/`) |
| nginx `types` maps `.txt` → `application/json` | Not a defect: `countries.txt` is JSON; SP-051 allows `application/json` or `text/plain`; client hashes/parses the body, not Content-Type |
