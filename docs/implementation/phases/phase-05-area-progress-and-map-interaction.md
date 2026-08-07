# Phase 5 — Area progress and map interaction

**Status:** In progress (phase-entry planning 2026-08-07)
**Depends on:** Phase 4
**Blocks:** Phases 7, 8

---

## Objective

Turn area assignment into the everyday progress experience: a focused area with
a name and a percentage, predictable focus behaviour as the user moves and pans,
completion states that read clearly at every zoom level, and rendering that
stays acceptable at city scale.

## Product-spec references

- §7 Focused area; area completion.
- §12.1–§12.5 Street, neighbourhood, and city zoom behaviour; country and world
  zoom explicitly excluded; focus behaviour rules.
- §15.4 Area assignment — a pixel contributes to the area containing it, and to
  only one.
- §18.6 Completed visual state.
- §31 "No selected exploration area" empty state.
- §33 Success indicators 5 and 12.
- §34 "Progress experience" and "Quality" launch requirements.

## Technical-audit references

- §7 Rendering feasibility, including the strategy comparison and the pixel
  count and file size estimates by region scale.
- §5 Feature-reuse matrix rows for area completion, city completion, and pixel
  overlay rendering.
- §18 UI architecture implications: the progress badge exists but is not
  area-scoped; focused-area details are an entirely new screen.
- Spike 1, with its pass criterion of at least 30 FPS at the 95th percentile
  while panning at zoom 14–16 with a city loaded, and a memory uplift under
  150 MB.

## Current code locations

Re-verified 2026-08-07 against the working tree (Phase 5 entry).

| Concern | Location | Observed state |
| --- | --- | --- |
| Overlay renderer | `libs/drape_frontend/street_pixel_renderer.cpp` / `.hpp` | One GPU circle per HEALPix cell via `gpu::Program::CirclePoint`; `kMinVisibleZoomLevel = 9`; `kBucketZoomLevel = 15`; radius table `kRadiusInPixel` 0.6–5.5 px across zooms 1–20. **No city-scale FPS/memory measurement yet** (SP-033). |
| Pixel colour | `libs/drape_frontend/street_pixel.cpp` | Derived from the explored bit |
| Layer toggle | `android/sdk/.../maplayer/Mode.java` `STREET_PIXELS` | Present alongside `TRAFFIC`, `SUBWAY`, `ISOLINES`, `OUTDOORS` |
| Progress surface | `android/app/.../MwmActivity.java` | Attaches `StreetPixelsManager` on start/stop; `onStreetPixelsStateChanged` updates map buttons; explore menu entries exist |
| Completion figure | `StreetPixelsManager::GetTotalExploredFraction` | Explored count over MWM-scoped `.pix` universe — **not** area-scoped |
| ExploreStats | `ExploreStatsService` / `StreetPixelsManager` weekly aggregates | Still keyed by MWM `countryId` / region id — not neighbourhood progress |
| Area assignment (Phase 4) | `ExplorationAreaResolver`, `SparseAssignmentStore` | Subdivision → settlement → no-area; sparse `.spx` rematerialize available offline |
| Area display name | `street_pixels::DisplayName` (`exploration_sidecar`) | Name from sidecar; automated tests forbid MWM-id fallback |
| Area-scoped progress | — | **Not found** |
| Focused-area details screen | — | **Not found** |
| Area / pixel hit testing | — | No dedicated street-pixel hit test. Area selection must use **polygon** hit-test, not pixel picking. |

**Difference from the technical audit:** Phase 4 delivered polygon + assignment
APIs the 2026-07-20 audit marked absent. Renderer shape matches the audit
(one circle per cell). Spike 1 performance measurement remains **undone**.

## Intended outcome

- A primary progress badge showing the focused area's name and personal
  completion percentage.
- Focus behaviour that follows the spec's five rules and makes numeric changes
  understandable when the focused area changes.
- Tapping an area focuses it and reveals its exact percentage.
- City-scale summary progress and shaded areas by completion.
- A distinct completed-area visual state.
- A designed and tested "no exploration area here" state.
- Measured rendering performance at city scale on a mid-tier device (or
  documented residual → Phase 10 if no device, with desktop secondary).

## Dependencies

- Phase 4 exit criteria met (2026-08-07). Device-walk residual R3 → Phase 10;
  narrowed R1 mapgen emit → pre-production — neither blocks Phase 5 entry.
- SP-033 rendering measurement recorded before SP-034+ coding starts (mirror
  Phase 4 SP-023/024 gate).

## Phase-entry investigation (2026-08-07)

### Confirmed gaps

- Progress is MWM-scoped (`GetTotalExploredFraction`); no per-area explored /
  total cache.
- Spec §7 completion formula markup is **blank** (OQ-1). Surrounding text
  intent is unambiguous: percentage of valid street pixels in the area that
  the user has explored; live + imported both count. **Do not invent a
  contested formula as Accepted SPD.** SP-034 locks a provisional formula or
  defers formal SPD until the maintainer confirms (see Open questions).
- Focus-selection engine (§12.5 five rules) — not found.
- Area boundary / completion shading overlay — not found.
- Area tap selection (polygon hit-test) and focused-area detail surface —
  not found.
- City aggregation badge — not found (settlement containment exists from
  Phase 4 for grouping).
- Completed-area visual (§18.6) and no-area empty state (§31) — not found.
- Rendering Spike 1 measurement — **Partial SP-033**: qualitative OK on Pixel
  3a; quantitative ≥30 FPS p95 / &lt;150 MB → Phase 10. Provisional SP-037
  input: keep one-circle-per-cell renderer.

### Work-item breakdown

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-033](../work-items/SP-033-city-scale-rendering-performance-spike.md) | Spike: city-scale street-pixel rendering performance (**entry gate**) |
| 2 | [SP-034](../work-items/SP-034-area-scoped-completion-computation.md) | Area-scoped completion computation and cache |
| 3 | [SP-035](../work-items/SP-035-primary-progress-badge-focused-area.md) | Primary progress badge bound to focused area |
| 4 | [SP-036](../work-items/SP-036-focus-selection-engine.md) | Focus-selection engine (§12.5) |
| 5 | [SP-037](../work-items/SP-037-area-boundary-rendering-and-shading.md) | Area boundary rendering and completion shading by zoom |
| 6 | [SP-038](../work-items/SP-038-area-tap-selection-and-detail-surface.md) | Area tap selection and focused-area detail surface |
| 7 | [SP-039](../work-items/SP-039-city-scale-aggregation-and-summary-badge.md) | City-scale aggregation and summary badge |
| 8 | [SP-040](../work-items/SP-040-completed-area-and-no-area-states.md) | Completed-area visual state and no-area empty state |
| 9 | [SP-041](../work-items/SP-041-phase5-end-to-end-validation.md) | Phase 5 end-to-end validation (**exit gate**) |

**Do not start SP-034+ coding until SP-033 measurement is recorded** (desktop
secondary OK if mid-tier Android device is deferred; device residual honesty
same pattern as Phase 4 R3 → Phase 10). SP-037+ additionally depend on the
SP-033 LOD outcome.

### Open questions

| Ref | Question | Disposition for Phase 5 |
| --- | --- | --- |
| OQ-1 (completion slice) | Spec §7 formula markup is blank. Intent from surrounding text: explored / total **valid street pixels in the area** (live + imported). Ownership/contested pieces of OQ-1 stay Phase 8. | **Open for SP-034.** Recommend provisional SPD in SP-034 **or** defer formal SPD until maintainer confirms. Do **not** invent a contested formula as Accepted SPD in planning docs. |
| Spike 1 / SP-033 | Does one-circle-per-cell meet ≥30 FPS p95 at zoom 14–16 and &lt;150 MB memory uplift on mid-tier Android? | **Pending SP-033.** If no device, record desktop secondary + residual → Phase 10; do not fake device numbers. |
| Badge vs recording focus | Spec §12.5 rules 1 and 2 can both apply when map centre ≠ user during recording. | Resolve in SP-036 with separate test cases per rule; escalate product conflict if observed. |

## Data and migration concerns

- Per-area totals and explored counts are derived data. Decide whether they are
  cached or recomputed; a full recount over a large country per frame is not
  viable, and a stale cache produces wrong percentages.
- Cached aggregates must be invalidated by: pixel collection, GPX import, map
  update rematch, and country configuration / policy change.
- City-level aggregation needs a city identifier per area, which comes from
  Phase 4's settlement containment relationship.
- Spec §18.5 allows storing the original 100% completion date locally. If that
  is introduced here rather than in Phase 7, it is new persisted state.

## Privacy and security implications

- Area percentages are local-only unless competition is enabled. Nothing in
  this phase uploads.
- The focused-area name is displayed prominently. Any screenshot or share
  surface built later inherits that. Keep the badge free of anything more
  precise than the area name. Never show MWM country id as the area name.
- Completion caches are location-derived data at rest; they live in the
  existing on-device stores and are removed with them.

## Automated testing strategy

- Completion arithmetic against fixtures with known totals, including an area
  with zero pixels and an area with all pixels explored.
- Cache invalidation on each of the four triggers above.
- Focus selection: each of the five spec §12.5 rules as a separate case,
  including the recentre-returns-to-current-area rule.
- No-area state: the badge and detail surface behave correctly when the user is
  outside any area.
- City aggregation: the sum over areas matches the city figure for a fixture
  city.
- Renderer regression via the existing `drape_frontend_tests` target where
  applicable.

## Manual validation strategy

- Walk across an area boundary during a session and confirm the badge switches
  cleanly and the new number is understandable.
- Pan away from the current location and confirm focus follows the map centre;
  recentre and confirm focus returns.
- Tap several areas and confirm each shows the correct exact percentage.
- Zoom from street to city scale and confirm the badge and rendering transition
  as specified.
- Load a large city, pan and zoom at zoom 14–16 on a mid-tier Android device,
  and record frame times and memory against the spike 1 pass criteria (SP-033 /
  SP-041).
- Complete a small area and confirm the completed visual state at every zoom.

## Entry criteria

- Phase 4 exit criteria met. **Met 2026-08-07** (device residual R3 → Phase 10;
  narrowed R1 mapgen emit → pre-production).
- A rendering performance measurement exists for at least one large city on a
  mid-tier device. **Partial (SP-033)** — qualitative OK on Pixel 3a
  (maintainer); quantitative Spike 1 FPS/memory → Phase 10. Unblocks SP-034.

## Exit criteria

1. The primary badge shows the focused area's name and correct percentage.
2. Focus behaviour matches all five rules in spec §12.5.
3. Tapping an area focuses it and reveals its exact percentage.
4. Area and city completion are correct for the installed map version.
5. Completed areas have a distinct visual state that survives zoom changes.
6. The no-area state is implemented and tested.
7. Rendering performance meets the recorded pass criteria on a mid-tier device,
   or a level-of-detail strategy is implemented and re-measured (device
   residual → Phase 10 if measurement deferred).
8. No country or world percentage is calculated or displayed.

## Explicit non-goals

- Country and world exploration percentages. Spec §12.4 excludes them from V1.
- Percentage labels scattered across every neighbourhood by default. Spec §12.1
  forbids it.
- Milestones and celebrations. Phase 7.
- Competition overlays, boss information, and rankings. Phase 8.
- Achievement or milestone history screens. Spec §18.5 excludes them.
- Custom map themes and advanced heatmaps. Post-V1.
- Street-pixel hit testing. Area selection uses polygons.
- Inventing a contested completion formula as Accepted SPD without maintainer
  confirmation (OQ-1 → SP-034).

## Known uncertainties

- Whether the current one-circle-per-cell renderer meets the performance target
  at large-city density. **Addressed by SP-033 measurement.**
- Whether area boundary rendering can reuse existing map overlay machinery or
  needs a new layer (SP-037; LOD informed by SP-033).
- How the badge should behave when the map centre sits in a different area from
  the user during an active recording session; spec §12.5 rules 1 and 2 can
  both apply (SP-036).
- What "may fade or aggregate to preserve readability" (spec §12.3) should
  concretely mean at city zoom (SP-037/039).
- Whether completion caches belong in `street_stats.db` or in a new store
  (SP-034).
- How area-name transitions should be animated so the numeric change reads as a
  context switch rather than as lost progress (SP-035).
- Exact completion formula lock for OQ-1 (SP-034; provisional SPD or defer).
