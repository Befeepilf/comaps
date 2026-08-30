# SP-098 investigation — independent map build and serve

**Authored:** 2026-08-29
**Scope:** Why Street Pixels needs its own MWM+`.spa` generate-and-publish
pipeline, what already exists, and the locks Phase 11 must not guess.
**Authority:** Snapshot of the working tree and accepted SPDs. P1–P10
locked 2026-08-29 as **SPD-087–096** in
[`../work-items/SP-098-map-pipeline-architecture-decisions.md`](../work-items/SP-098-map-pipeline-architecture-decisions.md)
(P9 override: extras on).

---

## 1. Why this is a new phase

Phase 4 residual SP-042–048 shipped **client** download of advertised `.spa`
beside MWM. SP-049–053 shipped **assemble + LAN serve** of a CDN-identical
tree. None of that **generates** MWMs or production dense `.spa` from OSM,
and the documented debug helper still **fetches Streifzug CDN** `meta/maps.json`
(`prepare_spa_debug_root`).

Community map servers in `docs/DEPLOY_OWN_MAP_SERVER.md` only **mirror**
official Streifzug MWMs. Using them as Street Pixels production would put
download load on Streifzug for a different app.

That is an **ops / origin** problem, not Android release hardening (Phase 10
adds no features) and not a Phase 5 UI exit. Option A in-pipeline mapgen
collectors remain an unallocated residual (**SPD-033** / **SPD-038**). This
phase orchestrates **Option B** (offline emit) plus `maps_generator`, on
hardware the project actually has.

---

## 2. Hardware that is in scope

Recorded operator inventory (2026-08-29 conversation; not a measured peak):

| Machine | Role |
| --- | --- |
| MacBook M1 Pro, 32 GiB RAM | **Build host.** `generator_tool`, `spa_emit_tool`, pix derive, assemble, sign, rsync. `NODE_STORAGE: map`, cap `THREADS_COUNT`. |
| Cheap VPS, 8 GiB RAM | **Serve only.** Static HTTP of an SP-050 tree. No `maps_generator`. |

The 256 GiB / `NODE_STORAGE: mem` figure in `map_generator.ini.default` is a
**full-planet** job. It is not the minimum for this phase. Finland-scale
extracts are the documented laptop path (`NODE_STORAGE: map` = “a few
countries”).

---

## 3. Target pipeline (after locks)

```text
Geofabrik / OSM PBF
        │
        ▼
 maps_generator (same git rev as the APK)
        │
        ├── {leaf}.mwm
        │
        ├── pix_derive_tool  ──► {leaf}.pix   (empty explored; U matches client)
        │
PBF ──► rings JSONL (osmium; FilterExplorationCandidate)
        │
        ▼
 spa_emit_tool --mode=production --pix_dir …
        │
        ▼
 assemble_spa_publish_tree  (+ Ed25519 countries.txt.sig)
        │
        ▼
 rsync → VPS document root
        │
        ▼
 static HTTP  {base}/meta/maps.json
              {base}/maps/{MAP_SERIES}/{v}/{leaf}.mwm|.spa
```

One operator entrypoint on the build host. The VPS does not build.

---

## 4. Current code (re-verified 2026-08-29)

| Piece | Location | Observed state |
| --- | --- | --- |
| MWM generation | `tools/python/maps_generator/` + `generator_tool` | Full pipeline. `PUBLISH_PATH` symlinks `{MAP_SERIES}/{mwm_version}/` under a `maps/` folder. **No** `meta/maps.json`. No `.spa`. Optional production extras (hotels, isolines, SRTM, subway). |
| Node RAM mode | `NODE_STORAGE` `map` / `raw` / `mem` | `mem` ~100 GiB, planet. `map` default for a few countries. `raw` least RAM. |
| Dense `.spa` emit | `tools/spa_emit_tool` | Production mode **requires** `--borders_dir` and `--pix_dir`. Geometry-only is not production. |
| Offline MWM→`.pix` | `tools/pix_derive_tool` | **SP-099 In review.** Shared `DeriveStreetPixelsUniverse` (15 m, `IsExplorable`). Empty explored/ever-live. Production U source for `spa_emit_tool --pix_dir`. |
| Rings | `tools/python/street_pixels_spike/extract_admin_place_polygons.py` | Finland-proven osmium spike; not an operator subcommand. |
| Country policy | `data/street_pixels/country_policies.json` | **FI only.** |
| Assemble | `tools/python/post_generation/assemble_spa_publish_tree.py` | SP-050 In review. CDN≡LAN tree (**SPD-035**). |
| Serve | `python3 -m street_pixels serve_spa_publish_tree` | SP-051 In review. Range GETs. |
| Debug prepare | `prepare_spa_debug_root` | Fetches **public Streifzug CDN** countries. Not a production origin. |
| Stock map hosts | `private.h` (gitignored) `DEFAULT_URLS_JSON`, `METASERVER_URL` | Streifzug CDN list. `configure.sh` curls `mapgen-fi-1.streifzug.app` unless `SKIP_MAP_DOWNLOAD=1`. |
| Countries signature | `COUNTRIES_TXT_SIGNATURE_HEX` | Streifzug public half. Custom server does **not** skip verify (**SPD-036**). |
| Layout | **SPD-035** / **SPD-039** | Locked. Do not invent a second URL scheme. |
| Custom Maps URL | Advanced setting | Never a build default (D12 / SP-004). Stock **default host list** is a different knob (`DEFAULT_URLS_JSON`). |

Finland leaf sizes in bundled `data/countries.txt` (CDN bytes, order-of-magnitude for disk planning): eight leaves ≈ 804 MiB + World ≈ 53 MiB + WorldCoasts ≈ 8.5 MiB. Working set for a Finland `maps_generator` run is tens of GiB, not 1 TiB.

---

## 5. Gaps this phase must close

1. **Pix derive** matching client eligibility/sampling, so dense `assign_count == |U|`.
2. **One operator CLI** that runs mapgen → derive → rings → emit → assemble → optional rsync, with explicit extract/country args and no silent Streifzug URLs.
3. **Street Pixels map identity:** own Ed25519 keys; stock APK host list; `configure.sh` never hits Streifzug for World.
4. **VPS serve recipe** for the SP-050 tree (TLS, Range, no gzip of binaries).
5. **A recorded Finland run** that never contacted Streifzug map hosts.

Not in this phase: Option A collectors; planet-quality World/coasts unless a later lock says otherwise; country policies beyond FI (incremental after the pipeline exists).

---

## 6. Spec / decision tensions (report, do not silently pick)

| Tension | Notes |
| --- | --- |
| Spec / **SPD-003** “wherever compatible Streifzug map data exists” | **SPD-087**: **format** compatibility (MWM + our `.spa`), **not** Streifzug CDN origin. Do not edit the spec. |
| README §3 “Automatic map updates” is post-V1 | This phase uses the **existing manual** download/update path. No new auto-update protocol. |
| **SPD-033** packaging is Phase 4 residual | Client/layout work stays there. Phase 11 is **origin + generate**. Do not reopen SP-042–048. |
| D12 Custom URL never a build default | Keep. Changing `DEFAULT_URLS_JSON` to a Street Pixels HTTPS origin is the stock path, not a baked LAN Custom Maps URL. |
| **SPD-084** reuse Streifzug **release machinery** | Listing/signing identity is residual brand. Map **file** origin is this phase. |

---

## 7. Operator defaults (locked)

**SPD-095:** hotels, isolines, SRTM, subway, UGC, Wikipedia/descriptions
**on** by default (map tool first). Build-host datasets only; skip+warn if
no independent source (**SPD-087**).

**SPD-094:** skip `Coastline` if extract coasts fail; omit `WorldCoasts`.
Accept missing ocean fill for the Finland first grain.

**SPD-092:** `MAP_SERIES` stays `2026.06.28` unless a compatibility bump.

---

## 8. What not to do

- Planet `NODE_STORAGE: mem` on the laptop or VPS.
- `maps_generator` on the 8 GiB VPS.
- Highway-proxy U as production assign (SP-044).
- `prepare_spa_debug_root` as the production countries source.
- Unsigned countries on the public host (**SPD-036**).
- Merging spa ads into git `data/countries.txt` before the stock URL serves matching blobs (**SPD-037**).
- Option A `StageMwm` collectors in this phase.
