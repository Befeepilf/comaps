# SP-097 — Phase 10 launch-requirement verification

**Phase:** 10 — Android release hardening
**Status:** Automated mapping recorded 2026-08-29. Device Residual.
  Brand Residual. Not Accepted. Phase 10 exit not met.
  **Device/manual hardware: Residual**
**Depends on:** SP-088–096 implemented or explicitly residualled /
  waived by SPD. All other phases at exit. Release-configured
  installable build.
**Notes:** Exit gate. Maintainer decides Phase 10 exit and public V1
  go/no-go. Agent does not mark Accepted. **Automated suites + evidence
  mapping are in scope.** Device/manual §34 observations that need a
  handset are residual (**SPD-077**, **SPD-078**, **SPD-083**,
  **SPD-086**). Do not mark Phase 10 exit met.

---

## Objective

Verify every product spec §34 line item with recorded evidence
attributable to a person, device, build, and date **where evidence
exists**, and map Phase 10 exit criteria 1–11 to pass / fail /
residual. Produce the S4 public Android V1 evidence pack as far as
automated suites and docs mapping allow.

Device/manual hardware observations are residual. Do not mark
Phase 10 exit met.

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
  Device/manual hardware bullets stay **residual** unless already
  evidenced without a new handset walk.
- Phase 10 exit criteria 1–11 in the same log. Do **not** mark
  Phase 10 exit met.
- Automated gate (H10 / **SPD-086** / README §8.1):
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
  Hardware execution of those journeys is residual.
- Defects found → owning WI or new SP-NNN; do not fix in this
  item except test-only harnesses needed to observe.

## Out-of-scope behavior

- New features.
- iOS, billing, post-V1 §35.
- Marking Phase 10 or S4 complete.
- Executing H1 device-matrix walks, H2 battery/Spike 1 on a handset,
  SP-095 walks, or other hardware observations (residual).
- Brand writing (app name, listing copy, privacy/terms URLs;
  SP-093 residual).
- Weakening tests.
- Performance work.

## Relevant product requirements

- Spec §33 (context only; hypotheses are not exit numbers),
  §34 (complete), §30–§32 as already covered by SP-090/091.
- All Phase 10 exit criteria in the phase file.
- SPD-001–076 plus H1–H10 **SPD-077–086**. Brand and on-device
  testing residuals as recorded in SP-088.

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
   pass/fail/residual. Device/manual hardware stays residual.
3. Automated gate counts recorded (**SPD-086**).
4. Maintainer accepts Phase 10 exit or records blockers. Agent does
   not mark Phase 10 exit met.

## Required automated tests

- Full list in In-scope. Record executed output only.

## Required manual validation

- Any §34 bullet not already evidenced on this build by SP-094 /
  SP-095. **Hardware execution is residual**; do not walk a
  handset in this item.

## Failure and rollback considerations

- Failed exit criterion blocks public V1. Report owning WI.
- Partial residuals require an explicit waiver SPD, not silence.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | [`validation/SP-097-validation-plan.md`](../validation/SP-097-validation-plan.md) |
| Evidence log | [`validation/SP-097-evidence-log.md`](../validation/SP-097-evidence-log.md) |
| Test output | SHA `c9336737a3e085275e7806317774c98ea2808542`. `street_pixels_tests` **499/499** All tests passed (Eligibility ran). Payload-shape **1/1**. Official smoke **exit=1** (`indexer_tests`/`map_tests`/`mwm_tests` abort on missing `World.mwm`; `platform_tests` missing after glaze compile fail; `routing_tests`/`search_tests` not reached). Separate `routing_tests` **307/307**. Separate `search_tests` abort on `World.mwm`. `:sdk:lintDebug` **FAILED** — **5 errors, 24 warnings**. `clang-format.sh` **exit=123** (clang-format-18 cannot parse `LeftWithLastLine`; CI uses 20) — **Residual env**, not a source-format Fail. Explorer friends-only; no backend tests. Logs under `/opt/cursor/artifacts/sp097_*.log`. Independent review 2026-08-29: §34 **Pass 48 / Residual 21 / Fail 0**; R6 inspect/delete Residual; clang-format Residual. |
| Device roster | Residual (SPD-077 matrix defined; not executed). Do not copy Pixel 3a as Phase 10 D2. |
| Exit criteria table | Evidence log: 1 Residual (21 Residual bullets); 2 Residual; 3 Residual; 4 Pass; 5 Residual; 6 Residual; 7 Residual; 8 Residual; 9 Residual; 10 Pass (SP-096 table, not Accepted); 11 Residual. Phase 10 exit **not met**. |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Independent review (2026-08-29): R6 was an over-claimed Pass (inspect/delete is device UI; `map_tests` aborted); clang-format labeled Fail despite 18-vs-20 env; tally 49/20 vs Exit 1 “21 Residual” | Mapping corrected: R6 Residual; clang-format Residual env; tally **48 Pass / 21 Residual / 0 Fail**. Tests not weakened. |
| Official smoke not green: missing `data/World.mwm` / `WorldCoasts.mwm` aborts `indexer_tests` (`CitiesBoundaries_Compression`), `map_tests` (`Bookmarks_Sorting`), `mwm_tests` (`ForEachFeatureID_Test`), and separate `search_tests` (`LocalityFinderTest_Smoke`) | Environment residual. Do not invent World maps. Do not skip tests. Later WI / ops data bundle. |
| `platform_tests` does not compile (glaze `std::expected` / Clang 18) | Environment residual. Not a Phase 10 coding fix in this WI. Official smoke dies with `Can't find test platform_tests`. |
| `:sdk:lintDebug` 5 errors (4× `MissingPermission` VIBRATE in `Utils.java`; 1× `WrongConstant` in `RecordingSessionDebug.java`) + 24 warnings | Fail for H10 lint-clean. Pre-existing vs SP-096. Own a Fix WI or triage/baseline. Do not sneak-fix here. |
| `clang-format.sh` exit 123 with clang-format-18 vs CI clang-format-20 (`AlignEscapedNewlines: LeftWithLastLine`) | Residual tooling. Re-run with clang-format-20. No source churn in this WI. |
| Device/manual hardware (background, screen-off, OEM D2, Helsinki, battery, Spike 1, lifecycle, traffic capture, §10 walk) | Residual SP-094 / SP-095. **SPD-077** / **SPD-078**. Do not copy Pixel 3a as D2. |
| Brand: app name, Play/F-Droid listing copy, privacy/terms URLs | Residual SP-093 / **SPD-080** / **SPD-084**. |
| Signed `google` release/beta APK not produced | Residual Ops (SP-096). Secrets absent. Unsigned is not exit #11. |
| Competition backend not in explorer `main` (`e13a124`); §26 #5 still open; N&lt;3 / decay / moderation / deletion *operational* unverified | Residual Ops (SP-096). Friends API is not a substitute. Do not run friends pytest as competition proof. |
| Listing still advertises GPX while public V1 gates GPX | Residual **SPD-084**. Not rewritten here. |
