# SP-031 — Evidence log

**Plan:** [SP-031-validation-plan.md](SP-031-validation-plan.md)
**Branch:** `cursor/sp-031-area-pipeline-validation-191e`
**Status:** In review (automated evidence recorded; device walks → Phase 10 residual; independent review honesty fixes applied)

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
| `/tmp/sp023` | **Absent** — Helsinki programmatic name spot-check and shipping-encoder FI size emit not run |
| Smoke / APK | Not run (agent desktop suites only) |

### Independent review re-verify (2026-08-07)

Docs-only delta on this branch; binaries match merge-base `e10111c537`.
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
| S2 | Shipping `.spa` rings size | **Not measured** | No FI shipping `.spa`; mapgen emit not wired; `/tmp/sp023` absent |
| S3 | Dense assignment blob size | **Not measured** | Same residual as S2 |
| S4 | Sparse `.spx` size | Formula in SP-030 note only | No device explored-fraction measurement |
| S5 | Policy filter admit counts | 2618 admitted / 64 unnamed / 69 policy_mismatch | From SP-026 evidence (prior FI JSONL run); not re-run here |
| S6 | Helsinki name spot-check | **Residual** | `/tmp/sp023` missing; SP-023 spike previously spot-checked Kamppi/Kallio/… |
| S7 | Mapgen emit status | **Not wired** | SP-026 discovered follow-up: collectors → `.spa` deferred |

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1 Fixture `.spa` round-trip | agent | **Pass** | `SpaSerdes_*` 4/4; sidecar TryLoad/assign round-trip |
| A2 Filter true closed / reject invented | agent | **Pass** | `ExplorationFilter_*` 6/6 |
| A3 Production mapgen emit | agent | **Residual** | Not wired (SP-026) |
| A4 Shipping-encoder size vs SP-023 | agent | **Residual** | Unmeasured; SPD-024 — no invented floor |
| A5 Offline FI from `/tmp/sp023` | agent | **Residual** | `/tmp/sp023` absent |
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
| F1 Helsinki names | agent | **Residual** | No `/tmp/sp023`; prior SP-023 manual 11/11 districts |
| F2 Settlement/rural/coastal visual | D1 | **Residual** | Phase 10 |
| F3 Device area walks | D1 | **Residual** | Phase 10 |
| G1 `street_pixels_areas_tests` | agent | **Pass** | **44/44** |
| G2 Rematch filter | agent | **Pass** | **18/18** |
| G3 AssignmentPersist filter | agent | **Pass** | **3/3** |
| G4 CountryConfig filter | agent | **Pass** | **11/11** |
| G5 Full `street_pixels_tests` | agent | **Pass** | **185/185**; PauseResume flake absent |

## Phase 4 exit criteria

| Exit # | Criterion | Pass / fail / residual | Evidence pointers |
| --- | --- | --- | --- |
| 1 | True closed polygons available for fixture country | **Residual** | A1–A2 / G1 fixtures+library green (**44/44**); A3 mapgen emit not wired; no FI production `.spa` in tree; SP-023 measured FI rings offline only — **full-country availability bar not Pass** |
| 2 | Versioned country config applied by priority | **Pass** | B1–B3; G4 **11/11**; shipped `data/street_pixels/country_policies.json` FI 10→9→11 / admin_8 |
| 3 | Every valid street pixel ≤1 area; deterministic | **Pass** | C1, C4–C5; assigner + assignment + sparse suites in G1; G3 **3/3** |
| 4 | Smallest-polygon + stable-id tie-break tested | **Pass** | C2–C3; assigner nested/tie-break + lookup mirrors |
| 5 | Settlement fallback | **Pass** | D1, D3; `exploration_area_resolver_tests` 5/5 in G1 |
| 6 | Outside settlements: exploration works, no area | **Pass (automated) + Residual (device)** | D2 automated; F2/F3 device visual → Phase 10 |
| 7 | Sidecar/assignment-blob size measured and accepted (no client numeric floor — SPD-024) | **Residual** | S1 SP-023 baseline only; S2–S3 shipping encoder unmeasured; **no numeric floor invented**; A4/A5 residual |
| 8 | No MWM country id as neighbourhood | **Pass (automated) + Residual (device UI)** | E1 green; E2 device UI → Phase 10 |

## Residuals (Phase 10 or owning SP)

| ID | Summary | Disposition |
| --- | --- | --- |
| R1 | Full generator mapgen emission (OSM collectors → `.spa`) not wired | SP-026 follow-up / pre-production emit item; blocks exit #1 Pass for shipping full country |
| R2 | Shipping-encoder FI / Helsinki `.spa` (+ assign) size not re-measured vs SP-023 | Exit #7 residual; re-measure when emit or `/tmp/sp023` offline harness available |
| R3 | Device walks (Helsinki names UX, rural/coastal, no MWM-id neighbourhood in UI) | Phase 10 — same pattern as SP-014 / SP-022 |
| R4 | `/tmp/sp023` absent — no programmatic Helsinki name / size emit this run | Residual; SP-023 spike previously validated 11/11 known districts |
| R5 | `PauseResume_…ImmediateResumeAdd_SplitsCorrectly` intermittent flake | Pre-existing; did not fail this run; not Phase 4 |

## Defects found

| ID | Summary | Owning WI | Disposition |
| --- | --- | --- | --- |
| — | None blocking suites | — | No production fixes on this validation branch |

## Owning WI acceptance pointer

| WI | Status at SP-031 run |
| --- | --- |
| SP-023–030 | Accepted (see README / work items) |
| SP-031 | **In review** — evidence recorded; maintainer decides Phase 4 exit |

## Maintainer exit decision

| Field | Value |
| --- | --- |
| Phase 4 exit | *(proposed: Accepted with residuals — do **not** set “Exit criteria met” until maintainer confirms)* |
| Proposed residuals | R1 mapgen emit; R2 shipping size; R3 device walks; R4 `/tmp/sp023` spot-check |
| Accepted by | |
| Accepted date | |
| Notes | Checklist: **1 Residual**; **2–5 Pass**; **6/8 Pass (automated) + Residual (device)**; **7 Residual** (SPD-024 — no floor). Fixtures/library do not satisfy exit #1’s full-country bar. |
