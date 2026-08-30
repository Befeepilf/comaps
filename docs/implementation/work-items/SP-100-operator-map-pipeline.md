# SP-100 — Operator map pipeline CLI

**Phase:** 11 — Independent map build and serve
**Status:** In review
**Depends on:** SP-098 lock (**SPD-088**, **SPD-089**, **SPD-090**, **SPD-094**, **SPD-095**, **SPD-096**); SP-099
**Unblocks:** SP-103

---

## Objective

Provide **one** build-host entrypoint that, given an OSM extract and a
country/leaf selector, runs:

1. `maps_generator` (same revision as the APK)
2. `pix_derive_tool` (SP-099)
3. rings JSONL extract (promote the SP-023 osmium script to a called step)
4. `spa_emit_tool --mode=production`
5. `assemble_spa_publish_tree` (SP-050)

and writes a ready SP-050 `--out` tree. Optional last step: rsync (SP-102
documents the remote layout; the CLI may `--rsync-dest` once that WI lands).

---

## Motivation

Today a maintainer must stitch ini files, spike scripts, and three CLIs.
Missed `--pix_dir` or a Streifzug `prepare_spa_debug_root` is the default
failure. “Seamless” is this command, not a VPS daemon.

---

## In-scope behavior

- Python module under `tools/python/street_pixels/` (e.g. `map_pipeline`)
  registered on `python3 -m street_pixels map_pipeline`.
- **Explicit** inputs only: `--pbf` / `PLANET_URL` as `file://` or Geofabrik
  HTTPS; `--countries` (default from lock P4: `World,Finland_*` or
  `Finland_*` plus World per P8); `--out`; `--map-series`; `--data-version`
  / generator version; `--iso`; `--policy`; `--borders-dir`; `--secret-key`
  optional until SP-101.
- Default ini fragment in-repo: `NODE_STORAGE: map`, low `THREADS_COUNT`,
  P9 extras **on** (**SPD-095**), no Streifzug map-CDN URLs (**SPD-087**).
  Extra feeds with no independent source: skip that stage with a warning.
- Stages skippable (`--from-stage`) so a failed spa emit does not rebuild
  MWMs.
- Refuse to run if `--cdn-base` / Streifzug hosts are set unless
  `--allow-comaps-origin` (default **off**; tests assert off).
- `--dry-run` prints the graph and paths.
- Promote `extract_admin_place_polygons.py` to an operator-called module
  without changing ring semantics (true closed rings only).
- Docs: operator README in the module + pointer from
  `docs/DEPLOY_OWN_MAP_SERVER.md` Street Pixels section (generate, not only
  serve).
- Unit tests: graph order, skip-coast vs WorldCoasts invariant, default
  origin denylist.

## Out-of-scope behavior

- HTTP server (SP-051 / SP-102).
- `private.h` (SP-101).
- Finland full-data evidence (SP-103).
- Option A.
- Running on the 8 GiB VPS as a supported mode (document unsupported).
- Planet `NODE_STORAGE: mem`.

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `tools/python/maps_generator/` | Stage driver (invoked as `python3 -m maps_generator`) |
| `tools/python/street_pixels/__main__.py` | Subcommands |
| `tools/python/street_pixels/map_pipeline.py` | Operator CLI |
| `tools/python/street_pixels/var/etc/map_pipeline.ini` | Default ini (`NODE_STORAGE: map`, `THREADS_COUNT: 4`) |
| `tools/python/post_generation/assemble_spa_publish_tree.py` | Assemble |
| `tools/python/street_pixels_spike/extract_admin_place_polygons.py` | Rings (called unchanged) |
| `tools/spa_emit_tool/` | Dense emit |
| `tools/pix_derive_tool/` | MWM → `.pix` |

## Acceptance criteria

1. `python3 -m street_pixels map_pipeline --help` documents stages.
2. Dry-run on a fake layout does not hit the network.
3. Default config contains no Streifzug map host strings.
4. `maps_generator` skip-coast vs World* rule is enforced or documented as a
   preflight error (P8).
5. Maintainer decides acceptance.

## Required automated tests

- Dry-run / denylist / stage order.
- Do not run full FI mapgen in CI.

## Required manual validation

- None in this WI; SP-103 is the real extract run.

## Failure and rollback considerations

- Do not default `prepare_spa_debug_root`.
- Do not download Streifzug World as a fallback if extract World fails.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-100-operator-map-pipeline-b3d3` |
| Commits | `888f2c80e` `[tools] Add street_pixels map_pipeline operator CLI`; `edde5d4b2` `[tools] Clarify map_pipeline wikipedia skip warning`; `c3ae80135` `[docs] Document map_pipeline generate path`; `a4c879d8c` `[tools] Fix map_pipeline MD5 sidecar and origin checks`; this `[docs]` commit |
| CLI | `python3 -m street_pixels map_pipeline` — stages mapgen → pix_derive → rings → spa_emit → assemble (optional rsync last) |
| Default grain | `--countries World,Finland_*`; `WorldCoasts` omitted; Coastline **not** skipped unless `--skip-coast` and World is absent from the **expanded** set |
| Origin | Default ini has no Streifzug map hosts; `--cdn-base` / Streifzug hosts (`*.streifzug.app`, `*.comaps.tech`, listed community mirrors) refused unless `--allow-comaps-origin` (default off). HTTPS `--pbf` limited to Geofabrik / planet.openstreetmap.org. |
| Tests | `cd tools/python && PYTHONPATH=. python3 -m unittest street_pixels.tests.test_map_pipeline` — **36/36** OK. Existing `test_prepare_spa_debug_root` + `test_serve_spa_publish_tree` — **24/24** OK. `--help` documents stages. Dry-run with `file:///tmp/finland.osm.pbf` does not invoke urllib / `maps_generator` / MD5 write / publish-tree mkdir. Full FI mapgen **not** run (SP-103). |
| Implemented by | Cloud agent (`befeepilf@protonmail.com`) |
| Reviewed by | Independent review agent (fixes in `a4c879d8c`; not Accepted) |
| Accepted by | — |

Phase 11 exit is **not** met.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Rsync / nginx / TLS | SP-102 (`--rsync-dest` is thin glue only) |
| Real FI extract run | SP-103 |
| `maps_generator/__main__.py` skip-coast iterates `options.countries` as characters | Left unfixed; operator CLI preflights the **expanded** country set |
| Local PBF without `.md5` never written (predicted `file://` short-circuit) | Fixed in `a4c879d8c` |
| Streifzug denylist substring holes (`*.streifzug.app` not listed; path false positives) | Fixed in `a4c879d8c` (hostname + suffix; HTTPS PBF allowlist) |
| Skip-token / from-stage / MD5 tests were incomplete (false greens) | Strengthened in `a4c879d8c` (36 tests) |
