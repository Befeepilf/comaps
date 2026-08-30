# SP-103 — Evidence log (Finland first-country pipeline run)

**Plan:** [SP-103-validation-plan.md](SP-103-validation-plan.md)
**Branch:** `cursor/sp-103-finland-first-run-b3d3`
**Evidence collection SHA:** `56980a8ce2a07899556080100c1a8eee8f91f463`
  (`[docs] Record SP-102 review-fix evidence`)
**Date:** 2026-08-30
**Status:** Dry-run and host facts recorded. Eight-leaf generate **not
executed**. Publish **not executed**. **Not Accepted.** Phase 11 exit
**not met.**

Agent does not mark SP-103 or Phase 11 complete. Do not copy this log as
a finished Finland generate.

Logs (this Cloud Agent run, not committed):
`/opt/cursor/artifacts/sp103_host_facts.log`,
`sp103_binaries.log`, `sp103_keys_check.log`,
`sp103_test_map_pipeline.log`, `sp103_dry_run.log`,
`sp103_dry_run_ini.txt`, `sp103_geofabrik_md5.log`,
`sp103_finland-latest.osm.pbf.md5`.

---

## Statement: eight-leaf generate did not run

Full eight-leaf Finland `maps_generator` / `map_pipeline` (without
`--dry-run`) **was not started** on this host.

Reason: this Cloud Agent VM has **~15 GiB RAM, 4 CPUs, no swap**.
**SPD-088** requires a **≥32 GiB** builder for `NODE_STORAGE: map`.
Starting mapgen here is an unsupported configuration and is expected to
OOM. Residual: maintainer **≥32 GiB MacBook**.

Additional blockers recorded (not used as a reason to “try anyway”):

- `generator_tool` binary **missing**
- `spa_emit_tool` binary **missing**
- No VPS in this environment (SP-102 origin documented only)

No CoMaps MWM fallback. No highway-proxy U. No fabricated leaf size
table. No PBF / `.mwm` / `.spa` / `.pix` committed.

---

## A1 — Host facts (executed 2026-08-30 01:16 UTC)

```text
$ git -C /workspace rev-parse --abbrev-ref HEAD
cursor/sp-103-finland-first-run-b3d3

$ git -C /workspace rev-parse HEAD
56980a8ce2a07899556080100c1a8eee8f91f463

$ free -h
               total        used        free      shared  buff/cache   available
Mem:            15Gi       960Mi       6.2Gi       9.2Mi       8.9Gi        14Gi
Swap:             0B          0B          0B

$ nproc
4

$ grep -E 'MemTotal|SwapTotal' /proc/meminfo
MemTotal:       16398384 kB
SwapTotal:             0 kB

$ df -h /
Filesystem      Size  Used Avail Use% Mounted on
overlay         252G   13G  227G   6% /
```

`swapon --show` printed nothing (no swap devices). CPU: 4× Intel Xeon
(KVM), `lscpu` recorded in `sp103_host_facts.log`.

RAM **< 32 GiB**. **SPD-088** builder requirement **not** met.

---

## A2 — Binaries (executed)

| Binary | Path | Result |
| --- | --- | --- |
| `pix_derive_tool` | `/workspace/omim-build-debug/pix_derive_tool` | **Present.** ELF 64-bit, 230785208 B, mtime 2026-08-29 23:55 UTC. sha256 `fd919692d7a4417ce4dc8be34037ee6483f54ad9ecf76fb7f664aa385d7c8dc2` |
| `generator_tool` | `/workspace/omim-build-debug/generator_tool` | **Missing.** `omim-build-debug/generator/generator_tool/` is a CMake directory only (no executable) |
| `spa_emit_tool` | `/workspace/omim-build-debug/spa_emit_tool` | **Missing.** `omim-build-debug/tools/spa_emit_tool/` is a CMake directory only (no executable) |

None of the three names exist on `PATH`. Dry-run therefore prints
`spa_emit_tool: spa_emit_tool` (unresolved name; not required when
`dry_run=True`). `pix_derive_tool` resolves to the binary above.

`omim-build-debug/` is untracked (gitignored build tree). Not committed.

---

## A3 — Unit tests (executed; no code change)

```text
$ cd /workspace/tools/python
$ PYTHONPATH=. python3 -m unittest street_pixels.tests.test_map_pipeline
Ran 40 tests in 0.070s
OK
```

Log: `/opt/cursor/artifacts/sp103_test_map_pipeline.log`. Existing
suite. Tests were **not** weakened, skipped, deleted, or narrowed.
`map_pipeline.py` was **not** edited.

---

## A4 / A5 — Finland dry-run (executed; no download)

Command (from `tools/python`):

```text
PYTHONPATH=. python3 -m street_pixels map_pipeline --dry-run \
  --pbf https://download.geofabrik.de/europe/finland-latest.osm.pbf \
  --out /tmp/sp103 \
  --countries 'World,Finland_*'
```

Exit **0**. Printed `dry-run: no subprocess, no network`.

**Did not download** the PBF. Before and after:

- `/tmp/sp103` and `/tmp/sp103.work` **do not exist**
- no `finland-latest.osm.pbf` under `/tmp` or `/workspace`

Stdout (full):

```text
Street Pixels map_pipeline (SP-100)
  stages: mapgen → pix_derive → rings → spa_emit → assemble
  pbf: https://download.geofabrik.de/europe/finland-latest.osm.pbf
  countries selector: World,Finland_*
  expanded countries: World,Finland_Eastern Finland_North,Finland_Eastern Finland_South,Finland_Northern Finland,Finland_Southern Finland_Helsinki,Finland_Southern Finland_Lappeenranta,Finland_Southern Finland_West,Finland_Western Finland_Jyvaskyla,Finland_Western Finland_Tampere
  omit WorldCoasts: True
  skip-coast: False
  NODE_STORAGE: map
  THREADS_COUNT: 4
  mapgen --production: False
  mapgen --skip: Ugc,RoutingTransit,Srtm,IsolinesInfo,DownloadDescriptions,Descriptions,Popularity,PopularityWorld,Reviews
  out: /tmp/sp103
  work_dir: /tmp/sp103.work
  ini: /tmp/sp103.work/map_pipeline.ini
  mapgen_out: /tmp/sp103.work/mapgen
  mwm_dir: (after mapgen)
  pix_dir: /tmp/sp103.work/pix
  rings: /tmp/sp103.work/rings.jsonl
  spa_dir: /tmp/sp103.work/spa
  policy: /workspace/data/street_pixels/country_policies.json
  borders: /workspace/data/borders
  map_series: 2026.06.28
  data_version: (from countries.txt)
  pix_derive_tool: /workspace/omim-build-debug/pix_derive_tool
  spa_emit_tool: spa_emit_tool
  rsync_dest: (none)
  VPS generate: unsupported (SPD-088); this CLI is build-host only
  dry-run: no subprocess, no network
warning: hotels: no independent source; skipping with warning (do not fetch CoMaps map hosts to complete extras)
warning: ugc: no independent source; skipping with warning (do not fetch CoMaps map hosts to complete extras)
warning: subway: no independent source; skipping with warning (do not fetch CoMaps map hosts to complete extras)
warning: srtm: no independent source; skipping with warning (do not fetch CoMaps map hosts to complete extras)
warning: isolines: no independent source; skipping with warning (do not fetch CoMaps map hosts to complete extras)
warning: wikipedia/descriptions: no local dump; skipping (pass --enable-wikipedia to download from Wikipedia; do not fetch CoMaps map hosts)
warning: reviews: no independent source; skipping with warning (do not fetch CoMaps map hosts to complete extras)
```

Expanded set (9 names): `World` + eight Finland leaves. **`WorldCoasts`
omitted.** `skip-coast: False` (World is in the grain; **SPD-094** skip
applies when extract coasts fail, not in this dry-run).

`data/borders/Finland_*.poly` on this tree (8 files, used for expand):

- `Finland_Eastern Finland_North`
- `Finland_Eastern Finland_South`
- `Finland_Northern Finland`
- `Finland_Southern Finland_Helsinki`
- `Finland_Southern Finland_Lappeenranta`
- `Finland_Southern Finland_West`
- `Finland_Western Finland_Jyvaskyla`
- `Finland_Western Finland_Tampere`

Dry-run does not write the ini file. Rendered `ini_text` from the same
`build_plan(..., dry_run=True)` (artifact `sp103_dry_run_ini.txt`):

```text
PLANET_URL: https://download.geofabrik.de/europe/finland-latest.osm.pbf
PLANET_MD5_URL: https://download.geofabrik.de/europe/finland-latest.osm.pbf.md5
NODE_STORAGE: map
THREADS_COUNT: 4
THREADS_COUNT_FEATURES_STAGE: 4
```

Empty extra-feed URLs (`HOTELS_URL:`, `UGC_URL:`, `SUBWAY_URL:`,
`SRTM_PATH:`, `ISOLINES_PATH:`, …). **SPD-095** extras default on; with
no independent source the plan skips those mapgen stages and warns.
It does **not** fill them from CoMaps map hosts.

Needle scan of printed plan fields + rendered ini:
`comaps.app`, `comaps.tech`, `mapgen-fi`, `cdn-us-1`, `cdn-fi-1`,
`cdn.comaps` — **absent**. No `*.comaps.app` URL was fetched as a
“check”.

---

## A6 — Geofabrik `.md5` only (executed; PBF not fetched)

Egress to `download.geofabrik.de` **allowed**. GET of the checksum URL
only:

```text
$ curl -sS --max-time 30 \
    -o /tmp/sp103_finland.md5 \
    https://download.geofabrik.de/europe/finland-latest.osm.pbf.md5
GET_EXIT=0
HTTP/1.1 200 OK
Date: Sun, 30 Aug 2026 01:17:16 GMT
X-Derived-From: europe/finland-260828.osm.pbf.md5
Content-Length: 57
```

Body (57 bytes):

```text
ab51ec4bf46b4b3c87941e6bdce385ff  finland-latest.osm.pbf
```

The **~700 MiB PBF was not downloaded.** Checksum is the current
Geofabrik `finland-latest` sidecar at that timestamp, derived from
`europe/finland-260828.osm.pbf.md5`. Re-fetch on the builder if the
latest extract has moved.

---

## A7 — Channel A keys (executed check; signing not run)

| Check | Result |
| --- | --- |
| Tracked `*.pem` / `*.key` outside `3party/` | None |
| Tracked `private.h` | Not tracked. Gitignored. Example only: `private.h.street-pixels.example` |
| `COUNTRIES_TXT_SIGNATURE_HEX` in the example | 64 zero hex chars — **not** a production key |
| `private.h` on disk | **Absent** |
| `countries_ed25519_secret.pem` | **Absent** |
| Dry-run `secret_key` | `None` |

No production Channel A keys in git. Channel A signing **not
executed**. Channel B was **not** used and is **not** the public path
(**SPD-037** / **SPD-091**). Do not advertise a stock APK against
unsigned or Channel-B-injected countries from this run.

---

## A8 — Publish / curl VPS (not executed)

No VPS in this environment. SP-102 documents nginx/Caddy + rsync with
placeholder host `maps.example.invalid` / `user@vps`. Live hostname is
ops, not in git.

Not executed: rsync, `GET /meta/maps.json`, Helsinki `.mwm`/`.spa`
curl, origin `latest` vs published `"v"`. Residual: SP-102/103
maintainer on the real origin after a qualifying generate.

---

## Acceptance criteria vs this run

| Criterion | This run |
| --- | --- |
| 1. Eight FI `{leaf}.spa` `format_version` 2, `assign_count == \|U\|`, `assign_count > 0` | **Not produced.** Generate did not run. |
| 2. Matching eight `.mwm` in the publish version dir | **Not produced.** |
| 3. Evidence: CoMaps map hosts not used (build + publish) | **Dry-run plan/ini:** no CoMaps hosts; PBF URL is Geofabrik; extras skipped rather than filled from CoMaps. **Build + publish:** not executed — residual. No CoMaps MWM fallback. |
| 4. Origin `meta/maps.json` `latest` equals published `"v"` | **Not executed** (no VPS). |
| 5. Maintainer decides acceptance | **Accepted by** empty. |

Leaf table: **not produced.** Do not invent sizes.

Helsinki `VerifyDenseAssignments`: **not executed.**

---

## Residuals (maintainer ≥32 GiB MacBook)

1. Run SP-100 **without** `--dry-run` on a **SPD-088** host with
   `generator_tool` and `spa_emit_tool` from the **same git revision**
   as the APK. Grain: Geofabrik Finland extract + `World,Finland_*`.
2. Record wall time, peak RAM, per-leaf MWM bytes, spa stats, Helsinki
   known-id / `VerifyDenseAssignments`.
3. Channel A sign if P5 keys exist **on that host**. If missing, stop
   short of stock-APK advertisement. Channel B is not the public path.
4. Publish to the SP-102 origin; `curl` `meta/maps.json` and a Helsinki
   object.
5. SP-104 phase-exit checklist. Device download residual if no handset.

Do **not** fall back to CoMaps MWMs. Do **not** use highway-proxy U if
derive fails.
