# SP-031 — Evidence log

**Plan:** [SP-031-validation-plan.md](SP-031-validation-plan.md)
**Branch:** `cursor/sp-032-phase4-residual-emit-191e` (includes SP-031 validation docs + SP-032 emit harness)
**Status:** In review (exit #1/#7 Pass via SP-032 offline emit; R3 device walks → Phase 10; mapgen production emit still follow-up; **not** unilaterally Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-07 |
| Git SHA (suite run tip) | `e10111c5372c676b8d016d2fa50a8c9afc98bf94` (`[map][docs] Merge SP-030 sparse assignment persistence`; docs-only branch delta after this SHA) |
| Build command | `./tools/unix/build_omim.sh -d -p /workspace street_pixels_areas_tests street_pixels_tests` |
| Build result | OK — linked `street_pixels_areas_tests` and `street_pixels_tests` (debug) |
| `street_pixels_areas_tests` | **44/44** All tests passed |
| Areas breakdown | filter 6, sidecar 8, serdes 4, assigner 7, assignment 6, exploration_area_resolver 5, sparse_assignment_store 8 |
| `street_pixels_tests --filter=Rematch` | **18/18** All tests passed |
| `street_pixels_tests --filter=AssignmentPersist` | **3/3** All tests passed |
| `street_pixels_tests --filter=CountryConfig` | **11/11** All tests passed |
| Full `street_pixels_tests` | **185/185** All tests passed |
| Known flake | `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` — **did not fail** on this run |
| `/tmp/sp023` | **Absent** on initial SP-031 run — rebuilt under SP-032 |
| Smoke / APK | Not run (agent desktop suites only) |

### Independent review re-verify (2026-08-07)

Docs-only delta on the SP-031 branch; binaries match merge-base `e10111c537`.
Re-ran without rebuild:

| Suite | Result |
| --- | --- |
| `street_pixels_areas_tests` | **44/44** All tests passed (`grep -c '^OK$'` → 44) |
| `--filter=Rematch` | **18/18** All tests passed |
| `--filter=AssignmentPersist` | **3/3** All tests passed |
| `--filter=CountryConfig` | **11/11** All tests passed |
| Full `street_pixels_tests` | **185/185** All tests passed; `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` → OK |

Source macro spot-check (same counts): areas UNIT_TEST files 6+8+4+7+6+5+8=44;
`street_pixels_tests` UNIT_TEST total 185; Rematch/AssignmentPersist/CountryConfig
name matches 18/3/11.

### SP-032 offline emit follow-up (2026-08-07)

Closes residuals R2/R4 and the exit #1 availability bar via offline harness
([SP-032](../work-items/SP-032-phase4-residual-spa-emit.md)); production mapgen
collectors→`.spa` remains a follow-up (narrowed R1).

| Field | Value |
| --- | --- |
| Branch / tip | `cursor/sp-032-phase4-residual-emit-191e` @ `ae7fff5f7` (independent review harden + re-verify) |
| Build | `./tools/unix/build_omim.sh -d -p /workspace spa_emit_tool street_pixels_areas_tests` — OK |
| `street_pixels_areas_tests` | **46/46** All tests passed (`SpaJsonlEmit_*` + prior 44) |
| `/tmp/sp023` | Rebuilt: Geofabrik `finland-latest.osm.pbf` (737 679 925 B) → `extract_admin_place_polygons.py` → **2751** JSONL rings |
| Emit | `./omim-build-debug/spa_emit_tool --rings=/tmp/sp023/finland_admin_place_rings.jsonl --policy=data/street_pixels/country_policies.json --iso=FI --out_dir=/tmp/sp032 --helsinki_poly=data/borders/Finland_Southern\ Finland_Helsinki.poly` |
| Outputs | `/tmp/sp032/Finland.spa`, `/tmp/sp032/Finland_Southern Finland_Helsinki.spa` (**not committed**) |
| Spot-check | **11/11** found + **11/11** name_match; tool exits non-zero on miss (review harden) |

### Suite command transcripts (counts)

```text
$ ./omim-build-debug/street_pixels_areas_tests
… (44 × Running / OK; ends with SelectSettlement / filter / sidecar / …) …
All tests passed.
# grep -c '^OK$' → 44

$ ./omim-build-debug/street_pixels_tests --filter=Rematch
… (18 × OK) …
All tests passed.
# grep -c '^OK$' → 18

$ ./omim-build-debug/street_pixels_tests --filter=AssignmentPersist
… (3 × OK) …
All tests passed.
# grep -c '^OK$' → 3

$ ./omim-build-debug/street_pixels_tests --filter=CountryConfig
… (11 × OK; includes FinlandFixturePriority, IgnoreFloorKeysNeverApply, LoadShippedFinlandFixture) …
All tests passed.
# grep -c '^OK$' → 11

$ ./omim-build-debug/street_pixels_tests
… (185 × OK) …
All tests passed.
# grep -c '^OK$' → 185
# PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly → OK (no flake this run)

# SP-032:
$ ./omim-build-debug/street_pixels_areas_tests
… (46 × OK; includes SpaJsonlEmit_TinyRoundTrip, SpaJsonlEmit_ParseNullAdminLevel) …
All tests passed.
# grep -c '^OK$' → 46
```

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |

## Finland / Helsinki measurements

| Slot | Metric | Value | Notes |
| --- | --- | --- | --- |
| S1 | SP-023 baseline | ~2.06 MiB national zlib coded_delta; ~0.52 MiB Helsinki | Reference only — spike encoder ≠ shipping `SaveOuterPath` |
| S2 | Shipping `.spa` rings size | **Measured (SP-032)** — FI country-concat **2 019 268 B (~1.93 MiB)**; Helsinki leaf **456 484 B (~0.44 MiB)** | Geometry-only (`assign_count=0`); `SaveOuterPath` via `WriteExplorationSidecar`; under `/tmp/sp032/` |
| S3 | Dense assignment blob size | **N/A this run** | Geometry-only emit; assign section **0 B**; measure when HEALPix samples available from mapgen emit |
| S4 | Sparse `.spx` size | Formula in SP-030 note only | No device explored-fraction measurement |
| S5 | Policy filter admit counts | **2618** admitted / **64** unnamed / **69** policy_mismatch | Re-confirmed SP-032 emit + `filter_rings_for_spa.py` |
| S6 | Helsinki name spot-check | **Pass — 11/11** known OSM ids (+ name_match 11/11) | Kamppi, Kallio, Punavuori, Ullanlinna, Etu-Töölö, Taka-Töölö, Lauttasaari, Eira, Katajanokka, Kruununhaka, Helsinki (admin_8 settlement) |
| S7 | Mapgen emit status | Offline harness **wired** (SP-032); production collectors→`.spa` **not wired** | Narrowed residual — does not block exit #1 fixture-country availability |

### Shipping vs SP-023 size table (SP-032)

| Artifact | SP-023 zlib(coded_delta) | Shipping `.spa` (`SaveOuterPath`) | Notes |
| --- | --- | --- | --- |
| FI country-concat (policy-admitted rings) | ~2.06 MiB (all 2751 rings) | **2 019 268 B (~1.93 MiB)** file; areas section 2 019 185 B; hdr 40 B; assign 0 | 2618 admitted areas; codecs differ — not a 1:1 compare |
| Helsinki MWM leaf | ~0.52 MiB zlib_coded (all attributed rings) | **456 484 B (~0.44 MiB)** file; areas 456 373 B; hdr 66 B; assign 0 | Centroid-in-Helsinki-border + policy filter → 694 areas |
| Dense assign column | — | **0 B** (geometry-only) | SPD-021 samples deferred to mapgen emit job |

No client numeric floor applied (SPD-024). Sizes are below the SP-023 zlib coded baselines for this FI snapshot; maintainer accepts measurement as exit #7 evidence.

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1 Fixture `.spa` round-trip | agent | **Pass** | `SpaSerdes_*` 4/4; sidecar TryLoad/assign round-trip; SP-032 `SpaJsonlEmit_*` 2/2 |
| A2 Filter true closed / reject invented | agent | **Pass** | `ExplorationFilter_*` 6/6 |
| A3 Production mapgen emit | agent | **Residual** | Collectors→`.spa` not wired (narrowed R1); offline emit covers fixture country |
| A4 Shipping-encoder size vs SP-023 | agent | **Pass** | S2 table; SPD-024 — no invented floor |
| A5 Offline FI from `/tmp/sp023` | agent | **Pass** | SP-032 `spa_emit_tool`; `/tmp/sp032/*.spa` |
| B1 Finland fixture priority | agent | **Pass** | `CountryConfig_FinlandFixturePriority`, `LoadShippedFinlandFixture` |
| B2 Priority applied | agent | **Pass** | `SubdivisionAssigner_PriorityPrefersConfiguredOrder` + CountryConfig 11/11 |
| B3 Ignore floor keys | agent | **Pass** | `CountryConfig_IgnoreFloorKeysNeverApply` |
| C1 Determinism / no dual | agent | **Pass** | Assigner + `LookupSubdivision_DeterminismAndNoDual` |
| C2 Nested smallest | agent | **Pass** | Assigner nested + `LookupSubdivision_NestedSmallestAndOutside` |
| C3 Stable-id tie-break | agent | **Pass** | Assigner + lookup TieBreak tests |
| C4 Client matches generator | agent | **Pass** | `LookupSubdivision_ClientMatchesGenerator` |
| C5 Version mismatch fail-closed | agent | **Pass** | Sidecar + assignment table version mismatch tests |
| D1 Settlement-only | agent | **Pass** | `SelectSettlement_SettlementOnlyAndRural` |
| D2 Rural no-area | agent | **Pass** | Same + sparse `MissingAreaIsNoneNoGrid` |
| D3 Subdivision over settlement | agent | **Pass** | `LookupExplorationArea_SubdivisionWinsOverSettlement` |
| D4 Sparse rematerialize | agent | **Pass** | Sparse store 8/8 + AssignmentPersist 3/3 |
| E1 DisplayName never MWM id | agent | **Pass** | `ExplorationSidecar_DisplayNameNeverFallsBackToMwmId` |
| E2 UI no MWM neighbourhood | D1 | **Residual** | Device walks deferred Phase 10 |
| F1 Helsinki names | agent | **Pass** | SP-032 known-id spot-check **11/11** found + name_match |
| F2 Settlement/rural/coastal visual | D1 | **Residual** | Phase 10 |
| F3 Device area walks | D1 | **Residual** | Phase 10 |
| G1 `street_pixels_areas_tests` | agent | **Pass** | **44/44** (SP-031); **46/46** after SP-032 |
| G2 Rematch filter | agent | **Pass** | **18/18** |
| G3 AssignmentPersist filter | agent | **Pass** | **3/3** |
| G4 CountryConfig filter | agent | **Pass** | **11/11** |
| G5 Full `street_pixels_tests` | agent | **Pass** | **185/185**; PauseResume flake absent |

## Phase 4 exit criteria

| Exit # | Criterion | Pass / fail / residual | Evidence pointers |
| --- | --- | --- | --- |
| 1 | True closed polygons available for fixture country | **Pass** | A5 offline FI `.spa` (2618 true closed policy-admitted rings); F1 11/11; A1–A2 / G1 library green. Production mapgen emit still follow-up (A3 / narrowed R1) — does not block fixture-country availability |
| 2 | Versioned country config applied by priority | **Pass** | B1–B3; G4 **11/11**; shipped `data/street_pixels/country_policies.json` FI 10→9→11 / admin_8 |
| 3 | Every valid street pixel ≤1 area; deterministic | **Pass** | C1, C4–C5; assigner + assignment + sparse suites in G1; G3 **3/3** |
| 4 | Smallest-polygon + stable-id tie-break tested | **Pass** | C2–C3; assigner nested/tie-break + lookup mirrors |
| 5 | Settlement fallback | **Pass** | D1, D3; `exploration_area_resolver_tests` 5/5 in G1 |
| 6 | Outside settlements: exploration works, no area | **Pass (automated) + Residual (device)** | D2 automated; F2/F3 device visual → Phase 10 |
| 7 | Sidecar/assignment-blob size measured and accepted (no client numeric floor — SPD-024) | **Pass** | S2 shipping sizes measured; table vs S1; **no numeric floor invented**; geometry-only assign=0 documented (S3 deferred) |
| 8 | No MWM country id as neighbourhood | **Pass (automated) + Residual (device UI)** | E1 green; E2 device UI → Phase 10 |

## Residuals (Phase 10 or owning SP)

| ID | Summary | Disposition |
| --- | --- | --- |
| R1 | Full generator mapgen emission (OSM collectors → `.spa`) not wired | **Narrowed** — pre-production / SP-026 follow-up; offline harness satisfies exit #1 fixture-country bar (SP-032) |
| R2 | Shipping-encoder FI / Helsinki `.spa` size vs SP-023 | **Closed (SP-032)** — measured; see size table |
| R3 | Device walks (Helsinki names UX, rural/coastal, no MWM-id neighbourhood in UI) | Phase 10 — same pattern as SP-014 / SP-022 |
| R4 | `/tmp/sp023` absent — Helsinki name / size emit | **Closed (SP-032)** — JSONL rebuilt; emit + 11/11 spot-check |
| R5 | `PauseResume_…ImmediateResumeAdd_SplitsCorrectly` intermittent flake | Pre-existing; did not fail this run; not Phase 4 |

## Defects found

| ID | Summary | Owning WI | Disposition |
| --- | --- | --- | --- |
| — | None blocking suites | — | No production fixes on this validation branch |

## Owning WI acceptance pointer

| WI | Status at SP-031 / SP-032 run |
| --- | --- |
| SP-023–030 | Accepted (see README / work items) |
| SP-031 | **In review** — exit #1/#7 Pass after SP-032; R3 Phase 10; maintainer decides Phase 4 exit |
| SP-032 | **In review** — offline emit harness + evidence |

## Maintainer exit decision

| Field | Value |
| --- | --- |
| Phase 4 exit | *(proposed: Accepted with residuals — do **not** set “Exit criteria met” until maintainer confirms)* |
| Proposed residuals | Narrowed R1 mapgen emit (pre-production); R3 device walks → Phase 10 |
| Accepted by | |
| Accepted date | |
| Notes | Checklist after SP-032: **1 Pass**; **2–5 Pass**; **6/8 Pass (automated) + Residual (device)**; **7 Pass** (SPD-024 — no floor). Offline FI `.spa` satisfies fixture-country availability; production collectors→`.spa` still follow-up. |
