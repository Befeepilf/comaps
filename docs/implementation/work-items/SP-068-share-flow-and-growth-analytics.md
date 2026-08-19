# SP-068 — Share flow and growth analytics

**Phase:** 7 — Milestones and share cards
**Status:** Planned
**Branch:**
**Depends on:** SP-067 card image/model; SP-062 M10; SPD-044 local-counter
  pattern (SP-060).
**Unblocks:** SP-069 exit #6 and #9

---

## Objective

Offer an explicit Share control on the 100% completion card that opens the
system share sheet only when tapped, and record count-only analytics for
card generation and share initiation with no location or area identifiers.

## Motivation

Spec §19.3 forbids auto-opening the share sheet. §32.4 requires two growth
counts. SP-003 / SP-060 left product analytics as local counters plus a
Phase 10 upload residual. Milestone/card events must follow that, not
Sentry.

## In-scope behavior

- Completion card UI has a **Share** button. Tapping it shares the
  transient card image (and optional first-person text) via Android
  `ACTION_SEND` for an **image**. It must not attach a track, KML, KMZ,
  GPX, bookmark file, or live location. Do not call
  `SharingUtils.shareLocation`.
- The sheet does not open when the card appears, when 100% fires, or on
  celebration animation end. Share chrome lives on the SP-065 surface;
  this item owns the tap handler.
- Share-time date control (M6): a single opt-in on the 100% card/share
  surface, **default off**. Off → card model omits the date. On → include
  the stored original 100% date only (no per-visit timestamps). No control
  → omit the date (do not default-on). If SP-062 instead locks “omit date
  from the V1 card,” drop this control.
- Anonymous sharing works with no account and no nickname.
- Local counters (uint64 in settings or the SP-060 helper style) — this
  item owns both:
  - completion card generated
  - share action initiated
- Tests: counters increment on the corresponding fake events; persisted
  payloads / keys contain no lat/lon, geometry, pixel id, OSM id, area
  name, or image bytes. Shared URI is the transient card image.
- If no privacy-safe upload sink exists, local-only + Phase 10 residual
  (same honesty as SPD-044). Public builds gain no new network endpoint.

## Out-of-scope behavior

- Compositor / deny list (SP-067).
- A new analytics vendor.
- Sentry `captureMessage` for shares.
- Sharing non-card content.
- Competition-specific share text beyond the stub (Phase 8).

## Relevant product requirements

- Spec §19.3, §32.4, §25.1, §34 Sharing.
- SP-062 M10; SPD-044 pattern.

## Relevant source files or symbols

- `android/app/src/main/java/app/organicmaps/util/SharingUtils.java`
- `routing::StreetExplorationRoutingAnalytics` (pattern to copy, not to
  overload with card events)
- SP-067 card output URI
- No card-generated / share-initiated counters (2026-08-19)

## Implementation notes / constraints

- Counts only. Spec: “They do not record which area.”
- Do not put the card bitmap into logs.
- Delete or overwrite the transient file after a successful share hand-off
  when practical.

## Acceptance criteria

1. Share sheet opens only after an explicit Share tap.
2. Shared payload is the card image (optional text), never a track, KML,
   KMZ, GPX, bookmark, or `shareLocation` extra.
3. Two counters exist and increment; tests prove no location/area fields.
4. Date is absent from the shared card unless the M6 opt-in is on;
   default off is tested (or omitted entirely if SP-062 locks that).
5. Upload is privacy-safe and documented, or residualled to Phase 10.
6. Sentry is not the sink.

## Required automated tests

- Counter increment + schema/keys have no location or area identifiers.
- Share invocation is gated on the explicit action (unit-level; no
  auto-trigger from the 100% fire helper).
- Shared URI is the transient card image, not a track/KML/GPX path.
- Date opt-in default off (unless M6 omits the date from V1).

## Required manual validation

- Reach 100% (or fixture): card appears, sheet stays closed; tap Share;
  sheet opens with the image. Device residual → SP-069 / Phase 10.

## Failure and rollback considerations

- Prefer no share action over auto-open or attaching a GPX.
- Do not skip the deny-list because share is “just an image”.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Upload residual | |
| Manual validation | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (filled during implementation) | |
