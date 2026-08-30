# SP-070 — Competition architecture decisions

**Phase:** 8 — Competition
**Status:** Accepted
**Branch:** `cursor/phase-08-work-items-c990`
**Depends on:** Phase 4 exit met. Product-owner locks 2026-08-23 (OQ-1,
  OQ-3, OQ-4 unique nicknames, OQ-6, OQ-7, C1–C10).
**Unblocks:** SP-071–079 (coding must not guess the locks listed here)
**Investigation note:**
  [`notes/SP-070-competition-architecture.md`](../notes/SP-070-competition-architecture.md)

---

## Objective

Record accepted decisions for ownership scoring, contested state, unique
nicknames, weekly week boundaries, friends visibility, competition API
host/path/region/retention, recency storage, consent re-prompt, backend
shape, clamp rules, and the §10 step 10 hint — so SP-071+ do not encode
guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

Phase 8 must not be decomposed until OQ-1 is answered. Coding a scoring
system around an undefined formula produces work that will be thrown away.
Remaining Phase 8 open questions (OQ-3, OQ-4, OQ-6, OQ-7) and the
implementation locks in the phase file would otherwise be decided silently
inside coding PRs.

---

## In-scope behavior

- Re-verify current code locations in `phases/phase-08-competition.md`
  against this working tree.
- Append **SPD-057–066**. Strike OQ-1, OQ-3, OQ-4, OQ-6, OQ-7. Annotate
  SP-071–079 and phase-08.
- Record the spec divergence on nickname uniqueness (SPD-059). Do not
  edit the product spec.
- Do not mark this work item or Phase 8 exit Accepted unilaterally.

## Out-of-scope behavior

- Implementing consent, scoring, upload, backend, or UI (SP-071–078).
- Editing the product spec or technical audit.
- Boss haptic (already out of V1, SPD-054).
- Deploying production hosting (ops names the EU region string; SP-075
  owns a non-SQLite backend environment in `comaps_backend`).

---

## Locked decisions → SPD-057–066

Product-owner locks 2026-08-23 (accept all recommended positions except
nicknames, which **are unique**):

| Lock | Choice | SPD |
| --- | --- | --- |
| OQ-1 score | Recency-weighted live coverage % of the area | **SPD-057** |
| OQ-1 contested | Runner-up ≥ 80% of leader; leader stays boss | **SPD-058** |
| OQ-4 | Public nicknames **unique** (spec §20.4 divergence) | **SPD-059** |
| OQ-3 | Monday 00:00 city IANA TZ, else UTC | **SPD-060** |
| OQ-6 | Hide friends in public Android V1 | **SPD-061** |
| OQ-7 | `https://api.streifzug.app/api/v1/competition/`; EU; retain until delete or 24 months idle | **SPD-062** |
| C1 / C2 | Sparse HEALPix recency; seed at first opt-in | **SPD-063** |
| C3 / C4 | Re-prompt consent; discard `explore_stats.json` | **SPD-064** |
| C5–C8 | Django app `competition/`; clamp; OSM ids; score version 1 | **SPD-065** |
| C9 | Hint at 30 newly explored live pixels | **SPD-066** |
| C10 | Boss haptic stays out of V1 | **SPD-054** (already) |

### OQ-1 score — recency-weighted live coverage

**Accepted** → **SPD-057**.

\[
\mathrm{ownership\_score} = 100 \times \frac{1}{T}\sum_{p \in L} 2^{-\Delta t_p / 30\,\mathrm{d}}
\]

Ever-live pixels only. \(T = 0\) → 0. Not pixel_count × percentage.

### OQ-1 contested — 80% of leader

**Accepted** → **SPD-058**.

Contested iff eligible boss, another eligible participant, and runner-up
score ≥ 80% of leader. Leader remains boss until overtaken.

### OQ-4 — unique nicknames

**Accepted** → **SPD-059**.

**Spec divergence.** §20.4 allows duplicate nicknames. V1 requires
uniqueness. Spec is not edited. Format still §21.1.

### Remaining locks

OQ-3, OQ-6, OQ-7, C1–C10 as in the table. Full text in `DECISIONS.md`.

---

## Acceptance criteria

1. SPD-057–066 present in `DECISIONS.md` with Status Accepted.
2. OQ-1, OQ-3, OQ-4, OQ-6, OQ-7 struck with references to those SPDs.
3. Nickname uniqueness recorded as a spec divergence; product spec not
   edited.
4. SP-071–079 and phase-08 reference the decision ids.
5. This work item is not marked Accepted by an agent.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of SPD-057–066.

## Failure and rollback considerations

- If production cannot host in the EU, do not silently ship another
  region; reopen OQ-7 / SPD-062.
- If unique nicknames prove unusable in testing, reopen SPD-059; do not
  silently allow duplicates.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-08-work-items-c990` |
| Product-owner lock | 2026-08-23 (unique nicknames; otherwise recommended positions) |
| Decision ids | SPD-057 (OQ-1 score), SPD-058 (OQ-1 contested), SPD-059 (OQ-4 unique), SPD-060 (OQ-3), SPD-061 (OQ-6), SPD-062 (OQ-7), SPD-063 (recency), SPD-064 (consent / stats file), SPD-065 (backend / clamp / OSM / version), SPD-066 (30-pixel hint) |
| Spec divergence | SPD-059 — unique nicknames vs spec §20.4 |
| Accepted by | Product owner |
| Accepted date | 2026-08-23 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `comaps_backend` is not in this checkout | SP-075–077 land in that repo; re-verify models there |
| Exact EU region string for the privacy policy | Ops lock; residual on SPD-062, not a scoring formula |
| `IdentityStore::IsValidUsername` is ASCII 3–20 | SP-071 widens to spec §21.1; uniqueness is server-side |
| `explore_stats.json` + 1-minute poll | Discard file (SPD-064); replace poll in SP-074 |
| No per-pixel recency today | SP-072 + SPD-063 |
| Card `m_competitionLine` stub | SP-078 fills §22.10 |
