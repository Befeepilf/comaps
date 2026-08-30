# SP-103 — Validation plan (Finland first-country run)

**Work item:** [SP-103](../work-items/SP-103-finland-first-country-run.md)
**Plan authored by:** Cursor Agent (`cursoragent@cursor.com`)
**Plan date:** 2026-08-30
**Branch:** `cursor/sp-103-finland-first-run-b3d3`
**Evidence:** [SP-103-evidence-log.md](SP-103-evidence-log.md)

This is a recorded pipeline run, not a feature. Agent does **not** mark
Accepted. Phase 11 exit is **not** met.

---

## Hardware lock (this environment)

**SPD-088** requires a **≥32 GiB** builder for `maps_generator` with
`NODE_STORAGE: map`. This Cloud Agent VM is **~15 GiB RAM, 4 CPUs, no
swap**. Full eight-leaf Finland mapgen here is an unsupported
configuration and is expected to OOM.

**Do not** start `maps_generator` / `map_pipeline` without `--dry-run`
for the full Finland grain on this host. Do not “try anyway”.

There is **no VPS** in this environment (SP-102 origin is documented
only).

---

## What this Cloud Agent run executes

| ID | Action | Pass condition |
| --- | --- | --- |
| A1 | Host facts | Record `git rev-parse HEAD`, `free -h`, `nproc`, disk, swap |
| A2 | Binaries | Record which of `pix_derive_tool`, `generator_tool`, `spa_emit_tool` exist |
| A3 | Unit tests | `PYTHONPATH=. python3 -m unittest street_pixels.tests.test_map_pipeline` — existing suite, no weakening |
| A4 | Dry-run | `map_pipeline --dry-run` with Geofabrik Finland PBF URL and `--countries 'World,Finland_*'` |
| A5 | Dry-run invariants | Does **not** download the PBF; expanded set is eight `Finland_*` leaves + `World`; omits `WorldCoasts`; no CoMaps map hosts in printed plan / rendered ini |
| A6 | Optional checksum | Fetch only `finland-latest.osm.pbf.md5` (small). Do **not** fetch the ~700 MiB PBF. Do **not** fetch any `*.comaps.app` URL |
| A7 | Keys | Confirm no production Ed25519 secret / live `private.h` in git. Do **not** invent Channel B as the public path |
| A8 | Publish | Do **not** rsync or `curl` a VPS that is not here |

## What this Cloud Agent run does not execute

- Eight-leaf `maps_generator` / full `map_pipeline` (no `--dry-run`)
- PBF download
- `.mwm` / `.spa` / `.pix` production
- `VerifyDenseAssignments` on Helsinki
- Channel A signing (no production keys in this environment)
- Channel B inject (not the public path — **SPD-037**)
- Publish / `curl` of `meta/maps.json` or Helsinki objects
- Fallback to CoMaps MWMs
- Highway-proxy U

Leaf size tables are **not** invented. They appear only after a real
generate on a qualifying builder.

---

## Residual maintainer run (≥32 GiB MacBook)

Execute SP-100 for real on a **SPD-088** builder. Same grain:
`--pbf https://download.geofabrik.de/europe/finland-latest.osm.pbf`
(or a pinned equivalent) `--countries 'World,Finland_*'`. Produce eight
leaf `.mwm` + dense `.spa`, assemble, sign with Channel A if P5 keys
exist on that host, publish to the SP-102 origin, `curl` inventory.

Record: git SHA, generator binary identity, PBF URL + checksum, wall
time, peak RAM, per-leaf MWM bytes, spa `area_count` / `assign_count` /
file bytes, Helsinki known-id spot-check, `VerifyDenseAssignments`.

If Channel A keys are missing, stop short of stock-APK advertisement.
Channel B remains debug-only.

Do **not** fall back to CoMaps MWMs. Do **not** use highway-proxy U.

Device download may be SP-104 or residual if no handset.

---

## Acceptance

Maintainer decides. Agent leaves **Accepted by** empty.
