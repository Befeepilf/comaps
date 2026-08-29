# SP-100 — Operator map pipeline CLI

**Phase:** 11 — Independent map build and serve
**Status:** Planned
**Depends on:** SP-098 lock (P2, P3, P4, P8, P9, P10); SP-099
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
Missed `--pix_dir` or a CoMaps `prepare_spa_debug_root` is the default
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
  P9 extras **commented off**, no CoMaps URLs.
- Stages skippable (`--from-stage`) so a failed spa emit does not rebuild
  MWMs.
- Refuse to run if `--cdn-base` / CoMaps hosts are set unless
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
| `tools/python/maps_generator/` | Stage driver |
| `tools/python/street_pixels/__main__.py` | Subcommands |
| `tools/python/post_generation/assemble_spa_publish_tree.py` | Assemble |
| `tools/python/street_pixels_spike/extract_admin_place_polygons.py` | Rings |
| `tools/spa_emit_tool/` | Dense emit |

## Acceptance criteria

1. `python3 -m street_pixels map_pipeline --help` documents stages.
2. Dry-run on a fake layout does not hit the network.
3. Default config contains no CoMaps map host strings.
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
- Do not download CoMaps World as a fallback if extract World fails.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | — |
| Implemented by | — |
| Accepted by | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Rsync / nginx | SP-102 |
| Real FI run | SP-103 |
