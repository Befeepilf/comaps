# Phase 3 — Exploration storage and map-update reconciliation

**Status:** Not started
**Depends on:** Phase 2
**Blocks:** Phases 4, 6, 8, 9

---

## Objective

Make personal exploration permanent and make its provenance explicit. Explored
pixels must survive a map-data update, and every explored pixel must record
whether it was collected live or imported, because almost every later phase
branches on that distinction.

## Product-spec references

- §3.6 Permanent personal progress.
- §7 Street pixel, explored pixel, live exploration pixel, pixel recency.
- §13.1–§13.4 Eligible and excluded routes; versioned map-data policy.
- §14.1–§14.3 Fixed HEALPix grid, ~10 metre path sampling, determinism.
- §15.2–§15.3 Permanent exploration state; imported-pixel behaviour.
- §27.1–§27.5 Map-data updates, recalculation, communicating reductions,
  previous completion, competition versioning.
- §34 "Core map and exploration" and "Offline and map updates".

## Technical-audit references

- §6 Map-data and street-pixel generation; the spec-eligibility-versus-code
  table; map version and history survival.
- §11 Local storage and data model; the preliminary entity model; storage
  growth estimates; concurrency notes.
- §14 Map updates and migrations; the recommended migration strategy.
- §22 Risk register: "Map update wipes progress".
- Spikes 2, 8, 9.

## Current code locations

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Pixel file | `{countryId}.pix` | Array of `int64_t` HEALPix identifiers; most significant bit is the explored flag; mmapped and read as `df::StreetPixel` |
| Bit accessors | `libs/drape_frontend/street_pixel.cpp` | `GetPixelId` masks `0x7FFF...`; `IsExplored` tests `0x8000...` |
| Accounting file | `{countryId}.pixa` | Bitmap, one bit per index in the `.pix` file, marking pixels already counted in statistics |
| Third extension | `.pixf` | Deleted by `CleanupStreetPixels` but **no writer exists in the tree**. Dead extension; confirm before relying on it. |
| Statistics DB | `libs/map/street_stats_db.cpp` | Tables `mwms(mwm_id, mwm_name)`, `street_exploration(mwm_id, feature_index, pixel_bitmask)`, `processed_tracks(geometry_hash, country_id)` |
| Map-data version | — | **Not stored** with pixels anywhere |
| Source flag | — | **Not stored.** Live GPS and GPX track replay set the same explored bit |
| Per-pixel timestamps | — | Not stored. `ExplorationDelta::m_eventTimeSec` exists only transiently for statistics aggregation |
| Wipe on update | `libs/map/framework.cpp` `OnCountryFileDownloaded` → `StreetPixelsManager::CleanupStreetPixels` | Deletes `.pix`, `.pixa`, `.pixf` and calls `StreetStatsDB::DeleteMwmData`. Also invoked from `OnCountryFileDelete` and `OnMapDeregistered`. |
| Derivation | `StreetPixelsManager::DeriveStreetPixelsFromFeatures`, `SegmentizeStreet` | Samples at `kSegmentLengthMeters = 15.0` metres. Spec requires ~10. |
| Track replay sampling | `StreetPixelsManager::UpdateStreetStatsForTrack` | Uses a separate hardcoded `10.0` metres |
| Eligibility | `StreetPixelsManager::IsExplorable` | Line geometry; classificator path begins with `highway`; excludes the `driveway` and `tunnel` third-level subtypes; excludes `hwtag=private`; requires bicycle **or** pedestrian access |
| Concurrency | `StreetStatsDB` recursive mutex; `.pix` shared mutex plus `msync` | Renderer reads spans concurrently |

**Differences from the technical audit:** none material. Two details the audit
did not surface: the `.pixf` extension is deleted but never written, and
derivation and track replay use two different hardcoded sampling distances.

## Intended outcome

- A pixel store that records, per explored pixel, whether the first exploration
  was live or imported, and that carries a map-data version.
- Map updates that rematch explored HEALPix identifiers against the newly
  derived pixel set instead of deleting them.
- Derivation sampling aligned with the spec, with one sampling constant rather
  than two.
- Eligibility tightened toward spec §13, or the divergence explicitly recorded.
- A migration that is safe to interrupt.

## Dependencies

- Phase 2. The source flag is only meaningful once "live" means "collected in a
  validated session".

## Proposed work-item breakdown

Not yet decomposed into work items. Likely shape, to be confirmed by a
Plan Mode investigation at phase entry:

1. Pixel-file format version and map-data version stamp.
2. Per-pixel source flag (`live`, `imported`, `both`) and where it lives — a
   second bit in the `.pix` entry, or a side table keyed by HEALPix identifier.
3. Replace wipe-on-update with rematch, including crash safety and a background
   progress state.
4. Align derivation sampling to the spec and unify the two sampling constants.
5. Tighten `IsExplorable` against spec §13, or record each divergence.
6. Recalculation of denominators after an update, with the spec's "there is
   more to explore" framing.

**Marked for later phase-specific Plan Mode investigation.** Source inspection
is not sufficient to choose between an in-`.pix` bit and a side table. That
choice depends on measured `.pix` growth, on `msync` cost, and on how the
renderer consumes spans, none of which can be settled by reading code.

## Data and migration concerns

This phase *is* the data and migration concern.

- The `.pix` entry currently spends 63 bits on a HEALPix identifier and 1 bit on
  explored state. At `nside = 1048576` the identifier needs about 42 bits, so
  spare bits exist — but changing the layout changes every file on every
  device. A format version must be added before, or in the same change as, the
  layout change.
- Rematch must not destroy the old explored set before the new one is durable.
  The audit's recommended sequence — keep the old explored identifiers aside,
  derive the new valid set, intersect, then commit — is sound. Add an explicit
  recovery path for a crash mid-migration.
- Changing derivation sampling from 15 m to 10 m changes the pixel universe and
  therefore every denominator. Do it in the same release as the rematch
  machinery, so the change is absorbed by the same "the map changed" user
  communication.
- `CleanupStreetPixels` has three callers. Only the download path becomes a
  rematch. Deleting a country's map and deregistering a map are legitimate
  reasons to drop derived data — but confirm whether the user's *explored*
  state should survive a map deletion and later redownload. The spec's
  permanence promise suggests it should.
- Growth: at roughly 8 bytes per pixel, a large city is a few megabytes. Adding
  per-pixel timestamps for every cell would change that materially. Store
  sparse live recency only where ownership needs it.

## Privacy and security implications

- Per-pixel data becomes richer in this phase. Everything added here stays on
  the device; nothing in Phase 3 creates an upload path.
- A per-pixel "first explored" timestamp is a location history. If added, it
  must be justified by a specific product requirement (spec §18.5 permits
  storing the original 100% completion date locally) and must never reach an
  upload payload or analytics.
- The `imported` flag is a privacy-relevant control, not only a scoring
  control: it is what keeps GPX history out of competition.

## Automated testing strategy

- Rematch, using synthetic old and new pixel sets: unchanged cells stay
  explored; removed cells disappear; added cells appear unexplored; the
  explored count is exactly the intersection size.
- Rematch interrupted at each stage, asserting no state is lost.
- Source-flag persistence across a save and reload cycle, and across a rematch.
- Format-version handling: an old-format file is read or migrated, not
  misinterpreted.
- Determinism: deriving twice from the same fixture yields byte-identical
  output.
- Sampling change: a fixture geometry produces the expected pixel count at the
  new sampling distance.
- Eligibility: a fixture set of features covering each spec §13.1 inclusion and
  §13.2 exclusion.

## Manual validation strategy

- Explore a known area, trigger a real country map update, and confirm the
  explored pixels are still green afterwards and that the percentage change is
  explainable by the streets that actually changed.
- Kill the app during migration and reopen; confirm no exploration is lost.
- Update a country while the map is being viewed; confirm the UI stays usable
  and shows an updating state.
- Delete a country and redownload it; confirm behaviour matches whatever the
  team decides for that case, and that the decision is recorded.

## Entry criteria

- Phase 2 exit criteria met.
- A Plan Mode investigation has produced a recommendation on flag storage
  layout, backed by measurement.

## Exit criteria

1. Every explored pixel records whether its first exploration was live or
   imported.
2. Pixel files carry a format version and the map-data version they were
   derived from.
3. A country map update rematches explored identifiers; no explored state is
   lost for cells that still exist.
4. Migration is crash-safe, verified by an interrupted-migration test.
5. Derivation sampling matches the spec and there is one sampling constant.
6. Eligibility either matches spec §13 or every divergence is recorded with a
   reason.
7. Denominators recalculate after an update and the reduction message follows
   spec §27.3.
8. Determinism is proven by a repeat-derivation test.

## Explicit non-goals

- Area assignment. Phase 4.
- Competitive recency and ownership scoring. Phase 8.
- Generator-side pixel precomputation. Explicitly deferred by the audit's
  recommendation to keep on-device derivation for V1.
- Changing the HEALPix `nside`. See OQ-8; changing it would redefine every
  pixel identifier and invalidate every stored file.
- Rendering changes.
- GPX import behaviour beyond setting the `imported` flag correctly when it
  reaches the store.

## Known uncertainties

- Whether the source flag belongs in the `.pix` entry or in a side table.
- Whether explored state should survive deleting and redownloading a country.
- Whether `.pixf` is genuinely dead, or written by code not found.
- Whether tightening `IsExplorable` is achievable purely client-side, or needs
  generator changes for tags that do not survive map generation. The audit
  flags bridge and tunnel handling as unresolved (OQ-5).
- How much of the spec §13 eligibility list is representable given what
  survives into MWM. `hwtag` values for foot, bicycle, and private are
  confirmed present; indoor, underground, proposed, construction, and
  emergency-only are not confirmed as filterable.
- Whether rematch can complete fast enough on a large country to avoid a
  visible stall on a mid-tier device.
