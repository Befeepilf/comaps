# SP-082 — Competition isolation on historical import

**Phase:** 9 — GPX and feature gating
**Status:** Accepted
**Branch:** `cursor/sp-082-competition-isolation-db9d`
**Depends on:** SP-080 (G1; isolation remains a data rule). SP-081
  dedicated path. Phase 8 recency / weekly / upload stores when present
  (SP-072–074); if a store is missing, residual that assertion rather
  than inventing scoring.
**Unblocks:** SP-087 exit criteria 1–2

---

## Objective

Prove, against the dedicated historical-import API (not only
`MarkImportedPixelsForTesting`), that imports never create or refresh
recency, never contribute to weekly counts, never affect ownership or
eligibility, and never enqueue a competition upload — including when
Explorer Pro is available and entitled.

## Motivation

Phase 8 tests already cover imported-only helpers. Phase 9 exit requires
isolation on the real import path, proven by test, holding regardless of
flag or entitlement (spec §7, §22.2, §24.1, SPD-011). A future caller
must not be able to “import live” by toggling Pro.

## In-scope behavior

- Fixture: GPX or in-memory track geometry through the SP-081 API.
- Assert against stores, not the network:
  - `live_recency.db` has no new rows for imported ids
  - weekly new-live count +0
  - ownership score 0 at 100% personal completion if never live
  - `Explore.CompetitionUploadPending` not set by import
  - `BuildCompetitionUploadSnapshot` competitive fields empty
- Import then live: ever-live set, recency written, weekly +1 once,
  personal completion does not double-count.
- Live then import: recency unchanged.
- Gate matrix: isolation holds for all four available×entitled
  combinations (including both true).
- Reuse existing tests; add GPX/path fixtures where they are only
  helper-based today (`SP-007` deferred `UpdateExploredPixels` bookmark
  fixture lands here).

## Out-of-scope behavior

- Changing scoring formulas (Phase 8 / SPD-057–060).
- Android UI gating (SP-083).
- Malformed/oversized files (SP-085).
- Weakening Phase 8 tests to pass.

## Relevant product requirements

- Spec §7, §15.3, §16.1, §22.2, §24.1, §29.2, §34 Explorer Pro.
- SPD-011, SPD-015, SPD-057, SPD-060.
- Phase 9 automated strategy bullets 1–4.

## Relevant source files or symbols

- SP-081 historical-import API
- `LiveRecencyStore`, `WeeklyCityLiveStore`, `CompetitionUploadService`
- `QueryCompetitionOwnership`, `BuildCompetitionUploadSnapshot`
- `street_pixels_tests/competition_ownership_tests.cpp`,
  `weekly_city_live_tests.cpp`, `competition_upload_tests.cpp`,
  `ever_live_tests.cpp`, `first_goal_tests.cpp`,
  `competition_hint_tests.cpp`, `area_milestone_manager_tests.cpp`

## Implementation notes / constraints

- Isolation belongs in the data layer. Do not wrap recency writes in
  `IsCapabilityEnabled`.
- First-goal and 30-pixel hint must still not advance on import
  (existing tests; keep).
- Milestone fire on import without haptic is allowed (SPD-026 / SP-062);
  add an explicit not-recording → 0 haptic assertion if missing.

## Acceptance criteria

1. Dedicated import increases personal completion and paints green
   (explored).
2. No recency write, weekly +0, ownership 0, no upload pending, empty
   competitive snapshot.
3. Import-then-live and live-then-import behave as spec §15.2–§15.3.
4. All four gate combinations preserve 1–3.
5. Existing Phase 8 isolation tests remain green.

## Required automated tests

- GPX/geometry → import API → store assertions listed above.
- `ExplorerPro_*` availability/entitlement scopes around import.
- Regression: `CompetitionUpload_ImportedOnlyZeroCompetitiveHttp`,
  `WeeklyCityLive_ImportOnlyDoesNot`,
  `CompetitionOwnership_ImportDoesNotWriteRecency`.

## Required manual validation

- With competition enabled (SP-087): imported area produces no ownership
  change and no weekly movement. Device residual → Phase 10.

## Failure and rollback considerations

- If a test fails because Phase 8 stores are absent, residual the
  assertion; do not stub a fake competitive write to make the test pass.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-082-competition-isolation-db9d` |
| Test output | See executed output below |
| Accepted by | Product owner |
| Accepted date | 2026-08-28 |

## Executed test output

Cwd `/workspace`. Binary `/home/ubuntu/omim-build-debug/street_pixels_tests`. SHA `145cc7f65`. `--data_path=/workspace/data --user_resource_path=/workspace/data`.

- `--filter=IsolationHistoricalImport` **14/14** All tests passed
- Named Phase 8 regressions (`CompetitionUpload_ImportedOnlyZeroCompetitiveHttp`, `WeeklyCityLive_ImportOnlyDoesNot`, `CompetitionOwnership_ImportDoesNotWriteRecency`, `FirstGoal_ImportDoesNotAdvance`, `CompetitionHint_ImportDoesNotAdvance`, `ExplorerPro_`) **12/12** All tests passed
- Device residual → SP-087 / Phase 10

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Isolation already holds on `ImportHistoricalTrack`; this item is tests-only confirmation of SPD-011 | Recorded |
| Device check (imported area, no ownership/weekly movement) unrun | SP-087 / Phase 10 |
| Public GPX surfaces still ungated | SP-083 |
