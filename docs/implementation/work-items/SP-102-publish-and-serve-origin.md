# SP-102 — Publish and serve origin

**Phase:** 11 — Independent map build and serve
**Status:** Planned
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
  builder, serve here; community CoMaps distributors remain MWM-only and
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
| `docs/DEPLOY_OWN_MAP_SERVER.md` | Existing serve docs |

## Acceptance criteria

1. Written VPS recipe a maintainer can follow without CoMaps.
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
| Branch | — |
| Implemented by | — |
| Accepted by | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Finland publish | SP-103 |
