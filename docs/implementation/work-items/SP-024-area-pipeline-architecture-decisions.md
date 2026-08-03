# SP-024 — Area-pipeline architecture decisions

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Record accepted decisions for polygon store location, assignment locus,
country-config format, suitability/privacy thresholds, and settlement geometry
— after SP-023 measurements — so later work items do not encode guesses.

## Motivation

SPD-006/007 decide *what* exploration areas are. They do not decide where
polygons live, whether assignment is on-device or precomputed, or how country
configuration is versioned. Coding SP-025+ without those answers creates
rework and risks violating size or privacy constraints.

## In-scope behavior

- Write `SPD-0xx` entries (or extend existing) covering at least:
  1. Polygon store: in-MWM section vs downloadable sidecar vs hybrid.
  2. Assignment locus: on-device at derive/rematch vs generator-precomputed.
  3. Country-config format, versioning, review process, keying by country.
  4. Concrete suitability thresholds (“meaningfully smaller”, meaningful pixel
     set) and optional min size/pixel count for anonymity (§23.4).
  5. Settlement geometry: keep three-box for containment vs true municipal
     polygons.
- Update phase-04 Known uncertainties that these close.
- Update README / phase entry status.

## Out-of-scope behavior

- Implementing generator or client assignment.
- Changing SPD-004 (no pilot allowlists).

## Relevant product requirements

- §3.5, §8.3–§8.8, §23.4; SPD-004, SPD-006, SPD-007.
- SP-023 evidence.

## Acceptance criteria

1. Each decision above is Accepted in `DECISIONS.md` with consequences.
2. Phase-04 entry criteria for “where polygons live” and “on-device vs
   precomputed” are marked met.
3. Downstream work items (SP-025+) reference the decision ids.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of SP-023 numbers before accepting store/assignment SPDs.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Decision ids | |
| Depends on SP-023 | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
