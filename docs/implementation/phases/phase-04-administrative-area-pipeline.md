# Phase 4 — Administrative-area pipeline

**Status:** Exit criteria met 2026-08-07 (device-walk residual R3 → Phase 10; narrowed R1 → Phase 4 residual / pre-production packaging SP-042+)
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

Re-verified 2026-08-07 at Phase 4 exit (supersedes 2026-08-03 entry snapshot).

| Concern | Location | Observed state |
| --- | --- | --- |
| Admin level retention | `data/mapcss-mapping.csv`, `data/classificator.txt` | Unchanged for drawable MWM types: 2–4 active; 7/9–11 deprecated; 5/6/8 unmapped. Exploration polygons do **not** depend on these typed entries (SPD-020 sidecar). |
| Place types | `data/mapcss-mapping.csv` | Search/label place types unchanged; exploration place rings come from `.spa` when policy admits closed rings |
| City boundary storage | `libs/indexer/city_boundary.hpp` | Still three-box for World search; **not** assignment authority (SPD-025) |
| Boundary build | `generator/collector_routing_city_boundaries.cpp`, `place_processor.cpp`, `cities_boundaries_builder.cpp` | World three-box path unchanged; exploration emit is offline `tools/spa_emit_tool/` (SP-032 geometry-only; **SP-044** production dense leaf emit Option B Accepted); in-pipeline collectors→`.spa` still unwired (Option A residual) |
| Exploration sidecar | `libs/street_pixels_areas/` (`.spa`, `DisplayName`, serdes) | Shipped format + library; FI fixture emit via SP-032 |
| Country config | `data/street_pixels/`, `street_pixels_config` | Versioned FI policy (SPD-023); SP-025 Accepted |
| Runtime assignment | `ExplorationAreaResolver`, `SubdivisionAssignmentTable`, `SparseAssignmentStore` | Deterministic subdiv → settlement → no-area; sparse `.spx` + rematerialize (SPD-021/022) |
| Area display name | `street_pixels::DisplayName` | Never falls back to MWM id (automated); device UI residual → Phase 10 |
| ExploreStats “region” | `StreetPixelsManager` / `ExploreStatsService` | Still MWM `countryId` for weekly aggregates — not neighbourhood progress (Phase 5) |

**Difference from the technical audit:** audit Spike 6 size measurement is
**done** (SP-023). Store/assignment architecture **closed** (SPD-020–025).
Area id / assignment APIs exist under `libs/street_pixels_areas/` (SP-026–030).

## Intended outcome

- A versioned, reviewable country configuration describing which
  `admin_level` values are valid for each country and in what priority order,
  with place-boundary fallbacks.
- Generator retention of the polygons that configuration requires, as real
  closed polygons rather than three-box approximations.
- A deterministic pixel-to-area assignment producing at most one area per
  pixel, with the smallest-polygon rule and a stable-identifier tie-break.
  Assignment for the valid universe is **generator-precomputed** into an
  offline blob (SPD-021); the client consumes/rematches that blob rather than
  running primary full-universe on-device PIP.
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
- Neighborhood polygon size for Finland is **measured** in SP-023 (~2.1 MiB
  country-concat zlib coded; Helsinki slice ~0.5 MiB). Store/assignment
  architecture is **recorded** as SPD-020–025 under SP-024 (work item In
  review — not yet Accepted).

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
| 10 | [SP-032](../work-items/SP-032-phase4-residual-spa-emit.md) | Offline `.spa` emit harness (SP-031 R1/R2/R4) |
| — | [SP-042](../work-items/SP-042-sidecar-shipping-decisions.md) | Sidecar shipping decisions (SPD-027–033); follow-ons SP-043–048 (pre-production packaging, not Phase 5) |
| — | [SP-043](../work-items/SP-043-spa-blob-contract-freeze.md) | Freeze production `.spa` blob contract (**SPD-034**; `format_version` 2) |
| — | [SP-044](../work-items/SP-044-production-spa-emit.md) | Production leaf `.spa` emit (R1; **Option B** offline batch; Option A residual) |
| — | [SP-045](../work-items/SP-045-countries-spa-meta.md) | Optional `spa` / `spa_sha1_base64` leaf fields in `countries.txt` (**SPD-028**; Accepted 2026-08-08) |

**SP-025+ unblocked** — SP-024 Accepted 2026-08-03 (SPD-020–025).

### Open decisions (for SP-024 after SP-023) — closed

1. Polygon store: in-MWM vs sidecar vs hybrid. **Closed by SPD-020** —
   per-country downloadable sidecar; World three-box remains search-only.
2. Assignment locus: on-device vs generator-precomputed. **Closed by SPD-021** —
   generator-precomputed offline blob; client consumes/rematches; no primary
   full-universe on-device PIP rematch.
3. Assignment persistence: full-universe area-id map vs sparse vs rematerialize
   on demand. **Closed by SPD-022** — sparse explored HEALPix→compact area
   index + rematerialize from dense uint16/uint32 sidecar map; no
   full-universe uint64 OSM ids; keyed by (map-data version, policy_version).
4. Country-config format / versioning / review process. **Closed by SPD-023** —
   versioned JSON under `data/street_pixels/` (exact files SP-025); monotonic
   `policy_version`; ISO 3166-1 alpha-2; PR review; Finland seed priority
   recorded.
5. Concrete suitability + privacy size thresholds. **Closed by SPD-024** —
   closed named config-level rings only; no invented numeric pixel/area
   floors; §23.4 anonymity stays server-side; measure before any client size
   gate.
6. Settlement geometry: three-box vs true municipal polygons. **Closed by
   SPD-025** — true municipal rings from the exploration sidecar; three-box
   `CityBoundary` is not assignment authority.

**Layering:** SP-028 assigns subdivision-or-none; SP-029 applies settlement
fallback / rural no-area on top. Do not invert that order.

## Data and migration concerns

- Exploration polygons and the dense assignment map ship as a **per-country
  sidecar** (SPD-020/022), not as mandatory in-MWM payload. Sidecar size is
  still a product-visible cost; SP-023 measured Finland rings (~2.1 MiB
  zlib coded national). Re-measure with the shipping encoder under SP-026/031.
- The country configuration is versioned independently of map data
  (`policy_version`, SPD-023). Assignment determinism is keyed on the pair
  (map-data version, policy_version). Both must be stored with assignments.
- A configuration change reassigns pixels without any map change. Percentages
  can move without the user updating anything. That needs the same honest
  communication as a map update.
- Boundaries change in OSM. An area can be split, merged, renamed, or removed
  between map versions. Decide what happens to a previously completed area
  whose polygon no longer exists; spec §27.4 allows keeping the original
  completion date locally.
- Client-durable assignment state is **sparse explored** HEALPix→compact area
  index; full-universe answers rematerialize from the dense uint16/uint32
  sidecar map (SPD-022). Do not keep full-universe uint64 OSM ids on device.

## Privacy and security implications

- Area identifiers are the only geographic data competition ever uploads. Their
  granularity is therefore the privacy floor of the entire competition feature.
  A very small area effectively identifies a user's neighbourhood.
- Spec §8.4 requires areas to be "meaningfully smaller than the containing
  settlement". V1 expresses that via country-config level selection and §8.8
  smallest-polygon assignment — **no invented numeric pixel/area floors**
  (SPD-024). Spec §23.4 sparse-area anonymity remains **server-side**. A
  client size or pixel-count gate needs follow-up measurement and a new SPD.
- Assignment must remain offline: no boundary lookup may become a network call.
  Compute locus is **generator-precomputed** (SPD-021); the client consumes the
  offline blob and must not phone home for area membership.

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
- Confirm sidecar (+ assignment blob) size against the measured budget
  (SPD-020 / exit #7) — not mandatory country-MWM growth.

## Entry criteria

- Phase 3 exit criteria met. **Met 2026-08-03** (device-walk residual → Phase 10).
- The area-pipeline investigation is complete with recorded measurements for at
  least one full country. **Met 2026-08-03** (SP-023 Finland accepted).
- A decision exists on where polygons live and whether assignment is on-device
  or precomputed. **Met** — SPD-020 (per-country sidecar), SPD-021
  (generator-precomputed). SP-024 **Accepted** 2026-08-03.

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
7. Sidecar (+ assignment blob) size impact is measured and accepted
   (SPD-020; no client numeric floor — SPD-024).
8. No MWM country identifier is presented anywhere as a neighbourhood.

### Phase 4 exit (2026-08-07)

**Status: Exit criteria met** 2026-08-07.

Evidence:
[`validation/SP-031-validation-plan.md`](../validation/SP-031-validation-plan.md),
[`validation/SP-031-evidence-log.md`](../validation/SP-031-evidence-log.md).
SP-031 / SP-032 **Accepted** 2026-08-07. Suites: `street_pixels_areas_tests`
46/46; full `street_pixels_tests` 185/185; Rematch 18; AssignmentPersist 3;
CountryConfig 11. SP-032 offline emit: FI ~1.93 MiB / Helsinki ~0.44 MiB;
Helsinki known ids **11/11**. Checklist: **1 Pass**; **2–5 Pass**; **6 / 8**
Pass (automated) + Residual (device); **7 Pass** (no SPD-024 floor).

**Residuals (do not block exit):**

| ID | Summary | Disposition |
| --- | --- | --- |
| R3 | Device walks (Helsinki UX, rural/coastal, no MWM-id neighbourhood in UI) | Phase 10 |
| R1 (narrowed) | Production leaf `.spa` emit + CDN leaf download / packaging not wired | **Phase 4 residual / pre-production packaging** — client/emit track SP-042–048 **Accepted** 2026-08-08 (shipping decisions [SP-042](../work-items/SP-042-sidecar-shipping-decisions.md) / **SPD-027–033**; emit **SP-044** Option B; countries meta **[SP-045](../work-items/SP-045-countries-spa-meta.md)**; download **[SP-046](../work-items/SP-046-spa-download-beside-mwm.md)**; lifecycle **[SP-047](../work-items/SP-047-spa-lifecycle-update-delete.md)**; validation **[SP-048](../work-items/SP-048-sidecar-shipping-validation.md)**). **LAN assemble/serve** — [SP-049](../work-items/SP-049-spa-distribute-layout-decisions.md)–[SP-053](../work-items/SP-053-spa-lan-device-validation.md) (SP-049 Accepted; SP-050–053 In review). **Independent generate+origin** — [Phase 11](phase-11-independent-map-build-and-serve.md) (SP-098–104 Planned; not a Phase 5 or Phase 10 exit). Option A mapgen collectors remain residual follow-up (not Phase 11). Offline harness (SP-032) satisfies fixture-country exit #1 |

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

- Whether full polygons can be retained at acceptable size. **Addressed by
  SP-023** (Finland affordable as sidecar); re-measure shipping codec in
  SP-026/031. Worldwide dense-admin countries still unmeasured.
- How many countries need bespoke configuration before coverage feels adequate,
  and who maintains that configuration. **Partial:** format/review locked by
  SPD-023; coverage breadth remains SP-025 incremental data work.
- Whether admin levels 5, 6, and 8 can be added to the classificator without
  disrupting upstream CoMaps behaviour that depends on the current mapping.
  Sidecar emission (SPD-020) may reduce pressure to map every level into
  drawable MWM types — still verify under SP-026.
- Whether the three-box `CityBoundary` approximation can be reused for
  settlement detection while true polygons are used for subdivisions, or
  whether settlements also need true polygons. **Closed by SPD-025** — true
  municipal rings from the sidecar; three-box is not assignment authority.
- How to keep divergence from upstream CoMaps manageable, given that the
  generator and classificator are shared with upstream.
- Whether "meaningfully smaller than the containing settlement" and
  "containing a meaningful street-pixel set" can be expressed as concrete
  numeric thresholds. **Closed for V1 by SPD-024** — config levels + §8.8;
  no invented floors; follow-up measurement before any client size gate.
- Whether area assignment on device is fast enough for a large country, or
  whether precomputation is required. **Closed by SPD-021** —
  generator-precomputed; on-device full-universe PIP is not the V1 rematch
  path.
- How to persist assignments at universe scale without careless O(N) bloat.
  **Closed by SPD-022**; implement and size-check in SP-030.
