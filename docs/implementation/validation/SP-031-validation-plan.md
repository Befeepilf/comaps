# SP-031 — Validation plan (reviewed before execution)

**Work item:** [SP-031](../work-items/SP-031-area-pipeline-end-to-end-validation.md)
**Plan reviewed by:** Maintainer
**Plan review date:** 2026-08-07
**Branch:** `cursor/sp-031-area-pipeline-validation-191e` (lands on `street-pixels`)

## Approved decisions

| ID | Decision |
| --- | --- |
| Fixture country | **Finland** (Helsinki / Uusimaa-class focus), same grain as SP-023 / SPD-023 seed. |
| Device walks | Deferred to **Phase 10** residual if OEM/device access blocks (same posture as SP-014 / SP-022). Not required to record automated exit coverage. |
| Aggressive OEM | Deferred to Phase 10. |
| SPD-024 floors | Exit #7 is sidecar / assignment-blob **size acceptance** only. There is **no** V1 numeric client pixel/area floor to validate. Do **not** invent floors in the evidence log. |
| Mapgen emit | Full generator mapgen emission into `.spa` remains a known SP-026 residual. Do not claim exit #1 Met for a shipping full-country blob until emit (or an equivalent offline FI `.spa`) is evidenced. |
| Shipping size | Exit #7 may stay **residual** until `SaveOuterPath` shipping encoder is re-measured vs SP-023 zlib `coded_delta` baseline (~2.06 MiB national / ~0.52 MiB Helsinki). |
| Helsinki names | Programmatic name spot-check only if `/tmp/sp023` (or equivalent FI rings) is present; otherwise residual. |
| Flake note | `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` is a known intermittent pre-existing flake. Do **not** fail Phase 4 on a single flake of that test; re-run once and record. |

## Scope

Evidence-only. No production behaviour changes on the validation branch except
defect fixes that block suites and are routed to owning SP-023–030 items
(prefer fix on owner; tiny validation-blocking fixes allowed with explicit note
in the evidence log). Maintainer decides Phase 4 exit after reviewing evidence.

Phase 4 modules under test: SP-023 (spike budget), SP-024 (SPD-020–025),
SP-025 (country config), SP-026 (`.spa` format / library), SP-027 (client load),
SP-028 (subdivision consume/verify), SP-029 (settlement fallback / no-area),
SP-030 (sparse `.spx` + rematerialize).

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Finland / Helsinki | Same handset class as SP-014 / SP-022 |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log.

**Preferred country:** Finland — SP-023 measured admin rings; SPD-023 FI seed
`admin_10` → `9` → `11` subdivisions, `admin_8` settlement.

## Scenario catalogue

Run automated blocks on agent. Device scenarios on **D1** when available.
Evidence: one row per scenario in
[`SP-031-evidence-log.md`](SP-031-evidence-log.md). Map each result to Phase 4
exit criteria 1–8.

### Block A — Polygons / sidecar format (exit 1, 7)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | Fixture `.spa` round-trip (rings + dense assign) | True closed rings deserialize; no three-box / place-node invented geometry in output | 1 |
| A2 | Filter admits named closed configured levels; rejects three-box / unnamed / node-invented | Filter tests green | 1 |
| A3 | Production mapgen emit (OSM collectors → `.spa`) | Wired and produces at least one full-country (FI) sidecar **or** residual recorded | 1 |
| A4 | Shipping-encoder size vs SP-023 budget | Measure FI / Helsinki `.spa` (+ assign blob) with `SaveOuterPath`; reconcile with ~2.06 MiB / ~0.52 MiB zlib coded_delta baseline. **No numeric client floor** (SPD-024) | 7 |
| A5 | Optional offline FI emit from `/tmp/sp023` JSONL | If data present: policy filter + write sidecar + size note; else residual | 1, 7 |

### Block B — Country config (exit 2)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | Shipped Finland fixture priority | FI loads; subdivision order 10→9→11; settlement admin_8; place_boundaries enabled | 2 |
| B2 | Priority applied vs default | Assigner / config tests choose configured order, not a single global rule | 2 |
| B3 | Ignore invented floor keys | Floor-like keys never applied (SPD-024) | 2, 7* |

\*Confirms SPD-024 posture in config loader; not a size measurement.

### Block C — Assignment determinism (exit 3, 4)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | Determinism / no dual assignment | Same fixture twice → identical dense map; no pixel assigned twice | 3 |
| C2 | Nested smallest-polygon | Nested valid polygons → smallest wins | 4 |
| C3 | Stable-id tie-break | Equal-area → lower stable OSM id (not iteration order) | 4 |
| C4 | Client consume matches generator | Lookup / verify path agrees with precomputed map | 3 |
| C5 | Version mismatch fail-closed | Wrong map-data / policy version → empty / no invent | 3 |

### Block D — Settlement fallback / no-area (exit 5, 6)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | Settlement-only city | No subdivision → settlement area from true municipal rings | 5 |
| D2 | Rural / outside settlements | Exploration allowed; area lookup returns none (no grid) | 6 |
| D3 | Subdivision wins over settlement | Layering: subdivision before settlement fallback | 5 |
| D4 | Sparse rematerialize / policy bump | `.spx` rematerialize keeps explored; missing area → none | 3, 5, 6 |

### Block E — Display / neighbourhood identity (exit 8)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | Display name never falls back to MWM country id | `ExplorationSidecar_DisplayNameNeverFallsBackToMwmId` green | 8 |
| E2 | Device / UI: no MWM id shown as neighbourhood | Visual confirm when walks run; else residual Phase 10 | 8 |

### Block F — Helsinki / full-country inspection (manual)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Known Helsinki subdivision names look right | Spot-check known districts (e.g. Kamppi, Kallio, Punavuori) when FI rings / `.spa` available | 1 |
| F2 | Settlement-only / rural / coastal as opportunity | Matches phase manual strategy; else residual | 5, 6 |
| F3 | Device area walks (explore → area assignment visible) | Deferred Phase 10 if no device | — |

### Block G — Automated suite (feeds all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| G1 | Full `street_pixels_areas_tests` | All pass; count recorded | 1–8 |
| G2 | `street_pixels_tests --filter=Rematch` | All pass; count recorded | 3* |
| G3 | `street_pixels_tests --filter=AssignmentPersist` | All pass; count recorded | 3 |
| G4 | `street_pixels_tests --filter=CountryConfig` | All pass; count recorded | 2 |
| G5 | Full `street_pixels_tests` | All pass; count recorded. Known flake: re-run `PauseResume_…ImmediateResumeAdd_SplitsCorrectly` once if sole failure | — |

\*Rematch filter is Phase 3 permanence + SP-030 assignment rematch smoke; not a substitute for area mapgen emit.

## Finland / Helsinki measurement slots

Fill in the evidence log when data exists. Prefer the same FI extract / SHA.

| Slot | What to record | How |
| --- | --- | --- |
| S1 | SP-023 baseline (reference) | Spike note: ~2.06 MiB national zlib coded_delta; ~0.52 MiB Helsinki |
| S2 | Shipping `.spa` rings size (FI / Helsinki) | Offline emit or mapgen output; `ls -l` |
| S3 | Dense assignment blob size | Same sidecar `assign` section / companion artifact |
| S4 | Sparse `.spx` size at explored fraction (optional) | SP-030 note formula; device or fixture |
| S5 | Policy filter admit counts | `filter_rings_for_spa.py` on FI JSONL if present |
| S6 | Helsinki name spot-check | Programmatic OSM id → name when `/tmp/sp023` present |
| S7 | Mapgen emit status | Wired / not wired (SP-026 residual) |

If shipping size is unknown, record **residual** for exit #7 — do not invent a
pass bar or numeric floor (SPD-024).

## Deferred / absorbed from owning items

| Source | Covered by |
| --- | --- |
| SP-023 FI size / Helsinki names | S1, F1, S6 |
| SP-025 Finland config | B1–B3, G4 |
| SP-026 format + mapgen residual | A1–A5, S2–S3, S7 |
| SP-027 load / display name | A1, E1 |
| SP-028 subdivision assign | C1–C5 |
| SP-029 settlement / no-area | D1–D3 |
| SP-030 sparse persist | D4, G3 |

## Residuals → Phase 10 / owning SP

Explicit candidates (add rows only when observed):

| Residual class | Example | Disposition |
| --- | --- | --- |
| Mapgen emit gap | Collectors not wired to write production `.spa` | Owning follow-up from SP-026; blocks claiming exit #1 Met for shipping FI |
| Shipping size unmeasured | No FI `.spa` byte size with `SaveOuterPath` | Exit #7 residual; re-measure when emit/offline harness available |
| Device / Helsinki walks | Pixel 3a area UX, coastal/rural visual | Phase 10 (SP-014/022 pattern) |
| `/tmp/sp023` absent | Cannot run programmatic Helsinki name check | Residual until spike data or shipping FI `.spa` restored |
| Pause-resume flake | `ImmediateResumeAdd_SplitsCorrectly` | Pre-existing; not Phase 4 |
| Dense-admin second country | Worldwide size expectations | Optional; Finland grounds V1 |
| Client numeric floors | Privacy / suitability pixel counts | Forbidden by SPD-024 until new SPD |

## Automated baseline (agent)

```bash
./tools/unix/build_omim.sh -d -p /workspace street_pixels_areas_tests street_pixels_tests
./omim-build-debug/street_pixels_areas_tests
./omim-build-debug/street_pixels_tests --filter=Rematch
./omim-build-debug/street_pixels_tests --filter=AssignmentPersist
./omim-build-debug/street_pixels_tests --filter=CountryConfig
./omim-build-debug/street_pixels_tests
```

Record results in the evidence log. Executor must paste **real** output counts
from this branch SHA.

## Phase 4 exit status (fill after evidence)

| Exit # | Criterion | Status | Evidence |
| --- | --- | --- | --- |
| 1 | True closed polygons available for fixture country | | A1–A5, G1; mapgen residual honest |
| 2 | Versioned country config applied by priority | | B1–B3, G4 |
| 3 | Every valid street pixel ≤1 area; deterministic | | C1, C4–C5, D4, G1/G3 |
| 4 | Smallest-polygon + stable-id tie-break tested | | C2–C3 |
| 5 | Settlement fallback | | D1, D3 |
| 6 | Outside settlements: exploration works, no area | | D2; device residual if walks skip |
| 7 | Sidecar/assignment-blob size measured and accepted (no client numeric floor — SPD-024) | | A4, S1–S3; residual OK if unmeasured |
| 8 | No MWM country id as neighbourhood | | E1; E2 device residual |

**Do not** mark “Exit criteria met” in phase docs unilaterally. Maintainer
decides Phase 4 exit (Accepted / Accepted with residuals / Blocked).

## Execution order

1. Automated baseline (agent) — rebuild + suites; log counts.
2. Confirm SP-023–030 Accepted (or record blocking residual).
3. Attempt `/tmp/sp023` Helsinki name + optional size emit; else residual.
4. Fill exit table honestly (mapgen emit, shipping size, device walks).
5. Update SP-031 work item → In review; README / phase-04 status; maintainer decides.
