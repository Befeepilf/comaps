# SP-024 — Area-pipeline architecture decisions

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-023 measurements accepted
**Unblocks:** SP-025+

---

## Objective

Record accepted decisions for polygon store location, assignment locus,
assignment persistence, country-config format, suitability/privacy thresholds,
and settlement geometry — after SP-023 measurements — so later work items do
not encode guesses.

## Motivation

SPD-006/007 decide *what* exploration areas are. They do not decide where
polygons live, whether assignment is on-device or precomputed, how assignments
are stored at Uusimaa-scale universe size, or how country configuration is
versioned. Coding SP-025+ without those answers creates rework and risks
violating size or privacy constraints.

## In-scope behavior

- Write `SPD-0xx` entries (or extend existing) covering at least:
  1. Polygon store: in-MWM section vs downloadable sidecar vs hybrid.
  2. Assignment locus: on-device at derive/rematch vs generator-precomputed.
  3. Assignment persistence: full-universe area-id map vs sparse
     (explored-only / no-area omitted) vs rematerialize on demand — keyed by
     (map-data version, policy version).
  4. Country-config format, versioning, review process, keying by country.
  5. Concrete suitability thresholds (“meaningfully smaller”, meaningful pixel
     set) and optional min size/pixel count for anonymity (§23.4).
  6. Settlement geometry: keep three-box for containment vs true municipal
     polygons.
- Soften phase-04 wording that currently assumes assignment is always on-device
  if SP-024 chooses precompute (offline invariant still holds: no network
  boundary lookup).
- Update phase-04 Known uncertainties / open decisions that these close.
- Update README / phase entry status.
- Annotate SP-025–030 scopes if a decision narrows them (e.g. precomputed
  locus reduces SP-027/028 client compute).

## Out-of-scope behavior

- Implementing generator or client assignment.
- Changing SPD-004 (no pilot allowlists).
- Inventing grids or place-node polygons if size is hard (SPD-006 / §8.3).

## Relevant product requirements

- §3.5, §8.3–§8.8, §23.4; SPD-004, SPD-006, SPD-007.
- SP-023 evidence.

## Acceptance criteria

1. Each decision above is Accepted in `DECISIONS.md` with consequences.
2. Phase-04 entry criteria for “where polygons live” and “on-device vs
   precomputed” are marked met.
3. Downstream work items (SP-025+) reference the decision ids; any conditional
   scope (precompute vs on-device) is noted in those items.
4. Maintainer review of SP-023 numbers before accepting store/assignment SPDs.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of SP-023 numbers before accepting store/assignment SPDs.

## Failure and rollback considerations

- If SP-023 numbers force settlement-only or deferred subdivisions for V1,
  record that as an Accepted product decision (may shrink Phase 4 exit or
  move subdivision emission later) — do not invent grids.

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
