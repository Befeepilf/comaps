# SP-023 — Spike: admin polygon retention size and coverage

**Phase:** 4 — Administrative-area pipeline
**Status:** Evidence recorded (not accepted)
**Branch:** `cursor/sp-023-admin-polygon-spike-191e`
**Depends on:** Phase 3 accepted (map-data version + rematch substrate available for sampling)
**Unblocks:** SP-024

---

## Objective

Measure the cost of retaining true closed administrative and place-boundary
polygons for at least one full country, and measure settlement-subdivision
coverage. Produce numbers that unblock store-location, assignment-locus, and
assignment-persistence decisions (SP-024). No product behaviour change required
to ship.

## Motivation

Phase 4 entry criteria require a recorded area-pipeline investigation with
measurement. Audit Spike 6 is still undone. Admin levels 5, 6, and 8 are absent
from the classificator; runtime `CityBoundary` is a three-box approximation.
Without size and coverage numbers, in-MWM vs sidecar and on-device vs
precomputed assignment are guesses (SPD-006 forbids inventing grids to dodge
the gap).

## In-scope behavior

- Prototype retention of **true closed rings** (not three-box) for configurable
  `admin_level` values (at least 5–11 as available in OSM for the chosen
  country) plus polygonal `place=suburb|quarter|neighbourhood` **only where a
  closed way/relation exists** (spec §8.3 forbids synthesising polygons around
  `place=*` nodes).
- Measure: polygon count, vertex count, serialized size (raw and any existing
  coding), delta vs current country MWM / World `cities_boundaries`.
- Coverage: % of settlements with ≥1 subdivision; sample of street pixels in
  subdivision vs settlement-only vs no-area (reuse an existing `.pix` universe
  if available — Uusimaa-class ~6.5×10⁶ cells from Phase 3).
- Rough assignment cost estimate: point-in-polygon for N pixels × M candidates
  (or a representative sample), on-device class hardware if practical.
- Rough **assignment-table size** estimate if every valid street pixel stores an
  area id (and variants: sparse explored-only, rematerialize on demand).
- Written recommendation inputs for SP-024: store location; assignment locus;
  persistence strategy.

## Out-of-scope behavior

- Shipping polygons to end users.
- Classificator / mapcss production changes (may prototype offline).
- Pixel→area persistence or UI.
- City allowlists (SPD-004).
- Accepting architecture decisions (SP-024).

## Relevant product requirements

- §3.5, §8.3–§8.6, §8.8; SPD-004, SPD-006, SPD-007.
- Phase 4 entry criteria.

## Relevant source files or symbols

- `generator/collector_routing_city_boundaries.cpp`, `place_processor.cpp`,
  `cities_boundaries_builder.cpp`
- `libs/indexer/city_boundary.hpp`
- `data/mapcss-mapping.csv`, `data/classificator.txt`
- Existing `{countryId}.pix` for coverage sampling if present

## Implementation notes / constraints

- Prefer one dense-admin European country already used in Phase 3 (Finland /
  Uusimaa-class) or another full-country MWM with rich admin 8–10.
- Reuse `PlaceBoundariesHolder` / collector substrate where possible — it
  already keeps true rings before boxification.
- Do not silently shrink product scope if size is large; report and let SP-024
  decide (sidecar, coarser levels, settlement-only until affordable).
- Android V1 / iOS out of scope for shipping; spike tooling may be desktop-only.

## Acceptance criteria

1. Size and coverage table recorded in this work item’s evidence (or linked
   spike note under `docs/implementation/`).
2. Explicit recommendation **inputs** for store location, assignment locus, and
   assignment persistence (full-universe map vs sparse vs recompute).
3. Phase 4 open decisions for SP-024 are listed with preferred options grounded
   in the numbers.
4. No production behaviour change required.

## Required automated tests

- None mandatory (spike). Prefer a reproducible script or documented generator
  flags so the measurement can be re-run.

## Required manual validation

- Spot-check that retained polygons look like real admin/place boundaries for
  one known city in the chosen country (closed geometry only).

## Failure and rollback considerations

- If size is unacceptable for in-MWM, that is a successful spike outcome, not a
  failure — feed SP-024.
- Prototype code need not land on `street-pixels` if measurements are recorded;
  if it does land, keep it behind non-shipping paths.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-023-admin-polygon-spike-191e` |
| Commits | `b9d0bf4ae` [tools] spike scripts; `7a6cb7e56` [docs] evidence + spike note; `03f15d4fa` [docs] evidence SHAs; plus independent-review fixes on this branch |
| Country measured | Finland (Geofabrik `finland-latest` snapshot 2026-08-02 as `finland-260802.osm.pbf`, 737 359 571 bytes, sha256 `a446647ff15a2fc334cc83be283cc637fd66ff560b166d589525793e5ffc2724`); Uusimaa-class focus via `Finland_Southern Finland_Helsinki` border |
| Size / coverage table | Full tables in [spikes/SP-023-finland-admin-polygons.md](../spikes/SP-023-finland-admin-polygons.md). Summary: **2 751** closed rings, **609 188** vertices; country-concat zlib(coded) **~2.06 MiB**; Helsinki-attributed zlib_coded **~0.52 MiB** (**~0.42 %** of Helsinki MWM 125 MiB). World `cities_boundaries` **1 079 477** B; `packed_polygons.bin` **3 676 511** B. Settlement subdivision coverage **37.3 %** national / **49.1 %** Helsinki-MWM (settlement = closed place city/town/village/municipality **or** admin_8). Highway HEALPix sample: **75.4 %** subdivision / **24.6 %** settlement fallback / **0 %** no-area inside border. admin_5/6 absent; place closed rings rare (45). |
| Assignment cost / table-size estimate | Universe = highway→HEALPix nside=1048576 @15 m proxy (**6 844 831** cells, ~1.05× Phase 3 N≈6.5e6). STRtree PIP ~**24.8 µs**/pt desktop → ~**2.7 min** for 6.5e6 (optimistic vs phone; **coverage proxy** — smallest-area among subdivisions, not full §8.8 level priority). Table @ N=6.5e6: uint16 full **~13 MiB**, uint32 **~26 MiB**, uint64 OSM id **~52 MiB**; sparse 1 % explored **~0.78 MiB**; rematerialize **0**. |
| Recommendation inputs | **Store:** prefer per-country sidecar (Finland zlib ~2.1 MiB; Helsinki ~0.5 MiB); in-MWM cheap for FI but optional/lazy preferred worldwide. **Assignment locus:** prefer generator-precomputed (or once-per-derive blob); on-device PIP is minutes on desktop — measure phone before primary. **Persistence:** sparse explored + rematerialize *or* full uint16/uint32 sidecar map; avoid full uint64 OSM ids. Finland config grain: admin_10 (+9/11) subdivisions, admin_8 settlement (SPD-007 load-bearing). Phase-04 open decisions 1–3 + 6 preferred inputs grounded; 4 (config format) and 5 (suitability/privacy thresholds) deferred. Not Accepted SPDs — for SP-024. |
| Test output | Scripts exit 0: `extract_admin_place_polygons.py`, `measure_sizes.py`, `coverage_and_assign.py` (logs under `/tmp/sp023/`). Tooling: `tools/python/street_pixels_spike/`. |
| Manual validation | Exported Helsinki metro GeoJSON; 500/500 closed rings. Spot-checked real districts by OSM id (11/11 known relations resolved): Kamppi r/184714, Kallio r/184765, Punavuori r/184703, Ullanlinna r/184702, Etu-Töölö r/184727, Helsinki admin_8 r/34914 (admin_10 kaupunginosat). |
| Implemented by | Cursor Agent (cloud) |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| No admin_5/6 in Finland; admin_10 dominates subdivisions; closed place=* rare | Feed SP-025 Finland country-config defaults — do not assume place polygons |
| Phone-class PIP / rematch not measured | Optional perf follow-up before locking on-device locus in SP-024 |
| Only one country measured | SP-024 may want a second dense-admin country before worldwide store policy |
| Spike `coded_delta` ≠ shipping geometry codec | Re-measure with production encoder in SP-026 |
| Per-area street-pixel counts (privacy / suitability floors) not computed | SP-024/025 suitability + §23.4 anonymity work |
| Spike PIP skips country-config level priority | SP-028 must implement full §8.8; do not copy spike assigner |
| Settlement % mixes place + admin_8 OSM objects | Quote carefully; optional municipality-only cut for SP-025 |
