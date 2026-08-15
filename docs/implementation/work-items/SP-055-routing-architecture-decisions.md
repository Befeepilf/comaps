# SP-055 — Exploration-routing architecture decisions

**Phase:** 6 — Exploration-aware routing
**Status:** In review
**Branch:** `cursor/phase-06-work-item-plan-35cf`
**Depends on:** Phase 3 exit met. Product-owner locks 2026-08-15 (R1–R12 as
  revised). SP-054 remains the Spike 7 measurement gate; it no longer
  chooses the algorithm.
**Unblocks:** SP-056+ coding after SP-054 recorded outcome (or explicit
  residual)

---

## Objective

Record accepted decisions for the explored set used by routing (OQ-2), mode
model and surfaces, Avoid semantics and fallback, mid-navigation stability,
analytics sink, and cross-leaf `.pix` lookup — so SP-056+ do not encode
guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

Phase 6 entry requires OQ-2 as an SPD. Phase-06 known uncertainties would
otherwise be decided silently inside coding PRs. Product owner locked
R1–R12 on 2026-08-15 (revisions to R3–R7; recommendations kept for the
rest).

---

## In-scope behavior

- Append **SPD-040–045**. Strike OQ-2. Annotate SP-056–061 and phase-06.
- Record the spec divergence on Avoid edge test and fallback (SPD-042).
  Do not edit the product spec.

## Out-of-scope behavior

- Implementing routing, Android UI, or analytics (SP-056–060).
- Editing the product spec or technical audit.
- Marking this work item or Phase 6 entry Accepted unilaterally.
- Pro-gating prefer/avoid, or deferring Avoid (SPD-009 still holds for V1
  inclusion).
- A min-connection second search (explicitly rejected, R7).

---

## Locked decisions (R1–R12 → SPD-040–045)

Product-owner locks 2026-08-15:

| Lock | Choice | SPD |
| --- | --- | --- |
| R1 | Personal explored set including imported (`IsExplored()`) | **SPD-040** (closes OQ-2) |
| R2 | Avoid is pedestrian + bicycle only; car Prefer may remain | **SPD-041** |
| R3 | Walk/cycle tabs; options are **Prefer / Avoid** (neither = standard) | **SPD-041** |
| R4 | Keep the 0–100 strength seekbar on Prefer (walk/bike/car). Max ETA / km deviation is post-V1 | **SPD-041** |
| R5 | Exclude an edge iff `exploredRatio == 1` | **SPD-042** |
| R6 | Clear no-route; explicit button to switch to Prefer with seekbar | **SPD-042** (fallback pair in SPD-009 superseded) |
| R7 | No min-connection search; fallback is Prefer with strength | **SPD-042** |
| R8 | §17.3 warning before Avoid is applied | **SPD-042** |
| R9 | Do not abandon a followed path because it turned green; off-route recalc uses SPD-042 fallback | **SPD-043** |
| R10 | Count-only analytics; avoid-fallback-prefer; upload residual if no sink | **SPD-044** |
| R11 | Per-segment MWM `.pix` lookup when installed | **SPD-045** |
| R12 | Strict pass = true exclusion of fully explored edges (R5). No second Avoid cost pass | **SPD-042** |

### R1 — OQ-2: personal explored set including imported

**Accepted** → **SPD-040**.

Prefer and Avoid use `IsExplored()` (live + imported-only). `IsEverLive()`
is unused for routing weights. Competition isolation stays a data rule.

**Reject.** Live-only routing weights for V1.

### R2 — Vehicle scope

**Accepted** → **SPD-041**.

Avoid is pedestrian and bicycle only. Car Prefer may remain. Do not ship
Avoid for cars in V1.

### R3 — UI surface and mode model

**Accepted** (revised) → **SPD-041**.

Named options are **Prefer** and **Avoid**, mutually exclusive, on the
walking and cycling routing-options tabs. Neither selected is ordinary
routing. Do not present a third labelled “Standard” mode.

**Reject.** Leaving Prefer only on the driving tab; stacking Prefer+Avoid
on at once; a dedicated exploration-routing activity.

### R4 — Strength slider

**Accepted** (revised) → **SPD-041**.

Keep the 0–100 strength seekbar in V1. It applies to Prefer on walk, bike,
and the existing car control. A later max-ETA or kilometre-deviation
control is post-V1, not a V1 substitute.

**Reject.** Hiding the walk/bike seekbar; inventing an ETA/km cap in V1.

### R5 — Strict Avoid edge test

**Accepted** (revised) → **SPD-042**.

Exclude an edge only when `exploredRatio == 1`. Partially explored edges
stay in the graph so Avoid does not routinely fail or produce extreme
detours. Unmatched samples are not explored.

**Reject.** `exploredRatio > 0` exclusion; treating 10× Prefer as Avoid.

**Spec note.** §17.3 “edges containing explored pixels” is implemented as
fully explored edges only. Recorded in SPD-042; spec not edited.

### R6 — No-route UX

**Accepted** (revised) → **SPD-042**.

If the strict pass finds no path, show a **clear no-route** result
(distinct code, not generic `RouteNotFound` as the Avoid failure). Offer a
simple control that switches to **Prefer with the strength seekbar**.
Never auto-switch.

**Reject.** Silent retry as Prefer; generic “open settings” as the only
Avoid failure UI.

### R7 — No min-connection search

**Accepted** (revised) → **SPD-042**.

Do not implement a second search that minimises explored distance, segment
count, or pixel count. Fallback is Prefer with strength (R6).

**Spec note.** §17.3 / §31 min-connection / return-to-normal pair is
replaced for V1 by Prefer+strength. Recorded in SPD-042.

### R8 — Warning

**Accepted** → **SPD-042**.

Show §17.3 warning **before** Avoid is applied: “This can produce very
long routes or no available route.”

**Reject.** Warning only after failure.

### R9 — Mid-navigation

**Accepted** → **SPD-043**.

While following an Avoid route, exploring remaining geometry must not by
itself abandon that path. Off-route / explicit recalc may re-apply Avoid;
on failure, show the R6 Prefer+seekbar control.

**Reject.** Re-running exclusion on every location update; freezing Avoid
across off-route rebuilds.

### R10 — Analytics sink

**Accepted** → **SPD-044**.

Count-only: prefer-used, avoid-used, avoid-fallback-prefer. No location.
Local API; upload residual → Phase 10 if no privacy-safe sink. Not Sentry.

### R11 — Cross-leaf `.pix` lookup

**Accepted** → **SPD-045**.

Consult the segment MWM’s `.pix` when installed. Missing file → not
explored. Do not load extra leaves into the renderer.

### R12 — Algorithm

**Accepted** → **SPD-042**.

Strict pass: **true exclusion** of edges with `exploredRatio == 1`. No
min-connection cost pass. SP-054 measures this locked shape (length, time,
no-route frequency); it does not re-open R5–R7.

**Reject.** Large finite penalty as the Avoid implementation; Prefer at
strength 100 labelled as Avoid.

---

## Acceptance criteria

1. SPD-040–045 present in `DECISIONS.md` with Status Accepted.
2. OQ-2 struck with reference to SPD-040.
3. SPD-009 fallback-offer pair marked superseded by SPD-042.
4. SP-056–061 and phase-06 reference the decision ids; min-connection is
   out of scope.
5. Spec divergence (R5, R7) is recorded; product spec is not edited.
6. This work item is not marked Accepted by an agent.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of SPD-040–045.

## Failure and rollback considerations

- If SP-054 shows Avoid is pathological even with `exploredRatio == 1`,
  **do not** defer Avoid without a new SPD that supersedes SPD-009. Escalate
  to product; do not silently ship Prefer-only.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-06-work-item-plan-35cf` |
| Product-owner lock | 2026-08-15 (R1–R12 as revised) |
| Decision ids | SPD-040 (OQ-2), SPD-041 (R2–R4), SPD-042 (R5–R8, R12; supersedes SPD-009 fallback pair), SPD-043 (R9), SPD-044 (R10), SPD-045 (R11) |
| OQ-2 | Closed by SPD-040 |
| Spec divergence | SPD-042 — fully-explored exclusion; Prefer+strength fallback |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| No product-analytics upload sink (SP-003) | SPD-044 local API + Phase 10 residual |
| Overlay loads one `.pix` (`m_countryId`) | SPD-045 in SP-056/057 |
| Spec §17.3 / §31 fallback pair unused in V1 | Recorded in SPD-042; do not edit spec |
| Max ETA / km deviation from optimal | Post-V1 (R4); not a V1 work item |
