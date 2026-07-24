# Phase 5 — Area progress and map interaction

**Status:** Not started
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

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Overlay renderer | `libs/drape_frontend/street_pixel_renderer.cpp` | One GPU circle per HEALPix cell using `gpu::Program::CirclePoint`; `kMinVisibleZoomLevel = 9`; `kBucketZoomLevel = 15`; radius-per-zoom table `kRadiusInPixel` running from 0.6 px at zoom 1 to 5.5 px at zoom 20 |
| Pixel colour | `libs/drape_frontend/street_pixel.cpp` | Derived from the explored bit |
| Layer toggle | `android/sdk/.../maplayer/Mode.java` `STREET_PIXELS` | Present alongside `TRAFFIC`, `SUBWAY`, `ISOLINES`, `OUTDOORS` |
| Progress surface | `android/app/.../MwmActivity.java` | Attaches `StreetPixelsManager` on start and stop; `onStreetPixelsStateChanged` updates map buttons; explore menu entries exist |
| Completion figure | `libs/map/street_pixels_manager.cpp` | Explored count over total, scoped to the MWM country, not to an area |
| Area-scoped progress | — | Not found |
| Focused-area details screen | — | Not found |
| Pixel hit testing | — | No dedicated street-pixel hit test. General tap and overlay picking exists. Area selection should use polygons, not pixel picking. |

**Difference from the technical audit:** none material for this phase.

## Intended outcome

- A primary progress badge showing the focused area's name and personal
  completion percentage.
- Focus behaviour that follows the spec's five rules and makes numeric changes
  understandable when the focused area changes.
- Tapping an area focuses it and reveals its exact percentage.
- City-scale summary progress and shaded areas by completion.
- A distinct completed-area visual state.
- A designed and tested "no exploration area here" state.
- Measured rendering performance at city scale on a mid-tier device.

## Dependencies

- Phase 4, for area identifiers, polygons, and per-pixel assignment.

## Proposed work-item breakdown

Not yet decomposed. Likely shape, to be confirmed at phase entry:

1. Area-scoped completion computation and caching.
2. Primary progress badge bound to the focused area.
3. Focus-selection engine implementing spec §12.5.
4. Area boundary rendering and completion shading by zoom.
5. Area selection by tap and the focused-area detail surface.
6. City-scale aggregation and summary badge.
7. Completed-area visual state.
8. Rendering performance measurement and, if needed, level-of-detail work.

**Marked for phase-specific Plan Mode investigation.** Whether item 8 is a
measurement task or a substantial rendering rework cannot be determined from
source inspection. Decompose after the measurement exists.

## Data and migration concerns

- Per-area totals and explored counts are derived data. Decide whether they are
  cached or recomputed; a full recount over a large country per frame is not
  viable, and a stale cache produces wrong percentages.
- Cached aggregates must be invalidated by: pixel collection, GPX import, map
  update rematch, and country configuration change.
- City-level aggregation needs a city identifier per area, which comes from
  Phase 4's containment relationship.
- Spec §18.5 allows storing the original 100% completion date locally. If that
  is introduced here rather than in Phase 7, it is new persisted state.

## Privacy and security implications

- Area percentages are local-only unless competition is enabled. Nothing in
  this phase uploads.
- The focused-area name is displayed prominently. Any screenshot or share
  surface built later inherits that. Keep the badge free of anything more
  precise than the area name.
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
  and record frame times and memory against the spike 1 pass criteria.
- Complete a small area and confirm the completed visual state at every zoom.

## Entry criteria

- Phase 4 exit criteria met.
- A rendering performance measurement exists for at least one large city on a
  mid-tier device.

## Exit criteria

1. The primary badge shows the focused area's name and correct percentage.
2. Focus behaviour matches all five rules in spec §12.5.
3. Tapping an area focuses it and reveals its exact percentage.
4. Area and city completion are correct for the installed map version.
5. Completed areas have a distinct visual state that survives zoom changes.
6. The no-area state is implemented and tested.
7. Rendering performance meets the recorded pass criteria on a mid-tier device,
   or a level-of-detail strategy is implemented and re-measured.
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

## Known uncertainties

- Whether the current one-circle-per-cell renderer meets the performance target
  at large-city density. This is the main open question in the phase.
- Whether area boundary rendering can reuse existing map overlay machinery or
  needs a new layer.
- How the badge should behave when the map centre sits in a different area from
  the user during an active recording session; spec §12.5 rules 1 and 2 can
  both apply.
- What "may fade or aggregate to preserve readability" (spec §12.3) should
  concretely mean at city zoom.
- Whether completion caches belong in `street_stats.db` or in a new store.
- How area-name transitions should be animated so the numeric change reads as a
  context switch rather than as lost progress.
