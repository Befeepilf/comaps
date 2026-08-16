# SP-034 — Area-scoped completion computation and cache

**Phase:** 5 — Area progress and map interaction
**Status:** Accepted 2026-08-07
**Branch:** `cursor/sp-033-034-area-completion-191e`
**Depends on:** Phase 4 Accepted; SP-033 measurement recorded (qualitative Pixel 3a
  OK; quantitative Spike 1 → Phase 10)
**Unblocks:** SP-035–041 progress UI and aggregation

---

## Objective

Compute and cache personal area completion as explored / total valid street
pixels in the area (live + imported), invalidated on collect, import, rematch,
and policy change. Lock the completion formula for Phase 5 (OQ-1 slice) without
inventing a contested Accepted SPD.

## Motivation

Phase 4 assignment gives per-pixel area membership. Everyday progress needs
fast, correct area-scoped percentages. Spec §7 formula markup is blank (OQ-1);
surrounding text intent is clear. MWM-scoped `GetTotalExploredFraction` is not
sufficient.

## In-scope behavior

- Area-scoped explored count and total valid street-pixel count per exploration
  area id (Phase 4 assignment).
- Percentage = explored / total for that area; live and imported both count
  (spec §7 surrounding text). Zero-total areas defined safely (no divide-by-zero
  UI nonsense).
- Cache (or equivalent derived store) suitable for badge/detail reads without
  full-country recount per frame.
- Invalidate on: pixel collection, GPX import, map-update rematch, policy /
  country-config change (and assignment rematerialize).
- **Formula lock:** either a provisional SPD documenting the intent formula, or
  an explicit deferral of formal SPD until maintainer confirms — recorded in
  this work item / `DECISIONS.md`. Do **not** invent a contested formula as
  Accepted SPD.
- Depends on Phase 4 assignment (`ExplorationAreaResolver` /
  `SparseAssignmentStore`); do not reimplement assignment.

## Out-of-scope behavior

- Badge UI binding (SP-035).
- Focus engine (SP-036).
- City aggregation UI (SP-039) — may expose APIs city rollup can consume.
- Competition scoring / ownership formulas (Phase 8 slice of OQ-1).
- Starting before SP-033 measurement is recorded.

## Relevant product requirements

- Spec §7 Area completion; §15.4; offline-first.
- OQ-1 (completion formula blank); DECISIONS §15.
- Phase 4 SPD-021/022 assignment substrate.

## Relevant source files or symbols

- `StreetPixelsManager::GetTotalExploredFraction` (MWM-scoped baseline)
- `ExplorationAreaResolver`, `SparseAssignmentStore`
- `street_stats` / `.pix` explored bits
- Possible new cache beside `street_stats.db` or sparse assignment

## Implementation notes / constraints

- Gate: **Do not start SP-034+ coding until SP-033 measurement is recorded**
  (mirror Phase 4 SP-023/024).
- Imported exploration affects personal completion; never competition (invariant).
- Prefer shared C++ core so iOS can adopt later (SPD-002); Android V1 UI comes
  later items.

## Acceptance criteria

1. Fixture areas with known totals produce correct percentages (including 0% and
   100%; zero-total handled).
2. Cache invalidates on collect, import, rematch, and policy change — tested.
3. OQ-1 completion slice disposition recorded (provisional SPD **or** explicit
   deferral awaiting maintainer) — not a silent invented Accepted formula.
4. No MWM country id used as area identity for completion rows.

## Required automated tests

- Completion arithmetic fixtures (known totals; zero; full).
- Invalidation triggers (four cases above).

## Required manual validation

- Spot-check one known area percentage against a hand count or fixture dump on
  desktop or device.

## Failure and rollback considerations

- Stale cache producing wrong % is a release blocker; prefer fail-closed
  recompute over serving stale.
- Do not weaken formula tests to match an undecided contested variant.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-033-034-area-completion-191e` |
| Formula disposition (provisional SPD id / deferral note) | **SPD-026** — personal explored/total; live+imported; zero-total → 0; OQ-1 ownership/contested remain open |
| Cache location / API | `libs/street_pixels_areas/area_completion_cache.*`; `StreetPixelsManager::GetAreaCompletion` / `RebuildAreaCompletionCache` / invalidate on collect·import·rematch·policy |
| Test output | `street_pixels_areas_tests` 50/50 (4 AreaCompletion_*); `street_pixels_tests` all green incl. 5 AreaCompletionManager_* |
| Accepted by | Maintainer (accept 2026-08-07) |
| Accepted date | 2026-08-07 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Rebuild scans full `.pix` universe (not incremental on collect) | Acceptable for V1; optimize later if city-scale rebuild is slow |
| `AreaCompletionCache::Build` on the GUI thread ANRs at city scale | Helsinki sidecar ~6.07M assignments: pix ~200 ms + spa ~280 ms + **Build ~5–6 s**. Two pan-end rebuilds on main exceeded the 5 s input timeout (Pixel 10a, 2026-08-17). Do **not** call `RebuildAreaCompletionCache` from `RefreshFocusFromViewport`, `SelectStreetPixelsFocusAt`, or `TryHandleExplorationAreaTap`. Country load already builds on background; notify focused-area progress when that rebuild finishes. After the cache exists, pan-end PIP is 2–6 ms |
| Quantitative Spike 1 still Phase 10 | SP-033 residual; does not block this item |
