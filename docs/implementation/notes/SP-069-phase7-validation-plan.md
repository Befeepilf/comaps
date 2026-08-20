# SP-069 — Phase 7 end-to-end validation (implementation plan)

**Status:** Draft plan for validation; not Accepted.
**Work item:** [`SP-069-phase7-end-to-end-validation.md`](../work-items/SP-069-phase7-end-to-end-validation.md)
**Date:** 2026-08-20
**Locks (do not re-open):** SPD-046–055 (M1–M10). SP-062–068 are implemented
or residualled. Agent does **not** mark Phase 7 exit Met or this work item
Accepted.

This note is the validation contract. Evidence-only. No production behaviour
changes. Do not weaken tests, fabricate device walks, or fill §22.10
competition-on copy.

---

## 0. Status of building blocks (verified 2026-08-20)

| Piece | Actual state | SP-069 action |
| --- | --- | --- |
| SP-063 store | `AreaMilestoneStore` OSM-id key; fire-once; original 100% date; rematch survival tests | Re-run store + manager filters; map to exit 1 and 8 |
| SP-064 first-goal | 10 newly explored live pixels; once per install | Re-run `FirstGoal_*`; map to exit 3 |
| SP-065 presentation | Queue 100>50>25; following does not stop route; 100% does not share | Re-run `AreaMilestonePresentation`; map to exits 1, 2, 6 |
| SP-066 haptics | Recording ∧ foreground ∧ toggle; one collection pulse; 50/100/first-goal patterns | Re-run `ExplorationHaptic`; map to exit 7 |
| SP-067 compositor | Permit-list `CompletionCardModel`; rings-only; deny-list tests | Re-run `CompletionCard_`; map to exits 4, 5 |
| SP-068 share | Explicit tap; date default off; count-only counters | Re-run `CompletionCardShare`; map to exits 6, 9 |
| Device / APK | No handset in this agent environment (same as SP-014 / SP-041) | Residual → Phase 10; do not fabricate |
| Full `street_pixels_tests` | SP-066 recorded abort at `Eligibility_IncludesCommonHighways` (no `data/classificator.txt`) | Re-attempt; if still abort, record environment residual; do not skip/weaken Eligibility |

No inline imports. No new comments. No formatting-only changes. No production
code unless a Phase 7 defect blocks a listed suite — then fix on the owning
SP-062–068 item, not by weakening SP-069.

---

## 1. Architecture

**Docs own the gate.** Primary output is
[`validation/SP-069-validation-plan.md`](../validation/SP-069-validation-plan.md)
and [`validation/SP-069-evidence-log.md`](../validation/SP-069-evidence-log.md).

**Suites own the proof.** Desktop filters listed in §3 are mandatory. Device
Block H is Phase 10 if no handset.

**Maintainer owns exit.** Status after evidence: work item **In review**.
README SP-069 In review. Phase 7 stays **In progress** until the maintainer
accepts exit (with residuals) or blocks.

---

## 2. Exact files

### 2.1 Add

| Path | Why |
| --- | --- |
| `docs/implementation/validation/SP-069-validation-plan.md` | Scenario catalogue + exit mapping |
| `docs/implementation/validation/SP-069-evidence-log.md` | Counts, SHA, pass/fail/residual table |

### 2.2 Edit

| Path | Why |
| --- | --- |
| `docs/implementation/work-items/SP-069-phase7-end-to-end-validation.md` | In progress → In review; fill completion evidence |
| `docs/implementation/README.md` | SP-069 status |
| `docs/implementation/phases/phase-07-milestones-and-share-cards.md` | Current-code / known-uncertainties if evidence changes them; **do not** mark exit Met |

### 2.3 Do not edit

Production C++ / Java / JNI / strings unless a listed suite proves a Phase 7
defect. `SharingUtils.java`, `qt/screenshoter.*`, Sentry, iOS, `values-en/strings.xml`,
classificator generation, Phase 8 competition copy.

---

## 3. Suites to run (record SHA + counts)

Minimum filters (Phase 7 modules):

| Filter / target | Exit # |
| --- | --- |
| `street_pixels_areas_tests --filter=AreaMilestone` | 1, 8 |
| `street_pixels_tests --filter=AreaMilestone` | 1, 2, 6, 8 |
| `street_pixels_tests --filter=FirstGoal` | 3, 7 |
| `street_pixels_tests --filter=ExplorationHaptic` | 7 |
| `street_pixels_tests --filter=CompletionCard_` | 4, 5 |
| `street_pixels_tests --filter=CompletionCardShare` | 6, 9 |

Also attempt:

| Target | Why |
| --- | --- |
| Full `street_pixels_areas_tests` | Broader Phase 5/7 store health |
| Full `street_pixels_tests` | Mandatory desktop suite; record Eligibility abort if still missing `classificator.txt` |

Build: `./tools/unix/build_omim.sh -d -p /workspace street_pixels_tests` and
`street_pixels_areas_tests` (`-p /workspace` before the target). Binaries:
`/workspace/omim-build-debug/street_pixels_tests` and
`/workspace/omim-build-debug/street_pixels_areas_tests`.

Write transcripts under `/opt/cursor/artifacts/` for the evidence log.

---

## 4. Exit mapping (fill in the evidence log)

| # | Criterion | Automated evidence | Manual / residual |
| --- | --- | --- | --- |
| 1 | Fire once per area per threshold; non-blocking | Store fire-once + triple-cross; presentation queue / one-at-a-time | Device 100% celebration → Phase 10 |
| 2 | Never interrupt routing / demand immediate interaction | `FollowingDoesNotStopRoute`; auto-ack exists; no `nativeCloseRouting` | Nav-on-device → Phase 10 |
| 3 | First-100 m appears and completes | `FirstGoal_*` (10 pixels, import, persist, once) | Device chip → Phase 10 |
| 4 | Cards at 100%; deny-list | `CompletionCard_DenyListFieldsAbsent` + rings-only + manager bind | Card image eyeball → Phase 10 |
| 5 | No competition / nickname | Compose without nickname/date; competition stub empty | First-person copy on device → Phase 10; §22.10 live sentences → Phase 8 |
| 6 | Explicit share; no auto-open | `HundredPercentDoesNotShare`; Prepare only from share path | Share-sheet eyeball → Phase 10 |
| 7 | Haptics predicate + toggle | `ExplorationHaptic_*` | Screen-off / background / toggle off feel → Phase 10 |
| 8 | §27.4 survival | OsmId stable; no re-fire after drop; previously-completed below 100 | Map-update walk → Phase 10 |
| 9 | Count-only growth analytics | Share analytics key/payload denylist; increment tests | Upload still Phase 10 (SPD-055) |

---

## 5. Expected residuals (do not hide)

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | No handset in this environment | Phase 10 (SP-014 / SP-041 pattern) |
| R2 | Competition-on leading/not-leading copy | Phase 8 (explicit in phase-07 manual strategy) |
| R3 | Growth-counter upload | Phase 10 / SPD-055 |
| R4 | 4 s auto-ack may delete PNG while a share target still reads it | Phase 10 (SP-068 left duration unchanged) |
| R5 | `onResume` rebind may increment generated and reset date checkbox | Phase 10 / polish |
| R6 | Full `street_pixels_tests` Eligibility abort if `classificator.txt` absent | Environment residual; not a Phase 7 product defect; do not weaken Eligibility |
| R7 | Shared PNG is outline-only; date/name in `EXTRA_TEXT` | Phase 10 eyeball; SPD-046 geometry is the lock |

---

## 6. Docs after tests

Work item **In review**; branch; test counts; device roster = deferred Phase 10;
exit table filled; agent recommendation for maintainer. README SP-069 In review.
Phase 7 **In progress**. Agent does **not** mark Accepted or exit Met.

---

## 7. Non-goals

- Marking Phase 7 Accepted.
- Phase 8 competition chrome or live §22.10 sentences.
- Changing 4 s auto-ack, compositor deny-list type, fire-once policy, or
  haptic waveforms.
- Generating `classificator.txt` as a product change.
- iOS.
