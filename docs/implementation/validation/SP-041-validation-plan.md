# SP-041 — Validation plan (Phase 5 exit)

**Work item:** [SP-041](../work-items/SP-041-phase5-end-to-end-validation.md)
**Plan authored by:** Agent
**Plan review date:** 2026-08-07
**Branch:** `cursor/sp-041-phase5-validation-191e` (lands on `street-pixels`)

## Approved decisions

| ID | Decision |
| --- | --- |
| Fixture country | **Finland** / Helsinki-grain fixtures (same as Phase 4 / SP-031). |
| Device walks | Deferred to **Phase 10** if OEM/device access blocks (SP-014 / SP-022 / SP-031 pattern). Automated exit coverage remains mandatory. |
| Spike 1 quantitative | Already **Partial SP-033** — qualitative Pixel 3a OK; FPS/memory numbers → Phase 10. SP-037 kept one-circle-per-cell; no renderer swap requiring new Spike 1. |
| Phase 5 Accepted | Maintainer decides after reviewing evidence. Agent does **not** mark Phase 5 exit Met unilaterally. |
| Country / world % | Explicit non-goal (§12.4). Suites must prove no invented country/world aggregate in overlay or city rollup. |
| Completion date | SP-040 deferred local 100% date to Phase 7 — not an exit blocker. |
| Check glyph | SP-040 `m_showCheck` reserved; outline+restrained fill is §18.6 core evidence. Glyph polish may residual. |

## Scope

Evidence-only. No production behaviour changes on this branch except defect
fixes that block suites (prefer fix on owning SP-033–040). Map each Phase 5
exit criterion (1–8) to pass / fail / residual with pointers into the evidence
log.

Phase 5 modules under test: SP-033 (spike residual), SP-034 (area completion),
SP-035 (badge), SP-036 (focus engine), SP-037 (overlay), SP-038 (tap/detail),
SP-039 (city rollup), SP-040 (completed / no-area).

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Finland / Helsinki | Same class as SP-033 qualitative / SP-014 |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log when walks run.

## Scenario catalogue

### Block A — Badge / focused-area progress (exit 1)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | Set focus shows DisplayName + fraction | `FocusedAreaBadge_SetFocusShowsNameAndFraction` | 1 |
| A2 | Blank name clears focus (no MWM id) | `FocusedAreaBadge_BlankNameClearsFocus` | 1, 6 |
| A3 | Focus change updates snapshot | `FocusedAreaBadge_FocusChangeUpdatesBadgeSnapshot` | 1 |
| A4 | Invalid cache → fraction invalid, name kept | `FocusedAreaBadge_InvalidCacheMarksFractionInvalid` | 1 |

### Block B — Focus rules §12.5 (exit 2)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | Rule 1 recording follows user area | `FocusSelection_Rule1_*` | 2 |
| B2 | Rule 2 pan → map centre | `FocusSelection_Rule2_*` | 2 |
| B3 | Recording > pan | `FocusSelection_Rule1OverRule2_*` | 2 |
| B4 | Rule 3 explicit tap | Engine + `FocusEngine_Manager_Rule3_ExplicitSelect` | 2, 3 |
| B5 | Rule 4 recentre → user | Engine + manager recentre | 2 |
| B6 | Rule 5 city scale → city summary | Engine + manager city summary | 2, 4 |
| B7 | Sticky explicit ignores idle pan refresh | `FocusEngine_Manager_ExplicitStickyIgnoresIdlePanRefresh` | 2, 3 |

### Block C — Tap / detail (exit 3)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | Tap prefers subdivision | `SelectAtPoint_PrefersSubdivision` / Lookup | 3 |
| C2 | Outside clears focus | `SelectAtPoint_OutsideClears` + no-area signal | 3, 6 |
| C3 | Polygon hit-test (not pixel pick) | LookupExplorationAreaAtPoint tests | 3 |

### Block D — Area + city completion (exit 4)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | Area completion cache fractions | `AreaCompletion_*` + manager rebuild | 4 |
| D2 | City rollup sum explored/total (not avg %) | `CityCompletion_MultiAreaSumsWithoutDoubleCount` + manager rollup | 4 |
| D3 | Settlement-only matches area row | `CityCompletion_SettlementOnlyMatchesAreaRow` | 4 |
| D4 | Fail-closed without `.pix` | `CitySummaryFailClosedWithoutPix` | 4 |

### Block E — Completed / no-area (exit 5, 6)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | Distinct completed style vs in-progress | `AreaOverlay_StyleCompletedDistinctFromInProgress` | 5 |
| E2 | Street completed outline-only | Same + street band asserts | 5 |
| E3 | 100% focus → `m_areaCompleted` | Badge 100% fixture | 5 |
| E4 | No-area signal; empty name ≠ MWM leaf | `FocusedAreaBadge_NoAreaSignalNeverUsesMwmId` | 6 |
| E5 | Engine None on no area | `FocusSelection_NoAreaYieldsNone` | 6 |

### Block F — Rendering (exit 7)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Cite SP-033 qualitative Pixel 3a | Recorded Partial; keep one-circle-per-cell | 7 |
| F2 | SP-037 additive overlay (no Spike 1 renderer swap) | Overlay suites green; LOD simplify rings | 7 |
| F3 | Quantitative Spike 1 FPS/memory | Residual → Phase 10 | 7 |
| F4 | Device re-spot-check after SP-037/040 chrome | Residual → Phase 10 if no device | 7 |

### Block G — No country / world % (exit 8)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| G1 | City cache never invents country/world rows | `CityCompletion_NoCountryWorldAggregate` | 8 |
| G2 | Overlay never invents country choropleth | `AreaOverlay_NoCountryChoropleth` | 8 |
| G3 | Badge/detail bind focused / city rollup only | Code review + badge tests; no country/world UI strings | 8 |

### Block H — Manual / device (all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| H1 | Boundary walk / pan / recentre focus | Visual §12.5 | 1, 2 |
| H2 | Tap areas → detail exact % | Visual | 3 |
| H3 | Zoom street → city summary % | Visual rollup | 4 |
| H4 | Completed chrome across zooms | Visual §18.6 | 5 |
| H5 | Leave settlement → empty state copy | Visual §31 | 6 |
| H6 | Confirm no country/world % in UI | Visual | 8 |

### Block I — Automated suites (feeds all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| I1 | Full `street_pixels_areas_tests` | All pass; count recorded | 1–8 |
| I2 | Full `street_pixels_tests` | All pass; count recorded | 1–8 |
| I3 | `--filter=Focus` | All pass | 2, 3, 4 |
| I4 | `--filter=AreaCompletion` | All pass | 4 |
| I5 | `--filter=FocusedArea` | All pass | 1, 5, 6 |
| I6 | `--filter=City` | All pass | 4 |

## Exit criteria mapping (fill in evidence log)

| # | Criterion | Evidence blocks |
| --- | --- | --- |
| 1 | Primary badge name + correct % | A, I5 |
| 2 | Focus matches §12.5 five rules | B, I3 |
| 3 | Tap focuses + exact % | C, I3 |
| 4 | Area and city completion correct | D, I4, I6 |
| 5 | Distinct completed visual | E1–E3, F |
| 6 | No-area implemented and tested | E4–E5, A2 |
| 7 | Rendering pass or LOD + residual | F |
| 8 | No country/world % | G |

## Non-goals for this gate

- Phase 7 milestones / share cards / completion-date cards.
- Phase 8 competition overlays.
- Marking Phase 5 **Accepted** without maintainer decision.
- Weakening tests.
