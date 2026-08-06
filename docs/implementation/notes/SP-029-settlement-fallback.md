# SP-029 — Settlement fallback layering

Product layering on SP-028 subdivision-or-none (SPD-007 / SPD-025):

1. **SP-028 assignable** (subdivision or place-boundary) from the dense
   `assign` column → keep.
2. Else **PIP** true municipal `Settlement` rings from the same `.spa`
   sidecar → assign that settlement (smallest `m_area`, then lower OSM id).
3. Else **no-area** (`nullptr`). Exploration and routing continue.

Constraints frozen here:

- Client PIP is **settlements only** (small M). Dense assign stays
  subdivision-only (SPD-021 / SP-028).
- Containment uses sidecar true rings, **not** `CitiesBoundariesTable`
  three-box `HasPoint`.
- Fail closed: version / universe mismatch, unknown HEALPix id, or OOB slot →
  no area (do not invent a settlement from a sample centre alone). Valid
  sentinel slots still run settlement PIP.
- No grids, no place-node invented polygons.

API: `SelectSettlementContaining`, `LookupExplorationArea` (slot / healpix +
sample centre), `ExplorationAreaResolver::TryLoad` (same gates as
`SubdivisionAssignmentTable`).
