# SP-055 — Exploration-routing architecture decisions

**Phase:** 6 — Exploration-aware routing
**Status:** Planned
**Branch:** `cursor/phase-06-work-item-plan-35cf` (planning); locks land as
  SPDs on the same or a follow-up commit after maintainer confirmation
**Depends on:** Phase 3 exit met. Group B (R12) depends on SP-054 recorded
  outcome or explicit residual
**Unblocks:** SP-056+ coding

---

## Objective

Record accepted decisions for the explored set used by routing (OQ-2), mode
model and surfaces, Avoid semantics and fallback, min-connection metric,
mid-navigation stability, analytics sink, and cross-leaf `.pix` lookup — so
SP-056+ do not encode guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

Phase 6 entry requires OQ-2 as an SPD. Phase-06 known uncertainties (penalty
vs exclusion, min-connection metric, mid-nav recompute, UI surface, lookup
cost, imported pixels) would otherwise be decided silently inside coding
PRs. Phase 4/5 used an explicit decisions item (SP-024 / SP-042 / SP-049)
before coding.

Recommended locks **R1–R12** below are **not** Accepted until the maintainer
confirms. Do not write `Status: Accepted` SPD entries on agent initiative.

---

## In-scope behavior

- Present R1–R12 with rationale, reject-list, and SPD mapping.
- After maintainer lock: append SPD entries, strike OQ-2 in `DECISIONS.md`
  §15 if R1 is Accepted, and annotate SP-056–061.
- Soften phase-06 wording that still treats these as open once locked.

## Out-of-scope behavior

- Implementing routing, Android UI, or analytics (SP-056–060).
- Editing the product spec or technical audit.
- Marking this work item or Phase 6 entry Accepted unilaterally.
- Pro-gating prefer/avoid, or deferring Avoid (contradicts SPD-009 / §29.1).

---

## Recommended locks (R1–R12)

Numbered **R-*** to avoid colliding with sidecar D1–D14. Proposed SPD ids
are sequential after SPD-039.

### Group A — lockable without SP-054 numbers

| Lock | Choice | Proposed SPD |
| --- | --- | --- |
| R1 | Personal explored set including imported (`IsExplored()`) | SPD-040 (closes OQ-2) |
| R2 | Avoid is pedestrian + bicycle only; car prefer may remain | SPD-041 |
| R3 | Walk/cycle routing-options tabs; modes Standard / Prefer / Avoid | SPD-041 (same) |
| R4 | No walk/bike strength seekbar; prefer uses internal default 50 | SPD-041 |
| R5 | Strict Avoid excludes an edge if `exploredRatio > 0` | SPD-042 |
| R6 | Distinct no-route result; never silent degrade | SPD-042 (affirms SPD-009) |
| R7 | Min-connection minimises explored **distance** (metres) | SPD-042 |
| R8 | §17.3 warning before Avoid applies; option label from §17.3; §31 explains the impossible-route state | SPD-042 |
| R9 | Do not abandon a followed path because it turned green; off-route recalc re-applies Avoid with the same fallback offer | SPD-043 |
| R10 | Count-only analytics API; no location; upload residual if no sink | SPD-044 |
| R11 | Per-segment MWM `.pix` lookup when the file is installed | SPD-045 |

### Group B — spike-gated

| Lock | Choice | Proposed SPD |
| --- | --- | --- |
| R12 | Strict pass = true exclusion (default rec.); min-connection pass = explored-distance cost / large finite penalty. Confirm or revise from SP-054 | SPD-042 field |

Maintainer may Accept Group A immediately and Group B after SP-054.

---

### R1 — OQ-2: personal explored set including imported

**Recommended.** Prefer-unexplored and Avoid use `IsExplored()` (live +
imported-only). `IsEverLive()` is unused for routing weights.

**Why.** Spec §17.2 talks about unvisited pixels on the personal map. Imported
GPX turns pixels green and counts for personal completion (SPD-026, §29.2).
Routing someone through green “unexplored” streets would contradict the map.
Competition isolation stays a **data** rule (no recency / ownership /
eligibility / weekly ranking from imports) and is not implemented inside
the router. Audit §12 already framed this as “imported affects personal
routing but not competition.”

**Reject.** Live-only routing weights for V1 (would treat imported-green
streets as unexplored).

**Current code.** Already matches R1 (`IsExplored()`). Locking R1 is
documentation + tests, not a behaviour flip.

### R2 — Vehicle scope

**Recommended.** Avoid is a V1 feature for **pedestrian and bicycle** routers
only. It must not apply to car/vehicle routing. Prefer may remain on the
existing car driving-options toggle (phase-06 non-goal: car routing
changes).

**Reject.** Shipping Avoid for cars in V1; hiding Prefer on car as a drive-by.

### R3 — UI surface and mode model

**Recommended.** Expose Prefer and Avoid on the **walking and cycling**
routing-options tabs (the surfaces `RoutingOptionsFragment` already hosts).
Modes are mutually exclusive: **Standard | Prefer unexplored | Avoid
explored**. Persisted in `StreetExplorationRoutingOptions` (reshape
`m_enabled` into a mode; migrate `enabled == true` → Prefer).

**Reject.** Leaving Prefer only on the driving tab; stacking Prefer+Avoid;
a third dedicated exploration-routing activity unless walk/bike tabs prove
unworkable.

### R4 — Strength slider

**Recommended.** No user-facing strength control on walk/bike. Prefer uses
internal `kDefaultStrength` (50) so routes “remain reasonably practical”
(§17.2). The existing car seekbar may stay (car non-goal). Do not add a
second persisted strength.

**Reject.** Making 0–100 a V1 walk/bike setting; defaulting Prefer to 100×
practicality risk without measurement.

### R5 — Strict Avoid edge test

**Recommended.** An edge is explored for Avoid iff at least one **matched**
HEALPix sample on the segment is `IsExplored()` (`exploredRatio > 0`).
Unmatched samples (no `.pix` hit) are not explored (same fail-open as
today’s 1.0 multiplier).

**Reject.** Soft 10× as the Avoid implementation; a hidden ratio threshold
other than zero.

### R6 — No silent fallback

**Recommended.** If the strict pass finds no path, the router returns a
**distinct** result (new `RouterResultCode`, JNI-mirrored). The UI offers
the two spec choices. Never auto-enable Prefer, never auto-clear Avoid,
never present generic `RouteNotFound` as the Avoid failure.

**Reject.** Catching `RouteNotFound` and silently retrying with Prefer or
standard.

### R7 — Minimum necessary explored connection

**Recommended.** Minimise **explored distance**: metres of geometry on edges
with `exploredRatio > 0`. Unexplored geometry keeps ordinary pedestrian /
bicycle weight. This matches “connection” / “small amount of explored
routing” (§17.3 / §31) better than segment count (games short vs long
edges) or pixel count (couples to HEALPix sampling).

**Reject.** Minimising explored segment count or explored pixel count as the
V1 cost.

### R8 — Warning and copy

**Recommended.** Show §17.3 warning **before** Avoid is used: “This can
produce very long routes or no available route.” Fallback option label:
“Allow the minimum necessary explored connection” (§17.3). Impossible-route
explanation may use §31 (“no fully unexplored route”). Second option:
“Return to normal routing” (both sections). Returning to normal **clears
Avoid** (and does not silently leave Prefer on unless the user had Prefer).

**Reject.** Warning only after failure; using different option semantics
than the spec pair.

### R9 — Mid-navigation

**Recommended.** While **following** an Avoid route, exploring remaining
geometry (pixels turning green) must **not** by itself trigger a re-search
that abandons the active path. Off-route / user-requested recalculation may
re-apply Avoid from the new position; if the strict pass fails, show the
same R6 fallback offer — do not silently inject explored edges.

**Reject.** Re-running strict exclusion on every location update; freezing
Avoid forever even after the user goes off-route.

### R10 — Analytics sink

**Recommended.** Count-only named events: prefer-used, avoid-used,
avoid-fallback-min-connection, avoid-fallback-normal. No origin,
destination, geometry, pixel ids, or area ids. Implement a shared counter
API. If no privacy-safe upload sink exists in-tree (SP-003 deferred events),
keep counters local and residual **upload** to Phase 10. Do not send these
events through Sentry.

**Reject.** A new telemetry backend in Phase 6; Sentry messages that include
routes.

### R11 — Cross-leaf `.pix` lookup

**Recommended.** `GetSegmentExplorationWeightMultiplier` must consult the
`.pix` for the **segment’s MWM** when that file is installed, not only the
overlay’s `m_countryId`. Missing `.pix` → unmatched → unexplored (1.0 / not
excluded). Do not load extra leaves into the renderer.

**Reject.** Leaving the mismatch early-return as V1 behaviour (silent wrong
Prefer/Avoid at leaf boundaries).

### R12 — Algorithm (after SP-054)

**Recommended default, confirm with numbers.** Strict pass: **true
exclusion** of explored edges (R5), because a large finite penalty can still
use an explored edge and that would be a silent rule break (R6 / SPD-009).
Min-connection pass: explored-distance primary cost (R7), implemented as a
second search (large finite penalty on explored metres is acceptable here
because the user **opted in**). If SP-054 shows exclusion is unusable
(&gt;2 s extra with no path on ordinary connected ODs, or instability),
record a revised R12 — do not silently ship penalty-as-Avoid.

**Reject.** Treating the existing 10× prefer multiplier as Avoid.

---

## Acceptance criteria

1. Maintainer has Accepted or explicitly revised R1–R11 (Group A) and R12
   (Group B, after SP-054 or with residual).
2. Corresponding SPD entries exist with Status Accepted.
3. OQ-2 struck in `DECISIONS.md` §15 iff R1 is Accepted.
4. SP-056–061 reference the decision ids; vehicle/UI/algorithm scope is
   no longer conditional in those items.
5. This work item is not marked Accepted by an agent.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of R1–R12 (and SP-054 numbers for R12).

## Failure and rollback considerations

- If SP-054 shows Avoid is pathological, **do not** defer Avoid without a
  new SPD that supersedes SPD-009. Escalate to product; do not silently
  ship Prefer-only.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-06-work-item-plan-35cf` |
| Group A locks | Recommended R1–R11 (not Accepted) |
| Group B locks | Recommended R12 pending SP-054 |
| Decision ids | (filled when Accepted) |
| OQ-2 | Open; recommended R1 |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| No product-analytics upload sink (SP-003) | R10 local API + Phase 10 residual |
| Overlay loads one `.pix` (`m_countryId`) | R11 in SP-057 (and prefer path in SP-056 if weights move) |
| §17.3 vs §31 fallback wording differs | R8 copy lock |
| Car estimator already applies prefer multiplier | R2: leave car prefer; do not enable Avoid on car |
