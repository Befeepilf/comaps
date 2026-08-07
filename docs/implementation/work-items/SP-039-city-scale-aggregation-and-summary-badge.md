# SP-039 — City-scale aggregation and summary badge

**Phase:** 5 — Area progress and map interaction
**Status:** Accepted 2026-08-07
**Branch:** `cursor/sp-039-city-aggregation-191e`
**Depends on:** Phase 4 Accepted (settlement containment); SP-033 measurement
  recorded; SP-034 area completion; SP-035/036 badge/focus integration
**Notes:** Settlement containment from Phase 4. SP-037+ note SP-033.

---

## Objective

Aggregate personal completion to city / settlement scale using Phase 4
settlement containment, and show a city-scale summary badge when zoom /
focus rules call for it (spec §12.3). Neighborhood-level completion remains the
main everyday metric where neighborhoods exist.

## Motivation

Spec §12.3: at city zoom the primary summary badge may show overall city
completion; administrative areas may be shaded by completion. Phase 4 provides
settlement rings and containment; Phase 5 must roll up area completions without
double-counting pixels.

## In-scope behavior

- City / settlement aggregation from area-scoped completion + Phase 4
  containment (true municipal rings; SPD-025).
- Summary badge mode at city zoom per §12.3 / §12.5 interactions.
- Correct arithmetic: each valid street pixel contributes to at most one area
  and thus once to the city rollup.
- No country or world aggregate.

## Out-of-scope behavior

- Country / world percentages (explicit non-goal).
- Competition weekly city leaderboard (Phase 8).
- Replacing neighbourhood badge as the default at street zoom.

## Relevant product requirements

- Spec §12.3; §8.5 settlement; §7 completion intent.
- SPD-007 / SPD-025 settlement geometry.

## Relevant source files or symbols

- `SelectSettlementContaining` / `ExplorationAreaResolver`
- SP-034 per-area cache
- SP-035 badge binding

## Implementation notes / constraints

- Do not start coding until SP-033 measurement is recorded.
- Aggregation must remain offline and map-version / policy-version aware.
- Prefer summing pixel counts (explored/total) over averaging area percentages.

## Acceptance criteria

1. Fixture city: sum of area explored/total matches city figure.
2. City summary badge appears under the specified zoom/focus conditions.
3. No country/world percentage computed or displayed.
4. Settlement-only cities (no subdivisions) show settlement completion
   consistently with Phase 4 fallback areas.

## Required automated tests

- City aggregation fixtures (multi-area; settlement-only; empty).

## Required manual validation

- Zoom to city scale over Helsinki (or fixture); confirm summary %.

## Failure and rollback considerations

- Do not average percentages across unequal areas; fix arithmetic instead.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-039-city-aggregation-191e` |
| Test output | `street_pixels_areas_tests` 66/66 (CityCompletion_* multi-area 1/2 rollup, settlement-only, cache, no country/world); `street_pixels_tests` 204/204 (CitySummaryUsesRollupFraction 0.5 vs settlement-only 0.0; fail-closed without .pix) |
| Manual validation | Device city-zoom Helsinki walk residual → SP-041 / Phase 10 |
| Accepted by | Maintainer (accept 2026-08-07) |
| Accepted date | 2026-08-07 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Assignable∈settlement via first ring vertex containment | Provisional; true municipal rings + Phase 4 PIP; residual if geometry edge cases appear |
| City fill still uses neighbourhood-baked overlay alpha | SP-037 stub; optional retune later |
| Device city-zoom summary % walk | SP-041 / Phase 10 |
