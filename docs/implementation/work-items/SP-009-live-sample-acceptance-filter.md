# SP-009 — Live sample acceptance filter

**Phase:** 2 — Recording and collection correctness
**Status:** In progress
**Branch:** `street-pixels`

---

## Objective

Reject location samples that are not good enough to prove the user was
somewhere, before they can turn a pixel green.

## Motivation

`StreetPixelsManager::OnLocationUpdate` performs no quality checks at all. It
takes whatever fix arrives and collects every pixel in range. There is no
accuracy check, no staleness check, no speed check, and no teleport check.

`GpsTrackFilter` exists and does some of this — but only for the recorded
track, not for pixel collection, and with a minimum horizontal accuracy of 250
metres, which is ten times looser than the spec's 25-metre gate. It also uses
an acceleration heuristic rather than the spec's implied-speed rule.

Product spec §16 opens by stating the application must never draw a continuous
explored line across a long loss of location data, and §16.2 lists the
acceptance defaults. Success indicator 10 in §33 is that GPS loss never creates
kilometre-long false exploration lines. Without this filter, a bad urban fix or
a ride in a car produces exploration the user did not earn, and once a pixel is
green it stays green.

This must land before SP-011, because interpolation needs defined accepted
endpoints.

## In-scope behavior

A sample acceptance filter applied to the live collection path, implementing
spec §16.2:

- Reject when reported horizontal accuracy is worse than 25 metres.
- Reject when the sample is stale.
- Reject when the operating system has marked it invalid.
- Reject when the implied speed from the previous accepted sample exceeds
  50 km/h.
- Reject implausible teleports.

Also in scope:

- Tracking the previous accepted sample per session, so implied speed is
  computable and resets correctly across sessions.
- Thresholds as named compile-time constants, matching spec defaults.
- A rejection reason available to callers, so SP-013 and the poor-GPS state can
  distinguish "waiting for accuracy" from "rejected as implausible".

## Out-of-scope behavior

- Interpolation. SP-011.
- Pause and interruption barriers. SP-010 and SP-013, though this filter must
  make the previous-accepted-sample state resettable so those work items can
  use it.
- Changing `GpsTrackFilter` or the recorded-track path. The spec's stricter
  rules are for exploration collection. Whether to also tighten the track
  filter is a separate question; record it as follow-up.
- The "Waiting for accurate location" UI state. That is Phase 5 or Phase 10 UI
  work; this work item only exposes the state.
- Making thresholds remotely configurable. The audit recommends shipping spec
  defaults as compile-time constants and allowing configuration only after
  field evidence.
- Retaining or logging raw rejected samples in release builds.

## Relevant product requirements

- §16.1 Competitive exploration is generated only from live location samples
  recorded during an active session.
- §16.2 Accepted sample defaults: accuracy 25 metres or better; not stale; not
  marked invalid; implied speed no more than 50 km/h; no implausible teleport.
- §16.4 Rejected gaps and jumps, including the rule that the new accepted point
  becomes the starting point for future interpolation, and that pixels may be
  collected around the new point but never along the invalid gap.
- §16.5 Poor GPS state: existing exploration remains safe; the application may
  show "Waiting for accurate location"; exploration resumes from the next
  accepted point; missing streets are not automatically filled.
- §15.5 Walking and cycling are treated equally, which constrains how
  aggressive the speed rule can be.
- §34 "GPS integrity" launch requirements.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.cpp`, `StreetPixelsManager::OnLocationUpdate`
- `libs/platform/location.hpp`, `location::GpsInfo` — available fields include
  timestamp, latitude, longitude, horizontal and vertical accuracy, altitude,
  bearing, speed, and source
- `libs/map/gps_track_filter.{hpp,cpp}` — read for its existing heuristics:
  `kMinHorizontalAccuracyMeters` at 250, 10-metre close-point decimation,
  2 m/s² acceleration limit, direction check, `HasSpeed()` requirement,
  predictor-point rejection
- The session module from SP-006, for per-session state
- `libs/geometry/distance_on_sphere.hpp` or the equivalent already used by
  `AddPixelsInRadius`, for distance computation

## Dependencies

- SP-007, so the filter applies only within a session.
- SP-002, for the tests.

## Proposed implementation approach

1. Add a filter component holding per-session state: the previous accepted
   sample and its timestamp. Keep it dependency-light and testable.
2. Implement the rules in cheapest-first order: OS-invalid, accuracy,
   staleness, then the pairwise implied-speed and teleport checks that need the
   previous sample.
3. Define "stale" explicitly. `GpsInfo` carries a timestamp; decide the maximum
   acceptable age against the current time and write down the reasoning. Note
   that a background-delivered batch of samples may legitimately be older than
   a foreground one.
4. Decide whether to use the reported `m_speed` or the speed implied by
   distance over time. The spec says "the implied speed from the previous
   accepted sample", so compute it; the reported speed may be used as a
   corroborating signal but should not replace the computation.
5. Define "implausible teleport" concretely. The implied-speed rule already
   catches most cases; a teleport is what remains when the time gap is large
   enough that even a high speed looks legal. Document the rule chosen.
6. Reset the previous-accepted-sample state on session start, on resume, and on
   any interruption, and expose that reset so SP-010 and SP-013 can call it.
7. Apply the filter before collection. A rejected sample collects nothing.
8. Expose the rejection reason.

## Acceptance criteria

1. A sample with horizontal accuracy worse than 25 metres collects nothing.
2. A sample with no accuracy information collects nothing.
3. A stale sample collects nothing, using the documented staleness rule.
4. A sample the OS marks invalid collects nothing.
5. A sample implying more than 50 km/h from the previous accepted sample
   collects nothing.
6. A teleport collects nothing, using the documented rule.
7. An accepted sample collects normally, and becomes the new reference point.
8. Rejection does not corrupt the reference point: after a rejection, the next
   accepted sample is compared against the last *accepted* sample, per spec
   §16.4.
9. Thresholds are named constants matching the spec.
10. A rejection reason is available to callers.
11. Normal walking and normal cycling are not over-rejected, verified in the
    field.

## Required automated tests

One test per rule, each with a boundary pair, in the SP-002 target:

- Accuracy 24 m accepted; 26 m rejected; missing accuracy rejected.
- Staleness just inside and just outside the threshold.
- OS-invalid sample rejected.
- Implied speed at roughly 45 km/h accepted; at roughly 55 km/h rejected.
- Teleport rejected.
- After a rejection, the next sample is evaluated against the last accepted
  sample, not against the rejected one.
- Session start resets the reference point, so the first sample of a session is
  never rejected for implied speed.
- A realistic synthetic walking sequence is fully accepted.
- A realistic synthetic cycling sequence at 25 km/h is fully accepted.
- A synthetic sequence with one bad urban fix in the middle rejects exactly
  that one sample.

## Required manual validation

Field testing is mandatory; the thresholds are the whole point and cannot be
validated synthetically.

- Walk a route in an open area and confirm collection is uninterrupted.
- Walk a route in an urban canyon or between tall buildings and confirm bad
  fixes are rejected without leaving large false gaps in legitimate coverage.
- Cycle a route at normal speed and confirm nothing legitimate is rejected.
  This is the main over-rejection risk.
- Ride as a passenger in a car along a road and confirm the speed rule
  suppresses collection.
- Travel through a tunnel and confirm no exploration appears where there was no
  signal.
- Record, for each route, how many samples were accepted and rejected and with
  which reasons, so the audit's spike 5 pass criteria — under 1 % false urban
  teleports and under 5 % missed legitimate outdoor bike segments — can be
  evaluated.

## Failure and rollback considerations

- **Over-rejection is the main risk.** A cyclist descending a hill can exceed
  50 km/h. A tightened accuracy gate can drop legitimate samples under tree
  cover. Both make the product feel broken in a way that is hard to attribute.
  Measure before concluding the thresholds are right.
- Under-rejection lets false exploration through, and it is permanent.
- A staleness rule that is too strict can reject legitimate batched background
  samples, which would silently break background recording — the exact thing
  SP-012 and SP-013 are trying to make reliable. Test with the screen off.
- If field evidence contradicts the spec defaults, do not quietly retune. Record
  the evidence and raise it as a decision. Spec §16.2 says the defaults may be
  adjusted through field testing, so adjustment is legitimate — but it is a
  documented change, not a silent one.
- Rollback is a revert. Pixels already collected stay collected.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | Pending human review (not committed) |
| Staleness rule chosen and rationale | Reject when `nowSec − m_timestamp > 120 s` (`kMaxSampleAgeSeconds`). Also reject non-monotonic or zero timestamps vs previous accepted. 30 s (interpolation gap) is too tight for OEM/screen-off batching; 120 s still rejects last-known-location hours later. Mandatory screen-off field check before acceptance. |
| Teleport rule chosen and rationale | Reject when distance from last accepted sample exceeds 200 m (`kMaxJumpMeters`), matching spec §16.3/§16.4 gap cap. Catches long slow jumps that pass the implied-speed check (e.g. tunnel exit kilometres away with a long Δt). Classified before implied-speed when both could apply. |
| Constants and values | `kMaxHorizontalAccuracyMeters = 25.0`; `kMaxSampleAgeSeconds = 120.0`; `kMaxImpliedSpeedMps = 50.0 / 3.6`; `kMaxJumpMeters = 200.0` in `libs/map/live_sample_acceptance_filter.hpp` |
| Test output | `street_pixels_tests` 71/71 passed (2026-07-27 local build) |
| Open-area walk: accepted / rejected counts | Pending field validation |
| Urban-canyon walk: accepted / rejected counts | Pending field validation |
| Cycling route: accepted / rejected counts | Pending field validation |
| Vehicle-passenger result | Pending field validation |
| Tunnel result | Pending field validation |
| Screen-off batched-sample result | Pending field validation |
| Test device models and OS versions | Pending field validation |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
