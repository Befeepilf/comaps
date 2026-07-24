# Phase 4 — Administrative-area pipeline

**Status:** Not started
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

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Admin level retention | `data/mapcss-mapping.csv` | Only `boundary\|administrative\|2`, `\|3`, `\|4` plus a generic `boundary\|administrative` are active. `\|7`, `\|9`, `\|10`, `\|11` are deprecated. Levels **5, 6, and 8 have no `boundary\|administrative` entry at all.** |
| Place types | `data/mapcss-mapping.csv` | `place\|suburb`, `place\|quarter`, `place\|neighbourhood` are active types, used for search and labels |
| City boundary storage | `libs/indexer/city_boundary.hpp` | A city is approximated by the intersection of a bounding box, a calipers box, and a diamond box. A point is inside only if inside all three. Not a true polygon. |
| Boundary build | `generator/cities_boundaries_builder.cpp`, `generator/place_processor.cpp` | Converts OSM administrative relation polygons into `indexer::CityBoundary`; filters oversized boundaries by area heuristic; serialises into the MWM `CITIES_BOUNDARIES_FILE_TAG` |
| Runtime lookup | `libs/search/cities_boundaries_table.hpp` | Search-oriented table |
| Area identifier | — | Not found. Runtime "regions" in `ExploreStatsService` are MWM country identifiers. |
| Pixel-to-area assignment | — | Not found |

**Difference from the technical audit:** the audit states that admin levels
2–4 are kept and 7, 9, 10, 11 are deprecated. That is correct but incomplete —
levels 5, 6, and 8 were never mapped at all, so the gap is wider than the audit
implies. Level 8 is the municipality level in many countries, which matters for
settlement detection as well as for subdivisions.

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

- Phase 3, for the map-data version stamp that area assignment must be keyed
  against.
- A recorded outcome from the area-pipeline investigation, including measured
  MWM size impact for at least one full country.

## Proposed work-item breakdown

Not yet decomposed. **Marked for phase-specific Plan Mode investigation before
any work item is written.** The investigation must answer, with measurement
rather than reasoning:

1. Which polygons must be retained to satisfy spec §8.3 for a representative
   set of countries, and what that costs in MWM size.
2. Whether polygons should live inside the MWM or in a sidecar package that can
   version independently of map data.
3. Whether assignment runs on device at derivation time or is precomputed by
   the generator.
4. How the country configuration is expressed, versioned, and reviewed.
5. What coverage looks like in practice across countries with sparse
   administrative data, and how often settlement fallback is the answer.

Only after those answers exist can this phase be split into reviewable work
items. Writing tickets now would encode guesses.

## Data and migration concerns

- Retaining full polygons increases MWM size for every user, including those
  who never enable competition. The size impact is a product-visible cost and
  needs a measured number.
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
  effectively an address. Record any such rule as a decision.
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

- Phase 3 exit criteria met.
- The area-pipeline investigation is complete with recorded measurements for at
  least one full country.
- A decision exists on where polygons live and whether assignment is on-device
  or precomputed.

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

- Whether full polygons can be retained at acceptable size. Unknown until
  measured.
- How many countries need bespoke configuration before coverage feels adequate,
  and who maintains that configuration.
- Whether admin levels 5, 6, and 8 can be added to the classificator without
  disrupting upstream CoMaps behaviour that depends on the current mapping.
- Whether the three-box `CityBoundary` approximation can be reused for
  settlement detection while true polygons are used for subdivisions, or
  whether settlements also need true polygons.
- How to keep divergence from upstream CoMaps manageable, given that the
  generator and classificator are shared with upstream.
- Whether "meaningfully smaller than the containing settlement" and
  "containing a meaningful street-pixel set" can be expressed as concrete
  thresholds, and what those thresholds are.
- Whether area assignment on device is fast enough for a large country, or
  whether precomputation is required.
