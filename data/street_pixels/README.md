# Street Pixels country policy data

Versioned exploration-area policy for the administrative-area pipeline
(SPD-023 / SP-025).

## Files

| File | Role |
| --- | --- |
| `country_policies.json` | Country exploration-area policy keyed by ISO 3166-1 alpha-2 |

## Schema overview

Top-level fields:

- `policy_version` (integer, required) — monotonic policy revision. Assignment
  determinism keys on `(map-data version, policy_version)`.
- `schema_version` (integer, required) — JSON shape version understood by the
  loader. Bump only when the document structure changes in a way that requires
  loader changes.
- `countries` (object, required) — map of ISO 3166-1 alpha-2 → country entry.

Per-country fields:

- `mwm_root_ids` (string array) — CoMaps MWM root ids that map to this country
  (exact match, or any leaf whose id starts with `<root>_`).
- `subdivision_admin_levels` (integer array) — OSM `admin_level` values for
  neighbourhood-scale areas, in priority order (first = highest priority).
- `settlement_admin_levels` (integer array) — OSM `admin_level` values used as
  settlement fallback (SPD-007 / SPD-025).
- `place_boundaries` (object, optional) — sparse `place=*` polygon supplement:
  - `enabled` (bool)
  - `place_types` (string array), e.g. `neighbourhood`, `quarter`, `suburb`

Unknown keys are ignored. There are **no** numeric suitability / privacy floor
fields (SPD-024). Do not add `min_pixels`, `min_area_m2`, or similar — the
loader will ignore them and must never apply them.

## PR review rules

1. Treat this directory as product-facing data. Every change lands through
   normal PR review (same bar as map policy, not a silent code tweak).
2. Expanding coverage is incremental data work: add or edit one country entry
   at a time with a short rationale (source of admin-level meaning, spike
   measurements if any).
3. No city allowlists, pilot-only lists, or invented grids / place-node
   polygons (SPD-004, product §8.3).

## When to bump `policy_version`

Bump `policy_version` (monotonically) whenever the **effective policy** for any
country changes in a way that can reassign pixels, including:

- Adding, removing, or reordering `subdivision_admin_levels` or
  `settlement_admin_levels`
- Enabling / disabling `place_boundaries` or changing `place_types`
- Changing `mwm_root_ids` so different MWMs resolve to a different policy

Do **not** bump solely for comments, README edits, or ignored unknown keys.

A `policy_version` bump can reassign areas without a map download; rematch is
SP-030.

## When to bump `schema_version`

Bump `schema_version` only when the JSON shape changes such that older loaders
cannot safely parse the file (renamed/required fields, structural changes).
Loader and schema must land together.

## Finland seed (SPD-023)

| Ring | Levels / types | Priority |
| --- | --- | --- |
| Subdivision | admin_10, then admin_9, then admin_11 | Highest first |
| Settlement | admin_8 | Fallback |
| Place boundaries | neighbourhood, quarter, suburb | Sparse supplement only |

Note: product spec §8.3 suggests a global preference of admin_11 then
admin_10. Finland follows SPD-023 measurements (10 → 9 → 11), which override
that global hint for this country.

## Unknown / unconfigured countries

ISO codes and MWM ids with no entry resolve to an unconfigured policy
(`configured=false`, empty level lists). Downstream assignment must fall
through settlement / no-area (SPD-007) and must never invent grids.
