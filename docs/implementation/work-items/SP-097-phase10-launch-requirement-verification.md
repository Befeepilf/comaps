# SP-097 — Phase 10 launch-requirement verification

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088–096 implemented or explicitly residualled /
  waived by SPD. All other phases at exit. Release-configured
  installable build.
**Notes:** Exit gate. Maintainer decides Phase 10 exit and public V1
  go/no-go. Agent does not mark Accepted.

---

## Objective

Verify every product spec §34 line item with recorded evidence
attributable to a person, device, build, and date, and map Phase 10
exit criteria 1–11 to pass / fail / residual. Produce the S4 public
Android V1 evidence pack.

---

## Motivation

SP-089–096 each close a slice. Launch requires the §34 checklist in
one place so a missing bullet cannot hide in a residual table.
Earlier phase exit gates (SP-014, SP-022, SP-031, SP-041, SP-061,
SP-069, SP-079, SP-087) are citable evidence where still valid;
this item re-checks anything that drifted.

---

## In-scope behavior

- Validation plan + evidence log:
  `docs/implementation/validation/SP-097-validation-plan.md`,
  `SP-097-evidence-log.md`.
- Every §34 bullet (core map, recording, GPS, progress, routing,
  privacy/competition, offline/updates, sharing, Explorer Pro,
  release governance, quality) → pass / fail / residual, with
  pointer to SP-094/SP-095/automated SHA or a new observation.
- Phase 10 exit criteria 1–11 in the same log.
- Automated gate (H10 / README §8.1):
  - `./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke`
  - `street_pixels_tests` (and areas/routing as relevant)
  - `cd android && ./gradlew -Pandroidauto=true lint`
  - `./tools/unix/clang-format.sh`
  - Backend tests if explorer checkout is present
  - SP-091 payload-shape test
- Confirm no known path reveals another user’s live or exact
  location (direct API + UI).
- Confirm public build: no purchase, no ungated GPX, no friends
  surface, no city allowlist.
- Fresh-install §10 journey and offline-only session including
  routing (cite SP-090/SP-095 if already done on the same build).
- Defects found → owning WI or new SP-NNN; do not fix in this
  item except test-only harnesses needed to observe.

## Out-of-scope behavior

- New features.
- iOS, billing, post-V1 §35.
- Marking Phase 10 or S4 complete.
- Weakening tests.
- Performance work.

## Relevant product requirements

- Spec §33 (context only; hypotheses are not exit numbers),
  §34 (complete), §30–§32 as already covered by SP-090/091.
- All Phase 10 exit criteria in the phase file.
- SPD-001–076 plus H1–H10 SPDs.

## Relevant source files or symbols

- Entire Street Pixels surface; this item’s primary output is
  validation docs.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-041 / SP-061 /
  SP-069 / SP-079 / SP-087.
- Cite prior logs by SHA and build; re-run if the binary moved.
- Map screenshots of live position are forbidden in published
  evidence.

## Acceptance criteria

1. Plan and evidence log exist and are filled for executed
   scenarios.
2. Every §34 bullet and every Phase 10 exit criterion has
   pass/fail/residual.
3. Automated gate counts recorded.
4. Maintainer accepts Phase 10 exit or records blockers.

## Required automated tests

- Full list in In-scope. Record executed output only.

## Required manual validation

- Any §34 bullet not already evidenced on this build by SP-094 /
  SP-095.

## Failure and rollback considerations

- Failed exit criterion blocks public V1. Report owning WI.
- Partial residuals require an explicit waiver SPD, not silence.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | |
| Evidence log | |
| Test output | |
| Device roster | |
| Exit criteria table | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
