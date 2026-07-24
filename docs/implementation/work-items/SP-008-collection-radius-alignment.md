# SP-008 — Align collection radius with the specified 25 metres

**Phase:** 2 — Recording and collection correctness
**Status:** Not started
**Branch:** `street-pixels/SP-008-collection-radius-alignment`

---

## Objective

Change the exploration collection radius from 20 metres to the 25 metres the
product specifies, and confirm it is not user-configurable.

## Motivation

`libs/map/street_pixels_manager.cpp` defines
`kExploreRadiusMeters = 20.0`. Product spec §15.1 fixes the V1 radius at 25
metres and states it is not user-configurable, and §30 confirms the radius is
not exposed as a setting.

The gap matters more than it looks. The radius is the product's stated model of
what "exploring" means — the spec justifies 25 metres as practical visual and
spatial exploration rather than exact foot placement. A 20 % smaller radius
makes exploration measurably harder and makes 100 % area completion harder
still, which changes how the whole progression feels.

This is deliberately a separate work item from SP-007. Both change how many
pixels a walk collects. Keeping them apart means that when SP-007's paired
validation walks produce different pixel counts than before, the difference is
attributable to the gate and nothing else.

## In-scope behavior

- `kExploreRadiusMeters` becomes 25.0.
- The derived radius in radians follows automatically; confirm it does.
- Confirming there is no setting, JNI entry point, or debug affordance that
  changes the radius at runtime.
- A test asserting the boundary behaviour, so the constant cannot drift
  unnoticed.

## Out-of-scope behavior

- Derivation sampling distance (`kSegmentLengthMeters`, currently 15 metres
  against a specified 10). That changes the pixel universe and every
  denominator, so it belongs in Phase 3 with the reconciliation machinery.
- The separate hardcoded 10-metre sampling in `UpdateStreetStatsForTrack`.
  Also Phase 3.
- Any other constant.
- Interpolation radius behaviour, since interpolation does not exist yet.
  SP-011 must use the same radius when it lands.
- Rendering radius, which is a display concern with its own per-zoom table.

## Relevant product requirements

- §15.1 The V1 exploration radius is fixed at 25 metres; any valid street pixel
  within 25 metres of an accepted live location or accepted interpolated
  segment may be collected; the radius is not user-configurable in V1.
- §30 The 25-metre exploration radius is not configurable in V1; internal
  HEALPix, GPS-filter, ownership-decay, and scoring parameters are not exposed
  as ordinary user settings.
- §34 "Core map and exploration": the 25-metre collection radius behaves
  consistently.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.cpp`: `kExploreRadiusMeters`, `kRadiusRads`,
  and `kEarthRadiusMeters`
- `libs/map/street_pixels_manager.cpp`: `AddPixelsInRadius`, the consumer
- `android/sdk/src/main/java/app/organicmaps/sdk/Framework.java`, checked to
  confirm no JNI entry point exposes the radius

## Dependencies

- SP-007, so that the paired validation walks for the gate are already recorded
  against the old radius.
- SP-002, for the test.

## Proposed implementation approach

1. Change the constant to 25.0.
2. Verify the radians conversion is derived rather than separately hardcoded.
3. Grep the tree for other appearances of 20 metres in an exploration context,
   in C++, Java, and any JNI surface.
4. Confirm no settings key, JNI method, or debug menu can change it.
5. Add a boundary test in the SP-002 target.
6. Measure the practical effect on one fixture: how many more pixels a fixed
   synthetic track collects at 25 metres versus 20. Record the number; it is
   useful context for anyone later comparing exploration data across builds.

## Acceptance criteria

1. The collection radius is 25 metres.
2. The radians value is derived from the metre constant, not separately
   maintained.
3. No runtime path can change the radius.
4. A test asserts collection at just under 25 metres and no collection at just
   over.
5. The pixel-count effect on a fixture track is measured and recorded.
6. Nothing else changed.

## Required automated tests

In the SP-002 target:

- A synthetic pixel at 24.0 metres from an accepted location is collected.
- A synthetic pixel at 26.0 metres is not collected.
- A synthetic pixel at 22.0 metres, which the old radius would have excluded,
  is now collected. This is the test that would have failed before the change.
- The radians conversion matches the metre value to a reasonable tolerance.

Boundary tests exactly at 25.0 metres are avoided, since floating-point
distance on a spherical calculation makes the exact boundary unstable and not
worth asserting.

## Required manual validation

- Walk a route with a pavement on one side and a parallel path 20 to 25 metres
  away. Confirm the parallel path now collects where it previously did not.
- Confirm no obviously wrong pixel is collected: crossing a bridge should not
  turn the road underneath green if that road is more than 25 metres away, but
  it may if it is closer, which is expected and specified behaviour.
- Confirm the app performs the same; a 25 % larger radius means more HEALPix
  cells queried per update. Watch for any responsiveness change during a
  session.

Record device, route, and observations.

## Failure and rollback considerations

- A larger radius collects more pixels per update, including some the user may
  feel they did not explore — for example, a parallel service road. This is
  the specified product behaviour and the spec explains the reasoning. It is
  not a defect.
- The wider query costs more per location update. Unlikely to matter given the
  update cadence, but confirm during manual validation.
- Once collected, pixels are permanent. A device that runs the 25-metre build
  and is then rolled back keeps the extra pixels. That is correct — exploration
  does not un-happen — but it means the change is not cleanly reversible in
  data, only in code.
- Rollback is a one-line revert.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commit | |
| Other 20-metre occurrences found | |
| Confirmation that no runtime path changes the radius | |
| Test output | |
| Fixture pixel-count delta, 20 m versus 25 m | |
| Manual walk observations | |
| Test device model and OS version | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
