# SP-037 — Exploration area overlay geometry

Display contract for `BuildAreaOverlayGeometry` after Helsinki/Espoo device
review (2026-08-13). Assignment is unchanged (SPD-023 Finland seed still
**admin_10, then 9, then 11**; settlement **admin_8**).

## What the overlay draws

Pixels tile; stored OSM rings nest. The overlay is **not** a Voronoi or pixel
hull. It draws true `.spa` rings:

1. Compact indices that appear in the assign column (subdivision / place
   winners).
2. If the column has any sentinel slot, every **settlement** ring, clipped by
   those winners (SPD-007 display of “next greater” land that has no
   neighbourhood assignment).
3. Geometry-only sidecars (empty assign column) emit nothing.

Clip order is `SubdivisionPriorityRank` (same as §8.8), then smallest
`m_area`, then OSM id. Each later polygon is `difference`’d against earlier
**original** rings. Hole rings are stroked. Fill triangulates a keyhole of the
clipped polygon; failure skips fill (outline stays). Do **not** fall back to
the unclipped outer.

`SimplifyRingForOverlay` (index stride to 48/96 verts) is **not** applied
after clip. Post-clip stride reopened triangular gaps and crossing edges.
City-scale LOD remains a residual if vertex load needs it; any future
simplify must preserve shared clipped edges.

## Explicitly not done

- **Voronoi / convex hull tiling** — would invent boundaries (§3.5 / §8.3 /
  SPD-004). Gaps that OSM does not cover stay empty except settlement
  leftover inside a municipal ring.
- **Finland `[11, 10, 9]`** — Helsinki osa-alueet (e.g. Maunula admin_11)
  still lose to the parent kaupunginosa (e.g. Oulunkylä admin_10). Product
  kept that grain for now.
- Municipal leftover can still include **water** that sits inside the
  settlement OSM ring but has no street-pixel assignment.

## Tests

`AreaOverlay_ClipNestedWinners`, `AreaOverlay_ClipPrefersConfiguredPriorityOverArea`,
`AreaOverlay_SettlementFallbackForSentinel`, plus existing winner-only cases.
`street_pixels_areas_tests` 87/87 (2026-08-13, `omim-build-debug`).
