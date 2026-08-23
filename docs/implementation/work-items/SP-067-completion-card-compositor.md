# SP-067 — Completion-card compositor

**Phase:** 7 — Milestones and share cards
**Status:** Accepted
**Branch:** `cursor/sp-067-completion-card-compositor-c417`
**Depends on:** SP-062 M1 (render path), M6 (date opt-in), M7 (competition
  stub). SP-063 original 100% date. SP-065 100% surface. `ExplorationArea`
  rings and `DisplayName`.
**Notes:** Coding waits on OQ-9 / OQ-14 / OQ-15 (draft SPD-046 / SPD-051 /
    SPD-052). Rasterise only after M1 Accept.
**Unblocks:** SP-068 share of a real image; SP-069 exit #4, #5

---

## Objective

Generate a 100% completion card that contains only permitted fields, with
the composed **model** asserted against an explicit deny list, and an image
that is a stylised boundary outline — not a map screenshot.

## Motivation

SPD-008 keeps share cards in V1. No neighbourhood compositor exists.
Generic `SharingUtils` shares KML/GPX/text and must not be reused as a
track dump. The card is the only image produced for the outside world.

## In-scope behavior

- Shared **card model** with only permitted fields:
  - area display name
  - “100% explored”
  - stylised boundary outline (from `m_rings`, not GPS, not HEALPix
    explored set, not live camera)
  - optional display name / nickname (omit when absent)
  - optional completion date (omit unless user opted in — M6)
  - subtle Street Pixels branding
  - competition line stub (empty / first-person when competition off)
- Explicit deny list in tests and in the model constructor. Reject / omit:
  - raw GPS route / track geometry
  - home location
  - live location / position marker
  - individual timestamps (visit-level)
  - other users’ personal information
  - explored-pixel corridor / recency
  - MWM country id as the title
  - coordinates in labels
- Composition succeeds with no nickname and no completion date (§19.2).
- Rasterise via the SP-062 M1 path. Transient file only; do not accumulate
  card images in shared storage. Delete after share or on dismiss.
- Android V1 draws the card UI from the model. If M1 places rasterisation
  on Android, JNI still exposes the model (not a screenshot bitmap from
  Drape).
- Analytics increment for “card generated” is **SP-068** (this item may
  expose a one-line hook; it does not own the counter). Payload must not
  include area id.

## Out-of-scope behavior

- Opening the OS share sheet (SP-068).
- Sharing anything other than this card.
- Phase 8 leading / not-leading copy beyond the stub.
- Storing cards in the gallery by default.

## Relevant product requirements

- Spec §19.1–§19.2, §22.10, §25, §34 Sharing.
- SPD-008; SP-062 M1/M6/M7.

## Relevant source files or symbols

- `street_pixels::CompletionCardModel`, `ComposeCompletionCard`, rings raster
- `street_pixels::ExplorationArea::m_rings`, `m_name` / `DisplayName`
- `SharingUtils` (do not attach tracks)
- Drape / `area_overlay` (in-app chrome only; not the share bitmap)
- `area_completion_card.xml`, `CompletionCardOutlineView`

## Implementation notes / constraints

- Privacy is construction, not review: the model type must not *have*
  fields for route, lat/lon, pixel ids, or other users.
- Offline generation. No network to fetch a static map.
- Do not screenshot `MapView` / Drape surface.

## Acceptance criteria

1. Automated test: card model contains only permitted fields; deny-list
   fields are absent (type-level and/or serialisation-level).
2. Composition succeeds with no nickname and no date.
3. Image is produced from the locked M1 path (recommended: boundary rings),
   not from a live map capture.
4. No route, home, live location, or per-visit timestamp appears in the
   model or in a documented fixture render inspection.
5. Transient output: no unbounded accumulation in shared storage.
6. The card is available from the 100% surface (exit #4 generate-at-100%),
   not only from a hidden debug path.

## Required automated tests

- Deny-list assertion on the composed model (required by phase-07
  automated strategy).
- Missing nickname / missing date still produce a card.
- Rings-only geometry: fixture with a known rectangle/outline; model
  geometry equals that outline, not a GPS line.

## Required manual validation

- Generate a card for a fixture/real area; inspect the image against the
  exclusion list. Device residual → SP-069 / Phase 10.

## Failure and rollback considerations

- If M1 cannot be implemented without a screenshot, stop and escalate;
  do not ship a leaking card.
- Prefer no share image over a map capture.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-067-completion-card-compositor-c417` |
| Test output | `street_pixels_tests --filter=CompletionCard` **10/10**; `AreaMilestonePresentation` **14/14** |
| M1 path used | rings-only / `CompletionCardModel` (headless C++ stroke + Android Canvas from JNI model; never Drape / `MapView`) |
| Manual image inspection | Residual → SP-069 / Phase 10 |
| Accepted by | Maintainer |
| Accepted date | 2026-08-20 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Device review 2026-08-22: 100% card looks like a plain unstyled white sheet. It should feel like a celebration (confetti / game-like congratulations), not a dialog. | Visual polish of the in-app card chrome. Keep SPD-046: outline is still rings-only, never a live map screenshot. |
| Device review 2026-08-22: district outline reads as a blank empty square. `?achievements` feeds a dummy mercator unit square `{{0,0}…{0,0}}`; `CompletionCardOutlineView` strokes only (4 px black, no fill) on white. | Debug fixture should use a recognisable ring. Real cards: filled / styled outline so the boundary is readable, still from `m_rings`. |
