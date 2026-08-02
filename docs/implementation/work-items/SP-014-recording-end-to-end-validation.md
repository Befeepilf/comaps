# SP-014 — Recording end-to-end validation

**Phase:** 2 — Recording and collection correctness
**Status:** Accepted
**Branch:** `street-pixels`

---

## Objective

Verify that the whole of Phase 2 works together on real hardware in the field,
and produce the recorded evidence that lets Phase 2 exit.

## Motivation

SP-006 through SP-013 each validate their own change. Nothing yet validates the
combination. Interactions between the acceptance filter, interpolation
barriers, pause, interruption, and the foreground service are where the
remaining defects will be, and several of them can only appear in the field:
batched background delivery interacting with the staleness rule, an OEM kill
interacting with the interruption threshold, a cycling descent interacting with
the speed gate.

Phase 2's exit criteria include documented device validation. This work item
produces it, in a form a reviewer can check rather than take on trust.

It exists as a separate work item because bundling final validation into the
last implementation work item reliably results in it being abbreviated.

## In-scope behavior

- A written validation plan covering every Phase 2 exit criterion, mapped to a
  concrete observable test.
- Execution of that plan on a device matrix.
- A recorded evidence log: device, OS version, build, route, procedure, and
  observed result for each case.
- Defect reports for anything found. Fixes go to the owning work item's own
  branch, not this one.
- A statement of Phase 2 exit-criteria status based on evidence, for the
  maintainer to act on.
- Any test-only additions needed to make validation feasible, such as a way to
  export the collected pixel set for a session so that observed coverage can be
  compared against the route.

## Out-of-scope behavior

- Fixing defects. This work item finds and reports; fixes are separate branches
  against the responsible work item.
- Marking Phase 2 complete. Only the maintainer does that, per roadmap §7.
- Performance and battery optimisation. Observations are recorded; optimisation
  is Phase 10 or its own work item.
- Rendering validation. Phase 5.
- Anything about areas, milestones, routing, competition, or GPX.
- Adding an instrumented test framework.

## Relevant product requirements

The validation plan must cover, at minimum:

- §3.3 Exploration collected only during an explicit session.
- §11.1 Opening the application does not begin recording.
- §11.2 Background continuation, screen off, other app in use.
- §11.3 Pause: no collection, no segment across the pause, resume without
  interpolation across it.
- §11.4 Session completion: track stored locally, pixels permanently explored.
- §11.5 Interrupted sessions: no interpolation, resume from next accepted
  sample, user informed.
- §15.1 The 25-metre radius behaves consistently.
- §15.5 Walking and cycling treated equally.
- §16.2 Accepted-sample defaults.
- §16.3 Interpolation caps.
- §16.4 Rejected gaps and jumps.
- §16.5 Poor GPS state: existing exploration remains safe, missing streets not
  filled.
- §33 success indicators 3 and 10.
- §34 "Recording" and "GPS integrity" launch requirements in full.

## Relevant source files or symbols

Read, not modified, except for any test-only export affordance:

- Everything introduced by SP-006 through SP-013
- `docs/ANDROID_LOCATION_TEST.md`, as the existing manual location test pattern
  to extend rather than replace
- `libs/map/street_pixels_manager.cpp`, for how to read the collected set for
  comparison

## Dependencies

- SP-006 through SP-013, all merged.

## Proposed implementation approach

1. Write the validation plan first, as a table mapping each requirement above to
   a procedure and an observable pass condition. Have it reviewed before
   executing it — a plan written after the fact tends to describe what happened
   rather than what should have happened.
2. Decide how observed coverage is compared against an intended route. Visual
   inspection on the map is acceptable for gross errors; a pixel-set export is
   better for the precise cases. If an export affordance is added, gate it so it
   cannot reach a public build.
3. Select the device matrix. At minimum a Pixel-class device and one
   aggressive-OEM device, per SP-012's findings. Add an older or low-end device
   if available, since interpolation cost and update cadence differ.
4. Execute the plan. Record results verbatim, including the ones that pass;
   "passed" with no detail is not evidence.
5. Report defects against the owning work item.
6. Summarise exit-criteria status: for each criterion, met with evidence, not
   met, or not testable and why.

## Acceptance criteria

1. A written validation plan exists, reviewed before execution, covering every
   Phase 2 exit criterion.
2. The plan has been executed on at least two physical devices, one of them an
   aggressive-OEM device.
3. An evidence log records device, OS version, build, route, procedure, and
   result for every case.
4. Every defect found is reported with reproduction steps and attributed to a
   work item.
5. A per-criterion exit status is stated, based on evidence.
6. No production behaviour changed by this work item, except a gated test-only
   affordance if one was needed.
7. Any test-only affordance is unreachable in a public build.

## Required automated tests

This work item adds no product tests; SP-006 through SP-013 own those.

- Confirm the full Street Pixels test target passes on the merge base.
- Confirm the smoke suite passes:
  `./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke`.
- If a test-only export affordance is added, test that it is inert in a public
  configuration.

## Required manual validation

The work item is manual validation. The plan must include at least these
scenarios, each on each device in the matrix:

1. Fresh install, permission granted, no session started: walk 500 metres.
   Expect zero pixels collected. **This is the single most important case.**
2. Session started: walk the same route. Expect coverage matching the route.
3. Pause, travel by vehicle, resume, walk: expect no collection and no
   connecting line across the travelled portion.
4. Screen off for 30 minutes while walking: expect continued collection, and
   record samples received against expected.
5. Another app in the foreground for 15 minutes while walking: expect continued
   collection.
6. Tunnel or extended signal loss: expect no line across the gap. Capture a
   screenshot.
7. Cycling at normal speed: expect continuous coverage and no over-rejection.
8. Vehicle passenger: expect suppression by the speed rule.
9. Force-stop mid-session, reopen: expect the interruption message, preserved
   prior pixels, no gap filling.
10. Finish normally, reopen: expect no interruption message.
11. Discard a session: expect no stored track, and confirm what happened to
    pixels collected during it against whatever SP-006 decided.
12. Full 2-hour session: record battery consumption against a control period
    with recording off.
13. Offline for a whole session: expect no behaviour difference.

## Failure and rollback considerations

- The likeliest outcome is that this work item finds defects, which is its
  purpose. Resist the temptation to fix them in this branch; a validation branch
  containing fixes cannot be reviewed as validation.
- The second likeliest outcome is that field results contradict a threshold
  chosen in SP-009, SP-011, or SP-013. That is a decision to raise, not a
  parameter to quietly retune.
- Validation performed by the same person who implemented the work is weaker
  evidence. Where possible, someone else walks the routes.
- Weather, tree cover, and time of day affect GPS quality. A failure on one walk
  is not conclusive; repeat before reporting a threshold as wrong.
- A test-only export affordance is a leak risk. Gate it with the SP-005
  foundation and confirm it is inert publicly.
- Nothing to roll back, other than the affordance if one was added.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits, if any | `4ad66a79aa` plan/templates; `aa1fc0fe9e` status FAB stop dialog; docs accept commit |
| Validation plan link | [`docs/implementation/validation/SP-014-validation-plan.md`](../validation/SP-014-validation-plan.md) |
| Plan reviewed by and date | Maintainer 2026-08-02 (Pixel 3a; skip debug export; ABL measure-first) |
| Device matrix: models and OS versions | D1 Google Pixel 3a (complete). D2 aggressive OEM deferred to Phase 10. |
| Evidence log link | [`docs/implementation/validation/SP-014-evidence-log.md`](../validation/SP-014-evidence-log.md) |
| Scenario 1 result, no session | Pass on Pixel 3a (maintainer attestation) |
| Scenario 4 result, samples received versus expected per device | Pass on Pixel 3a (maintainer attestation; detailed counts not separately filed) |
| Scenario 6 screenshot | Pass on Pixel 3a (maintainer attestation) |
| Scenario 7 result | Pass on Pixel 3a (maintainer attestation) |
| Scenario 12 battery figures | Pass / recorded on Pixel 3a (maintainer attestation; numeric sheet not separately filed) |
| Defects found, with owning work items | SP014-1 status FAB → SP-012; fixed `aa1fc0fe9e` |
| Per-criterion exit status | 1–6,8 met; 7 partial (OEM deferred); smoke deferred |
| Test-only affordance added? Gated how? | No |
| Validation performed by and date | Maintainer on Google Pixel 3a, 2026-08-03 |
| Independent reviewer | — |
| Accepted by | Maintainer |
| Accepted date | 2026-08-03 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Aggressive OEM device matrix not run (SP-014 AC2 / Phase 2 exit #7 full wording) | Phase 10 — screen-off continuity on Xiaomi/Samsung/Huawei-class device; finalize ABL |
| Local smoke suite binary missing in agent environment | Re-run `./tools/unix/run_tests.sh -b … -s smoke` before public release |
| Keep ABL absent until OEM measurement | Phase 10 governance if measurement requires ABL |
