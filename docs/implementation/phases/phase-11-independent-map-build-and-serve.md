# Phase 11 — Independent map build and serve

**Status:** In progress (SP-098 **Accepted** 2026-08-29 — **SPD-087–096**;
SP-099, SP-100, SP-101, and SP-102 **In review**; coding SP-103+ may proceed; exit **not met**)
**Depends on:** Phase 4 residual client/layout track (SP-042–051 tools;
**SPD-027–039**). Does **not** depend on Phase 5–10 exit.
**Blocks:** public S4 stock map URLs (**SPD-093** / **SPD-087**). Does
**not** block Phase 10 exit.

---

## Objective

Stand up a **Street Pixels–owned** pipeline that generates MWM leaves and
production dense `{leaf}.spa` from OSM extracts, assembles the locked CDN≡LAN
publish tree, and serves that tree from our HTTP origin — without fetching
map bytes from CoMaps CDNs.

This phase is operator infrastructure plus the small client/config changes
that point stock builds at that origin. It does not add product features.
It does not implement in-pipeline mapgen `.spa` collectors (Option A).

## Product-spec references

- Geographic coverage: wherever compatible CoMaps **map data** exists
  (format; origin is ours — **SPD-087**).
- §8.3 country-configured polygons; sidecar already **SPD-020**.
- §27.3 / §34 “Offline and map updates”: **manual** map updates remain; no
  automatic map-update protocol (README §3 post-V1).
- §34 “No downloaded map” empty state — unchanged; the files come from our
  host.

## Technical-audit references

- Audit §6 / Spike 6 administrative polygons — emit is Option B (`spa_emit_tool`).
- Audit mapgen / CDN is dated; **verify this file’s code table**, not the 2026-07-20
  audit, for current tools.

## Current code locations

Verified 2026-08-29. Detail:
[`notes/SP-098-map-pipeline-architecture.md`](../notes/SP-098-map-pipeline-architecture.md).

| Concern | Location | Observed state |
| --- | --- | --- |
| MWM generation | `tools/python/maps_generator/`, `generator_tool` | Country extracts supported. Planet `mem` mode is optional and out of this phase’s hardware. `PUBLISH_PATH` is `maps/{series}/{v}/` only — no `meta/maps.json`, no `.spa`. |
| Operator generate CLI | `python3 -m street_pixels map_pipeline` | SP-100 **In review**. Glue Option B: mapgen → pix_derive → rings → spa_emit → assemble (+ optional rsync). VPS generate unsupported. |
| Dense `.spa` | `tools/spa_emit_tool` | Needs leaf `.pix` + rings JSONL + FI policy. |
| MWM→`.pix` | `tools/pix_derive_tool` (`DeriveStreetPixelsUniverse`) | Packaging CLI (SP-099 **In review**). Same 15 m / `IsExplorable` as the client. Empty explored/ever-live. |
| Rings extract | `street_pixels_spike/extract_admin_place_polygons.py` | Spike, FI-proven; called by `map_pipeline` (ring semantics unchanged). |
| Assemble / serve | `assemble_spa_publish_tree`, `serve_spa_publish_tree` | SP-050/051 In review. **Reuse.** LAN: SP-051. Public origin: nginx/Caddy in front of the same root (SP-102 **In review**). |
| Debug CDN fetch | `prepare_spa_debug_root` | Hits public CoMaps `meta/maps.json`. Not production origin. |
| Stock map URLs | untracked gitignored `private.h` + `private.h.street-pixels.example` | Template placeholder `https://maps.example.invalid/`; `METASERVER_URL` empty; `MAP_SERIES` `2026.06.28`. Clones copy the example via `ensure-private-h` / CMake when `private.h` is missing. `configure.sh` calls `configure-world` and refuses CoMaps map hosts. Public origin recipe: SP-102 (`origin.nginx.conf` / `origin.Caddyfile`; live hostname is ops). |
| Policy | `data/street_pixels/country_policies.json` | FI only. |
| Layout / ads / sig | **SPD-035–039**, **SPD-028**, **SPD-036** | Locked. Phase 11 must not invent a second protocol. |

**Difference from the technical audit (2026-07-20):** client `.spa` download
and LAN assemble/serve exist. VPS origin recipe exists (SP-102 **In review**);
live Finland origin is SP-103. Option A still unwired.

## Intended outcome

- Maintainer can, on the 32 GiB build host, produce Finland’s eight leaves
  plus an extract-sourced `World.mwm`, matching `.spa`, signed `countries.txt`,
  and `meta/maps.json`, then publish to the 8 GiB VPS.
- A Street Pixels APK built with `SKIP_MAP_DOWNLOAD=1` and our `private.h`
  downloads those files without contacting CoMaps map hosts.
- Dense `assign[]` matches client universe U (pix derive, not highway proxy).
- Layout remains **SPD-035**. Signatures still verify (**SPD-036**).

## Dependencies

- **SPD-027–039** and SP-044/045/046 tools — already Accepted or In review.
- `generator_tool` from the **same git revision** as the APK (`maps_generator`
  README).
- Product-owner lock of P1–P10 (**SP-098 Accepted**; **SPD-087–096**).

Does not wait for Phase 5–10 exit. Helsinki device walks that today residual
to SP-053 / Phase 10 may consume this origin instead of CoMaps.

## Carried residuals this phase absorbs or leaves

| Residual | Disposition |
| --- | --- |
| SP-044 offline MWM→`.pix` | **SP-099** |
| Eight-leaf FI dense `.spa` with real \|U\| | **SP-103** after SP-099/100 |
| Option A `StageMwm` collectors | **Out** — **SPD-089** |
| SP-050–053 LAN tools | **Reuse**; do not rewrite the layout |
| `prepare_spa_debug_root` CoMaps fetch | Production path **must not** use it (**SPD-087**) |
| Country policies beyond FI | After pipeline exists; not a Phase 11 exit |
| Planet-quality World + WorldCoasts | Residual (**SPD-094**) |

## Work-item breakdown

Coding SP-103+ may proceed (SP-098 **Accepted**; SP-099–102 **In review**).

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-098](../work-items/SP-098-map-pipeline-architecture-decisions.md) | Architecture decisions (**Accepted** 2026-08-29; **SPD-087–096**) |
| 2 | [SP-099](../work-items/SP-099-offline-mwm-pix-derive.md) | Offline leaf MWM → `.pix` derive matching the client (**In review**; not Accepted) |
| 3 | [SP-100](../work-items/SP-100-operator-map-pipeline.md) | Operator CLI: extract → mapgen → pix → rings → spa → assemble (**In review**; not Accepted) |
| 4 | [SP-101](../work-items/SP-101-independent-map-identity.md) | Own map keys, stock host list, `configure.sh` without CoMaps (**In review**; not Accepted) |
| 5 | [SP-102](../work-items/SP-102-publish-and-serve-origin.md) | VPS static origin, rsync, TLS, Range (**In review**; not Accepted) |
| 6 | [SP-103](../work-items/SP-103-finland-first-country-run.md) | Recorded Finland generate+publish with no CoMaps map fetch |
| 7 | [SP-104](../work-items/SP-104-phase11-end-to-end-validation.md) | Phase 11 exit validation |

## Pipeline (locked layout, recommended glue)

See the investigation note §3. HTTP contract is already **SPD-035**:

```text
{base}/meta/maps.json
{base}/maps/{MAP_SERIES}/{dataVersion}/countries.txt[.sig]
{base}/maps/{MAP_SERIES}/{dataVersion}/{mwmLeafId}.mwm
{base}/maps/{MAP_SERIES}/{dataVersion}/{mwmLeafId}.spa
```

## Data and migration concerns

- `map_data_version` on `.spa` must pair the MWM version (SP-050 already
  fail-closes on spa/MWM mismatch).
- Client rematch on map update is Phase 3 (**SPD-013**); this phase only
  publishes a new version directory + bumped `"v"` (**SPD-036**).
- Do not commit binaries, PBF, `.pix`, or secret keys. `private.h` stays
  gitignored.

## Privacy and security implications

- Map generation uses public OSM extracts. No GPS or `.pix` explored bits
  leave the device (derive for packaging writes **empty** explored sets).
- Ed25519 secret stays on the build host, not the VPS git checkout.
- Public origin still requires valid `countries.txt.sig`. No signature bypass
  for “our” server.

## Automated testing strategy

- SP-099: derive→`ScanUniverseAscending` vs a fixture MWM; header/version;
  empty explored; 15 m / `IsExplorable` alignment tests.
- SP-100: dry-run / stage graph unit tests; refuse CoMaps base URLs unless
  an explicit override that tests assert is off by default.
- SP-101: no production test can require the gitignored `private.h`; document
  a template. Signature round-trip with a **test** key, not production keys.
- SP-102: reuse SP-051 Range / health tests against a local root; snippet
  tests for committed nginx/Caddy (`gzip off`, no debug routes).
- SP-104: evidence log of a real Finland run (not CI).

## Manual validation strategy

- SP-103/104: curl `meta/maps.json` and a Helsinki `.mwm`/`.spa` from the VPS;
  APK with our `DEFAULT_URLS_JSON` downloads both; `HasRemoteSpa`; no CoMaps
  map host in a traffic capture **or** a documented hosts-file/DNS block of
  the CoMaps list during the run.
- Device walk remains Phase 10 residual if hardware is residual; this phase’s
  exit is **pipeline evidence**, not Helsinki UX.

## Entry criteria

- SP-042–048 Accepted; SP-049–051 tools present in tree.
- Product-owner lock of P1–P10 (SP-098 **Accepted**; **SPD-087–096**).
- Build host with ≥32 GiB RAM and tens of GiB free disk; VPS reachable for
  SP-102+.

## Exit criteria

1. P1–P10 recorded as Accepted SPDs (or amended locks with rationale).
2. Offline MWM→`.pix` derive exists and is the only production U source for
   emit (no highway proxy).
3. One documented operator command produces the SP-050 tree from an OSM
   extract for Finland (eight leaves) without CoMaps map URLs.
4. VPS (or equivalent) serves that tree; Range GET works for a large MWM.
5. Stock-path advertisement: signed countries with spa fields; Channel B
   not required for the recorded run if P5 keys exist.
6. `configure.sh` / World bootstrap documented without CoMaps (`SKIP_MAP_DOWNLOAD`
   + self-generated World, or fetch from **our** origin).
7. Evidence log: commands, versions, artifact sizes, and a statement that
   CoMaps map hosts were not used.
8. Option A still explicitly out; worldwide policies beyond FI not required
   for this phase’s exit.

## Explicit non-goals

- Option A OSM collectors / `StageMwm` `.spa` emit.
- Full-planet generation; 256 GiB builders.
- Automatic client map updates.
- Country allowlists or “Finland-only app” runtime (product stays worldwide;
  **files** may land incrementally).
- iOS; Explorer Pro purchasing.
- Replacing SP-050/051 with a new layout.
- Editing the product spec or technical audit.
- Competition API hosting (Phase 8 / **SPD-062**).

## Known uncertainties

P1–P10 are **locked** 2026-08-29 via
[`SP-098`](../work-items/SP-098-map-pipeline-architecture-decisions.md) as
**SPD-087–096**. **OQ-40–OQ-49** are closed. P9 is an override (extras **on**).

| Ref | Accepted lock | SPD |
| --- | --- | --- |
| P1 | Stock APK must not list CoMaps map hosts | **SPD-087** |
| P2 | ≥32 GiB builder; 8 GiB VPS serve-only | **SPD-088** |
| P3 | Glue Option B; Option A residual | **SPD-089** |
| P4 | Eight FI leaves + extract World | **SPD-090** |
| P5 | Our Ed25519; Channel A on the public origin | **SPD-091** |
| P6 | Keep `MAP_SERIES` `2026.06.28` | **SPD-092** |
| P7 | Not a Phase 10 blocker; S4 must not ship CoMaps map URLs | **SPD-093** |
| P8 | Skip coastline if extract coasts fail | **SPD-094** |
| P9 | Extras **on** (map tool first) | **SPD-095** |
| P10 | One build-host CLI + rsync | **SPD-096** |

SP-100 residual: extra feeds with no independent source skip+warn (**SPD-095**),
never CoMaps map CDN.
