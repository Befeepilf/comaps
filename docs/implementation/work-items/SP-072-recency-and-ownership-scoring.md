# SP-072 — Recency store, ownership, eligibility, contested, unclaimed

**Phase:** 8 — Competition
**Status:** Not started
**Branch:**
**Depends on:** SP-070 (SPD-057, SPD-058, SPD-063); Phase 3 ever-live bit;
  Phase 4 area OSM ids
**Unblocks:** SP-074 (aggregates), SP-078 (area snapshot chrome)

---

## Objective

Store sparse last-live-visit recency, compute ownership scores from
SPD-057, and evaluate boss eligibility, unclaimed, and contested
(SPD-058) locally. Imported pixels never contribute.

## Motivation

No recency or ownership code exists. `.pix` has an ever-live bit only
(SPD-015). Competition scoring must stay local, offline, and live-only.

## In-scope behavior

- Sparse HEALPix → last-live-visit map for ever-live cells only
  (SPD-063). Not in `.pix`. Not a full-universe timestamp table.
- On first competition opt-in, seed `last_live_visit = consent time` for
  currently ever-live pixels. After that, only validated live sessions
  update recency.
- Ownership score per area: SPD-057. \(T = 0\) → 0.
- Eligibility: spec §22.5 (2% live coverage, 50 unique live pixels waived
  if \(T < 50\), score ≥ 0.5).
- Unclaimed: no eligible participant, or previous boss decayed below the
  minimum, or all eligible participants left.
- Contested: SPD-058 (runner-up ≥ 80% of leader among eligible).
- Query API: for an area OSM id, return local score, live coverage %,
  eligible, and local ranking inputs needed by SP-074 / SP-078.
- Crash-safe writes.

## Out-of-scope behavior

- Weekly city counts (SP-073).
- Upload (SP-074).
- Server-side decay (SP-075).
- UI (SP-078).
- Boss haptic (SPD-054).

## Relevant product requirements

- Spec §22.1–§22.9, §15.2–§15.3.
- SPD-015, SPD-026 (personal % is a different number), SPD-057, SPD-058,
  SPD-063.

## Relevant source files or symbols

- `StreetPixelsManager`, `IsEverLive()`, live collection path
- `street_pixels::ExplorationArea::m_osmId`, assignment sidecar
- `area_milestones.db` pattern (SQLite WAL) as a storage precedent — do
  not mix recency into milestone tables

## Implementation notes / constraints

- Shared C++. Android does not reimplement the formula.
- Imported-only cells: no timestamp, no score contribution, several
  tests.
- Server snapshot scores are not required for local computation; UI may
  show local score offline with a stale-ranking label (SP-078).

## Acceptance criteria

1. Recency weight ≈ 1.0 immediately, 0.5 at 30 days, 0.25 at 60, 0.125
   at 90; revisit restores ≈ 1.0.
2. Ownership fixtures match SPD-057; imported pixels never affect score,
   eligibility, or contested.
3. Eligibility conditions fail independently; \(T < 50\) waives the
   50-pixel rule only.
4. Contested holds at 80% and fails below; unclaimed when no eligible
   boss.
5. Opt-in seed covers current ever-live cells once.

## Required automated tests

- Decay table at 0 / 30 / 60 / 90 days and revisit restore.
- Score = 100 on just-visited full live coverage; 0 on imported-only.
- Eligibility: each of the three conditions independently; small-area
  waiver.
- Contested at 0.80 vs 0.79 relative gap.
- Seed-on-opt-in; second opt-in does not re-seed already timestamped
  cells.
- No `.pix` format change.

## Required manual validation

- Device residual → SP-079 / Phase 10.

## Failure and rollback considerations

- Prefer omitting competition chrome over writing timestamps into `.pix`.
- Do not count imported pixels to “make scores interesting”.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Store location | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
