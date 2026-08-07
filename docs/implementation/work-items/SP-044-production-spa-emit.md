# SP-044 — Production leaf `.spa` emit (Phase 4 R1)

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** In review
**Branch:** `cursor/sp-042-sidecar-shipping-fe62`
**Depends on:** SP-043 In review / Accepted (**SPD-034** frozen v2 contract);
  SP-032 Accepted (`spa_emit_tool` / `spa_jsonl`); SPD-020–025; SPD-032/034
**Unblocks:** SP-045 (countries meta can advertise real leaf blobs); SP-048
  packaging validation; CDN publish tree for FI

---

## Objective

Close narrowed Phase 4 residual **R1 emit**: produce **production-grade**
per-MWM-leaf `{mwmLeafId}.spa` files suitable for CDN packaging for **at least
Finland** (all eight `countries.txt` leaves), with:

- `format_version = 2`, `nside = 1048576`, `universe_order = AscendingNest`
- Dense `assign[]` for ascending NEST universe **U** of that leaf’s valid street
  pixels (not geometry-only)
- True closed rings filtered by country policy (`FilterExplorationCandidate`)
- Settlements present in the sidecar but **not** assign targets

Do **not** invent grids. Do **not** wire `countries.txt` meta (**SP-045**) or
client download (**SP-046**).

---

## Recommended approach — Option B (offline production batch)

### Verdict

**Implement Option B as SP-044.** Record full in-pipeline mapgen collector
wiring (**Option A**) as an explicit follow-up residual. Option C alone does not
close R1 without a ring source and a matching universe **U**.

| Option | Summary | Invasiveness | Closes R1 packaging? |
| --- | --- | --- | --- |
| **A** | OSM collectors inside `generator/` + emit beside each leaf `.mwm` during `maps_generator` | **Huge** — new collectors, classificator/place paths untouched today, `StageMwm` hook, universe derive during mapgen, blast radius on every country build | Yes, eventually — too large for one WI |
| **B** | Offline production batch tool: rings + policy + leaf universe → `{leaf}.spa` into a publish tree | **Small–medium** — extend `spa_emit_tool` / `spa_jsonl`; optional spatial acceleration; no PlaceProcessor / three-box changes | **Yes** for FI CDN emit |
| **C** | End-of-country hook that only runs assigner on polygons collected elsewhere | Medium — still needs ring source + **U**; adds `maps_generator` stage without solving collectors | Partial; collapses into B or A |

### Rationale (grounded in tree)

1. **`generator/` links `street_pixels_areas` but does not call it.**
   `generator/CMakeLists.txt` already PUBLIC-links `street_pixels_config` and
   `street_pixels_areas`; no `.cpp` under `generator/` references
   `WriteExplorationSidecar`, `FilterExplorationCandidate`, or `.spa`. SP-026
   deferred the mapgen hook; SP-032 shipped offline geometry-only emit.

2. **Exploration rings are not produced by PlaceProcessor today.**
   `PlaceProcessor` / `collector_routing_city_boundaries` feed World
   three-box search boundaries. Exploration must use true closed rings +
   `FilterExplorationCandidate` (SPD-020/025; §8.3). Production can reuse the
   proven SP-023 osmium JSONL extract + `spa_jsonl` — not invent a second
   collector in this WI.

3. **Dense assign needs leaf universe U that matches client `.pix`.**
   `assign[i]` ↔ ascending NEST `U[i]` (`ScanUniverseAscending`, SPD-034 /
   SP-028). `.pix` is derived **on-device** from MWM features
   (`DeriveStreetPixelsFromFeatures`), not during mapgen. Highway→HEALPix
   proxy (SP-023) is measurement-only and must **not** ship. Production emit
   must take **U** from a leaf `.pix` (preferred input) or an offline derive
   that matches client eligibility/sampling (15 m, `IsExplorable`).

4. **Finland CDN grain is eight leaves, not a country-concat.**
   `countries.txt` children: `Finland_Southern Finland_Helsinki`, … (8 leaves).
   SP-032 `Finland.spa` country-concat was a size measurement artifact only.
   Production emit writes eight `{mwmLeafId}.spa` files.

5. **Option A is multi-WI.** New OSM collectors, affiliation into leaves,
   `maps_generator` stage, and universe derive inside mapgen touch the highest
   blast-radius subsystem. That can follow once B proves FI leaf blobs.

### Option B pipeline (this WI)

```
Geofabrik FI PBF ──► extract_admin_place_polygons.py ──► rings JSONL
                                                              │
data/street_pixels/country_policies.json (FI) ────────────────┤
data/borders/Finland_*.poly (centroid leaf attribution) ──────┤
leaf .pix OR leaf .mwm → Derive → ascending U + centres ──────┤
                                                              ▼
                    spa_emit_tool (production mode)
                      FilterExplorationCandidate
                      WriteExplorationSidecar(..., sampleCentres)
                                                              ▼
                    publish_tree/{mwmLeafId}.spa  (v2 + dense assign)
```

---

## Motivation

Phase 4 exit used offline FI geometry-only `.spa` (SP-032). Shipping decisions
(SPD-027–033) and blob freeze (SPD-034 / SP-043) are in place. CDN packaging
still has no production leaf blobs with dense assign. R1 emit must close before
meta advertisement (SP-045) and client download (SP-046) are meaningful.

---

## In-scope behavior

- Work-item file + README / phase-04 index (this planning commit); then
  implementation on the same WI:
- Extend `tools/spa_emit_tool/` (or sibling production CLI) beyond SP-032
  geometry-only:
  - Emit **all eight** FI leaf `{mwmLeafId}.spa` matching `countries.txt` /
    `data/borders/Finland_*.poly` leaf ids.
  - `format_version` 2 with frozen `nside` / AscendingNest (SPD-034).
  - Dense assign from ascending NEST **U** of that leaf (from `.pix` via
    `ScanUniverseAscending` + HEALPix centres, or offline derive matching
    client sampling).
  - Rings via existing `spa_jsonl` + `FilterExplorationCandidate` (FI policy);
    leaf attribution = centroid-in-border (same as SP-023/032) unless a
    documented conflict forces affiliation PIP.
  - Settlements admitted into areas section; `AssignSubdivision` /
    `IsAssignable()` must never target them.
- Measure and record per-leaf section sizes (areas + assign) under `/tmp/`
  (not committed).
- Spot-check Helsinki known OSM ids (reuse SP-032 list); verify
  `assign_count == |U|`, ascending slot contract, and
  `VerifyDenseAssignments` on a sampled or full Helsinki run.
- Automated tests: synthetic dense-assign emit→load; reject geometry-only as
  the production path default; settlement-not-in-assign regression.
- Docs: SP-026 note emission pointer; phase-04 R1 emit half → this WI;
  discovered-follow-up for Option A.

### Spatial acceleration (conditional in-scope)

`BuildDenseAssignments` is currently O(|U| × |areas|) and rebuilds `RegionD`
per `Contains` call. Helsinki-class |U| ≈ 6.5×10⁶ with hundreds of candidates
may be too slow without a bbox / STRtree (or equivalent) prefilter. If
full-leaf emit is impractical on desktop without it, **add acceleration inside
`street_pixels_areas` as part of this WI** — do not invent grids or skip dense
assign.

---

## Out-of-scope behavior

- **`countries.txt` / `spa` / `spa_sha1_base64` meta — SP-045 (SPD-028).**
- **Client leaf download — SP-046 (SPD-027/031).**
- **Update / delete lifecycle — SP-047; incomplete/retry signaling — SP-048.**
- Full in-pipeline OSM collectors + `maps_generator` StageMwm emit (**Option A**)
  — follow-up residual, not this WI’s acceptance bar.
- Editing product spec or technical audit.
- Inventing grids, place-node polygons, or three-box exploration geometry.
- Changing `nside` / universe order (frozen SPD-017 / SPD-034).
- Worldwide country configs beyond FI seed.
- Committing PBF / JSONL / `.spa` / `.pix` / MWM binaries.
- Marking this WI or Phase 4 R1 Accepted/Complete unilaterally.

---

## Relevant product requirements / decisions

- SPD-017 (`nside`), SPD-020–022, SPD-023 (FI policy), SPD-024, SPD-025
  (settlement rings in sidecar, not assign targets).
- SPD-021 (generator-precomputed dense map — offline batch counts as the
  precompute job for V1 packaging).
- SPD-032 / **SPD-034** (v2 header freeze); SPD-033 (pre-production track).
- Spec §8.3 (no place-node invention), §8.8 (priority / smallest / stable id).
- Notes: SP-026 format; SP-028 universe order; SP-032 harness.

---

## Relevant source files or symbols

### Reuse / extend

| Path / symbol | Role |
| --- | --- |
| `tools/spa_emit_tool/spa_emit_tool.cpp` | CLI; upgrade to dense production emit |
| `libs/street_pixels_areas/spa_jsonl.*` | JSONL parse / filter / leaf centroid filter |
| `FilterExplorationCandidate` | Policy gate |
| `WriteExplorationSidecar` | v2 writer + dense assign from sample centres |
| `BuildDenseAssignments` / `AssignSubdivision` | §8.8; settlements not assignable |
| `ScanUniverseAscending` (`libs/map/street_pixels_file.*`) | Ordered U from `.pix` |
| `hp::GetHealpixBase().pix2ang` | Cell centres → mercator samples |
| `VerifyDenseAssignments` | Fixture / offline cross-check |
| `data/street_pixels/country_policies.json` | FI policy |
| `data/borders/Finland_*.poly` | Eight leaf borders |
| `tools/python/street_pixels_spike/extract_admin_place_polygons.py` | Ring source (osmium); not mapgen |

### Do **not** modify for exploration emit

| Path | Why |
| --- | --- |
| `generator/place_processor.*` | Three-box / World search path |
| `generator/collector_routing_city_boundaries.*` | Same |
| `generator/cities_boundaries_builder.*` | Search-only CityBoundary |

### Linked but unused today (Option A residual)

| Path | Note |
| --- | --- |
| `generator/CMakeLists.txt` → `street_pixels_areas` | Link only; no emit call sites |
| `tools/python/maps_generator/.../stages_declaration.py` `StageMwm` | No `.spa` stage |

### Likely new / touched for Option B

- `tools/spa_emit_tool/` — production flags (`--pix` / `--mwm`, `--borders_dir`,
  `--publish_dir`, dense mode default).
- `libs/street_pixels_areas/` — helpers to build sample centres from ascending
  nest ids; optional spatial index for `BuildDenseAssignments`.
- CMake link: `spa_emit_tool` may need `map` (or a thin healpix/centre helper)
  for `.pix` scan + centres — call out if linking `map` is too heavy and extract
  a smaller helper instead.
- `libs/street_pixels_areas/street_pixels_areas_tests/` — dense FI-scale
  synthetic tests (not full FI PBF in CI).
- Docs: this file; `notes/SP-026-spa-format.md` emission section; README;
  phase-04 R1 row.

---

## Environment feasibility (dense assign)

Measured/known:

| Fact | Value |
| --- | --- |
| This VM | ~15 GiB RAM, 4× Xeon cores, ~228 GiB free disk |
| FI data in workspace now | **Absent** — must fetch Geofabrik PBF (~0.7 GiB) + leaf MWMs / derive `.pix` |
| Helsinki-class \|U\| (SP-023 proxy) | ~6.5×10⁶–6.8×10⁶ |
| SP-023 Python PIP estimate | ~2.7 min @ ~25 µs/pt with STRtree |
| Assign column size (uint16, N≈6.5e6) | ~12.4 MiB; centres `PointD` ~100 MiB — fits RAM per leaf |
| FI leaves | 8 — process **sequentially** (one leaf U in memory) |

Honest assessment:

- **Helsinki leaf dense assign is feasible** on this class of machine if PIP is
  accelerated or runs for minutes–tens of minutes.
- **Naive C++ `BuildDenseAssignments` may be too slow** (no spatial index;
  rebuilds `RegionD` per hit-test). Budget acceleration or accept long wall
  time; do not drop dense assign.
- **All eight FI leaves** are feasible as a batch job (hours worst case), not as
  a CI unit test. CI stays synthetic / tiny universe.
- Highway proxy **U** must not be used for production blobs.

---

## Acceptance criteria

1. Offline production tool builds and writes **eight** FI leaf
   `{mwmLeafId}.spa` files under a local publish dir (not committed), leaf ids
   matching `countries.txt` / borders.
2. Each file is format_version **2** with `nside=1048576`, AscendingNest;
   `assign_count > 0` and `assign_count == |U|` for that leaf’s universe.
3. Rings are true closed OSM geometry filtered by FI `FilterExplorationCandidate`;
   no three-box / place-node invention; settlements present but never appear as
   assign targets (spot-check / unit test).
4. Helsinki known-id spot-check still passes; `VerifyDenseAssignments` holds for
   Helsinki (full or documented sample with fail-closed on mismatch).
5. Section sizes recorded (areas + assign) for at least Helsinki + one rural
   leaf; evidence note or validation log updated (no numeric floor — SPD-024).
6. `street_pixels_areas_tests` green with new dense-emit cases; existing suite
   not weakened.
7. Docs: SP-026 emission pointer; phase-04 R1 emit → Option B closed for FI
   packaging; Option A residual recorded; this file Status **In review** after
   impl (not Accepted by agent).
8. Maintainer decides acceptance.

---

## Required automated tests

- Dense emit→load round-trip (tiny synthetic U + rings); header v2 fields.
- Settlements in areas, never in assign column / `IsAssignable` path.
- AscendingNest slot order: scrambled input centres rejected or sorted before
  write (tool must emit ascending U order only).
- Geometry-only (`assign_count=0`) remains allowed for fixtures / SP-032 path
  but is **not** the production default.
- Existing `street_pixels_areas_tests` still pass.

## Required manual / offline validation

- Rebuild rings JSONL from FI PBF when absent (`street_pixels_spike`).
- Obtain leaf `.pix` (derive from downloaded leaf MWM) for each FI leaf.
- Run production emit for all eight leaves; record sizes + Helsinki verify.
- Do not commit binaries.

## Failure and rollback considerations

- Do not ship highway-proxy U as production assign.
- Do not weaken tests or skip dense assign to “pass” size.
- Do not hook PlaceProcessor three-box into exploration emit.
- If full mapgen wiring is demanded instead of B, split: keep this WI as B and
  open a follow-up for Option A — do not silently expand scope mid-flight.

---

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `1cbf20acf` `[map] Add spatial acceleration for dense .spa assignment`; `1c571cf6e` `[tools] Extend spa_emit_tool for dense production leaf emit`; `[docs] Record SP-044 production .spa emit evidence` (this commit) |
| Tool | `tools/spa_emit_tool` — default `--mode=production` (dense); `--mode=geometry_only` fixtures |
| Emit outputs | local `/tmp/sp044/` only (not committed) |
| Size table | Helsinki leaf (full rings + **tiny** \|U\|=4 smoke): file **456 516 B**, areas **456 386**, assign **8**, area_count **694**, assign_count **4**; known-id **11/11**. Full eight-leaf dense with real leaf `.pix` \|U\| — **residual** (no leaf `.pix` / offline derive in this env) |
| Test output | `street_pixels_areas_tests` **78/78** OK (includes 5 new `spa_dense_emit_tests` / sample-centres cases). `spa_emit_tool --help` smoke OK |
| Decision ids | SPD-020–025, SPD-032, SPD-034; SPD-033 track |
| Implemented by | Cloud agent (SP-044 Option B) |
| Accepted by | — |
| Accepted date | — |

### Offline FI batch notes (this run)

| Step | Result |
| --- | --- |
| Geofabrik FI PBF | Fetched to `/tmp/sp044/finland-latest.osm.pbf` (~704 MiB) |
| Rings JSONL | Extracted: 2751 kept (`finland_admin_place_rings.jsonl`) |
| Leaf `.pix` | **Absent** — on-device derive / leaf MWM→`.pix` not available here; highway proxy **not** used |
| Production tool smoke | Helsinki leaf with full rings + synthetic \|U\|=4: v2 header, dense verify, **11/11** spot-check |
| Eight-leaf full-\|U\| emit | **Residual** for human/offline once leaf `.pix` exist |

---

## Discovered follow-up / residuals

| Finding | Proposed disposition |
| --- | --- |
| Full OSM collectors + `maps_generator` StageMwm emit beside `.mwm` (Option A) | **Follow-up WI** after B proves FI CDN blobs; do not block SP-045/046 on A |
| `generator/` links `street_pixels_areas` with zero call sites | Remains until Option A; harmless |
| `BuildDenseAssignments` lacks spatial index | **Done in SP-044** — cached `RegionD` + boost STRtree bbox prefilter |
| Offline derive helper vs linking `libs/map` into emit tool | **Done** — light `sample_centres` + chealpix + thin `.pix` scan (no map link) |
| Leaf attribution: centroid vs full affiliation for cross-border rings | Centroid = SP-023/032; revisit if spot-checks show systematic mis-leaf |
| Dense assign size for all eight FI leaves (device budget) | Measure under full-\|U\| batch once leaf `.pix` available; no SPD-024 floor |
| Eight FI leaf production `.spa` with real leaf `.pix` \|U\| | **Residual** — human/offline batch; tool ready |
| Offline leaf MWM → `.pix` derive helper for packaging | **Residual** — preferred input remains client-matching `.pix` |
| `countries.txt` spa advertisement | **SP-045** |
| Client download / lifecycle / retry | **SP-046–048** |
| Worldwide countries beyond FI | Incremental config + same Option B job; not this WI |
