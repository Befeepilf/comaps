# Phase 4 — Administrative-area pipeline

**Status:** In progress (phase-entry investigation complete 2026-08-03; work items Planned)
**Depends on:** Phase 3
**Blocks:** Phases 5, 7, 8

---

## Objective

Give the product real geographic units. Produce, from OSM data, the closed
administrative and place polygons that define exploration areas, select them
using a versioned country-specific configuration, and assign every valid street
pixel to at most one area, deterministically.

This is the largest and least certain phase in the plan. It requires generator
and map-data work, not only client work.

## Product-spec references

- §3.5 Meaningful geographical units; no invented grid areas; no polygons
  around point-only place tags.
- §7 Focused area.
- §8.2 Settlement detection.
- §8.3 Exploration-area selection policy and the country-configuration
  principle.
- §8.4 Candidate-area suitability.
- §8.5 Settlement fallback.
- §8.6 Behaviour outside recognised settlements.
- §8.8 Deterministic area assignment, including the smallest-polygon and
  stable-identifier tie-breaks.
- §34 "Core map and exploration": versioned country-specific configuration
  applied deterministically.

## Technical-audit references

- §10 Administrative-area feasibility, including the recommended deterministic
  algorithm and the geographic edge-case list.
- §22 Risk register: "Admin boundary inconsistency", rated Critical.
- Spike 6, complexity XL.
- §24 "Replace", which proposed narrowing to curated pilot cities. That
  proposal is **overridden** by SPD-004 and SPD-006.

## Current code locations

Re-verified 2026-08-03 against the working tree (supersedes 2026-07-25 notes
where they differ — none material).

| Concern | Location | Observed state |
| --- | --- | --- |
| Admin level retention | `data/mapcss-mapping.csv`, `data/classificator.txt` | Only `boundary\|administrative\|2`, `\|3`, `\|4` plus generic `boundary\|administrative` are active. `\|7`, `\|9`, `\|10`, `\|11` deprecated. Levels **5, 6, and 8 have no typed entry**. Classificator `administrative` children = 2,3,4 only. |
| Place types | `data/mapcss-mapping.csv` | `place\|suburb`, `place\|quarter`, `place\|neighbourhood` active for search/labels (`IsSuburbChecker`); **not** exploration polygons |
| City boundary storage | `libs/indexer/city_boundary.hpp` | Still bbox ∩ calipers ∩ diamond; point inside only if inside all three |
| Boundary build | `generator/collector_routing_city_boundaries.cpp`, `place_processor.cpp`, `cities_boundaries_builder.cpp` | Collector keeps **true rings** in `PlaceBoundariesHolder`; `PlaceProcessor` boxifies via `CityBoundary(poly)`, may drop oversized non-honest-city boundaries (`exactArea > 20×` population circle); serialises World MWM `CITIES_BOUNDARIES_FILE_TAG` |
| Runtime lookup | `libs/search/cities_boundaries_table.hpp` | World-MWM only; `Has`/`Get`/`HasPoint` — no admin_level, stable OSM area id, or assignment API |
| Area identifier | — | **Not found** |
| Pixel-to-area assignment | — | **Not found** |
| ExploreStats “region” | `StreetPixelsManager` → `ExploreStats` | Still MWM `countryId`, not neighbourhood |

**Difference from the technical audit:** audit’s 2–4 kept / 7,9–11 deprecated
claim remains correct but incomplete — 5, 6, 8 were never mapped (phase doc
already noted). Spike 6 size measurement is **still undone**.

## Intended outcome

- A versioned, reviewable country configuration describing which
  `admin_level` values are valid for each country and in what priority order,
  with place-boundary fallbacks.
- Generator retention of the polygons that configuration requires, as real
  closed polygons rather than three-box approximations.
- A deterministic pixel-to-area assignment producing at most one area per
  pixel, with the smallest-polygon rule and a stable-identifier tie-break.
- Settlement fallback where no subdivision exists.
- A defined and tested "no area here" state.

## Dependencies

- Phase 3 complete (map-data version stamp; rematch hooks for reassignment).
- SP-023 size/coverage spike recorded before SP-024 architecture decisions.
- SP-024 decisions before generator/client implementation (SP-025+).

## Phase-entry investigation (2026-08-03)

### Confirmed gaps

- No exploration-area id or pixel→area code in tree.
- True rings exist only as a generator intermediate before three-box approx.
- Sidecar patterns exist (`packed_polygons.bin`, per-country `.pix`/`.pixr`)
  and in-MWM optional sections exist (`cities_boundaries`, `isolines`, …) —
  neither hosts exploration areas yet.
- Neighborhood polygon **size is still unknown** — blocks store/assignment
  product decisions.

### Work-item breakdown

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-023](../work-items/SP-023-admin-polygon-size-spike.md) | Spike: admin polygon retention size and coverage |
| 2 | [SP-024](../work-items/SP-024-area-pipeline-architecture-decisions.md) | Area-pipeline architecture decisions (store, assignment locus, config) |
| 3 | [SP-025](../work-items/SP-025-country-config-schema.md) | Versioned country-config schema |
| 4 | [SP-026](../work-items/SP-026-generator-true-polygons.md) | Generator: emit true closed exploration polygons |
| 5 | [SP-027](../work-items/SP-027-client-polygon-runtime-api.md) | Client runtime polygon API |
| 6 | [SP-028](../work-items/SP-028-pixel-to-area-assignment.md) | Deterministic pixel-to-area assignment |
| 7 | [SP-029](../work-items/SP-029-settlement-fallback-and-no-area.md) | Settlement fallback and no-area state |
| 8 | [SP-030](../work-items/SP-030-assignment-persistence-and-rematch.md) | Persist assignments and rematch hooks |
| 9 | [SP-031](../work-items/SP-031-area-pipeline-end-to-end-validation.md) | Area-pipeline end-to-end validation |

**Do not start SP-025+ coding until SP-023 numbers and SP-024 decisions exist.**

### Open decisions (for SP-024 after SP-023)

1. Polygon store: in-MWM vs sidecar vs hybrid.
2. Assignment locus: on-device vs generator-precomputed.
3. Country-config format / versioning / review process.
4. Concrete suitability + privacy size thresholds.
5. Settlement geometry: three-box vs true municipal polygons.

## Data and migration concerns

- Retaining full polygons increases MWM size for every user, including those
  who never enable competition. The size impact is a product-visible cost and
  needs a measured number (SP-023).
- The country configuration is versioned independently of map data. Assignment
  determinism is therefore keyed on the pair (map-data version, policy
  version). Both must be stored with assignments.
- A configuration change reassigns pixels without any map change. Percentages
  can move without the user updating anything. That needs the same honest
  communication as a map update.
- Boundaries change in OSM. An area can be split, merged, renamed, or removed
  between map versions. Decide what happens to a previously completed area
  whose polygon no longer exists; spec §27.4 allows keeping the original
  completion date locally.
- Assignment state is new persisted data, keyed by HEALPix identifier.

## Privacy and security implications

- Area identifiers are the only geographic data competition ever uploads. Their
  granularity is therefore the privacy floor of the entire competition feature.
  A very small area effectively identifies a user's neighbourhood.
- Spec §8.4 requires areas to be "meaningfully smaller than the containing
  settlement", which pushes toward granularity. Spec §23.4 requires anonymity
  where fewer than three participants exist. Consider whether a minimum area
  size or minimum pixel count is needed so that an area identifier is not
  effectively an address. Record any such rule as a decision (SP-024).
- Assignment happens entirely on device. No boundary lookup may become a
  network call.

## Automated testing strategy

- Determinism: assigning the same fixture twice under the same map-data and
  policy versions produces identical results.
- Smallest-polygon rule: a pixel inside nested valid polygons is assigned to the
  smallest.
- Tie-break: equal-area polygons resolve by stable identifier, not by iteration
  order.
- Exactly-one-area: no fixture pixel is assigned twice.
- Settlement fallback: a settlement fixture with no subdivisions yields one area
  covering the settlement.
- Outside settlements: pixels are collectable and have no area.
- Country configuration: a fixture configuration selects the expected level in
  a country where the default priority would choose differently.
- Generator tests for polygon retention in `generator/generator_tests/`.

## Manual validation strategy

- Inspect assignment against a real city with dense subdivisions and confirm
  area names and boundaries look right to someone who knows the city.
- Inspect a real city with no subdivisions and confirm settlement fallback
  produces one sensible area.
- Inspect a rural area and confirm exploration works with no area shown.
- Inspect a coastal or island municipality with fragmented polygons.
- Inspect a city that straddles an administrative boundary.
- Confirm MWM size change against the measured budget.

## Entry criteria

- Phase 3 exit criteria met. **Met 2026-08-03** (device-walk residual → Phase 10).
- The area-pipeline investigation is complete with recorded measurements for at
  least one full country. **Pending SP-023.**
- A decision exists on where polygons live and whether assignment is on-device
  or precomputed. **Pending SP-024.**

## Exit criteria

1. Polygons required by the country configuration are available to the client
   for at least one full country, as real closed polygons.
2. A versioned country configuration exists, is reviewable as data, and is
   applied by priority order rather than by a single global rule.
3. Every valid street pixel is assigned to at most one area, deterministically
   for a fixed map-data and policy version pair.
4. The smallest-polygon rule and stable-identifier tie-break are implemented and
   tested.
5. Settlement fallback works where no suitable subdivision exists.
6. Outside recognised settlements, exploration and routing work and no area is
   claimed.
7. MWM or sidecar size impact is measured and accepted.
8. No MWM country identifier is presented anywhere as a neighbourhood.

## Explicit non-goals

- Any city allowlist, pilot-only behaviour, or city-specific restriction. See
  SPD-004.
- Grid or generated fallback areas. Spec §8.6 forbids them in V1.
- Polygons synthesised around `place=*` nodes. Spec §8.3 item 5 forbids it.
- Area progress UI. Phase 5.
- Competition scoring. Phase 8.
- Complete worldwide country configuration coverage. Configuration expands
  incrementally; settlement fallback and the no-area state cover the rest.
- Country and world aggregate percentages. Post-V1.

## Known uncertainties

This phase has the highest uncertainty in the plan.

- Whether full polygons can be retained at acceptable size. **Owned by SP-023.**
- How many countries need bespoke configuration before coverage feels adequate,
  and who maintains that configuration. **Owned by SP-024/025.**
- Whether admin levels 5, 6, and 8 can be added to the classificator without
  disrupting upstream CoMaps behaviour that depends on the current mapping.
- Whether the three-box `CityBoundary` approximation can be reused for
  settlement detection while true polygons are used for subdivisions, or
  whether settlements also need true polygons. **Owned by SP-024.**
- How to keep divergence from upstream CoMaps manageable, given that the
  generator and classificator are shared with upstream.
- Whether "meaningfully smaller than the containing settlement" and
  "containing a meaningful street-pixel set" can be expressed as concrete
  thresholds, and what those thresholds are. **Owned by SP-024.**
- Whether area assignment on device is fast enough for a large country, or
  whether precomputation is required. **Owned by SP-023/024.**
