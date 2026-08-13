# SP-037 — Area boundary rendering and completion shading by zoom

**Phase:** 5 — Area progress and map interaction
**Status:** Accepted 2026-08-07
**Branch:** `cursor/sp-037-area-boundary-shading-191e`
**Depends on:** Phase 4 Accepted; **SP-033 outcome** (LOD / renderer strategy);
  SP-034 completion data for shading
**Notes:** SP-033 provisional: keep one-circle-per-cell; ring LOD via simplify.

---

## Objective

Render exploration-area boundaries and completion-based shading across street /
neighbourhood / city zoom levels per spec §12, informed by the SP-033
performance outcome (keep current pixel overlay, add LOD, or aggregate).

## Motivation

Users need to see area shape and relative completion while panning. Spec §12.2–
§12.3 allow fade/aggregate for readability. Blindly drawing every ring plus
every circle may fail Spike 1; SP-033 decides.

## In-scope behavior

- Area boundary overlay from Phase 4 true polygons (`.spa`).
- Completion shading by area using SP-034 percentages.
- Zoom-dependent LOD / fade / aggregate behaviour consistent with §12 and
  SP-033 recommendation.
- Preserve street-pixel red/green semantics; do not replace them with shading
  alone at street zoom unless measurement says otherwise and spec allows.

## Out-of-scope behavior

- Tap hit-testing (SP-038).
- Completed-only distinct chrome beyond shading (SP-040 may add outline/check).
- Country/world choropleth (forbidden).
- Ignoring SP-033 and shipping unbounded full-detail rings at city scale.

## Relevant product requirements

- Spec §12.1–§12.3; §18.6 overlap noted but owned with SP-040.
- Audit Spike 1 / SP-033 pass criteria.

## Relevant source files or symbols

- `libs/drape_frontend/street_pixel_renderer.*`
- Phase 4 sidecar geometry APIs
- Existing drape overlay / shape machinery (reuse vs new layer — decide in plan)

## Implementation notes / constraints

- **SP-033 gate:** coding starts only after measurement recorded; LOD design
  cites those numbers.
- Offline geometry only.
- Prefer additive drape layer over restructuring unrelated map engines.

## Acceptance criteria

1. Boundaries visible at appropriate zooms; completion shading reads correctly
   for fixture areas.
2. Strategy matches SP-033 recommendation (or documents deliberate deviation
   with maintainer approval).
3. No country/world percentage choropleth.
4. Automated or golden regression where practical (`drape_frontend_tests`).

## Required automated tests

- Renderer / overlay regression as applicable; geometry fixture smoke.

## Required manual validation

- Zoom street → neighbourhood → city; confirm boundaries and shading; spot FPS
  if device available.

## Failure and rollback considerations

- If FPS regresses below Spike 1 after shading, residual LOD work or Phase 10
  device re-measure — do not ship knowingly broken city-scale performance.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-037-area-boundary-shading-191e` |
| SP-033 outcome cited | Provisional keep one-circle-per-cell (Accepted 2026-08-07); quantitative Spike 1 → Phase 10 |
| LOD / strategy summary | Pixel circles unchanged. Additive `ExplorationAreaOverlayRenderer`: outlines all visible zooms ≥9; fills at neighbourhood/city (≤15); street outline-only. Ring simplify for city/neighbourhood vertex caps. |
| Test output | `street_pixels_areas_tests` 62/62 (5 AreaOverlay_*); `street_pixels_tests` 199/199 |
| Manual validation | Device zoom walk / FPS spot-check residual → SP-041 / Phase 10 |
| Accepted by | Maintainer (accept 2026-08-07) |
| Accepted date | 2026-08-07 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Quantitative Spike 1 still Phase 10; shading may need re-measure | SP-041 / Phase 10 residual |
| City fill uses neighbourhood-baked alpha (not per-band rebuild) | Acceptable stub; SP-039 may retune |
| Completed-area chrome beyond shading | SP-040 |
| Tap hit-test on polygons | SP-038 |
| Initialized previously-null `m_drapeApiBuilder` in BackendRenderer | Latent fix bundled with overlay builder ctor |
| Post-clip 96-vert index stride left holes/overlaps | Fixed: clip in §8.8 order, no stride after clip; see `notes/SP-037-overlay-geometry.md` |
| Sentinel pixels had no overlay (outside neighbourhoods / OSM gaps) | Fixed: settlement leftover when assign column has sentinels (SPD-007) |
| Voronoi / hull tiling of pixels | Rejected — would invent area geometry (SPD-004) |
| Finland admin_11 (e.g. Maunula) swallowed by admin_10 (Oulunkylä) | Deferred; SPD-023 10→9→11 kept |
| Settlement leftover can include water inside the municipal OSM ring | Residual; no Voronoi/pixel hull |
| `SimplifyRingForOverlay` unused on clipped rings | Residual city LOD if vertex load needs a topology-preserving simplify |
