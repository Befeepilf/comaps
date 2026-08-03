# Phase 3 — Exploration storage and map-update reconciliation

**Status:** In progress (phase-entry investigation complete; work items Planned)
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

Re-verified 2026-08-03 against the working tree (supersedes 2026-07-25 notes
where they differ).

| Concern | Location | Observed state |
| --- | --- | --- |
| Pixel file | `{countryId}.pix` | Headerless array of `int64_t`; MSB = explored; mmapped as `df::StreetPixel` |
| Bit accessors | `libs/drape_frontend/street_pixel.cpp` | `GetPixelId` masks `0x7FFF...`; `IsExplored` tests `0x8000...` |
| HEALPix id width | `nside = 1048576` NEST | Id range needs **44 bits** (`12 * nside² − 1`); **19 spare bits** remain in the 63-bit payload. Earlier phase text said ~42 bits — corrected. |
| Accounting file | `{countryId}.pixa` | Bitmap, one bit per **index** in `.pix` (not HEALPix id) |
| Third extension | `.pixf` | Deleted by `CleanupStreetPixels`; **no reader or writer in tree**. Confirmed dead. |
| Statistics DB | `libs/map/street_stats_db.cpp` | `mwms`, `street_exploration`, `processed_tracks`. Country completion fraction comes from `.pix` counts, not SQLite. |
| Map-data version | — | **Not stored** |
| Source / ever-live | — | **Not stored.** Live `OnLocationUpdate` and bookmark-track replay both only set the explored bit |
| Wipe on update | `Framework::OnCountryFileDownloaded` → `CleanupStreetPixels` | Deletes `.pix`/`.pixa`/`.pixf` and `street_exploration` rows. **Does not clear `processed_tracks`**, so saved-track replay usually cannot rebuild wiped exploration. |
| Derivation sampling | `kSegmentLengthMeters = 15.0` | Spec ~10 m; **SPD-019 locks V1 at 15 m** |
| Live / track sampling | `kInterpolationStepMeters = 10.0` via `ComputeTrackPixels` | Legacy `UpdateStreetStatsForTrack` still hardcodes `10.0`; **SP-019 aligns both to 15 m** |
| Eligibility | `IsExplorable` / `IsExplorableFeature` | highway lines; excludes driveway/tunnel/no-access/`hwtag=private`/construction/elevator/raceway; motorway needs `hwtag-yesbicycle`; bridges include. Remaining §13 gaps: [SP-020 divergence register](../work-items/SP-020-eligibility-policy-alignment.md#divergence-register-spec-13-vs-client) |
| Renderer | `StreetPixelRenderer` | Consumes `span<StreetPixel>`; uses `GetPixelId`, `IsExplored`, `GetPoint`, `GetColor` only — no provenance |

**Differences from the technical audit:** material additions above — 44-bit id
width, dead `.pixf`, and `processed_tracks` surviving wipe.

## Intended outcome

- A pixel store that records, per explored pixel, whether it is ever-live or
  imported-only (one bit in `.pix` — SPD-015), and that carries a map-data
  version header.
- Map updates that rematch explored HEALPix identifiers against the newly
  derived pixel set instead of deleting them, without duplicating ~50 MB
  regional files longer than an atomic replace requires.
- Map delete frees the full `.pix` while retaining a compact explored-only
  archive for redownload rematch (SPD-016).
- Derivation / live / track path sampling unified at **15 m** (SPD-019), with one
  sampling constant rather than dual 15/10 values. Spec §14 ~10 m remains
  product intent; V1 deliberately does not densify the universe.
- Eligibility tightened toward spec §13, or the divergence explicitly recorded.
- A migration that is safe to interrupt.
- `nside` unchanged (SPD-017).

## Dependencies

- Phase 2. The source flag is only meaningful once "live" means "collected in a
  validated session".
- Entry criterion met: storage-layout recommendation revised and accepted after
  Uusimaa ~50 MB measurement (SPD-015–018).

## Phase-entry investigation (2026-08-03, revised same day)

### Measured storage baseline

| Region | `.pix` size | Approx. cells @ 8 B |
| --- | --- | --- |
| Uusimaa (maintainer device) | **~50 MB** | **~6.5×10⁶** |

Audit planning tables (~0.5–2.7 MB city / ~8 MB mega) **underestimated**
regional scale. All Phase 3 designs must treat tens-of-megabytes `.pix` files
as normal: no duplicate full-universe copies left on disk, no SQLite row per
cell, rematch must stream / use explored-only working sets, and mmap peak
memory is already ~size of `.pix`.

### Source-flag storage decision (revised)

**Accepted (SPD-015): one ever-live bit in `.pix` — not a side table, not a
two-bit `both` enum.**

| Option | Storage at Uusimaa scale | Verdict |
| --- | --- | --- |
| One ever-live bit inside each existing `int64_t` | **+0 bytes** on disk and in mmap | **Chosen** |
| Two-bit `live` / `imported` / `both` in-entry | Also +0 bytes | Unnecessary — “both” collapses to ever-live for V1 consumers |
| Sparse SQLite `(countryId, healpixId, source)` | ~5–28 MB raw at 10–50 % explored; **≫100 MB** plausible with row/index overhead | Rejected after measurement |

Draft phase-entry text preferred a side table; a later draft used two source
bits. Both are **superseded** by SPD-015 (one ever-live bit) after the Uusimaa
measurement and maintainer simplification. Rematch keys by HEALPix id: scan
the old `.pix` for explored entries, copy id + explored + ever-live into the
new universe. `.pixa` remains index-coupled accounting only.

**Bit layout (SP-016 to implement):** bit 63 = explored (unchanged); one further
high bit = ever-live (`0` = imported-only when explored, `1` = live-eligible,
including imported-then-later-live); low 44 bits = HEALPix id.
`GetPixelId()` must mask flag bits so renderer density bucketing cannot see
provenance. Live sets the bit; import must not clear it.

**Phase 8 recency:** still sparse and out of `.pix` (timestamps would blow the
50 MB class of files). Not Phase 3.

### Other accepted decisions

1. **SPD-016** — Explored + ever-live survive map delete/redownload via a
   **compact explored-only archive**, not retention of the full `.pix`.
2. **SPD-017** — `nside = 1048576` locked for V1 (OQ-8 closed).
3. **SPD-018** — `.pixf` is dead.

### Work-item breakdown

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-015](../work-items/SP-015-pixel-file-format-and-map-version.md) | Pixel-file format version and map-data version header |
| 2 | [SP-016](../work-items/SP-016-exploration-source-flag-store.md) | Per-pixel ever-live bit in `.pix` |
| 3 | [SP-017](../work-items/SP-017-crash-safe-map-update-rematch.md) | Crash-safe rematch on map update |
| 4 | [SP-018](../work-items/SP-018-exploration-survives-map-delete.md) | Explored state survives map delete and redownload |
| 5 | [SP-019](../work-items/SP-019-derivation-sampling-alignment.md) | Unify path sampling at 15 m (SPD-019) |
| 6 | [SP-020](../work-items/SP-020-eligibility-policy-alignment.md) | Eligibility vs spec §13 — tighten or record |
| 7 | [SP-021](../work-items/SP-021-denominator-recalc-and-update-messaging.md) | Denominator recalculation and §27.3 messaging |
| 8 | [SP-022](../work-items/SP-022-exploration-storage-end-to-end-validation.md) | Phase 3 end-to-end validation |

SP-019 unifies live/track sampling to the existing 15 m derivation step
(SPD-019). It does **not** densify `.pix` (rejects the earlier 15→10 m plan).
Rematch (SP-017) is already landed and is not required solely for this unify.

## Data and migration concerns

This phase *is* the data and migration concern.

- Regional `.pix` files are large (Uusimaa ~50 MB). Designs must not duplicate
  the full universe on disk during rematch longer than needed for atomic
  replace; prefer temp file + rename. Mid-rematch RAM should favour streaming
  scans and explored-only hash sets, not a second full 50 MB mirror plus a
  SQLite copy of every id.
- The `.pix` entry spends 1 bit on explored and 1 bit on ever-live (SPD-015).
  Ids need 44 bits at locked `nside`. Format version header is required; keep
  it small and fixed-size.
- Rematch must not destroy the old explored set before the new one is durable.
  Crash recovery required. Reconcile `processed_tracks` so wipe/rematch cannot
  strand exploration behind a geometry-hash ledger.
- Map delete retention (SPD-016 / SP-018) writes a compact explored-only
  archive (id + packed explored/ever-live flags), not a kept full `.pix`.
- Path sampling is unified at 15 m (SPD-019 / SP-019). Do **not** densify
  derivation to 10 m; that plan was rejected after the Uusimaa size measurement.
- `.pixa` is ~1/64 of `.pix` (~0.8 MB at Uusimaa scale) and stays index-coupled
  accounting only.
- Phase 8 must not put per-pixel timestamps into `.pix`.

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
- Source-flag / ever-live persistence across a save and reload cycle, and across a rematch.
- Format-version handling: an old-format file is read or migrated, not
  misinterpreted.
- Determinism: deriving twice from the same fixture yields byte-identical
  output.
- Sampling change: live/track fixtures produce the expected sample counts at
  the unified 15 m step; derive determinism remains at 15 m.
- Eligibility: a fixture set of features covering each spec §13.1 inclusion and
  §13.2 exclusion.

## Manual validation strategy

- Explore a known area, trigger a real country map update, and confirm the
  explored pixels are still green afterwards and that the percentage change is
  explainable by the streets that actually changed.
- Kill the app during migration and reopen; confirm no exploration is lost.
- Update a country while the map is being viewed; confirm the UI stays usable
  and shows an updating state.
- Delete a country and redownload it; confirm explored ∩ new universe and
  ever-live bits survive via the compact archive (SPD-016 / SP-018).

## Entry criteria

- Phase 2 exit criteria met.
- A Plan Mode investigation has produced a recommendation on flag storage
  layout, backed by measurement.

## Exit criteria

1. Every explored pixel records whether it is ever-live or imported-only.
2. Pixel files carry a format version and the map-data version they were
   derived from.
3. A country map update rematches explored identifiers; no explored state is
   lost for cells that still exist.
4. Migration is crash-safe, verified by an interrupted-migration test.
5. Path sampling is unified at 15 m (SPD-019) with one sampling constant
   (recorded V1 divergence from spec §14 ~10 m).
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
- Changing the HEALPix `nside`. **Locked** by SPD-017 (`nside = 1048576`).
- Rendering changes.
- GPX import behaviour beyond setting ever-live correctly when exploration
  reaches the store (clear on first import-only explore; never clear if set).

## Known uncertainties

- ~~Whether the source flag belongs in the `.pix` entry or in a side table.~~
  **SPD-015:** one ever-live bit in `.pix` (not a side table; not a `both`
  enum; revised after Uusimaa ~50 MB measurement).
- ~~Whether explored state should survive deleting and redownloading a country.~~
  **SPD-016:** yes, via compact explored-only archive (SP-018).
- ~~OQ-8 `nside`.~~ **SPD-017:** locked at 1048576 for V1.
- ~~Whether tightening `IsExplorable` is achievable purely client-side, or needs
  generator changes for tags that do not survive map generation. The audit
  flags bridge and tunnel handling as unresolved (OQ-5).~~ **SP-020 / OQ-5
  closed:** bridges include; tunnels exclude; motorway/motorway_link requires
  `hwtag-yesbicycle`. Client tightened construction/elevator/raceway/driveway/
  no-access/private; indoor, subway-passage, emergency-only, proposed remain
  generator gaps; parking_aisle/busway and trunk-without-yesbicycle are residual
  includes — see
  [SP-020 divergence register](../work-items/SP-020-eligibility-policy-alignment.md#divergence-register-spec-13-vs-client).
- How much of the spec §13 eligibility list is representable given what
  survives into MWM — answered for V1 in SP-020 (enforceable vs recorded).
- Whether rematch can complete fast enough on a large country (~50 MB `.pix`)
  without a visible stall or large RAM spike — measured in SP-017 / SP-022 on
  Uusimaa-class data.
- ~~How much `.pix` grows under 10 m derivation vs 15 m at regional scale.~~
  **SPD-019:** V1 stays at 15 m; densification rejected. Uusimaa baseline ~50 MB
  unchanged by SP-019 by design.
- How bookmark-track replay should set ever-live before Phase 9's dedicated GPX
  importer exists — decided inside SP-016 (must not clear ever-live for
  live-session pixels).
