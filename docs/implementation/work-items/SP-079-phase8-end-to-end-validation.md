# SP-079 — Phase 8 end-to-end validation

**Phase:** 8 — Competition
**Status:** In review
**Branch:** `cursor/sp-079-phase8-end-to-end-validation-f95c`
**Depends on:** SP-070–078 implemented or explicitly residualled
**Notes:** Exit gate. Device residual → Phase 10 pattern (SP-014 / SP-041
  / SP-061 / SP-069). Maintainer decides Phase 8 exit; agent does not
  mark Accepted.

---

## Objective

Validate Phase 8 exit criteria with automated fixtures and documented
manual/device inspection. Produce a validation plan + evidence log.
Maintainer decides Phase 8 exit.

## Motivation

SP-071–078 each validate locally. Exit needs combined evidence: consent,
unique nicknames, aggregate-only delayed upload, ownership / eligibility /
decay / contested / unclaimed, weekly board without revisits/imports,
server sparse-area anonymity, nickname moderation, deletion, and no
presence leak.

## In-scope behavior

- Validation plan + evidence log under
  `docs/implementation/validation/`
  (`SP-079-validation-plan.md`, `SP-079-evidence-log.md`).
- Map each Phase 8 exit criterion (1–12) to pass / fail / residual.
- Re-run relevant automated suites; record counts. Minimum:
  `street_pixels_tests` (recency, ownership, eligibility, weekly,
  upload cadence, payload deny-list, card copy, 30-pixel hint) and
  backend competition tests when that checkout is present.
- Manual (phase-08 strategy): opt-in vs §20.2; traffic capture; opt-out
  zero upload; offline queue; N < 3 nicknames; decay without opening the
  app (server); delete profile / local exploration intact; no presence
  copy.
- Device residual honesty if no handset: Phase 10; do not fabricate.
- Backend-missing residual: record if `comaps_backend` is not in the
  environment; do not fake ingest tests.

## Out-of-scope behavior

- New features beyond defect fixes routed to owning SP-071–078.
- Marking Phase 8 Accepted or exit Met without maintainer decision.
- Weakening tests to pass the gate.
- Friends feature revival.

## Relevant product requirements

- Phase 8 exit criteria 1–12.
- Spec §20–§26, §22.10, §34 Privacy and competition.
- SPD-014, SPD-057–066.

## Relevant source files or symbols

- All Phase 8 competition modules; validation docs are this item’s
  primary output.

## Implementation notes / constraints

- Evidence-only discipline from SP-014 / SP-022 / SP-031 / SP-041 /
  SP-061 / SP-069.
- Desktop suites remain mandatory.

## Acceptance criteria

1. Validation plan and evidence log exist and are filled for executed
   scenarios.
2. Each Phase 8 exit criterion has pass/fail/residual.
3. Automated suites green; counts recorded.
4. Maintainer accepts Phase 8 exit or records residuals (incl. Phase 10
   / backend ops).

## Required automated tests

- Full relevant unit targets listed above.

## Required manual validation

- Phase-08 manual strategy on device if available; else explicit Phase 10
  residual. Traffic capture required for the upload deny-list if a device
  is available.

## Failure and rollback considerations

- Failed exit criterion blocks phase completion; report owning WI; do
  not weaken tests.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | [SP-079-validation-plan.md](../validation/SP-079-validation-plan.md) |
| Evidence log | [SP-079-evidence-log.md](../validation/SP-079-evidence-log.md) |
| Test output | Client SHA `b8fec313eacf28641e66b57b947a2c106cb6c804`. Explorer SHA `a2875770bc72b68917b58356d17adfb39af2ea10` on `cursor/sp-077-nickname-moderation-deletion-f95c`. `IdentityStore_|FriendsManager_|ExploreStatsUpload_` **31/31**; `BackendConfig_|ExploreStatsUpload_|FriendsManager_` **29/29**; `CompetitionUpload_` **22/22**; `CompetitionOwnership_` **18/18**; `CompetitionDeletion_` **2/2**; `CompetitionSnapshot_` **5/5**; `CompetitionHint_` **7/7**; `CompetitionHintCopy_` **2/2**; card/ranking/sparse/chrome/copy/hintcopy **15/15**; `WeeklyCityLive_` **12/12**; `Competition` **97/97**; areas `OwnershipScoring_|LiveRecency_|WeeklyCity` **37/37**; full `street_pixels_areas_tests` **151/151**; full `street_pixels_tests` **423/423** (classificator present; Eligibility OK); explorer `uv run pytest -q` **108 passed**, 4 warnings. Transcripts under `/opt/cursor/artifacts/sp079_*.log`. |
| Device roster / residual | `adb: command not found`. D1 Pixel 3a / D2 OEM / M1–M8 deferred → Phase 10. Map screenshots forbidden. |
| Exit criteria table | Evidence log — **1–12 Pass (automated) + Residual** (SP-071 not accepted on 1–3; device Phase 10; listed owning-item residuals). Not Fail. Not Accepted. |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| No handset: opt-in walk, traffic capture, opt-out zero upload, offline queue, `N < 3`, decay-without-app, delete+local intact, presence eyeball | Phase 10. Map screenshots remain forbidden. |
| SP-071 still In progress / not accepted | Maintainer; residual on exits 1–3 |
| friends_signup_* nickname toasts | Later copy cleanup; SPD-061 |
| Weekly GET not JNI-wired | Later client WI / Phase 10 |
| Boss haptic out | SPD-054 |
| CompetitionHint_ misses CompetitionHintCopy_ | Mitigated by extra filter (`sp079_street_pixels_hintcopy.log` 2/2) |
| Client 7-day-gates after admin reset | SP-077 follow-up still open |
| HTTP 409 rename_limited mapped to Collision | Residual on SP-077 |
| Failed POST /leave has no retry queue | Residual on SP-077 |
| V1 blocked-list small whole-token set | Residual |
| Unicode script-table vs server | Server remains authority |
| city_timezones.py empty dict | SP-076; SPD-060 keep |
| Postgres production deploy | Ops / Phase 10 |
| Exact EU region string | SPD-062 ops |
| QueryCompetitionOwnership loaded-span only | SP-072 / SP-074 |
| Revoke does not delete live_recency.db rows | SP-072 |
| Weekly crash window drops increment | SP-073 |
| Ever-live-flip vs newly-explored-only §24.1 | SP-073 product lock |
| /stats/upload leftover client symbol | Leave unused |
| classificator.txt Eligibility abort if absent | Not triggered this run (file present; Eligibility OK in full suite 423/423). Keep as environment residual if a later environment lacks the file. |
| Phase 8 exit Met? | Maintainer only |
