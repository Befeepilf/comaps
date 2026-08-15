# SP-038 — Overlay area-label taps

Tap contract after SP-037 overlay labels (2026-08-15). SP-037 draws clipped
rings **and** area names at `m_labelPoint` via `gui::StaticLabel` inside the
exploration overlay batch (`ExplorationAreaOverlayItem::m_name`,
`m_labelPoint`). Those captions are **not** CoMaps POI overlay handles, so
`GetVisiblePOI` never selects them.

## What opens the exploration sheet

User map taps (`BuildInfo::Source::User`) only when:

1. Draw scale ≥ `kAreaOverlayMinZoom` (9).
2. Tap pixel hits the overlay label AABB stored when pushing overlay geometry
   (`compactIndex`, `labelPoint`, estimated half-size from name length and
   font 28).
3. `SelectFocusedAreaExplicit(compactIndex)` succeeds.

Opens the focused-area detail sheet only (`DeactivateMapSelection` in
`Framework::OnTapEvent`). Does not clear focus on label miss.

## What keeps the place page

- Shop / amenity POI, bookmark, track, my-position, route point (including
  point POIs that overlap a label AABB — `m_isPointFeature` wins over overlay
  label).
- Building, landuse, empty map (no overlay label hit).
- OSM `place=*` captions that are **not** drawn as Street Pixels overlay labels.
- Search / bookmark `BuildInfo` sources (not intercepted).

Empty-map taps no longer open the no-area sheet; SP-040 owns other empty-state
entry.

## Relation to polygon hit-test

Polygon PIP (`LookupExplorationAreaAtPoint`) remains for focus engine inputs
(pan centre, GPS, recording follow) and `HasExplorationAreaAtPoint` tests.
Explicit tap selection uses **overlay label → compact index**, not tap-point
PIP. Aligns with spec §12.1 (labels not on every neighbourhood) and SP-037
affordance.

## Tests

`ClassifyMapTap_*`, `ShouldOpenExplorationDetail_*`, `HitExplorationAreaLabel_*`
in `exploration_area_tap_tests.cpp`. Branch
`cursor/area-overlay-label-taps-3365`: `street_pixels_areas_tests` **105/105**;
`street_pixels_tests --filter=Focus` **18/18** (2026-08-15).

Device tap confirmation → Phase 10 (SP-041 R1).
