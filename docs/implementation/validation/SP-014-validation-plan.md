# SP-014 — Validation plan (reviewed before execution)

**Work item:** [SP-014](../work-items/SP-014-recording-end-to-end-validation.md)
**Plan reviewed by:** Maintainer
**Plan review date:** 2026-08-02
**Branch:** `street-pixels`

## Approved decisions

| ID | Decision |
| --- | --- |
| D1 device | Google Pixel 3a (Pixel-class / stock slot) |
| D2 device | Deferred to Phase 10 (aggressive OEM continuity) — SP-014 accepted on Pixel 3a with exit #7 partial |
| Debug pixel/sample export | Skipped for v1 matrix; visual inspection + screenshots |
| ABL | Remain absent until screen-off continuity numbers (Block B) say otherwise |

## Scope

Evidence-only. No production behaviour changes. Defects are reported against owning work items (SP-006–013), not fixed on the validation branch. Maintainer decides Phase 2 exit after reviewing evidence.

## Device matrix

| Slot | Model | OS / skin | Battery exemption | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | *(fill)* | Approved |
| D2 | *(aggressive OEM TBD)* | | | Xiaomi / Samsung / Huawei class |
| D3 | optional older/low-end | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and versionName in the evidence log.

**Recording location interval:** 1000 ms (`LocationHelper` track mode). Screen-off 30 min → ~1800 expected provider updates if continuous.

## Scenario catalogue

Run each scenario on **D1 and D2** unless noted. Evidence: one row per (scenario × device) in [`SP-014-evidence-log.md`](SP-014-evidence-log.md).

### Block A — Gate and session (do first)

| ID | Scenario | Pass condition |
| --- | --- | --- |
| A1 | No session; walk ~500 m | **Zero** new green pixels (most important case) |
| A2 | Session on; same route | Coverage matches walked streets |
| A3 | Discard after collecting | No stored track; pixels remain explored |
| A4 | Finish; reopen | No interruption toast; track present |
| A5 | Deny location at start | Map usable; recording rationale; no silent start |
| A6 | Notification Pause / Resume / Stop | State matches UI + notification; FGS while Recording/Paused |
| A7 | Foreground haptics | Pulse when collecting on-screen; none screen-off |

### Block B — Continuity / ABL input (Phase 2 exit #7)

| ID | Scenario | Pass / measurement |
| --- | --- | --- |
| B4 | Screen off 30 min walk | Continued collection; record received vs expected (~1800); no false interrupt if gaps &lt; 60 s |
| B5 | Other app FG 15 min walk | Continued collection (~900 expected) |
| B12 | 2 h session vs control | **Record** battery only (no Phase 2 pass bar) |
| B13 | Full session offline | Same gate/pause/finish as online |
| B-OEM | Natural OEM kill (D2) | Interrupt UX on reopen; prior pixels kept; no gap fill |

### Block C — Pause / interpolation barriers

| ID | Scenario | Pass condition |
| --- | --- | --- |
| C3 | Pause → vehicle ≥1–2 km → resume → walk | No pixels on paused segment; no connecting green; saved track split (not one chord) |
| C-note | Live drape may look continuous | Known SP-010 D2; do **not** fail Phase 2 on overlay alone |

### Block D — GPS integrity / acceptance

| ID | Scenario | Pass / target |
| --- | --- | --- |
| D-open | Open-area ≥1 km | Continuous coverage |
| D-canyon | Urban canyon | Bad fixes rejected; no large false green |
| D7 | Cycling normal speed | Continuous; &lt;5% missed legitimate bike segments (spike 5) |
| D8 | Vehicle passenger | Speed rule suppresses collection |
| D6 | Tunnel / signal loss | No line across gap; **screenshot required** |
| D-batch | Screen-off batched samples (during B4) | Age ≤120 s accepted; no false interrupt if gap &lt; 60 s |

### Block E — Interruption recovery

| ID | Scenario | Pass condition |
| --- | --- | --- |
| E9 | Force-stop mid-session; reopen | Interrupt toast; prior pixels; no gap fill; force-finish per SP-013 D1 |
| E-reboot | Reboot mid-session | Same as E9 |
| E-air | Airplane ≥90 s while Recording | Interrupt ~60 s; stay Recording; barrier; no fill |
| E-orphan | Tracker on without breadcrumb (if reproducible) | Quiet stop; no interrupt toast |

### Block F — Spot checks

| ID | Scenario | Pass |
| --- | --- | --- |
| F-radius | Visual ~25 m band | No radius setting |
| F-poor | Poor GPS | Existing green stays; no invented streets |

## Deferred items absorbed

| Source | Covered by |
| --- | --- |
| SP-009 field walks | Block D + B4 |
| SP-010 bus test | C3 |
| SP-011 cycling / tunnel / battery | D7, D6, B12 |
| SP-012 screen-off / notification / ABL | B4, B5, A6; ABL decision after B4/B5 |
| SP-013 force-stop / reboot / OEM / airplane / clean FP | E9, E-reboot, B-OEM, E-air, A4 |

## ABL decision rule (after Block B)

1. Run B4 + B5 on D1 (and D2 when available) **without** ABL.
2. If continuity OK → keep ABL out; document recommendation.
3. If poor on OEM only → product choice (messaging vs ABL vs exemption guidance); ABL implementation is Phase 10.
4. If poor on both → strong ABL signal for Phase 10; **do not** add ABL inside SP-014 to force a pass.

## Automated baseline (agent)

```bash
./tools/unix/build_omim.sh -d street_pixels_tests
../omim-build-debug/street_pixels_tests
./tools/unix/run_tests.sh -b ../omim-build-debug -f "gps_track"   # if available
./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke
```

Record results in the evidence log.

## Phase 2 exit status (fill after walks)

| Exit # | Criterion | Status | Evidence |
| --- | --- | --- | --- |
| 1 | No pixels outside active non-paused session | | Automated SP-007 + A1/A2 |
| 2 | Start/pause/resume/finish/discard in shared code | | SP-006–012 + A6 |
| 3 | §16.2 acceptance + boundaries | | SP-009 + Block D |
| 4 | §16.3 interpolation + barriers | | SP-011 + C3/D6/D7 |
| 5 | Interrupted sessions informed, no auto-fill | | SP-013 + Block E |
| 6 | 25 m radius, not configurable | | SP-008 + F-radius |
| 7 | Background/screen-off on ≥2 devices incl. aggressive OEM | **Blocked until D2 named and B4 run** | B4/B5 |
| 8 | `gps_track_*` still pass | | Automated baseline |

## Execution order

1. Automated baseline (agent) — done when logged.
2. Human: A1 → A2 on Pixel 3a; stop if A1 fails.
3. A3–A7, then B4/B5 on Pixel 3a.
4. C3, D6–D8, Block E as opportunity allows.
5. Name D2 and repeat critical scenarios (especially B4, E9, B-OEM).
6. Fill exit table; maintainer decides Phase 2 exit.
