# SP-080 — GPX and feature-gating architecture decisions

**Phase:** 9 — GPX and feature gating
**Status:** Accepted
**Branch:** `cursor/sp-phase9-residuals-db9d`
**Depends on:** Phase 3 exit met; SP-005 Accepted. Product-owner lock of
  G1–G10 on 2026-08-28 (recommended positions).
**Unblocks:** SP-081–087 (coding must not guess the locks listed here)
**Investigation note:**
  [`notes/SP-080-gpx-feature-gating-architecture.md`](../notes/SP-080-gpx-feature-gating-architecture.md)

---

## Objective

Record accepted decisions for the historical-import path versus free
bookmark import, stored-track-on-import, `geometry_hash` duplicate policy,
V1 “advanced track management” scope, historical sampling (no live GPS
rules, no cross-segment fill), public share-sheet behaviour, internal
debug entitlement, the Explorer Pro information page, and monetisation
analytics — so SP-081+ do not encode guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

Phase 9 entry is met. The phase file’s known uncertainties would otherwise
be decided silently inside coding PRs. Public V1 must hide exploration-
affecting GPX tooling and must never show a non-functional purchase
action (SPD-010, SPD-011). Competition isolation is a data rule, not a
gate.

A dedicated importer is still required even though `MarkExploredPixelIds`
already uses imported-only semantics: today every bookmark track is
replayed, `Track::GetGeometry` concatenates segments, and GPX surfaces
are free.

---

## In-scope behavior

- Re-verify current code locations in
  `phases/phase-09-gpx-and-feature-gating.md` against this working tree.
- Append **SPD-067–076**. Strike OQ-20–OQ-29. Annotate SP-081–087 and
  phase-09. Product-owner lock 2026-08-28 accepted the recommended
  positions.
- Annotate SP-081–087 and phase-09 with the decision ids.
- Strike or add OQ rows in `DECISIONS.md` §15 for G1–G10.
- Record why Spike 9 is not a separate work item (G10).
- Do not edit the product spec or technical audit.

## Out-of-scope behavior

- Implementing the importer, gates, settings, robustness, or analytics
  (SP-081–086).
- Google Play Billing, purchase flow, restoration, pricing, store
  entitlement validation (SPD-010).
- iOS StoreKit.
- Additional export formats beyond existing GPX/KML/KMZ.
- Making competition a paid feature (spec §29.1).
- Server-side entitlement validation.
- Removing free KML/KMZ bookmark import used for ordinary map bookmarks.
- Inventing track merge/split/join tools that do not exist today.

---

## Locked decisions → SPD-067–076

Product-owner lock 2026-08-28 accepted every recommended G1–G10 position.
Full text in `DECISIONS.md`.

| Ref | Question | Locked position | Why | OQ / SPD |
| --- | --- | --- | --- | --- |
| G1 | Bookmark reuse vs dedicated entry; which tracks paint pixels? | **Dedicated historical-import path** is the only writer of imported pixels. Free KML/KMZ bookmark import does **not** paint pixels. Live-saved recordings do **not** replay through import (live collection already ran). GPX files with tracks are Pro. Retire “replay every bookmark track” in `UpdateExploredPixels`. | Spec §29.2 gates GPX track import, not ordinary bookmarks. Today KML tracks and saved recordings also paint via `UpdateExploredPixels`, which can fill pause gaps (`GetGeometry` concatenates segments) and is a Pro loophole. | OQ-20 / SPD-067 |
| G2 | Create a stored track on import? Does delete un-explore? | **Yes**, import materialises a local bookmark/track the user can inspect and delete. Deleting the track **does not** un-explore pixels (`processed_tracks` row stays). Exploration is permanent (spec §3.6, §15.2). | Code already saves GPX as a KML category. Un-exploring on delete would be a new destructive action the spec does not grant. | OQ-21 / SPD-068 |
| G3 | Is `geometry_hash` enough to skip duplicates? | **Keep** mercator x,y-only hash per country. Identical geometry skips. Timestamp- or metadata-only re-export skips. Any point add/remove/move reprocesses. Do not hash timestamps, altitude, name, or file bytes. | Stable against Garmin/OSMAnd re-save of the same points. Tiny GPS noise re-import is acceptable (pixels already explored are no-ops). | OQ-22 / SPD-069 |
| G4 | What is V1 “advanced track management”? | **Batch GPX import** (existing multi-URI) plus the Pro import/export surfaces. No new merge/split/join. Basic list, rename, colour, show/hide, and delete of **own recordings** stay free (spec §29.1 local track storage, §30 local recording management). `Capability::AdvancedTrackManagement` gates batch import; `GpxImport` / `GpxExport` gate single-file tools. | Spec names batch import separately. Inventing merge tools is scope. Free recording management must survive the gate. | OQ-23 / SPD-070 |
| G5 | Historical sampling vs live GPS rules? | **15 m geometric sampling per track segment** (SPD-019). Do **not** apply live accuracy, speed, staleness, pause, or 30 s / 200 m gap filters. Do **not** sample across segment joins or GPX `trkseg` gaps. Do **not** place pixels from interpolated GPX timestamps. Skip non-finite or out-of-range coordinates. | Spec §16 is live-only. Phase 2 rules on sparse historical timestamps would reject most GPX. Cross-segment fill violates the pause/gap invariant. | OQ-24 / SPD-071 |
| G6 | Share-sheet / VIEW / SEND when the gate is closed? | Processor **refuses GPX** (no pixel paint, no silent bookmark-only import of tracks). No purchase / upgrade CTA. KML/KMZ bookmark import remains. Manifest filters may stay (OS still offers the app); the handler no-ops GPX when `IsCapabilityEnabled(GpxImport)` is false. | Public V1 must expose no GPX tooling (phase-09 exit #4). Android cannot hide intent-filters per user. A buy button would violate SPD-010. | OQ-25 / SPD-072 |
| G7 | How do internal Pro builds become entitled? | **Debug entitlement source** installed only when Pro capabilities are on **and** a separate debug-only BuildConfig is true. Never in public release/beta with capabilities off. `StubEntitlementSource` stays always-false and never reads `ExplorerPro.Entitled`. Tests prove release-shaped configs cannot grant. Grant symbols compiled out of non-debug Android. | Gate is available ∧ entitled (SPD-011). Capabilities-on + stub-false means internal builds cannot exercise tools. A public grant path is forbidden. | OQ-26 / SPD-073 |
| G8 | Explorer Pro information page? | Short in-app explanation, **no price, no buy, no restore**. Shown only when the Pro **capability** is available in the build. Flag-off public builds have no page. | Spec §32.5 measures “information page viewed”. SPD-010 forbids a purchase action. | OQ-27 / SPD-074 |
| G9 | Monetisation analytics? | Count-only local uint64 (SPD-044 / SPD-055 pattern): info page viewed, GPX import usage, GPX export usage. Increment **only** when the matching capability is available. No lat/lon, file name, track geometry, area id, or pixel id. Upload residual → Phase 10. Not Sentry. No events when Pro is off in the build. | Spec §32.5 “measure when enabled in a build”. Purchase conversion is post-V1 (SPD-010). | OQ-28 / SPD-075 |
| G10 | Separate Spike 9 work item? | **No.** Isolation is already a data-layer property; SP-082 proves it on the dedicated path. Large-import memory is SP-085 (measure; chunk only if needed). | Audit Spike 9 mixed isolation (done) with 10k-point memory (unknown). A blocking spike would delay gating for a conditional optimisation. | OQ-29 / SPD-076 |

### G1 — dedicated path (load-bearing)

**Must be decided in this item.**

Inspect: `UpdateExploredPixels` walks `ForEachTrackSortedByTimestamp` and
paints every track; `SaveTrackRecording` then `OnBookmarksCreated` feeds
live-saved geometry into that painter; `Track::GetGeometry` flattens
segments.

Reject: gating only the Android file picker while leaving bookmark-track
replay as a free pixel writer; applying live GPS filters to GPX;
un-exploring pixels when a track is deleted.

Also lock, do not hide in coding items:

- Isolation stays in `MarkExploredPixelIds` / live-only side effects, never
  inside `IsCapabilityEnabled` (SPD-011).
- Personal completion includes imported (SPD-026); routing uses
  `IsExplored()` (SPD-040). Import may fire 25/50/100 with no haptic.
- Waypoints-only GPX (no tracks) does not paint pixels. Recommended with
  G6: still refuse the GPX file when the gate is closed, rather than a
  second “bookmarks-only GPX” mode in V1.

### G7 — internal entitlement (entry for manual Pro UX)

Without a lock, Phase 9 exit “Pro-enabled internal build, tools appear
and work” is impossible: stub entitlement always denies. Do not weaken
`IsCapabilityEnabled` to “available only”.

---

## Accepted SPD text

Full text lives in `DECISIONS.md` as **SPD-067–076** (Status Accepted).
Last Accepted SPD is **SPD-076**.

---

## Acceptance criteria

1. G1–G10 are recorded as Accepted SPD-067–076. OQ-20–OQ-29 are struck.
2. Product spec and technical audit are not edited.
3. SP-081–087 and phase-09 reference the decision ids.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Product-owner lock of G1–G10 (received 2026-08-28).

## Failure and rollback considerations

- If G1 is rejected in favour of “flag the existing bookmark import”,
  SP-081 must still stop live-saved replay across concatenated segments
  (pause-gap invariant). Do not silently keep catch-all replay.
- If G7 is rejected, do not open the stub as a grant path; reopen the
  internal-testing question.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-phase9-residuals-db9d` |
| Product-owner lock | 2026-08-28 (recommended G1–G10 positions) |
| Decision ids | SPD-067 (G1/OQ-20), SPD-068 (G2/OQ-21), SPD-069 (G3/OQ-22), SPD-070 (G4/OQ-23), SPD-071 (G5/OQ-24), SPD-072 (G6/OQ-25), SPD-073 (G7/OQ-26), SPD-074 (G8/OQ-27), SPD-075 (G9/OQ-28), SPD-076 (G10/OQ-29) |
| Accepted by | Product owner |
| Accepted date | 2026-08-28 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| `Track::GetGeometry` concatenates segments; import sampling across joins can paint a corridor across a pause or `trkseg` gap | G5 / SP-081: sample per segment |
| SP-034 text says invalidate cache on GPX import; code increments without full invalidate (`AreaCompletionManager_ImportIncrementsWithoutInvalidating`) | Keep incremental behaviour; do not “fix” back to full invalidate in Phase 9 |
| Java cannot query `IsCapabilityEnabled` (setter JNI only) | SP-083 adds getters |
| Capabilities-on + stub-false cannot exercise Pro UX | G7 |
| Failed GPX parse may log the entire file via `ReadAsString` | SP-085: do not dump huge untrusted input |
