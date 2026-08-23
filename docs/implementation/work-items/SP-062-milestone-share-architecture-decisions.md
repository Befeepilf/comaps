# SP-062 — Milestone and share-card architecture decisions

**Phase:** 7 — Milestones and share cards
**Status:** Accepted
**Branch:** `cursor/phase-07-work-items-aee9`
**Depends on:** Phase 5 exit (SP-041) for coding of SP-063+; this item is
  docs / `DECISIONS.md` only and may run while SP-041 awaits acceptance.
**Unblocks:** SP-063–069 (coding must not guess the locks listed here)
**Investigation note:**
  [`notes/SP-062-milestone-share-architecture.md`](../notes/SP-062-milestone-share-architecture.md)

---

## Objective

Record accepted decisions for milestone persistence, first-100-metres
conversion, re-fire after map update, multi-area celebration queue, completion
date default, and how the stylised map on the completion card is rendered — so
SP-063+ do not encode guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

Phase 7 entry requires a decision on stylised-map rendering. The phase file's
known uncertainties would otherwise be decided silently inside coding PRs.
Card composition is the only image this product produces for the outside
world; its exclusion list is a hard requirement (SPD-008, spec §19.1).

---

## In-scope behavior

- Re-verify current code locations in
  `phases/phase-07-milestones-and-share-cards.md` against the working tree.
- Investigate the three compositor candidates named in the phase file
  (Drape screenshot, polygon geometry, Android-side composition) against
  spec §19.1 exclusions. Record which path can produce a boundary outline
  without route, home, live location, track, or position-marker leakage.
- Append **SPD** entries covering at least the locks in the table below.
  Do **not** mark them Accepted unless the maintainer has locked them.
  If the maintainer has not yet locked, record recommended positions and
  leave Status explicit (proposed / awaiting lock) — do not invent an
  Accepted formula (SP-034 pattern).
- Annotate SP-063–069 and phase-07 with the decision ids.
- Record the spec §10 step 10 30-pixels ≈ 300 m hint as evidence for the
  100 m conversion, or reject it with a written alternative.
- Do not edit the product spec or technical audit.

## Out-of-scope behavior

- Implementing milestone store, UI, haptics, compositor, or share (SP-063–068).
- Achievement lists, trophy cabinets, streaks (spec §18.5, post-V1).
- Competition copy beyond a stub contract for §22.10 (Phase 8 fills it).
- Boss haptic (§28.3 becoming area boss) — Phase 8.
- First-launch competition hint at ~300 m / 30 pixels (spec §10 step 10) —
  Phase 8.
- Marking this work item or Phase 7 entry Accepted unilaterally.
- A dedicated measurement spike unless source inspection cannot choose a
  compositor path that satisfies the deny list. If a spike is required,
  record it as discovered follow-up rather than guessing the path in a
  coding item.

---

## Locks this item must record (recommended positions, not Accepted)

These are starting recommendations from the phase file, spec, and 2026-08-19
code inspection. SP-062 implementation confirms them as **proposals**. They
are recorded as OQ-9–OQ-18 in `DECISIONS.md` §15 (draft SPD-046–055). They
are **not** Accepted. Maintainer lock promotes each OQ to the matching SPD.

| Ref | Question | Recommended position | Why | OQ / draft SPD |
| --- | --- | --- | --- | --- |
| M1 | How is the stylised map rendered? | **Recommended** deny-list-safe path: off-map composition from `ExplorationArea::m_rings` (outer rings only; holes are not stored). Never a live Drape / `MapView` screenshot. Shared **card model** in `libs/`; Android (or a headless rasteriser fed only that model) draws the image. Spec §19.1 allows “stylized map **or** boundary outline”; rings are the outline branch, not a ban on every non-screenshot stylised drawing. | Exclusion list forbids route, home, live location, tracks, and position marker. A map screenshot would leak those. Rings already exist. Phase-07 entry criterion. | OQ-9 / draft SPD-046 |
| M2 | What is “approximately 100 metres of new live street pixels”? | **Recommended** reading of spec §10 step 10 (`30 new live pixels ≈ 300 m`): **10 new live pixels**. This is **not** geodesic 100 m and **not** a HEALPix-area formula. **Product choice (must lock):** (a) newly explored cells only, or (b) cells whose `IsEverLive` becomes set (imported→live flip). Current `numNewlyExploredPixels` implements (a) only. Collection radius is 25 m (§15.1), so one accepted update can collect ≥10 cells and complete the goal in a single pulse. Imported-only writes never count. | Step 9 is ~100 m; step 10 equates 30 pixels ≈ 300 m. Those two readings disagree with a walking-distance interpretation. Do **not** encode either count in SP-064 until this lock is Accepted. | OQ-10 / draft SPD-047 |
| M3 | Where does milestone fired-state live? | New local persisted store keyed by **OSM id** (not compact index, not MWM id), plus threshold. Completion date for 100% lives in the same store. Not `settings.ini` (unbounded rows). Not mixed into `StreetStatsDB` feature-bitmask tables. | Compact index is sidecar-local and can change on `.spa` regen. OSM id is the stable area identity Phase 4 already persists. `street_stats.db` is per-feature bitmasks. | OQ-11 / draft SPD-048 |
| M4 | Re-fire after a map update drops then restores a threshold? | **Does not re-fire.** Fired-state and original 100% date survive rematch / policy change / `.spa` refetch. | Phase-07 default. Spec §27.4: original date remains; UI may say previously completed. Achievement already happened. | OQ-12 / draft SPD-049 |
| M5 | Several areas cross thresholds in one update / session? | Queue non-blocking acknowledgments. Never interrupt active routing (`IsRoutingFollowing`). Show one at a time. 100% outranks 50% outranks 25%. First-goal is independent of area queue. | Spec §18.4 must not interrupt routing or demand immediate interaction. | OQ-13 / draft SPD-050 |
| M6 | Completion date on the card by default? | Store the original 100% date locally always (needed for §27.4). **Privacy recommendation (not required by “optional”):** card shows it only if the user opts in at share time; default off. Spec §19.1 “optional” ≠ a toggle; V1 could instead omit the date from the card entirely. Owner of the control: SP-068. | Weak temporal signal. Do not default-on if no control exists. | OQ-14 / draft SPD-051 |
| M7 | Competition line on the card? | Card works with no profile and no nickname (§19.2). Provide a stub string/hook for §22.10; Phase 8 fills leading / not-leading copy. Never imply completion was invalid. | Phase 8 is not a blocking dependency. | OQ-15 / draft SPD-052 |
| M8 | First-100 m lifetime? | Once per install. Appears on first recording start. Completes at M2. Never returns. Not per-session, not per-area. | Spec §10 steps 6 and 9: contextual onboarding, not an achievement system. | OQ-16 / draft SPD-053 |
| M9 | Haptics predicate and pulse count? | Pulse iff recording (not paused) **and** app foreground **and** exploration-haptics toggle on (default on). One pulse per collecting update, not per pixel. Stronger patterns for first-100 m complete, 50%, 100%. Boss pattern out of scope. Shared C++ predicate; Android supplies foreground via existing `nativeOnTransit` / `EnterForeground`. | Spec §28.1–§28.4. Today's `TriggerCollectionVibration` pulses once per pixel when `numNewlyExploredPixels > 1` and ignores foreground and the missing toggle. | OQ-17 / draft SPD-054 |
| M10 | Growth analytics? | Count-only: card generated, share initiated. No area name, OSM id, coordinates, or image. Local counters (SPD-044 pattern). Upload residual → Phase 10 if no sink. Not Sentry. | Spec §32.4, §25.1. | OQ-18 / draft SPD-055 |

### M1 — compositor path (entry criterion)

**Must be decided in this item.**

Inspect: `ExplorationArea::m_rings` (outers only), `area_overlay.cpp`
(already draws rings in Drape for in-app chrome — that path is **not** a
share image), `SharingUtils` (KML/GPX/text/`shareLocation` — not a card),
`platform::Vibrate*`, no screenshot API for neighbourhood cards.

Reject: capturing the visible map; drawing explored HEALPix cells onto
the card; including the user location puck.

Also lock, do not hide in coding items:

- City-summary rollup (`FocusedAreaProgress.m_citySummary`, SP-039) does
  **not** fire area 25/50/100 or a city share card. Spec §18.1 is per
  exploration area.
- Personal % includes imported (SPD-026), so import can cross 25/50/100
  and show a card with **no** haptic (not recording). Confirm; do not
  require an active session to fire area milestones.

---

## Proposed SPD drafts (not Accepted)

Full draft text for maintainer lock. Each is **Status: Proposed — awaiting
maintainer lock**. Last Accepted SPD remains **SPD-045**. These drafts are
**not** numbered sections in `DECISIONS.md`.

### Draft SPD-046 (M1 / OQ-9)

**Decision.** Completion-card geometry is a rings-only outline, never a live
map screenshot. V1 share image composed off-map from
`ExplorationArea::m_rings` (outers only). Shared card model in `libs/`;
Android or headless rasteriser given only that model. Never capture Drape /
`MapView`; never draw explored HEALPix, route, home, live location, track,
or position marker. Do not use MWM / country id as the title. Do not put
lat/lon, `geo:`, or ge0 URLs on the card. No individual timestamps and no
other users’ personal information. `area_overlay` stays in-app.
City-summary does not fire area 25/50/100 or a city share card.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-047 (M2 / OQ-10)

**Decision.** Proposed V1 first-100 m counts newly explored cells only
(today’s `numNewlyExploredPixels`), not `IsEverLive` flips; the threshold is
10 new live pixels (from §10 step 10: 30 ≈ 300 m) unless the maintainer
picks otherwise. This is not geodesic 100 m and not a HEALPix-area formula.
Import-only writes never count. (a) vs (b) remains a product choice until
Accept. Do not encode the count in SP-064 until Accepted. A 25 m collection
pulse can complete the goal in one update.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-048 (M3 / OQ-11)

**Decision.** New local store keyed by OSM id + threshold; 100% date in the
same store. Not `settings.ini` unbounded; not `StreetStatsDB` bitmask
tables. Compact index cache hint only.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-049 (M4 / OQ-12)

**Decision.** Does not re-fire. Fired-state and original date survive
rematch / policy / `.spa` refetch.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-050 (M5 / OQ-13)

**Decision.** Queue; one at a time; 100% > 50% > 25%; never interrupt
`IsRoutingFollowing`. First-goal independent. City-summary does not
enqueue.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-051 (M6 / OQ-14)

**Decision.** Store date always. Card shows only if share-time opt-in,
default off. Owner SP-068. Alternative: omit date from V1 card (then drop
control). Do not default-on with no control.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-052 (M7 / OQ-15)

**Decision.** Card works with no profile/nickname. Stub for §22.10; Phase 8
fills. Never imply completion invalid.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-053 (M8 / OQ-16)

**Decision.** First-100 m once per install. Incomplete persists across
sessions until complete.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-054 (M9 / OQ-17)

**Decision.** Pulse iff recording ∧ foreground ∧ toggle (default on). One
collection pulse per update. Stronger patterns: first-100 m complete, 50%,
100%. No extra for 25%. Boss out of scope. Wire `EnterForeground` into
Street Pixels (today it does not). Area milestones may fire without haptic
on import while not recording.

**Status.** Proposed — awaiting maintainer lock.

### Draft SPD-055 (M10 / OQ-18)

**Decision.** Count-only: card generated, share initiated. No area id.
Local settings uint64. Upload residual Phase 10. Not Sentry. Do not add
§32.1 / §32.2 extra events in Phase 7.

**Status.** Proposed — awaiting maintainer lock.

---

## Acceptance criteria

1. Each lock M1–M10 is either Accepted in `DECISIONS.md` with consequences,
   or explicitly recorded as awaiting maintainer lock (no silent Accepted
   invention).
2. M1 (stylised map path) is decided in enough detail that SP-067 can
   implement without choosing among Drape / rings / Android ad hoc.
3. Phase-07 entry criterion “a decision exists on how the stylised map is
   rendered” is marked met **only** after M1 is Accepted by the maintainer.
4. SP-063–069 reference the decision ids; any conditional scope is noted
   in those items.
5. Product spec is not edited. Spec/code/audit differences are reported.
6. This work item is not marked Accepted by an agent.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of M1–M10 / resulting SPDs.

## Failure and rollback considerations

- If no compositor path can satisfy the deny list without a prototype,
  record a spike follow-up and **do not** start SP-067.
- Do not weaken §19.1 exclusions to make a Drape screenshot viable.
- Do not key fired-state on compact index if OSM id is available.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-07-work-items-aee9` |
| Decision ids | OQ-9–OQ-18 (draft SPD-046–055) |
| M1 compositor path | **Proposed** — rings-only outline from `ExplorationArea::m_rings` (outers); never Drape / `MapView` screenshot |
| 100 m conversion | **Proposed** — 10 new live pixels from §10 step 10; recommend (a) newly explored only |
| Store | **Proposed** — new local sqlite keyed by OSM id + threshold |
| Re-fire | **Proposed** — does not re-fire |
| Accepted by | Maintainer |
| Accepted date | 2026-08-19 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `qt/screenshoter.*` captures the live desktop map for QA (points / rects / KML). | Not a neighbourhood-card path. Do not reuse for SP-067. |
| Spec §32.1 / §32.2 list extra activation and engagement events (first 100 m, first 25/50, first area completed, …). | Not M10. M10 is §32.4 only (card generated, share initiated). Do not add those events in Phase 7. |
| In-app `area_overlay` clip (SP-037 nested-winner `difference`) is not share geometry. | Card uses unclipped `ExplorationArea::m_rings` outers. Overlay stays in-app. |
| No compositor spike. | None unless the maintainer wants a non-outline stylised map; that is a follow-up, not an SP-067 guess. |
