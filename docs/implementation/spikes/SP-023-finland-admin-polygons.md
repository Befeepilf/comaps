# SP-023 spike note — Finland admin / place polygon retention

**Work item:** [SP-023](../work-items/SP-023-admin-polygon-size-spike.md)  
**Date:** 2026-08-03  
**Branch:** `cursor/sp-023-admin-polygon-spike-191e`  
**Scope:** Desktop-only measurement. No production behaviour change.  
**Source PBF:** Geofabrik `finland-latest.osm.pbf` snapshot used as
`finland-260802.osm.pbf` (737 359 571 bytes;
sha256 `a446647ff15a2fc334cc83be283cc637fd66ff560b166d589525793e5ffc2724`)  
**Universe:** highway→HEALPix `nside=1048576` proxy at 15 m (no `.pix` in workspace)

Scripts: `tools/python/street_pixels_spike/`. Raw outputs under `/tmp/sp023/` (not committed).

---

## 1. What was retained

True closed rings only (osmium Area assembly). Never invents polygons around
`place=*` nodes.

| Class | Count | Vertices | Notes |
| --- | ---: | ---: | --- |
| admin_5 | 0 | 0 | Absent in Finland OSM extract |
| admin_6 | 0 | 0 | Absent in Finland OSM extract |
| admin_7 | 79 | 112 326 | Present (coarse) |
| admin_8 | 337 | 223 063 | Municipalities (`kunta`) — settlement geometry |
| admin_9 | 270 | 79 736 | Subdivision |
| admin_10 | 1 803 | 172 891 | Dominant subdivision grain (Helsinki kaupunginosat) |
| admin_11 | 217 | 18 194 | Finer parts |
| place_suburb | 21 | 1 435 | Closed polygonal only |
| place_quarter | 4 | 285 | Closed polygonal only |
| place_neighbourhood | 20 | 1 258 | Closed polygonal only |
| **Total** | **2 751** | **609 188** | |

Place polygons are rare versus admin_10. Exploration-area candidates for Finland
are effectively **admin_9–11** (subdivisions) with **admin_8** settlement fallback
(SPD-007), not place=* rings.

---

## 2. Serialized size (D1)

Encoding definitions (spike approximations, not shipping codecs):

- **raw_pod** — float64 lon/lat per vertex + ring headers  
- **coded_delta** — 1e7-scaled int, delta + zigzag varint  
- **zlib_*** — zlib level 9 on that encoding (per-polygon sum, or country concat)

### By class

| Class | raw_pod | coded_delta | zlib_raw | zlib_coded |
| --- | ---: | ---: | ---: | ---: |
| admin_7 | 1 797 904 | 520 074 | 1 386 161 | 495 878 |
| admin_8 | 3 572 028 | 1 029 611 | 2 767 662 | 986 632 |
| admin_9 | 1 278 036 | 370 116 | 999 297 | 360 832 |
| admin_10 | 2 780 772 | 787 526 | 2 229 826 | 796 442 |
| admin_11 | 292 840 | 80 433 | 234 625 | 82 157 |
| place_* (all) | 48 016 | 12 867 | 38 445 | 13 322 |
| **All retained** | **9 769 596** | **2 800 627** | **7 656 016** | **2 735 263** |
| Subdivisions only (9–11 + place) | 4 399 664 | 1 250 942 | 3 502 193 | 1 252 753 |
| Exploration (8 + subs) | 7 971 692 | 2 280 553 | 6 269 855 | 2 239 385 |

Country-concat zlib(coded_delta) over all rings: **2 159 831 bytes (~2.06 MiB)**.

### By Finland MWM border (centroid attribution)

| MWM border | Count | coded_delta | zlib_coded |
| --- | ---: | ---: | ---: |
| Finland_Southern Finland_Helsinki | 700 | 554 475 | 541 281 |
| Finland_Western Finland_Tampere | 695 | 485 651 | 482 171 |
| Finland_Southern Finland_West | 538 | 458 669 | 451 011 |
| Finland_Northern Finland | 251 | 444 943 | 428 559 |
| Finland_Western Finland_Jyvaskyla | 207 | 223 856 | 219 731 |
| Finland_Southern Finland_Lappeenranta | 183 | 180 564 | 177 692 |
| Finland_Eastern Finland_North | 98 | 176 142 | 171 340 |
| Finland_Eastern Finland_South | 78 | 275 759 | 262 899 |
| (unattributed) | 1 | 568 | 579 |

Helsinki MWM slice ≈ **0.53 MiB** coded / **0.52 MiB** zlib_coded — **~0.42 %** of the
Helsinki MWM file (131 251 927 bytes).

---

## 3. Package baselines and deltas (D2)

| Artifact | Bytes |
| --- | ---: |
| World.mwm | 53 412 092 |
| World `cities_boundaries` section | **1 079 477** (~1.03 MiB) |
| Finland_Southern Finland_Helsinki.mwm | 131 251 927 (~125.2 MiB) |
| `data/packed_polygons.bin` | 3 676 511 (~3.51 MiB) |
| Finland all rings, country-concat zlib(coded) | 2 159 831 (~2.06 MiB) |
| Helsinki-attributed zlib_coded | 541 281 (~0.52 MiB) |

Ratios (Finland country-concat zlib coded):

- vs World `cities_boundaries`: **×2.00**
- vs `packed_polygons.bin`: **×0.59**
- Helsinki coded_delta vs Helsinki MWM: **+0.42 %** if inlined

**Interpretation:** True-ring retention for a dense-admin country is small versus
a regional MWM and in the same ballpark as existing sidecars / World city boxes.
Large absolute size is not the blocker for Finland; store *location* and
assignment *persistence* still matter for worldwide scaling and rematch cost.

---

## 4. Coverage (D3)

Settlements = closed `place=city|town|village|municipality` **or** `admin_level=8`
(distinct OSM objects; a nested `place=town` and its municipality `admin_8` can
both count — breadth for SPD-007, not a municipality census).  
Subdivision = admin_9–11 + closed place suburb/quarter/neighbourhood.  
A settlement “has subdivision” if a subdivision polygon overlaps it with
intersection area ≳ 1000 m² (rough filter).

| Scope | Settlements | With ≥1 subdivision | % |
| --- | ---: | ---: | ---: |
| National Finland | 402 | 150 | **37.3 %** |
| Helsinki MWM border | 53 | 26 | **49.1 %** |

SPD-007 settlement fallback is therefore required for a large fraction of
municipalities even in Uusimaa-class data.

### Highway pixel buckets (Helsinki MWM border, 50 000 unique HEALPix sample)

| Bucket | Count | % |
| --- | ---: | ---: |
| In subdivision (smallest-area PIP) | 37 705 | **75.4 %** |
| Settlement fallback only | 12 295 | **24.6 %** |
| No area | 0 | **0.0 %** |

No-area is ~0 inside this border because Finnish land is tiled by admin_8
municipalities; rural *competition* absence is about missing *subdivision*, not
missing settlement. Outside any settlement (ocean / abroad) remains no-area per
SPD-007 — this sample was highway points inside the MWM land border.

---

## 5. Universe proxy (D3/D4)

No `{countryId}.pix` in the workspace. Proxy:

1. Extract highways in Helsinki MWM `.poly` bbox (857 851 ways).  
2. Densify centerlines at **15 m**.  
3. Keep points inside the border polygon.  
4. Map to HEALPix **nest**, **nside = 1 048 576** (SPD-017).  
5. Unique cell count = proxy universe.

| Metric | Value |
| --- | ---: |
| Densified points inside border | 7 988 180 |
| Unique HEALPix cells | **6 844 831** |
| Phase 3 Uusimaa-class reference N | 6 500 000 |
| Proxy / reference | **1.053** |

Proxy is within ~5 % of the Phase 3 figure — adequate for assignment-table and
PIP extrapolations.

---

## 6. Assignment cost (D4)

STRtree query + `covers` + smallest `area_m2` then ascending `osm_id`.  
**Coverage/cost proxy only:** candidates are all subdivision classes together
(admin_9–11 + closed place); this does **not** apply country-config level
priority before smallest-area (full §8.8 stack is SP-028). Fallback: settlements
intersecting the Helsinki border.

| Metric | Value |
| --- | --- |
| Sample N | 50 000 unique proxy pixels |
| Sample wall time | 1.24 s |
| Per-point | **~24.8 µs** (desktop x86_64 VM) |
| Est. full proxy (6.84e6) | ~170 s (~2.8 min) |
| Est. N = 6.5e6 | ~161 s (~2.7 min) |

**Caveat:** Desktop timing is an optimistic lower bound for on-device rematch.
Phone-class hardware and cold storage I/O are not measured. Still: full-universe
PIP at rematch is **minutes**, not hours, for Uusimaa-class on a fast desktop —
precompute remains attractive if rematch must be interactive on device.

---

## 7. Assignment-table size estimates (D5) for N ≈ 6.5×10⁶

M ≈ 2 335 subdivision candidates (index fits in uint16).

| Strategy | Approx. bytes | Notes |
| --- | ---: | --- |
| Full universe uint16 area index | **13 000 000** (~12.4 MiB) | Needs stable dense id space |
| Full universe uint32 area index | **26 000 000** (~24.8 MiB) | Safer id width |
| Full universe uint64 OSM id | **52 000 000** (~49.6 MiB) | No indirection table |
| Sparse 1 % explored (u64 healpix + u32) | **780 000** | Explored-only |
| Sparse 10 % explored | **7 800 000** | |
| Rematerialize on demand | **0** | Pay PIP / download at derive |

Compare: Helsinki MWM ~125 MiB; full uint32 map ~25 MiB is material but smaller
than the map; sparse explored-only is far smaller for typical users.

---

## 8. Manual validation (Helsinki)

Exported `/tmp/sp023/helsinki_subdivisions.geojson` (500 features in metro bbox).

Programmatic checks:

- **Closed rings:** 500/500 GeoJSON features; 745/745 subdivisions intersecting
  the Helsinki MWM border had closed outers.
- **Names:** 744/745 named.
- **Known OSM relation IDs** (spot-check dict in `coverage_and_assign.py` —
  not an allowlist; all 11 resolved in the retained rings):

| Name | OSM | Class | Vertices |
| --- | --- | --- | ---: |
| Kamppi | relation/184714 | admin_10 | 247 |
| Kallio | relation/184765 | admin_10 | 187 |
| Punavuori | relation/184703 | admin_10 | 247 |
| Ullanlinna | relation/184702 | admin_10 | 191 |
| Katajanokka | relation/184711 | admin_10 | 13 |
| Kruununhaka | relation/184712 | admin_10 | 124 |
| Etu-Töölö | relation/184727 | admin_10 | 145 |
| Taka-Töölö | relation/184728 | admin_10 | 95 |
| Lauttasaari | relation/184655 | admin_10 | 31 |
| Eira | relation/184668 | admin_10 | 82 |
| Helsinki (municipality) | relation/34914 | admin_8 | 1 255 |

Nesting: Helsinki admin_8 present; closed place polygons inside the border are
few (7), so place-fallback is secondary to admin_10 for Finland.

---

## 9. Recommendation **inputs** for SP-024 (not Accepted SPDs)

These are measurement-grounded **inputs**. SP-024 decides; do not treat as
accepted architecture.

Map to Phase 4 open decisions (phase-04 §“Open decisions”):

| # | Open decision | Preferred input (grounded) |
| --- | --- | --- |
| 1 | Polygon store | Per-country sidecar (see below) |
| 2 | Assignment locus | Generator-precomputed / once-per-derive (see below) |
| 3 | Assignment persistence | Sparse explored + rematerialize *or* full uint16/uint32 sidecar |
| 4 | Country-config format / versioning | Not sized here — schema still SP-025; Finland grain inputs below |
| 5 | Suitability + privacy thresholds | Not computed (pixel counts per area) — follow-up |
| 6 | Settlement geometry three-box vs true | True admin_8 rings affordable for FI (~1.0 MiB coded national) |

### Store location

| Option | Grounding |
| --- | --- |
| **Preferred input: per-country downloadable sidecar** (pattern like `.pix` / `packed_polygons`) | Finland all-rings zlib ~2.1 MiB; Helsinki slice ~0.5 MiB. Keeps competition-optional data out of every MWM download. Sidecar size ≪ regional MWM. |
| In-MWM optional section | Helsinki +0.4 % is cheap *for Finland*, but worldwide admin-dense countries and users who never enable areas would still pay if the section is not optional/lazy. |
| Hybrid (settlement boxes in World, true rings in country sidecar) | World `cities_boundaries` already ~1.0 MiB three-box; true municipal rings for settlement fallback could stay country-local. |

### Assignment locus

| Option | Grounding |
| --- | --- |
| **Preferred input: generator-precomputed** for the valid-universe map (or at derive time once per map version) | Desktop PIP ~2.7 min for 6.5e6; on-device rematch after every map/policy change would be painful if similar or slower. Offline invariant holds if the blob ships with the map/sidecar. |
| On-device at derive/rematch | Feasible as fallback (minutes on desktop-class); needs phone measurement before choosing as primary. |

### Assignment persistence

| Option | Grounding |
| --- | --- |
| **Preferred input: sparse explored + rematerialize for unexplored** *or* full uint16/uint32 map in sidecar | Full uint32 ~26 MiB/Uusimaa is real device cost; sparse explored-only matches SPD-016 archive thinking; rematerialize avoids store but needs fast assignment (precomputed blob beats live PIP). |
| Full-universe uint64 OSM ids | ~52 MiB — likely too heavy vs index + small id table. |

### Other SP-024 topics (inputs only)

- **Country config:** Finland should list admin_10 (and optionally 9/11) as
  subdivision priority; admin_8 as settlement; admin_5/6 unused; place=* closed
  as sparse supplement — not primary.
- **Settlement geometry:** True admin_8 rings (~1.0 MiB coded national) are
  affordable vs three-box World section if settlement containment must be exact;
  three-box may remain for search-only.
- **Suitability / privacy:** admin_10 Helsinki districts are the natural grain;
  measure pixel counts per area in a follow-up before locking anonymity floors
  (§23.4).
- **Coverage gap:** ~37 % national / ~49 % Helsinki-MWM settlements lack
  subdivisions → SPD-007 fallback is load-bearing, not edge-case.

---

## 10. Repro commands

```bash
python3 -m venv /tmp/sp023/venv
/tmp/sp023/venv/bin/pip install osmium shapely pyproj healpy
# fetch PBF + MWMs as in tools/python/street_pixels_spike/README.md
PY=/tmp/sp023/venv/bin/python
ROOT=tools/python/street_pixels_spike
OUT=/tmp/sp023
$PY $ROOT/extract_admin_place_polygons.py --pbf $OUT/finland-latest.osm.pbf \
  --out-jsonl $OUT/finland_admin_place_rings.jsonl \
  --out-geojson-helsinki $OUT/helsinki_subdivisions.geojson
$PY $ROOT/measure_sizes.py --rings $OUT/finland_admin_place_rings.jsonl \
  --borders-dir data/borders --world-mwm $OUT/World.mwm \
  --helsinki-mwm $OUT/Finland_Southern_Finland_Helsinki.mwm \
  --packed-polygons data/packed_polygons.bin --out $OUT/size_report.json
$PY $ROOT/coverage_and_assign.py --rings $OUT/finland_admin_place_rings.jsonl \
  --pbf $OUT/finland-latest.osm.pbf \
  --helsinki-poly "data/borders/Finland_Southern Finland_Helsinki.poly" \
  --out $OUT/coverage_report.json --universe-mode highway_healpix_proxy
```

Exit status of all three scripts in this run: **0**.

---

## 11. Follow-ups (not started)

| Finding | Proposed disposition |
| --- | --- |
| No admin_5/6 in Finland; grain is 8 + 10 | Feed SP-025 Finland country-config defaults |
| Place closed polygons ≪ admin_10 | Do not rely on place=* for Finland coverage |
| Phone-class PIP not measured | Optional SP-023 follow-up or SP-028 perf gate |
| Second dense-admin country not measured | SP-024 may want one more country before locking worldwide store policy |
| Pixel-count per area (privacy floor) | Measure in SP-024/025 suitability work |
| Exact production geometry codec vs spike coded_delta | SP-026 should re-measure with shipping encoder |
| PIP sample skips country-config level priority | SP-028 must implement full §8.8 stack; do not copy spike assigner |
| Settlement denominator mixes place + admin_8 objects | Document when quoting %; optional municipality-only cut for SP-025 |
