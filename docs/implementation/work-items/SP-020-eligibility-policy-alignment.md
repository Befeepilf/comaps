# SP-020 — Eligibility vs spec §13 — tighten or record

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** In progress
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

- `libs/map/street_pixels_manager.cpp` — `IsExplorable` / `IsExplorableFeature`
- `libs/indexer/feature_data.cpp` — `hwtag` normalisation
- Classificator paths for highway subtypes
- `libs/map/street_pixels_tests/eligibility_tests.cpp`

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

## Client-enforced rules (post SP-020)

| Spec rule | Enforcement |
| --- | --- |
| §13.1 line outdoor highway geometry | `GeomType::Line` + classificator path `highway-*` |
| §13.1 public streets / roads / links / service / pedestrian / cycleway / footway / steps / path / track | Included when highway line and not hard-excluded |
| §13.1 bridges on eligible land routes | Third-level `bridge` not excluded |
| §13.1 motorway / motorway_link (incl. bridge) with explicit bicycle | `path[1]` motorway or motorway_link requires `hwtag-yesbicycle` |
| §13.2 access=private | `hwtag-private` (generator maps `access=private`) |
| §13.2 access=no | `hwtag-private` (generator maps negative `access` including `no`) plus `path[2]=="no-access"` for tracks |
| §13.2 inaccessible to both foot and bike | `hwtag-nobicycle` ∧ `hwtag-nofoot` (yes* overrides) |
| §13.2 underground / tunnel | `path[2]=="tunnel"` → `hardExclude` |
| §13.2 construction-only | `path[1]=="construction"` (incl. subtypes) → `hardExclude` |
| §13.2 elevator / raceway (operational) | `path[1]` elevator or raceway → `hardExclude` |
| §13.2 driveway | `path[2]=="driveway"` → `hardExclude` |
| §13.2 ferry / waterborne / aerial | Not `highway` — excluded by highway gate (not `PedestrianModel::IsRoad`) |

## Divergence register (spec §13 vs client)

| Spec §13 rule | Status | Owner / reason |
| --- | --- | --- |
| Indoor corridors and passages | **Unmet** | Generator — no durable indoor type survives into MWM highway lines for client filter |
| Subway / metro passages | **Unmet / approx** | No subway-passage highway type; tunnel footways excluded; non-tunnel indoor subway ways need generator tag survival |
| Emergency-only routes | **Unmet** | Generator — no `emergency`-only highway type in classificator for client filter |
| Proposed routes | **Unmet** | Generator — proposed not present as a filterable highway subtype in installed MWMs |
| Underground / outdoor underpass via tunnel | **Accepted approx** | Client excludes all `path[2]=="tunnel"`, including outdoor underpasses that spec may intend as eligible land routes |
| Clearly restricted operational (parking aisle / busway) | **Residual include** | Still explorable (`highway-service-parking_aisle`, `highway-busway`); not treated as clearly restricted operational infrastructure in V1 client filter |
| Ferry / waterborne / aerial | **Enforced via gate** | Not `highway` lines — out via highway gate; fixtures cover ferry / aerialway / pier; do not use `PedestrianModel::IsRoad` (includes ferry) |
| access=no | **Enforced** | Generator emits `hwtag-private` for negative access; tracks also get `highway-track-no-access` |
| Trunk / controlled-access beyond motorway | **Residual include** | Spec mentions controlled-access with explicit bicycle; V1 only gates `motorway` / `motorway_link` (not `trunk`) |

Linked from [phase-03](../phases/phase-03-exploration-storage-and-reconciliation.md).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | _(not committed — awaiting review)_ |
| Divergence register link | This work item § Divergence register; phase-03 eligibility row |
| OQ-5 outcome | **Closed** — bridges include; tunnels exclude; motorway/motorway_link (incl. bridge) require `hwtag-yesbicycle`. Recorded in `DECISIONS.md` §15. |
| Test output | `ninja street_pixels_tests` OK. `--filter=Eligib` 9/9 All tests passed; full suite 164/164 All tests passed (2026-08-03). |
| Manual validation | Deferred — spot-check private / pedestrian path on device |
| Implemented by | Agent |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Indoor / subway-passage / emergency-only / proposed exclusions need generator tag survival | Post-V1 generator / map-data policy work; do not fake client filters |
| Tunnel exclude over-reaches outdoor underpasses | Accepted V1 approx; revisit only with product + tag design |
| parking_aisle / busway remain explorable | Residual; product call if operational infrastructure should drop |
| Trunk not gated like motorway for explicit bicycle | Residual; product call if controlled-access trunk should require yesbicycle |
| Tightening drops bare-motorway pixels from universe | Rematch absorbs after SP-017; expect coverage reduction on motorway-only geometry |
