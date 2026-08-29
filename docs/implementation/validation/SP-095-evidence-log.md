# SP-095 — Evidence log

**Plan:** [SP-095-validation-plan.md](SP-095-validation-plan.md)
**Branch:** `cursor/sp-095-device-matrix-residual-6383`
**Status:** Roster recorded 2026-08-29. **Every Device-verify row Residual
  (not executed).** Agent does **not** mark Accepted.

This Phase 10 coding slice records the H7 / **SPD-083** Device-verify
**roster** only (product-owner lock 2026-08-29). No handset run. Do
not read empty cells as zero. Do not treat unit-test pointers as
device Pass. Do not copy Pixel 3a results from SP-014 as if they
closed Phase 10 **D2** OEM.

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date (roster recorded) | 2026-08-29 |
| Parent SHA (`street-pixels`) | `8f30cbccfbe91ea1199f29a788bbf3a85a4f8908` (`Merge branch 'cursor/sp-094-battery-protocol-6383'`) |
| Walk APK SHA | *(empty — not executed)* |
| `versionName` / `versionCode` | |
| Build type | Prefer release/beta when later executed. **Not built or installed in this slice.** |
| Flavor | |
| Map package / `.spa` versions | |
| `adb` / handset | **Not used.** This slice must not run the H1 matrix, OEM continuity, Helsinki walks, traffic capture, or other hardware walks. |
| Device ids | **Empty.** No serials, marketing names, or OS builds invented. |
| Screenshots | **None.** Live-position map shots are location data; they do not belong in this public log. |

### Automated pointers (not a device substitute)

Not re-run in this slice. Green unit tests, if later run, still do
not close Device-verify rows. Desktop suites are not a substitute
(work-item required automated tests: none).

## Device roster

| Slot | Model | OS / skin | Saver / exemptions | Walker |
| --- | --- | --- | --- | --- |
| D1 Pixel-class | | | | |
| D2 aggressive OEM | | | | |
| D3 optional | | | | |

**SPD-077** locks D1 + D2. Slots are unnamed here because no handset
run exists. Do **not** copy Pixel 3a / Pixel 10a ids from SP-014 /
SP-041 into this log as if they executed the Phase 10 matrix.

Optional **D3** is unused (SPD-077: only if D1/D2 are the same
generation). No D3 rows below.

## Prior D1-class work (citation only — not Phase 10 close)

SP-014 recorded maintainer attestation 2026-08-03: planned checks
**Pass** on **Google Pixel 3a** (D1-class / stock). Aggressive OEM
**was not run**. Phase 2 exit #7 remains **partial**. Source:
[`SP-014-evidence-log.md`](SP-014-evidence-log.md).

That Pixel 3a evidence may be *cited* as prior D1-class work. It does
**not**:

- fill the D1 cells in this Phase 10 log (different APK / SHA / date;
  Phase 10 walks are unexecuted);
- close **D2** OEM screen-off or B-OEM;
- close Helsinki UX, traffic capture, routing, milestones, GPX, or
  friends-absent eyeball on the Phase 10 matrix;
- authorise adding ABL. **SPD-082** keeps ABL absent; a D2 Fail needs
  a new SPD.

SP-022 / SP-031 / SP-041 / SP-061 / SP-069 / SP-079 / SP-087 device
rows were already residual to Phase 10; none of those logs supply a
Phase 10 Pass here.

SP-033 qualitative Pixel 3a and SP-094 empty measurement cells are
**not** copied. Quantitative Spike 1 / battery remain
[`SP-094-evidence-log.md`](SP-094-evidence-log.md) (also Residual).

## Block SP-014 — screen-off / OEM / no gap-fill

Carried ids: [`SP-014-validation-plan.md`](SP-014-validation-plan.md).
B12 is **not** here (SP-094 Bat-A/B). B4/B5 pass conditions include
recording received vs expected (~1800 / ~900). Scenario ids
D-open / D-canyon / D6–D8 / D-batch are GPS integrity, **not** matrix
slots.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| A1 no session ~500 m | D1 | **Residual** | |
| A1 no session ~500 m | D2 | **Residual** | |
| A2 session on | D1 | **Residual** | |
| A2 session on | D2 | **Residual** | |
| A3 discard after collecting | D1 | **Residual** | |
| A3 discard after collecting | D2 | **Residual** | |
| A4 finish; reopen | D1 | **Residual** | |
| A4 finish; reopen | D2 | **Residual** | |
| A5 deny location at start | D1 | **Residual** | |
| A5 deny location at start | D2 | **Residual** | |
| A6 notification Pause / Resume / Stop | D1 | **Residual** | |
| A6 notification Pause / Resume / Stop | D2 | **Residual** | |
| A7 foreground haptics | D1 | **Residual** | |
| A7 foreground haptics | D2 | **Residual** | |
| B4 screen off 30 min walk | D1 | **Residual** | Prior Pixel 3a SP-014 Pass is citation only. Record received vs expected (~1800). |
| B4 screen off 30 min walk | D2 | **Residual** | Closes SP-014 exit #7 posture only when recorded. Record received vs expected (~1800). |
| B5 other app FG 15 min | D1 | **Residual** | ~900 expected. |
| B5 other app FG 15 min | D2 | **Residual** | ~900 expected. |
| B13 full session offline | D1 | **Residual** | |
| B13 full session offline | D2 | **Residual** | |
| B-OEM natural OEM kill | D1 | **Residual** | Stock Pixel-class may not exhibit OEM kill. |
| B-OEM natural OEM kill | D2 | **Residual** | Required. Interrupt UX; prior pixels; **no gap fill**. Do not add ABL (**SPD-082**). |
| C3 pause / vehicle / resume | D1 | **Residual** | No interpolated exploration across pause. |
| C3 pause / vehicle / resume | D2 | **Residual** | |
| D-open ≥1 km | D1 | **Residual** | |
| D-open ≥1 km | D2 | **Residual** | |
| D-canyon | D1 | **Residual** | |
| D-canyon | D2 | **Residual** | |
| D7 cycling | D1 | **Residual** | |
| D7 cycling | D2 | **Residual** | |
| D8 vehicle passenger | D1 | **Residual** | |
| D8 vehicle passenger | D2 | **Residual** | |
| D6 tunnel / signal loss | D1 | **Residual** | No public live-position screenshot. |
| D6 tunnel / signal loss | D2 | **Residual** | |
| D-batch screen-off batched samples | D1 | **Residual** | During B4. |
| D-batch screen-off batched samples | D2 | **Residual** | |
| E9 force-stop mid-session | D1 | **Residual** | No gap fill. |
| E9 force-stop mid-session | D2 | **Residual** | |
| E-reboot mid-session | D1 | **Residual** | |
| E-reboot mid-session | D2 | **Residual** | |
| E-air airplane ≥90 s | D1 | **Residual** | |
| E-air airplane ≥90 s | D2 | **Residual** | |
| E-orphan tracker without breadcrumb | D1 | **Residual** | If reproducible. |
| E-orphan tracker without breadcrumb | D2 | **Residual** | |
| F-radius ~25 m band | D1 | **Residual** | |
| F-radius ~25 m band | D2 | **Residual** | |
| F-poor GPS | D1 | **Residual** | |
| F-poor GPS | D2 | **Residual** | |

## Block SP-022 — permanence / rematch

Carried ids: [`SP-022-validation-plan.md`](SP-022-validation-plan.md).
Uusimaa S1–S8 measurement cells stay empty. Scenario **D1–D2** in this
block are delete / redownload, **not** matrix slots.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| A1 live explore; force-stop; reopen | D1 | **Residual** | |
| A1 live explore; force-stop; reopen | D2 | **Residual** | |
| A2 import-only then live upgrade | D1 | **Residual** | |
| A2 import-only then live upgrade | D2 | **Residual** | |
| A3 headered `.pix` | D1 | **Residual** | |
| A3 headered `.pix` | D2 | **Residual** | |
| A4 legacy headerless migrate | D1 | **Residual** | If a fixture exists. |
| A4 legacy headerless migrate | D2 | **Residual** | |
| B1 real country update rematch | D1 | **Residual** | Prefer Uusimaa. |
| B1 real country update rematch | D2 | **Residual** | |
| B2 rematch with map on-screen | D1 | **Residual** | |
| B2 rematch with map on-screen | D2 | **Residual** | |
| B3 §27.3 fraction-drop toast | D1 | **Residual** | |
| B3 §27.3 fraction-drop toast | D2 | **Residual** | |
| B4 no false more-to-explore toast | D1 | **Residual** | |
| B4 no false more-to-explore toast | D2 | **Residual** | |
| B-time Uusimaa-class rematch timing | D1 | **Residual** | As available. |
| B-time Uusimaa-class rematch timing | D2 | **Residual** | |
| C1 force-stop mid-rematch | D1 | **Residual** | |
| C1 force-stop mid-rematch | D2 | **Residual** | |
| C2 kill during large derive | D1 | **Residual** | Optional. |
| C2 kill during large derive | D2 | **Residual** | |
| D1 delete country → `.pixr` | D1 | **Residual** | |
| D1 delete country → `.pixr` | D2 | **Residual** | |
| D2 redownload rematch | D1 | **Residual** | |
| D2 redownload rematch | D2 | **Residual** | |
| E1 15 m unify / no densify | D1 | **Residual** | |
| E1 15 m unify / no densify | D2 | **Residual** | |
| E2 eligibility spot-check | D1 | **Residual** | |
| E2 eligibility spot-check | D2 | **Residual** | |

## Block SP-031 — Helsinki names / rural / coastal / settlement (R3)

Carried ids: [`SP-031-validation-plan.md`](SP-031-validation-plan.md).
If `.spa` is missing at execution → **Blocked**, do not substitute a
city without administrative polygons. Scenario **D1–D3** in this block
are settlement / rural / subdivision visual, **not** matrix slots.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| E2 no MWM id as neighbourhood | D1 | **Residual** | |
| E2 no MWM id as neighbourhood | D2 | **Residual** | |
| F1 Helsinki subdivision names | D1 | **Residual** | Kamppi / Kallio / Punavuori when FI `.spa` present. |
| F1 Helsinki subdivision names | D2 | **Residual** | |
| F2 rural / coastal / settlement-only | D1 | **Residual** | |
| F2 rural / coastal / settlement-only | D2 | **Residual** | |
| F3 explore → area assignment visible | D1 | **Residual** | |
| F3 explore → area assignment visible | D2 | **Residual** | |
| D1 settlement-only city (visual) | D1 | **Residual** | |
| D1 settlement-only city (visual) | D2 | **Residual** | |
| D2 rural / outside settlements | D1 | **Residual** | Exploration allowed; no grid. |
| D2 rural / outside settlements | D2 | **Residual** | |
| D3 subdivision wins over settlement | D1 | **Residual** | |
| D3 subdivision wins over settlement | D2 | **Residual** | |

## Block SP-041 — Helsinki UX (H1–H6 **here** are not Phase 10 locks)

Carried ids: [`SP-041-validation-plan.md`](SP-041-validation-plan.md)
Block H. **Not** SPD-077–082.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| H1 boundary walk / pan / recentre | D1 | **Residual** | SP-041 H1. Not Phase 10 H1 (matrix). |
| H1 boundary walk / pan / recentre | D2 | **Residual** | |
| H2 tap → detail exact % | D1 | **Residual** | SP-041 H2. Not Phase 10 H2 (SPD-078 Spike 1 / battery, SP-094). |
| H2 tap → detail exact % | D2 | **Residual** | |
| H3 zoom street → city summary % | D1 | **Residual** | |
| H3 zoom street → city summary % | D2 | **Residual** | |
| H4 completed chrome across zooms | D1 | **Residual** | |
| H4 completed chrome across zooms | D2 | **Residual** | |
| H5 leave settlement → §31 empty | D1 | **Residual** | |
| H5 leave settlement → §31 empty | D2 | **Residual** | |
| H6 no country/world % UI | D1 | **Residual** | SP-041 H6. Not Phase 10 H6 (ABL). |
| H6 no country/world % UI | D2 | **Residual** | |
| F4 chrome re-spot-check | D1 | **Residual** | After SP-037/040 / SP-089 glyph if present. |
| F4 chrome re-spot-check | D2 | **Residual** | |

## Block SP-061 — routing on device

Carried ids: [`SP-061-validation-plan.md`](SP-061-validation-plan.md)
Block I.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| I1 / D4 SP-058 copy | D1 | **Residual** | |
| I1 / D4 SP-058 copy | D2 | **Residual** | |
| I2 / A12 Prefer walk/bike | D1 | **Residual** | |
| I2 / A12 Prefer walk/bike | D2 | **Residual** | |
| I3 / B7 Avoid possible route | D1 | **Residual** | |
| I3 / B7 Avoid possible route | D2 | **Residual** | |
| I4 / C11 no-route Prefer control | D1 | **Residual** | |
| I4 / C11 no-route Prefer control | D2 | **Residual** | |
| I5 / E7 / C12 off-route Prefer | D1 | **Residual** | Observe SP-089 Fix; do not re-implement. |
| I5 / E7 / C12 off-route Prefer | D2 | **Residual** | |
| I6 remaining routing walks | D1 | **Residual** | Mid-nav stability included. |
| I6 remaining routing walks | D2 | **Residual** | |

## Block SP-069 — milestones / share / haptics / nav

Carried ids: [`SP-069-validation-plan.md`](SP-069-validation-plan.md)
Block I.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| I1 25/50/100 celebration | D1 | **Residual** | Non-blocking. |
| I1 25/50/100 celebration | D2 | **Residual** | |
| I2 card vs deny-list | D1 | **Residual** | Eyeball PNG; no live map shot. |
| I2 card vs deny-list | D2 | **Residual** | |
| I3 competition-off copy | D1 | **Residual** | |
| I3 competition-off copy | D2 | **Residual** | |
| I4 explicit share | D1 | **Residual** | |
| I4 explicit share | D2 | **Residual** | |
| I5 haptics predicate | D1 | **Residual** | |
| I5 haptics predicate | D2 | **Residual** | |
| I6 nav not interrupted | D1 | **Residual** | |
| I6 nav not interrupted | D2 | **Residual** | |
| first-100 m (Block C behaviour on device) | D1 | **Residual** | As opportunity. Not a Block C unit-test id. |
| first-100 m (Block C behaviour on device) | D2 | **Residual** | |
| I7 §22.10 competition-on sentences | D1 | **Residual** | Observe with SP-079 if competition is on. Do not fail SP-069’s V1 stub. |
| I7 §22.10 competition-on sentences | D2 | **Residual** | |

## Block SP-079 — opt-in / traffic capture / opt-out / delete

Carried ids: [`SP-079-validation-plan.md`](SP-079-validation-plan.md)
Block M. Traffic capture **required** on at least one device *when
executed*.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| M1 opt-in vs §20.2 | D1 | **Residual** | |
| M1 opt-in vs §20.2 | D2 | **Residual** | |
| M2 traffic capture | D1 | **Residual** | No coordinates in upload. |
| M2 traffic capture | D2 | **Residual** | At least one of D1/D2 required when executed. |
| M3 opt-out zero upload | D1 | **Residual** | |
| M3 opt-out zero upload | D2 | **Residual** | |
| M4 offline queue then flush | D1 | **Residual** | |
| M4 offline queue then flush | D2 | **Residual** | |
| M5 N&lt;3 nicknames | D1 | **Residual** | If a sparse fixture exists. |
| M5 N&lt;3 nicknames | D2 | **Residual** | |
| M6 decay without opening app | D1 | **Residual** | As opportunity. |
| M6 decay without opening app | D2 | **Residual** | |
| M7 delete profile; local intact | D1 | **Residual** | |
| M7 delete profile; local intact | D2 | **Residual** | |
| M8 no presence copy | D1 | **Residual** | |
| M8 no presence copy | D2 | **Residual** | |

## Block SP-087 — GPX public vs Pro

Carried ids: [`SP-087-validation-plan.md`](SP-087-validation-plan.md)
Block M.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| M1 multi-hour GPX | D1 | **Residual** | No map screenshot. |
| M1 multi-hour GPX | D2 | **Residual** | |
| M2 competition on; no ownership/weekly | D1 | **Residual** | Import must not move competition. |
| M2 competition on; no ownership/weekly | D2 | **Residual** | |
| M3 import over already-live | D1 | **Residual** | |
| M3 import over already-live | D2 | **Residual** | |
| M4 public APK no GPX / no purchase | D1 | **Residual** | Share-sheet GPX refused when gated. |
| M4 public APK no GPX / no purchase | D2 | **Residual** | |
| M5 Pro-internal tools | D1 | **Residual** | |
| M5 Pro-internal tools | D2 | **Residual** | |
| M6 batch-import | D1 | **Residual** | |
| M6 batch-import | D2 | **Residual** | |
| M7 analytics readout | D1 | **Residual** | Optional. |
| M7 analytics readout | D2 | **Residual** | |

## Block observe — first-run / §31 / friends

Scripts already written in SP-090 and **SPD-085**. Not new walks.
§31 is nine distinct states; do not collapse them into one Pass.

| Scenario | Device | Result | Notes |
| --- | --- | --- | --- |
| SP-090 §10 first-run click-through | D1 | **Residual** | Five steps in SP-090: map open without location on splash; spec card Start exploring; session-only location rationale; FGS/screen-off explanation, no ABL; recording control + first-100 m, no full tutorial. |
| SP-090 §10 first-run click-through | D2 | **Residual** | |
| SP-090 §31 location denied | D1 | **Residual** | Map stays up; settings + continue browsing. |
| SP-090 §31 location denied | D2 | **Residual** | |
| SP-090 §31 background location denied | D1 | **Residual** | No ABL. FGS / notification / pause copy. |
| SP-090 §31 background location denied | D2 | **Residual** | |
| SP-090 §31 no downloaded map | D1 | **Residual** | |
| SP-090 §31 no downloaded map | D2 | **Residual** | |
| SP-090 §31 poor GPS accuracy | D1 | **Residual** | Waiting badge; no interpolation. |
| SP-090 §31 poor GPS accuracy | D2 | **Residual** | |
| SP-090 §31 interrupted recording | D1 | **Residual** | |
| SP-090 §31 interrupted recording | D2 | **Residual** | |
| SP-090 §31 no selected exploration area | D1 | **Residual** | |
| SP-090 §31 no selected exploration area | D2 | **Residual** | |
| SP-090 §31 no local competitors | D1 | **Residual** | Weekly board hidden when empty. |
| SP-090 §31 no local competitors | D2 | **Residual** | |
| SP-090 §31 no competition connectivity | D1 | **Residual** | Queue copy. |
| SP-090 §31 no competition connectivity | D2 | **Residual** | |
| SP-090 §31 avoid-explored impossible | D1 | **Residual** | Observe SP-089 Prefer + normal fallback. |
| SP-090 §31 avoid-explored impossible | D2 | **Residual** | |
| SPD-085 friends must not appear | D1 | **Residual** | Public APK. |
| SPD-085 friends must not appear | D2 | **Residual** | |

## Not this log (point only)

| Topic | Where | This slice |
| --- | --- | --- |
| Spike 1 R1–R3, Bat-A/B, CS1, L1–L9 | [`SP-094-evidence-log.md`](SP-094-evidence-log.md) | Residual there. Not duplicated as executed here. |
| SP-014 B12 2 h battery | SP-094 Bat-A/B | Absorbed. |
| Spike 7 / SP-054 city-scale | H7 Measure (not this Device-verify roster) | Routing *device* walks are SP-061 I* above. |
| Privacy/terms URL landing | SP-093 | Residual. Do not retarget here. |

## Phase 10 exit mapping (not closed)

| Exit # | Criterion | Result in this slice | Evidence |
| --- | --- | --- | --- |
| 1 | §34 verified with recorded device evidence | **Residual** | Roster exists; every walk row Residual |
| 2 | §31 empty/error observed | **Residual** | Nine SP-090 §31 states × D1/D2 Residual |
| — | SP-014 exit #7 OEM screen-off on D2 | **Residual** | B4/B-OEM D2 empty. Pixel 3a citation is not D2. |
| 6–8 | Battery / rendering / lifecycle | **Residual** | SP-094, not this item |
| — | Phase 6 Spike 7 city-scale | **Residual** | H7 Measure (SP-054), not this roster |

Do **not** mark Phase 10 exit met.

## Defects found this slice

None from device execution (none attempted). No walk results, device
ids, or pass/fail invented.
