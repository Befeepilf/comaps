# SP-020 — Eligibility vs spec §13 — tighten or record

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** Planned
**Branch:** `street-pixels`

---

## Objective

Bring `IsExplorable` as close as client-side MWM types allow to product spec
§13, and record every remaining divergence with a reason (including OQ-5
outcomes that cannot be fixed without generator work).

## Motivation

Current eligibility: line `highway` features; excludes `driveway` and `tunnel`
third-level subtypes; excludes `hwtag=private`; requires bicycle or pedestrian
access. Spec §13 lists a broader inclusion/exclusion policy. Phase exit
criterion 6 allows either match or explicit recorded divergence — not silent
drift.

## In-scope behavior

- Tag-survival audit against classificator / feature typing: what §13 rules are
  representable in installed MWMs.
- Tighten client filters where tags survive (e.g. confirmed `hwtag` foot/bike/
  private, tunnel/driveway paths already used).
- Written divergence register (phase file or `DECISIONS.md` / work-item
  evidence) for each unmet §13 rule, with whether generator work is required.
- Advance or explicitly leave open OQ-5 (bridge/tunnel/motorway+bicycle) with
  evidence.
- Fixture tests for each inclusion/exclusion the client can enforce.

## Out-of-scope behavior

- Generator pipeline changes to preserve new tags (record as follow-up; do not
  expand V1 generator scope inside this item unless already trivial and
  approved).
- Area assignment (Phase 4).
- Sampling (SP-019).

## Relevant product requirements

- §13.1–§13.4 Eligible and excluded routes; versioned map-data policy.
- OQ-5.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.cpp` — `IsExplorable`
- `libs/indexer/feature_data.cpp` — `hwtag` normalisation
- Classificator paths for highway subtypes
- `libs/map/street_pixels_tests/*`

## Implementation notes / constraints

- Prefer recording honest gaps over pretending client filters exist for absent
  tags.
- Any behaviour change that removes previously explorable pixels interacts with
  rematch; land after SP-017 or trigger universe rebuild deliberately.

## Acceptance criteria

1. Every §13.1 / §13.2 rule is either enforced in `IsExplorable` or listed with
   reason and owner (client vs generator).
2. OQ-5 has a recorded outcome or a narrowed residual question.
3. Automated fixtures cover enforceable rules.
4. Divergences are linked from the phase file.

## Required automated tests

- Fixture features for enforceable include/exclude cases.
- Regression: private / driveway / tunnel / no-access still excluded as today
  unless intentionally changed and documented.

## Required manual validation

- Spot-check a known private road / pedestrian path on device against expected
  red-pixel presence or absence.

## Failure and rollback considerations

- Tightening that drops large urban coverage needs product acknowledgement
  before merge.
- Rollback is revert of filter changes; rematch absorbs universe diff if
  already shipping.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Divergence register link | |
| OQ-5 outcome | |
| Test output | |
| Manual validation | |
| Implemented by | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
