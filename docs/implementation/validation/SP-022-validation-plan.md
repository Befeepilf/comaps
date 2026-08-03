# SP-022 — Validation plan (reviewed before execution)

**Work item:** [SP-022](../work-items/SP-022-exploration-storage-end-to-end-validation.md)
**Plan reviewed by:** Maintainer
**Plan review date:** 2026-08-03
**Branch:** `street-pixels`

## Approved decisions

| ID | Decision |
| --- | --- |
| D1 device | Google Pixel 3a (same handset class as SP-014). Region choice matters more than OEM for Phase 3. |
| D2 aggressive OEM | Deferred to Phase 10 (same posture as SP-014 exit #7 residual). Not required to exit Phase 3. |
| Rematch region | Prefer **Finland Uusimaa** (or equally large ~50 MB `.pix` region) for rematch wall-time, RAM feel, delete/redownload `.pixr` size, and confirmation that SPD-019 did not densify `.pix`. |
| Small-country fallback | If Uusimaa update is unavailable during walks, run permanence/toast scenarios on a smaller country; still require one Uusimaa-class size/timing measurement when a ~50 MB `.pix` is present. |
| Debug export | Skipped; visual greens + screenshots + on-device file-size notes where useful. |
| Device walks at Phase 3 Accept | Deferred to Phase 10 residual; Phase 3 Accepted on automated exit coverage (SP-015–021 + suite). |
| SP-015 | Accepted 2026-08-03. |
| Flake note | `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` is a known intermittent pre-existing flake. Do **not** fail Phase 3 on a single flake of that test; re-run once and record. |

## Scope

Evidence-only. No production behaviour changes on the validation branch except
defect fixes routed to owning SP-015–021 items (prefer fix on owner; tiny
validation-blocking fixes allowed with explicit note in the evidence log).
Maintainer decides Phase 3 exit after reviewing evidence.

Phase 3 modules under test: SP-015 (header / map-data version), SP-016
(ever-live), SP-017 (rematch), SP-018 (`.pixr` delete/redownload), SP-019
(15 m unify), SP-020 (eligibility + divergence register), SP-021 (§27.3 toast /
fraction).

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Prefer Uusimaa | Approved proposal |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log.

**Preferred country:** Uusimaa (Finland) — maintainer baseline ~50 MB `.pix`
(~6.5×10⁶ cells). Use the same country for rematch timing, delete/redownload
archive-size notes, and post-SPD-019 size check.

## Scenario catalogue

Run on **D1** unless noted. Evidence: one row per scenario in
[`SP-022-evidence-log.md`](SP-022-evidence-log.md). Map each result to Phase 3
exit criteria 1–8.

### Block A — Ever-live / imported-only + header (exit 1, 2)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | Live session: explore streets; finish; force-stop; reopen | Prior greens remain; live-explored cells are ever-live (competition-eligible provenance). Visual + any debug/log confirmation available. | 1 |
| A2 | Import-only path (bookmark-track replay / available import path): explore cells that were not live | Those cells stay explored with ever-live clear; a later live walk over the same cells upgrades to ever-live and never clears it | 1 |
| A3 | Fresh derive / update produces headered `.pix` | File is headered (not raw legacy); format version readable; map-data version stamped from country MWM version | 2 |
| A4 | Upgrade path: device with legacy headerless `.pix` (if available) → install walk APK → open country | Explored greens survive first launch; file migrates to headered form without wipe | 2 |

### Block B — Rematch on country update + §27.3 messaging (exit 3, 7)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | Explore a known pocket on Uusimaa (or chosen region); trigger a **real** country map update; wait for rematch | Greens that still exist remain green; percentage change is explainable by streets added/removed (not “progress deleted”) | 3, 7 |
| B2 | Same update while map of that country is on-screen | UI stays usable; updating / loading state appears; map does not hard-freeze for the whole rematch | 3 |
| B3 | After rematch with denominator growth (or any fraction drop from rematch) | Toast / message uses `street_pixels_more_to_explore`: “%1$s map was updated. Streets may have been added or removed… Your progress is still saved.” Never claims personal progress was deleted | 7 |
| B4 | After rematch with no material fraction drop | No false “more to explore” toast (or pending cleared appropriately) | 7 |
| B-time | Uusimaa-class rematch (or derive if no update) | Record wall-clock feel / stopwatch and subjective RAM / jank notes in evidence log measurement slots | — (feeds residual) |

### Block C — Crash / kill mid-migration (exit 4)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | Start country update / rematch; force-stop app mid-migration (during download or while rematch/updating state visible); reopen | No exploration loss vs last durable set; rematch recovers or rolls back cleanly; greens that existed before kill remain for surviving ids | 4 |
| C2 | Optional: kill during first-open derive of a large country | Same permanence rule — no blank wipe of prior `.pixr` / prior durable explored set | 4 |

Automated interrupt coverage (`Rematch_InterruptBeforeRenameKeepsOld` and related) is necessary but not sufficient; C1 is the device gate.

### Block D — Delete → redownload → `.pixr` (SPD-016 / SP-018)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | Explore; note greens; **delete** country map | Full `{countryId}.pix` gone (tens of MB freed); compact `{countryId}.pixr` remains; explored-only archive ≪ former `.pix` | 3* |
| D2 | Redownload same country; wait for rematch from `.pixr` | Explored ∩ new universe still green; ever-live bits preserved for survivors; `.pixr` dropped or replaced per SP-018 | 1, 3* |

\*Exit criteria 1 and 3 are the permanence/rematch product bars; SPD-016 / SP-018 is the delete/redownload path that must also pass for Phase 3 exit (phase manual strategy + SP-022 in-scope).

### Block E — 15 m sampling + eligibility spot-check (exit 5, 6)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | Confirm V1 sampling unify (SPD-019) | Automated: live/track/derive share 15 m; no Street Pixels 10 m sampling constant remains. Device: Uusimaa `.pix` size remains ~50 MB class (not densified). Record size before/after walks if re-derived | 5 |
| E2 | Eligibility spot-check vs [SP-020 register](../work-items/SP-020-eligibility-policy-alignment.md#divergence-register-spec-13-vs-client) | Private / no-access road: no (or expected absent) exploreable coverage. Pedestrian / footway / cycleway as available: explorable. Tunnel: not explorable. Bridge on land route: explorable. Motorway without bicycle tag: not in universe. Do **not** fail on recorded generator gaps (indoor, subway-passage, emergency-only, proposed) or residual includes (parking_aisle / busway / ungated trunk) | 6 |

### Block F — Determinism / automated suite (exit 8)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Full `street_pixels_tests` | All pass; count recorded. Known flake: re-run `PauseResume_TrackBoundary_ImmediateResumeAdd_SplitsCorrectly` once if it fails alone | 8 |
| F2 | Determinism pointer | Repeat-derivation / file round-trip tests in suite green (SP-015/019 coverage). No separate device derive-twice required unless suite gaps found | 8 |
| F3 | Rematch / archive / ever-live / eligibility filters | `--filter=Rematch`, `Archive`, `EverLive`/`StreetPixel`, `Eligib`, `StreetPixelsFile` green as smoke subsets if full suite already logged | 8 |

## Uusimaa size / timing measurement slots

Fill in the evidence log. Prefer the same device + same country for all rows.

| Slot | What to record | How |
| --- | --- | --- |
| S1 | `{countryId}.pix` size after derive (or current) | Files app / `adb shell ls -l` on writable street-pixels path |
| S2 | Explored fraction / rough explored count if visible | UI percentage or log |
| S3 | Rematch wall time (update path) | Stopwatch from update start → greens stable / Ready + toast if any |
| S4 | Subjective UI during rematch | Usable / janky / frozen; updating indicator yes/no |
| S5 | Peak memory feel (optional) | Android profiler or “no OOM / no multi-second freeze” |
| S6 | Post-delete `.pixr` size vs former `.pix` | Confirm ≪ full universe |
| S7 | Post-redownload `.pix` size | Back to ~50 MB class for Uusimaa |
| S8 | SPD-019 densify check | `.pix` did **not** grow toward a 10 m densified universe; stays ~50 MB baseline |

If rematch on Uusimaa is too slow or RAM-spiky for V1 comfort, record a **Phase 10 residual** — do not silently shrink product scope inside SP-022.

## Deferred / absorbed from owning items

| Source | Covered by |
| --- | --- |
| SP-015 manual legacy migrate | A4 |
| SP-016 ever-live device check | A1, A2 |
| SP-017 rematch + Uusimaa timing | B1, B2, B-time, S3–S5 |
| SP-018 delete/redownload | D1, D2, S6–S7 |
| SP-019 no densify | E1, S1, S8 |
| SP-020 private / pedestrian spot-check | E2 |
| SP-021 toast / fraction | B3, B4 |

## Residuals → Phase 10

Explicit candidates (add rows only when observed):

| Residual class | Example | Disposition |
| --- | --- | --- |
| Large-country rematch timing / RAM | Uusimaa rematch stalls UI or spikes RSS | Phase 10 performance; do not fail permanence if greens survive |
| Aggressive OEM | Delete/update continuity quirks | Phase 10 (SP-014 D2 pattern) |
| Multi-country pending toast slot | Overlapping rematches overwrite single pending (SP-021 follow-up) | Phase 10 if multi-update UX needed |
| SP-020 residual includes / generator gaps | parking_aisle, busway, trunk, indoor, … | Already recorded; not Phase 3 blockers |
| Smoke suite / full APK CI | Agent env gaps | Phase 10 release hardening |
| Pause-resume flake | `ImmediateResumeAdd_SplitsCorrectly` | Pre-existing; not Phase 3 |

## Automated baseline (agent)

```bash
./tools/unix/build_omim.sh -d street_pixels_tests
# or: ninja -C ../omim-build-debug street_pixels_tests
../omim-build-debug/street_pixels_tests
../omim-build-debug/street_pixels_tests --filter=Rematch
../omim-build-debug/street_pixels_tests --filter=Archive
```

Record results in the evidence log. Plan-time re-verify (2026-08-03): **171/171**
All tests passed on existing debug binary (git SHA `949e04621e` tip; working
tree may include uncommitted SP-021 wiring — executor must re-run on the
walk APK SHA).

## Phase 3 exit status (fill after walks)

| Exit # | Criterion | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Every explored pixel records ever-live vs imported-only | | A1/A2 + ever-live tests |
| 2 | Pixel files carry format + map-data version | | A3/A4 + StreetPixelsFile tests; SP-015 Accepted |
| 3 | Country update rematches; no loss for surviving cells | | B1/B2 + Rematch tests |
| 4 | Migration crash-safe (interrupted) | | C1 + Rematch interrupt tests |
| 5 | Path sampling unified at 15 m (SPD-019); no densify | | E1 + segment/derive tests; S1/S8 |
| 6 | Eligibility matches §13 or divergences recorded | | E2 + SP-020 register + Eligib tests |
| 7 | Denominators recalculate; §27.3 reduction message | | B3/B4 + Rematch fraction tests + `street_pixels_more_to_explore` |
| 8 | Determinism proven by repeat-derivation test | | F1/F2 |
| — | Delete → redownload via `.pixr` (SPD-016) | | D1/D2 (required for phase manual strategy) |

## Execution order

1. Automated baseline (agent) — re-run on walk SHA; log count.
2. Confirm SP-015 Accepted (or record residual blocking exit #2).
3. Human D1: A1 → A3 (stop if exploration wipe on reopen).
4. B1–B4 on Uusimaa if update available; fill S3–S5. Else smaller country for B1–B4 + separate S1/S8 size slot on Uusimaa.
5. C1 force-stop mid-update.
6. D1 → D2 delete/redownload; S6–S7.
7. E2 eligibility spot-checks as opportunity allows; E1 size note.
8. Fill exit table + residuals; maintainer decides Phase 3 exit.
