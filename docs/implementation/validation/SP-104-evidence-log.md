# SP-104 — Evidence log (Phase 11 end-to-end validation)

**Plan:** [SP-104-validation-plan.md](SP-104-validation-plan.md)
**Branch:** `cursor/sp-104-phase11-exit-evidence-b3d3`
**Evidence collection SHA:** `48e02e7bf69d25f8ed03030a3f250c178da02137`
  (`[docs] Record SP-103 Finland dry-run evidence`)
**Date:** 2026-08-30
**Status:** Exit checklist recorded. Eight-leaf Finland generate **did
not run** (SP-103). **Not Accepted.** Phase 11 exit **not met.**

Agent does not mark SP-104, SP-099–103, or Phase 11 complete. Do not
copy this log as a finished Finland generate or live origin proof.

Uncommitted run logs (this Cloud Agent; not in git):
`/opt/cursor/artifacts/sp104_host_facts.log`,
`sp104_binaries.log`, `sp104_option_a_grep.log`,
`sp104_countries_spa.log`, `sp104_identity_check.log`,
`sp104_related_shas.log`, `sp104_python_unittests.log`,
`sp104_pixderive.log`, `sp104_eligibility.log`.

---

## Statement: Phase 11 exit not met

Incomplete Finland run → Phase 11 exit **not met**.

SP-103 (2026-08-30) recorded a **dry-run only**. Full eight-leaf
`maps_generator` / `map_pipeline` **was not started**. Reason: this VM
has **~15 GiB RAM, 4 CPUs, no swap**. **SPD-088** requires **≥32 GiB**.
`generator_tool` and `spa_emit_tool` binaries are **missing**. No VPS.
No Channel A production keys in git.

This SP-104 pass re-verified the tree and re-ran focused tests. It did
**not** start Finland mapgen. It did **not** fabricate VPS curl, APK
download, leaf size tables, or a Streifzug-free live origin.

No Streifzug map fetch was recorded on the **stock** path (template
`DEFAULT_URLS_JSON`, `configure.sh` fail-closed resolver, `map_pipeline`
denylist). That is **not** a Pass of exit 3–5 or 7.

---

## E1 — Host facts (executed 2026-08-30)

```text
$ git -C /workspace rev-parse --abbrev-ref HEAD
cursor/sp-104-phase11-exit-evidence-b3d3

$ git -C /workspace rev-parse HEAD
48e02e7bf69d25f8ed03030a3f250c178da02137

$ free -h
               total        used        free      shared  buff/cache   available
Mem:            15Gi       963Mi       6.2Gi       9.2Mi       8.9Gi        14Gi
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

RAM **< 32 GiB**. **SPD-088** builder requirement **not** met. Same
class of host as SP-103.

### Binaries (this tree; untracked `omim-build-debug/`)

| Binary | Path | Result |
| --- | --- | --- |
| `pix_derive_tool` | `/workspace/omim-build-debug/pix_derive_tool` | **Present.** 230785208 B, mtime 2026-08-29 23:55 UTC. sha256 `fd919692d7a4417ce4dc8be34037ee6483f54ad9ecf76fb7f664aa385d7c8dc2` |
| `street_pixels_tests` | `/workspace/omim-build-debug/street_pixels_tests` | **Present.** 236683256 B, same mtime. sha256 `60cb1594216fae02828fa9baf0fae7cdc87bbbe6c7f1ad590c9a980e11921831` |
| `generator_tool` | `/workspace/omim-build-debug/generator_tool` | **Missing** |
| `spa_emit_tool` | `/workspace/omim-build-debug/spa_emit_tool` | **Missing** |

No full desktop rebuild was started.

### Related work-item SHAs (ancestors of this `HEAD`)

| Item | Status | SHA | Subject |
| --- | --- | --- | --- |
| SP-098 | **Accepted** 2026-08-29 | `1f4a5a88d1faeafa7d057caecef3770c845a8a55` | `[docs] Record Phase 11 P1–P10 locks as SPD-087–096` |
| SP-099 | In review | `cd6cf918ee009e58dd3067e4754db39cd81b2807` | `[docs] Record SP-099 review-fix evidence` |
| SP-100 | In review | `f14802093873fca7f9378889b3bdbfa548ba40bd` | `[docs] Record SP-100 review-fix evidence` |
| SP-101 | In review | `be31c6edad9f733f48e3ac024de6a956e513a7c2` | `[docs] Document untracked private.h copy for clones` |
| SP-102 | In review | `56980a8ce2a07899556080100c1a8eee8f91f463` | `[docs] Record SP-102 review-fix evidence` |
| SP-103 | In review | `48e02e7bf69d25f8ed03030a3f250c178da02137` | `[docs] Record SP-103 Finland dry-run evidence` |

---

## Phase 11 exit criteria 1–8

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | P1–P10 recorded as Accepted SPDs | **Pass** | `docs/implementation/DECISIONS.md` **SPD-087–096**. SP-098 **Accepted** 2026-08-29 (`1f4a5a88d`). OQ-40–OQ-49 closed. P9 extras **on** (**SPD-095**). No Streifzug map fetch on the recorded stock path (see E8) |
| 2 | Offline MWM→`.pix` derive exists; only production U source for emit; no highway proxy | **Pass** (tool + wiring); **residual** on-device U | `tools/pix_derive_tool` present. Shared `DeriveStreetPixelsUniverse` (`libs/map/street_pixels_manager.cpp` / `street_pixels_pix_derive.cpp`). `map_pipeline` spa_emit argv includes `--pix_dir`. No `highway proxy` / `HighwayProxy` under `tools/`. Fixture `PixDerive_*` **6/6**. On-device first-open U vs packaging U **not** compared (no handset / no FI MWM) |
| 3 | One operator command produces the SP-050 tree from a Finland OSM extract (eight leaves) without Streifzug map URLs | **Residual** | SP-103 dry-run only: `map_pipeline --dry-run --pbf https://download.geofabrik.de/europe/finland-latest.osm.pbf --countries 'World,Finland_*'` — eight `Finland_*` + `World`, no PBF download, no Streifzug in plan/ini. Generate **not executed**. `generator_tool` / `spa_emit_tool` missing here |
| 4 | VPS (or equivalent) serves that tree; Range GET works for a large MWM | **Residual** | Committed snippets `origin.nginx.conf` / `origin.Caddyfile` (`gzip off`, placeholder `maps.example.invalid`, no `/spa/`, no debug routes). Local SP-051 Range unit tests **Pass** (not a live origin). No VPS; no `curl` of `meta/maps.json` or Helsinki objects |
| 5 | Stock-path advertisement: signed countries with spa fields; Channel B not required if P5 keys exist | **Residual** | Git `data/countries.txt` has **0** `"spa"` / `"spa_sha1_base64"` nodes (`v` 260714, `map_series` `2026.06.28`) — correct until blobs exist (**SPD-037**). No Channel A secret in git; template `COUNTRIES_TXT_SIGNATURE_HEX` is 64 zero hex chars. Signing **not executed**. Channel B **not** used |
| 6 | `configure.sh` / World bootstrap documented without Streifzug | **Pass** (fail-closed path); **residual** live origin fetch | `configure.sh` calls `map_identity configure-world`. Template origin `https://maps.example.invalid/`. Tests refuse Streifzug / LAN. Live fetch from a real Street Pixels origin is SP-102 residual (placeholder host; no VPS) |
| 7 | Evidence log: commands, versions, artifact sizes, Streifzug map hosts not used | **Residual** | [SP-103-evidence-log.md](SP-103-evidence-log.md) has dry-run + host facts + Geofabrik md5 sidecar only. **No** per-leaf MWM/spa sizes. This log does not invent them |
| 8 | Option A still explicitly out; worldwide policies beyond FI not required | **Pass** | **SPD-089**. `generator/CMakeLists.txt:262` still links `street_pixels_areas`. Grep of `generator/**/*.{cpp,hpp,h,cc}` for `EmitSpa`, `spa_emit`, `WriteSpa`, `StageMwm`, `street_pixels`: **no matches**. Policy file countries keys: `['FI']` only |

**Overall:** Phase 11 exit **not met** (exit 3, 4, 5, and 7 Residual; incomplete Finland generate).

No **Fail** row. Residual is not Pass.

---

## E4 — Layout, spa ads, signature, D12 (tree)

### SPD-035 layout — confirmed

Publish/serve still document one tree (no `/spa/` scheme):

```text
{base}/meta/maps.json
{base}/maps/{MAP_SERIES}/{dataVersion}/countries.txt[.sig]
{base}/maps/{MAP_SERIES}/{dataVersion}/{mwmLeafId}.mwm
{base}/maps/{MAP_SERIES}/{dataVersion}/{mwmLeafId}.spa
```

Pointers: `tools/python/post_generation/assemble_spa_publish_tree.py`
header; `tools/python/street_pixels/serve_spa_publish_tree.py` header;
`map_identity.join_world_url` test `test_join_world_url_uses_spd035_layout`;
origin snippets assert `not "/spa/"` (`test_origin_configs`).
`MAP_SERIES` remains `2026.06.28` (**SPD-092**) in the template and
`map_pipeline.DEFAULT_MAP_SERIES`.

### SPD-028 spa ads — not merged into git `countries.txt`

`data/countries.txt` JSON walk: `spa_bearing_nodes 0`. `rg` for
`spa_sha1` / `"spa"` in that file: **0** matches. Advertisement remains
optional published fields; stock git countries stay spa-free until the
origin serves matching blobs (**SPD-037**). `prepare_spa_debug_root`
Channel B inject is debug-only and tests assert it does not touch the
repo (`test_channel_b_writes_inject_without_touching_repo`).

### SPD-036 signature — not skipped

`libs/storage/storage.cpp` still `VerifyEd25519` on countries apply.
Empty / wrong-size / failed signature **does not** persist countries.
`CustomMapServerUrl` is **not** referenced in `storage.cpp` (no
custom-server bypass). Assemble writes `countries.txt.sig` only with
`--secret-key`. Channel A `prepare_spa_debug_root` requires
`--secret-key`. Throwaway Ed25519 round-trip tests passed (E6).
Production keys absent; that is Residual exit 5, not a skip of verify.

### D12 — no LAN Custom Maps default

`private.h.street-pixels.example`:

```text
#define METASERVER_URL ""
#define DEFAULT_URLS_JSON R"([ "https://maps.example.invalid/" ])"
#define COUNTRIES_TXT_SIGNATURE_HEX "0000000000000000000000000000000000000000000000000000000000000000"
#define MAP_SERIES "2026.06.28"
```

RFC 2606 `.invalid` placeholder, not a private-range IP. `git cat-file
-e HEAD:private.h` fails (`private.h` untracked). Tests:
`test_example_private_h_has_placeholder_origin_not_comaps_or_lan`,
`test_lan_maps_base_url_refused`. Custom Maps remains a user Advanced
override.

Residual (not P1 Fail): `DEFAULT_CONNECTION_CHECK_IP` is still
`151.101.195.52` (connectivity check, not a map CDN — SP-101 follow-up).

---

## E5 — Option A still absent (executed grep)

```text
$ rg -n 'EmitSpa|spa_emit|WriteSpa|StageMwm' /workspace/generator --glob '*.{cpp,hpp,h,cc}'
# (no matches)

$ rg -n 'street_pixels' /workspace/generator --glob '*.{cpp,hpp,h,cc}'
# (no matches)

$ rg -n 'street_pixels_areas' /workspace/generator
generator/CMakeLists.txt:262:    street_pixels_areas
```

Link without emit call sites. Residual **SPD-089** (unallocated; not a
Phase 11 coding item).

---

## E6 — Focused Python tests (executed; no code change)

Command:

```text
cd /workspace/tools/python
PYTHONPATH=. python3 -m unittest \
  street_pixels.tests.test_map_pipeline \
  street_pixels.tests.test_map_identity \
  street_pixels.tests.test_origin_configs \
  street_pixels.tests.test_serve_spa_publish_tree \
  street_pixels.tests.test_prepare_spa_debug_root
```

Loader counts at run time: `test_map_pipeline` **40**,
`test_map_identity` **25**, `test_origin_configs` **3**,
`test_serve_spa_publish_tree` **18**, `test_prepare_spa_debug_root` **6**.

```text
Ran 92 tests in 5.331s
OK
```

Log: `/opt/cursor/artifacts/sp104_python_unittests.log`. Existing
suites. Tests were **not** weakened, skipped, deleted, or narrowed.
Production Python/C++ was **not** edited on this branch.

`test_prepare_spa_debug_root` still covers the **debug** Streifzug-fetch
helper. That helper is **not** the stock path (`map_pipeline.py`
states `prepare_spa_debug_root is not this path`). Its default bases
listing Streifzug hosts remains a documented debug residual, not a P1
Fail on the stock APK origin.

---

## E7 — PixDerive / Eligibility (existing binary; no rebuild)

```text
$ /workspace/omim-build-debug/street_pixels_tests \
    --data_path=/workspace/data --user_resource_path=/workspace/data \
    --filter='PixDerive'
Running pix_derive_tests.cpp::PixDerive_SegmentizeStreetUses15mSampling
OK
Running pix_derive_tests.cpp::PixDerive_EmptyOutDirIsBadOutput
OK
Running pix_derive_tests.cpp::PixDerive_WriteUnexploredUniversePixFailClosedOnEmpty
OK
Running pix_derive_tests.cpp::PixDerive_FailClosedMissingAndCorruptMwm
OK
Running pix_derive_tests.cpp::PixDerive_UniverseRoundTripOnFixtureMwm
# DeriveStreetPixelsUniverse on data/minsk-pass.mwm → |U|=24069, 3718 streets
OK
Running pix_derive_tests.cpp::PixDerive_FailClosedOnWorldAndWorldCoastsFilenames
OK
All tests passed.
```

`grep -c '^OK$'` = **6**; `grep -c '^Running '` = **6**.

```text
$ /workspace/omim-build-debug/street_pixels_tests \
    --data_path=/workspace/data --user_resource_path=/workspace/data \
    --filter='Eligibility'
```

**10/10** All tests passed (`grep -c '^OK$'` = 10). **Substring
over-count:** 9 `Eligibility_*` plus
`CompetitionOwnership_ImportedOnlyDoesNotAffectEligibilityOrContested`.
Dedicated `eligibility_tests.cpp` names: **9/9**. `data/classificator.txt`
was present (35452 B). Binary is the 2026-08-29 build (not rebuilt to
this docs SHA).

---

## E8 — Stock path vs Streifzug (P1)

Recorded stock path in git:

- Template `DEFAULT_URLS_JSON` = `https://maps.example.invalid/` (no
  `streifzug.app` / `comaps.tech`).
- `configure.sh` → `map_identity configure-world`; Streifzug
  `MAPS_BASE_URL` / `mapgen-fi-1.streifzug.app` refused in tests (executed
  `ERROR: refusing Streifzug map host 'mapgen-fi-1.streifzug.app'…` then **ok**).
- `map_pipeline` default ini / plan refuse Streifzug unless
  `--allow-comaps-origin` (default **off**). HTTPS `--pbf` allowlist is
  Geofabrik / planet.openstreetmap.org.
- SP-103 dry-run PBF URL was Geofabrik; extras skip+warn rather than
  filling from Streifzug CDNs.

**No** Streifzug map fetch on the recorded stock path → not a P1 Fail.

Not claimed: live APK traffic capture, live origin GET, or Finland
generate without Streifzug (those are Residual).

Debug-only `prepare_spa_debug_root` default bases still include Streifzug
CDNs (SP-101 residual). Production CLI must not use it.

---

## Device / APK / live origin (out of scope here)

| Check | Result |
| --- | --- |
| Spec §34 device matrix | Out of scope (Phase 10) |
| APK with template `private.h` downloading Helsinki `.mwm` then `.spa` | **Residual** — no handset, no FI tree, no live origin |
| `HasRemoteSpa` / SHA on device | **Residual** |
| Hosts-file/DNS block of Streifzug map peers | **Residual** |
| Live `curl` `https://<origin>/meta/maps.json` | **Residual** — hostname is ops; git has `maps.example.invalid` only |

---

## Residuals (maintainer)

1. Eight-leaf FI generate + dense `.spa` on a **≥32 GiB** builder
   (`generator_tool` + `spa_emit_tool` same revision as the APK) —
   SP-103 / **SPD-088**.
2. Channel A sign if P5 keys exist on that host; do not use Channel B
   as the public path (**SPD-037**).
3. Publish + live Range GET / `curl` inventory on the SP-102 origin.
4. On-device U compare vs `pix_derive_tool` on the same leaf MWM.
5. Device download of Helsinki `.mwm`+`.spa` from our origin (Phase 10
   / SP-095 residual if no handset).
6. Option A still unallocated (**SPD-089**).
7. `DEFAULT_CONNECTION_CHECK_IP` still Streifzug Fastly (connectivity, not
   map CDN).
8. Country policies beyond FI — after Phase 11 exit.

Do **not** fall back to Streifzug MWMs. Do **not** use highway-proxy U.
Do **not** merge spa ads into git `data/countries.txt` until the stock
URL serves matching blobs.
