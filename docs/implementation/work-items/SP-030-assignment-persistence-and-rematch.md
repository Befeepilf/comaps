# SP-030 — Persist area assignments and rematch hooks

**Phase:** 4 — Administrative-area pipeline
**Status:** In review
**Branch:** `cursor/sp-030-assignment-persistence-191e`
**Depends on:** SP-024 Accepted (SPD-022 persistence), SP-028/029 (assignment
  results), Phase 3 rematch hooks (map-data version)

---

## Objective

Persist pixel→area assignments keyed by (map-data version, policy_version),
and reconcile them on map update / policy change without wiping exploration
(Phase 3 rematch integration).

V1 strategy (SPD-022): **sparse** explored HEALPix→compact area index on
device; **rematerialize** from the dense uint16/uint32 sidecar map; **no**
full-universe uint64 OSM id table.

## Motivation

Assignments are new durable state. Spec §8.8 requires every **valid street
pixel** to have a deterministic assignment (or none). Map updates and config
changes can reassign pixels; percentages may move. Spec §27.4 allows keeping
prior completion dates locally when an area disappears.

## In-scope behavior

- On-disk sparse explored map + rematerialize from sidecar dense compact index
  (SPD-022).
- Size-conscious for Uusimaa-scale universes (~6.5×10⁶ cells); document
  measured or budgeted size against SP-023 estimates (uint16/uint32 sidecar
  map; sparse explored local).
- Rebuild/rematch on map-data version change and policy_version change by
  applying the new precomputed blob (SPD-021) — not full-universe client PIP.
- Define behaviour when an area id disappears (keep local completion date if
  stored; no invented replacement area / no grid).
- Hook messages if percentages change due to reassignment (may reuse SP-021
  framing patterns; no Phase 5 UI required).
- Tests for rematch of assignments across synthetic version bumps.

## Out-of-scope behavior

- Area progress UI (Phase 5).
- Competition upload of area aggregates (Phase 8).
- Changing Phase 3 rematch of explored bits (integrate with it).
- Full-universe uint64 OSM id persistence (rejected by SPD-022).

## Relevant product requirements

- §8.8, §27.4; Phase 3 rematch invariants; offline-first; SPD-022.

## Acceptance criteria

1. Assignments survive process restart per SPD-022 (sparse + rematerialize).
2. Map-data rematch refreshes assignments for surviving pixels; exploration
   bits are not wiped.
3. Missing area after update does not invent a grid replacement.
4. Size strategy documented against SP-023 estimates / SPD-022.
5. Policy-version bump triggers reassignment without requiring a map download
   if config shipped independently.

## Required automated tests

- Persist/reload round-trip.
- Version-bump reassignment fixture (map-data and policy).

## Failure and rollback considerations

- Corrupt assignment store: rebuild from sidecar dense map / policy rather than
  wiping `.pix` exploration.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-030-assignment-persistence-191e` |
| Commits | `666018c0e` sparse store+tests; `ea8019044` manager hooks+tests; `f0abc0abf` docs |
| Format | sparse explored + dense uint16/uint32 sidecar rematerialize (SPD-022); `.spx` magic SPX1 |
| Decision ids (SP-024) | SPD-022 (primary); SPD-021 |
| Size note | See `notes/SP-030-sparse-assignment.md` — sparse 1 % @ uint16 ≈ 0.65 MiB vs SP-023 ~0.78 MiB / full uint32 ~26 MiB |
| API symbols | `SparseAssignmentStore`, `EnsureSparseAssignmentStore`, `TryLoadSparseAssignmentStore`, `ScanUniverseAscending`, `RematerializeAssignmentsOnPolicyBump`, `TakePendingAssignmentRematch` |
| Test output | `./tools/unix/build_omim.sh -d -p /workspace street_pixels_areas_tests` OK; `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "Sparse\|Spx\|AssignmentPersist\|Rematerialize"` → areas 7/7 OK + manager 3/3 OK. Full `./street_pixels_areas_tests` → **All tests passed** (43). `street_pixels_tests --filter=Rematch` → All tests passed. Full `./street_pixels_tests` → 184/185 OK; 1 pre-existing env fail `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` (missing `./data/sp010_gpstrack_test.bin`, unrelated). |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Manager refresh uses HEALPix cell centres as settlement sample centres | Acceptable for rematch; live collection should pass true sample centres when productized |
| Completion-date retention for disappeared areas (§27.4) | No completion-date store yet (Phase 5); missing compact index → none, no grid |
| Percentage-change messaging for area rematch | Pending `AssignmentRematchSignal` only; UI reuse of SP-021 toast deferred |
| | |
