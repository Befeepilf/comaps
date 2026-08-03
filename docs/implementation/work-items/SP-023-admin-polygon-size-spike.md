# SP-023 — Spike: admin polygon retention size and coverage

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Measure the cost of retaining true closed administrative and place-boundary
polygons for at least one full country, and measure settlement-subdivision
coverage. Produce numbers that unblock store-location and assignment-locus
decisions (SP-024). No product behaviour change required to ship.

## Motivation

Phase 4 entry criteria require a recorded area-pipeline investigation with
measurement. Audit Spike 6 is still undone. Admin levels 5, 6, and 8 are absent
from the classificator; runtime `CityBoundary` is a three-box approximation.
Without size and coverage numbers, in-MWM vs sidecar and on-device vs
precomputed assignment are guesses (SPD-006 forbids inventing grids to dodge
the gap).

## In-scope behavior

- Prototype retention of **true rings** (not three-box) for configurable
  `admin_level` values (at least 5–11 as available in OSM for the chosen
  country) plus polygonal `place=suburb|quarter|neighbourhood` where present.
- Measure: polygon count, vertex count, serialized size (raw and any existing
  coding), delta vs current country MWM / World `cities_boundaries`.
- Coverage: % of settlements with ≥1 subdivision; sample of street pixels in
  subdivision vs settlement-only vs no-area (reuse an existing `.pix` universe
  if available).
- Rough assignment cost estimate: point-in-polygon for N pixels × M candidates
  (or a representative sample).
- Written recommendation: in-MWM vs sidecar (or hybrid); whether on-device
  assignment looks plausible for Uusimaa-class pixel counts (~6.5×10⁶).

## Out-of-scope behavior

- Shipping polygons to end users.
- Classificator / mapcss production changes (may prototype offline).
- Pixel→area persistence or UI.
- City allowlists (SPD-004).

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

## Acceptance criteria

1. Size and coverage table recorded in this work item’s evidence (or linked
   spike note under `docs/implementation/`).
2. Explicit recommendation for store location and assignment locus.
3. Phase 4 open decisions for SP-024 are listed with preferred options.
4. No production behaviour change required.

## Required automated tests

- None mandatory (spike). Prefer a reproducible script or documented generator
  flags so the measurement can be re-run.

## Required manual validation

- Spot-check that retained polygons look like real admin/place boundaries for
  one known city in the chosen country.

## Failure and rollback considerations

- If size is unacceptable for in-MWM, that is a successful spike outcome, not a
  failure — feed SP-024.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Country measured | |
| Size / coverage table | |
| Recommendation | |
| Test output | |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
