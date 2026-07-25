# SP-011 — Segment interpolation with pause and interruption barriers

**Phase:** 2 — Recording and collection correctness
**Status:** Not started
**Branch:** `street-pixels`

---

## Objective

Collect pixels along the path between two consecutive accepted samples when the
gap between them is small enough to be trustworthy, and never across a gap that
is not — including every gap caused by a pause, an interruption, or a rejected
sample.

## Motivation

There is no interpolation in the live pixel path today. Collection happens in a
disc around each accepted fix and nowhere else. At a walking pace with frequent
updates that mostly works, but on a bicycle, or whenever the platform delivers
samples less often, it leaves gaps: the user rides down a street and gets a
dotted line of green rather than a street.

Product spec §16.3 defines interpolation and its caps, and §15.1 says pixels
within 25 metres of an accepted interpolated segment may be collected. So
interpolation is required, not optional.

This is where the originally suggested work item "no interpolation across
paused or interrupted periods" lands — but it had to be split, because there is
nothing to prohibit until interpolation exists. Building the capability and its
prohibitions in the same work item is correct: they are the same mechanism seen
from two sides, and shipping the capability without the barriers would create
exactly the false-exploration failure the product promises to avoid.

## In-scope behavior

Interpolate between two consecutive accepted samples when all of the following
hold, per spec §16.3:

- Both samples pass the SP-009 acceptance filter.
- The time gap is no more than 30 seconds.
- The distance is no more than 200 metres.
- The implied speed is no more than 50 km/h.
- Neither sample follows a pause, an interrupted session, or a rejected jump.

Also in scope:

- Collecting pixels within 25 metres of the valid interpolated segment.
- Barriers: after a pause, after a resume, after an interruption, after a
  rejected sample, and at session start, the next accepted sample begins a new
  interpolation origin with no segment drawn to it.
- Caps as named constants matching the spec.

## Out-of-scope behavior

- Changing the acceptance filter. SP-009 owns it.
- Changing pause semantics. SP-010 owns them; this work item consumes the
  barrier.
- Interruption detection. SP-013 owns it; this work item consumes the barrier.
  The two work items must agree on the barrier mechanism, so whichever lands
  second adapts rather than adding a parallel one.
- Interpolating the recorded track itself. The track stores accepted samples;
  interpolation is a collection concern, not a storage concern. If the track
  visually needs connecting, that is a rendering question and is out of scope.
- GPX import interpolation. Phase 9; the spec is explicit that historical
  timestamps and sparse points differ and must not blindly reuse live rules.
- The existing `extrapolator.cpp` display extrapolation, which is unrelated.

## Relevant product requirements

- §16 opening statement: the application must never draw a continuous explored
  line across a long loss of location data.
- §16.3 Interpolation and its five conditions.
- §16.4 Rejected gaps and jumps: no connecting line when the time gap exceeds
  30 seconds, the distance exceeds 200 metres, the implied speed exceeds
  50 km/h, either sample has unacceptable accuracy, recording was paused,
  background tracking was interrupted, or the location jumps after signal loss.
  The new accepted point becomes the starting point for future interpolation.
  Pixels may be collected around the new point itself, but never along the
  invalid gap.
- §15.1 Any valid street pixel within 25 metres of an accepted live location or
  accepted interpolated segment may be collected.
- §16.5 Missing streets are not automatically filled.
- §33 success indicator 10: GPS loss never creates kilometre-long false
  exploration lines.
- §34 "GPS integrity": normal valid samples are interpolated safely; signal
  loss does not paint a straight explored line.

## Relevant source files or symbols

- The acceptance filter from SP-009, including its rejection reason and its
  reference-point reset
- The session state from SP-006 and the pause barrier from SP-010
- `libs/map/street_pixels_manager.cpp`: `AddPixelsInRadius`, and
  `SegmentizeStreet` as an existing example of walking a segment at a fixed
  step
- `libs/map/street_pixels_manager.cpp`: `kExploreRadiusMeters`, now 25 after
  SP-008
- `libs/geometry/`, for distance and interpolation along a great circle

## Dependencies

- SP-009 for accepted endpoints.
- SP-010 for the pause barrier.
- SP-008, so interpolation uses the correct radius from the start.

## Proposed implementation approach

1. Decide the collection geometry. Two options: sample the segment at fixed
   intervals and collect a disc at each sample, or compute the set of HEALPix
   cells within 25 metres of the segment directly. The spec describes the
   latter; the former is simpler and already has a precedent in
   `SegmentizeStreet`. If sampling, choose a step small enough that the discs
   overlap — with a 25-metre radius, a step meaningfully below 25 metres leaves
   no holes. Record the choice and the step.
2. Implement the five conditions as an explicit predicate, evaluated before any
   collection.
3. Implement the barrier as a single flag or reason carried alongside the
   previous accepted sample, meaning "this sample cannot be an interpolation
   origin". Set it at session start, after pause, after resume, after
   interruption, and after any rejection. Coordinate the mechanism with SP-010
   and SP-013 rather than adding a second one.
4. Collect pixels along the segment when interpolation is allowed, and around
   the point only when it is not.
5. Verify cost. A 200-metre segment at a small step is many HEALPix queries per
   update; measure it against the update cadence.

## Acceptance criteria

1. Two accepted samples 20 seconds and 100 metres apart produce collection
   along the connecting path.
2. Samples 40 seconds apart produce no connecting collection; the second point
   still collects its own disc.
3. Samples 300 metres apart produce no connecting collection.
4. Samples implying more than 50 km/h produce no connecting collection.
5. The first accepted sample after a pause produces no connecting collection to
   the pre-pause sample.
6. The first accepted sample after a resume produces no connecting collection.
7. The first accepted sample after an interruption produces no connecting
   collection.
8. The first accepted sample after a rejected sample produces no connecting
   collection to either the rejected sample or the one before it.
9. The first accepted sample of a session produces no connecting collection.
10. Interpolated collection uses the same 25-metre radius as point collection.
11. Per-update cost is measured and acceptable at the platform's update cadence.

## Required automated tests

In the SP-002 target. The barrier tests are the important ones; write them
first.

- Within all caps: connecting pixels collected, and the collected set matches
  the expected cells for the fixture geometry.
- Time gap just under and just over 30 seconds.
- Distance just under and just over 200 metres.
- Implied speed just under and just over 50 km/h.
- Barrier after pause, after resume, after interruption, after rejection, and
  at session start — five separate tests, each asserting that no cell between
  the two points is collected while the endpoint's own disc is.
- A long invalid gap collects nothing along its length, expressed as: for a
  10-kilometre gap, the number of collected cells is bounded by what two discs
  can contain.
- A realistic synthetic cycling sequence produces continuous coverage rather
  than a dotted line.

## Required manual validation

- Cycle a straight street at normal speed and confirm continuous green coverage
  rather than dots.
- Walk a route and confirm coverage is continuous and not wider than expected.
- Walk through a tunnel or under heavy cover, losing signal for several
  minutes, and confirm no line appears across the gap. Photograph or screenshot
  the result; this is the single most important piece of evidence in Phase 2.
- Pause, travel by vehicle, resume; confirm no connecting line. This repeats
  SP-010's bus test with interpolation now enabled, which is when the failure
  would actually manifest.
- Confirm no responsiveness or battery regression during an extended session.

## Failure and rollback considerations

- **The catastrophic failure is a missing barrier.** A single unguarded path
  produces kilometre-long false exploration that is permanent and visible, and
  it directly contradicts a stated success indicator. This is why every barrier
  has its own test.
- The barrier mechanism is shared with SP-010 and SP-013. If each work item
  adds its own flag, one will eventually be missed. Insist on a single
  mechanism.
- Interpolation increases per-update work substantially. On a slow device with
  frequent updates this could affect responsiveness or battery. Measure.
- Too coarse a sampling step leaves holes in coverage, which looks like a bug to
  the user; too fine wastes work. Choose deliberately and record the step.
- Interpolation makes collection more generous, so any acceptance-filter
  weakness is amplified. If SP-009's thresholds are wrong, this is where it
  shows.
- Rollback is a revert, but pixels collected by faulty interpolation are
  permanent. If a barrier defect ships and reaches real devices, the follow-up
  is a data question, not just a code fix — which is another reason for the
  device validation above to happen before merge.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Collection geometry chosen and step size | |
| Barrier mechanism, shared with SP-010 and SP-013 | |
| Constants and values | |
| Test output, including all five barrier tests | |
| Cycling route: coverage continuity | |
| Signal-loss test: gap length and result, with screenshot | |
| Pause-and-travel test result | |
| Per-update cost measurement | |
| Battery observation over an extended session | |
| Test device models and OS versions | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
