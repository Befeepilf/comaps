# SP-095 — Validation plan (roster only; not executed in this slice)

**Work item:** [SP-095](../work-items/SP-095-device-matrix-residual-close-out.md)
**Plan authored by:** Agent
**Plan date:** 2026-08-29
**Branch:** `cursor/sp-095-device-matrix-residual-6383` (lands on `street-pixels`)

**This Phase 10 coding slice does not execute this plan.** Do not run
the H1 matrix, OEM continuity, Helsinki walks, traffic capture, first-run
click-through, or any other hardware walk. Do not fabricate walk
results, device ids, screenshots, packet captures, or pass/fail.
A later work item executes the carried scripts on hardware and fills
[`SP-095-evidence-log.md`](SP-095-evidence-log.md).

This item is the **Device-verify roster and pointers**. It does **not**
rewrite or weaken the carried plans. When a later WI walks a device, it
runs those plans’ scenario ids as written.

## ID collision warning

Several “H” and “D” series exist. Do **not** mix them.

| Series | What it is | This plan |
| --- | --- | --- |
| Phase 10 locks **H1–H10** | Launch-governance decisions **SPD-077–086** | **H1** = device *matrix* (**SPD-077**: D1 Pixel-class + D2 aggressive OEM). **H2** = Spike 1 bar + battery *protocol* (**SPD-078** / SP-094). **H6** = ABL stays absent (**SPD-082**). **H7** = residual *classification* (**SPD-083** Device-verify → this item). Not walk scripts. |
| SP-041 scenarios **H1–H6** | Helsinki *UX* walks (badge / tap / city zoom / completed chrome / §31 empty / no country-world %) | **In this roster.** Carried from [`SP-041-validation-plan.md`](SP-041-validation-plan.md) Block H. These are **not** Phase 10 locks H1–H6. |
| SP-061 Block H | Phase 6 *non-goals* (car no Avoid, no min-connection, …) | Not Helsinki. Not Phase 10 H1. |
| SP-069 Block H | Growth-analytics unit scenarios | Not Helsinki. Not Phase 10 H1. |
| SP-087 Block H | Monetisation-analytics unit scenarios | Not Helsinki. Not Phase 10 H1. Device walks are Block M. |
| SP-094 **R / Bat / CS / L** | Spike 1, H2 battery, cold start, data-loss lifecycle | **Out of this item.** Point at [`SP-094-validation-plan.md`](SP-094-validation-plan.md). Also residual; do not duplicate as executed here. |
| Matrix **D1 / D2 / D3** | SPD-077 slots (Pixel-class / aggressive OEM / optional second API) | Only the evidence-log **Device** column. |
| SP-014 **D-open / D-canyon / D6–D8 / D-batch** | GPS integrity / tunnel / batched samples | Scenario ids. **Not** matrix slots. |
| SP-022 **D1–D2** | Delete country → `.pixr`; redownload rematch | Scenario ids. **Not** matrix slots. |
| SP-031 **D1–D3** | Settlement-only city; rural/outside settlements; subdivision wins (device visual) | Scenario ids. **Not** matrix slots. |
| SP-061 **D4** | Device SP-058 warning steps (paired with I1) | Scenario id. **Not** matrix slot D1. |

Phase 10 **H1** is the matrix *lock*. SP-041 **H1** is a Helsinki
boundary-walk / pan / recentre UX scenario. They are unrelated.
Phase 10 **H2** is the Spike 1 / battery protocol lock. SP-041 **H2**
is tap-to-detail %.

## Approved decisions

| ID | Decision |
| --- | --- |
| This slice | Roster + evidence-log *template* only. Device execution **Residual** (product-owner lock 2026-08-29). Agent does **not** mark SP-095 or Phase 10 Accepted. |
| Matrix | **D1** Pixel-class required. **D2** one aggressive-OEM skin **required** (**SPD-077**). Optional **D3** a second API level (Android 10–12 vs 14–15) **only if** D1/D2 are the same generation (SPD-077). Do not shrink to D1-only without a new SPD. |
| Scripts | Reuse SP-014, SP-022, SP-031, SP-041, SP-061, SP-069, SP-079, SP-087 as written. Do **not** invent a weaker walk set. |
| ABL | Stay absent (**SPD-082**). Do not add `ACCESS_BACKGROUND_LOCATION` to force OEM continuity. SP-014’s older “ABL in Phase 10 if OEM fails” rule is **superseded** by SPD-082: a later D2 Fail needs a **new SPD**, not a silent permission add. |
| Friends | Must **not** appear on public builds (**SPD-085** / SPD-061). Walks fail if friend settings, add-friend deep links, or friend-facing nickname copy surface. |
| Helsinki | Fixture country, **not** an allowlist. Worldwide product. Missing `.spa` → Helsinki area-UX rows **Blocked** / Residual; do **not** substitute a city without administrative polygons and call it SP-031 R3. Enabler: SP-053 LAN `.spa` download. |
| Location data | Map screenshots that show a live position are location data. Do **not** put them in Sentry, this public evidence log, or a public artefact. Text logs + device metadata are the record. |
| Battery / Spike 1 | Quantitative Spike 1 and H2 battery protocol belong to **SP-094** (also residual). Point there. Do not copy empty number cells as SP-095 Pass. SP-014 **B12** (2 h record-battery-only) is absorbed by SP-094 Bat-A/B; this roster does not re-run it. |
| Spike 7 / SP-054 | City-scale routing measurement is H7 **Measure**, not this Device-verify roster. Do not invent a weaker city-scale walk here. Routing *device* walks remain SP-061 I1–I6. |
| Brand / Help URLs | Residual elsewhere (SP-093 / **SPD-080**). Do not retarget Help URLs in this item. |
| Fabrication | Forbidden. Empty device cells until a handset run exists. |

## Scope

When a later WI executes this plan: run the carried Device-verify
scripts on the **SPD-077** matrix (D1 + D2; D3 only if that optional
slot is used), record who / device / OS / build / procedure / result,
and leave Pass / Fail / Residual / Blocked honest.

Evidence-only on the executing branch except defects routed to an
owning WI or a new SP-NNN. This roster does not fix defects found on
walks.

## Device matrix (**SPD-077**)

| Slot | Model | OS / skin | Battery saver / exemptions | Notes |
| --- | --- | --- | --- | --- |
| D1 | Pixel-class already used in this project (Pixel 3a and/or Pixel 10a) | *(fill on walk)* | *(fill)* | Required. Prior SP-014 Pixel 3a work is **citation only** (see evidence log); it does not fill this Phase 10 row. |
| D2 | One aggressive-OEM skin (Xiaomi / HyperOS, Samsung with aggressive sleep, or Huawei) | *(fill)* | *(fill)* | **Required** by **SPD-077**. Screen-off, OEM kill, and Helsinki walks are defined on D1+D2. Not optional. |
| D3 | Optional second API level (Android 10–12 vs 14–15) if D1/D2 are the same generation | | | Use only if that SPD-077 condition holds. Do not invent a third OEM to skip D2. |

**Build for walks (later execution):** same APK / git SHA on every
device. Prefer release or beta. Record SHA, `versionName`,
`versionCode`, flavor, and map-package / `.spa` versions in every row.

**Walker / attestor:** named person. “Tested, works” is not a record.

## Evidence rules (later execution)

Every evidence-log row must record:

| Field | Rule |
| --- | --- |
| Who | Named walker / attestor |
| Device | Slot + model + (if known) serial / marketing name. Do not invent ids in this protocol-only slice. |
| OS / skin | Version and OEM skin |
| Build | Type (release / beta / debug), flavor, git SHA, `versionName` / `versionCode`, map package / `.spa` versions |
| Procedure | Carried scenario id from the named plan (not a paraphrased weaker script) |
| Result | **Pass** / **Fail** / **Residual** / **Blocked** |
| Screenshots | No live-position map shots in this public log. Tunnel-gap / overlay chrome notes may be text. SP-014 D6 historically asked for a screenshot — keep it off the public log; store privately if the executing WI needs it. |

Helsinki area UX without `.spa`: **Blocked**, not Fail, and not a
fabricated Pass on an empty overlay.

Unit tests may be cited as **pointers**. They do **not** substitute
for Device-verify rows.

## Carried script roster (reuse; do not weaken)

Run on **D1 and D2** unless a carried plan says D2-only (B-OEM) or
D3 is in use. One evidence row per (scenario × device). Full pass
conditions stay in the linked plans.

### SP-014 — screen-off, OEM, no gap-fill, pause/resume

Plan: [`SP-014-validation-plan.md`](SP-014-validation-plan.md).

| Carried ids | What |
| --- | --- |
| A1–A7 | Gate and session (no pixels off-session; discard/finish; deny location; notification Pause/Resume/Stop; foreground haptics) |
| B4 | Screen off 30 min walk — continued collection; **record received vs expected (~1800)**; no false interrupt if gaps &lt; 60 s |
| B5 | Other app foreground 15 min walk — continued collection (**~900 expected**) |
| B13 | Full session offline |
| B-OEM | Natural OEM kill (**D2** required). Interrupt UX on reopen; prior pixels kept; **no gap fill**. Do not add ABL (**SPD-082**). |
| C3 | Pause → vehicle ≥1–2 km → resume → walk. No pixels on paused segment; no connecting green; track split |
| D-open, D-canyon, D7, D8, D6, D-batch | GPS integrity / tunnel gap / batched screen-off samples |
| E9, E-reboot, E-air, E-orphan | Interruption recovery; **no gap fill** |
| F-radius, F-poor | 25 m band visual; poor GPS does not invent streets |

**Not this item:** B12 (2 h battery vs control) → [`SP-094-validation-plan.md`](SP-094-validation-plan.md) Bat-A/B.

SP-094 Bat-FGS is a *measurement note* from the multi-hour session, not
this OEM-kill script.

### SP-022 — permanence / rematch / Uusimaa timing

Plan: [`SP-022-validation-plan.md`](SP-022-validation-plan.md).
Scenario ids **D1–D2** in this block are delete / redownload, **not**
matrix slots.

| Carried ids | What |
| --- | --- |
| A1–A4 | Ever-live / imported-only; headered `.pix`; legacy migrate if a fixture exists |
| B1–B4, B-time | Real country update rematch; §27.3 `street_pixels_more_to_explore`; no false toast; Uusimaa-scale wall-clock as available |
| C1–C2 | Force-stop mid-rematch / optional kill during large derive |
| D1–D2 | Delete country → `.pixr` remains; redownload rematch |
| E1–E2 | 15 m unify / `.pix` not densified; eligibility spot-check vs SP-020 register |

S1–S8 measurement slots in that plan stay empty until walked.

### SP-031 — Helsinki names, rural/coastal, settlement (R3)

Plan: [`SP-031-validation-plan.md`](SP-031-validation-plan.md). Device
block, not the automated A–C/G suites. Scenario ids **D1–D3** in this
block are settlement / rural / subdivision visual, **not** matrix slots.

| Carried ids | What |
| --- | --- |
| E2 | Device / UI: no MWM id shown as neighbourhood |
| F1 | Known Helsinki subdivision names (Kamppi, Kallio, Punavuori, …) when FI `.spa` is on device |
| F2 | Settlement-only / rural / coastal as opportunity |
| F3 | Explore → area assignment visible |
| D1–D3 (device visual) | Settlement-only city; rural/outside settlements (exploration allowed, no grid); subdivision wins over settlement |

Automated E1 / Helsinki known-id spot-check under SP-032 does **not**
close F1–F3.

### SP-041 — Helsinki UX (scenario ids H1–H6 **here**)

Plan: [`SP-041-validation-plan.md`](SP-041-validation-plan.md) **Block H**.
These are **Helsinki UX**, not Phase 10 locks H1–H6.

| Carried id | What | Phase 10 lock? |
| --- | --- | --- |
| **H1** | Boundary walk / pan / recentre focus (§12.5) | **No.** Not SPD-077. |
| **H2** | Tap areas → detail exact % | **No.** Not SPD-078 (Phase 10 H2 Spike 1 / battery). |
| **H3** | Zoom street → city summary % | No |
| **H4** | Completed chrome across zooms (§18.6) | No |
| **H5** | Leave settlement → empty state copy (§31) | No |
| **H6** | Confirm no country/world % in UI | **No.** Not SPD-082. |

Needs `.spa` via SP-053. F3 quantitative Spike 1 → SP-094. F4 device
re-spot-check after chrome stays in this roster (same Helsinki walk).

### SP-061 — Prefer/Avoid on device

Plan: [`SP-061-validation-plan.md`](SP-061-validation-plan.md) Block I
(and the device rows A12, B7, C11, C12, D4, E7).

| Carried ids | What |
| --- | --- |
| I1 / D4 | SP-058 warning + no-route copy exactly |
| I2 / A12 | Prefer on walk/bike |
| I3 / B7 | Avoid possible route |
| I4 / C11 | SP-058 no-route Prefer control |
| I5 / E7 / C12 | Avoid follow; GPS off-route after **SP-089** Fix (Prefer+seekbar). Observe the landed dialog; do not re-implement. |
| I6 | Remaining device walks in that plan |

Car vs Avoid remains a non-goal (SP-061 Block H). Mid-nav stability
is I5 / E7.

### SP-069 — milestones / share / haptics / nav

Plan: [`SP-069-validation-plan.md`](SP-069-validation-plan.md) Block I.

| Carried ids | What |
| --- | --- |
| I1 | 25 / 50 / 100 celebration; non-blocking |
| I2 | Card image vs deny-list (eyeball PNG; no live map shot) |
| I3 | Competition-off first-person copy |
| I4 | Explicit share sheet only on tap |
| I5 | Haptics predicate (screen-off / background / toggle off) |
| I6 | Navigation not interrupted |
| first-100 m | Block C *behaviour* on device as opportunity on the same walk. **Not** a Block C unit-test id. No separate I-id in the carried plan. |
| I7 | §22.10 competition-on sentences — observe with SP-079 if competition is on; do not fail SP-069’s original V1 stub |

I7 (§22.10 competition-on sentences) is Phase 8 chrome; observe with
SP-079 if competition is on, do not fail SP-069’s original V1 stub.

### SP-079 — competition opt-in and traffic capture

Plan: [`SP-079-validation-plan.md`](SP-079-validation-plan.md) **Block M**.

| Carried ids | What |
| --- | --- |
| M1 | Opt-in vs spec §20.2 item by item |
| M2 | Traffic capture during recording with competition enabled — **required** on at least one device when executed (upload deny-list: no coordinates) |
| M3 | Opt-out zero upload |
| M4 | Offline queue then flush; stale labelled |
| M5 | N&lt;3 nicknames if a sparse fixture exists |
| M6 | Decay without opening the app (as opportunity / backend) |
| M7 | Delete profile; local exploration intact |
| M8 | No presence copy |

Map screenshots remain forbidden even when a device appears.

### SP-087 — GPX public vs Pro surfaces

Plan: [`SP-087-validation-plan.md`](SP-087-validation-plan.md) **Block M**.

| Carried ids | What |
| --- | --- |
| M1 | Multi-hour GPX → green + area % (no map screenshot) |
| M2–M3 | Competition on: import does not move weekly / ownership |
| M4 | Public APK: no GPX tools, no purchase, share-sheet GPX refused when gated; inflated settings dump |
| M5 | Pro-internal (debug-entitle) tools work |
| M6 | Batch-import several files |
| M7 | Optional analytics readout |

### Also observe (existing scripts; not new walks)

| Source | Script | Notes |
| --- | --- | --- |
| [SP-090](../work-items/SP-090-settings-empty-states-first-run.md) §10 first-run | The five click-through steps already written there | Residual with this item; do not invent a shorter first-run. |
| SP-090 §31 matrix | Each implemented empty/error state (nine rows; copy/actions landed in SP-090) | Hardware eyeball is this roster / SP-097. Do not collapse the matrix into one Pass. |
| **SPD-085** | Friends must not appear | Public build: no friend settings, no add-friend intents, no friend-facing nickname copy. |

## Automated pointers (optional in the executing WI)

None new required. Do **not** substitute desktop suites for this
item. Green unit tests do not close Device-verify rows.

This roster-only slice does **not** require those commands and does
**not** run `adb`.

## Mapping to Phase 10 exit (still residual)

| Exit # | Criterion | This roster | Status in this slice |
| --- | --- | --- | --- |
| 1 | §34 line items with recorded evidence | Device/manual hardware observations | **Execution Residual** (SP-097 maps; this roster is the walk list) |
| 2 | §31 empty/error observed | SP-090 §31 matrix rows (nine states × D1/D2) | **Execution Residual** |
| 6–8 | Battery / Spike 1 / lifecycle | **Not this item** | SP-094 residual |
| — | SP-014 exit #7 OEM screen-off | B4 + B-OEM on D2 | **Residual** until D2 is walked. Pixel 3a SP-014 does **not** close it. |
| — | Phase 6 Spike 7 city-scale | **Not this item** | H7 Measure (SP-054). Routing device walks are SP-061 I* here. |

Do **not** mark Phase 10 exit met.

## Execution order (later handset WI)

1. Confirm release/beta APK SHA; install on D1 (then D2). Record map / `.spa` versions.
2. Confirm Helsinki / Uusimaa MWM and `.spa`. If `.spa` missing, SP-031 F* and SP-041 H1–H6 are **Blocked**.
3. SP-090 first-run on a fresh install (D1 at least); §31 empty/error matrix as opportunity on the same devices.
4. SP-014 Block A, then B4/B5 (record ~1800 / ~900), then B-OEM on D2, then C/D/E as opportunity.
5. SP-022 rematch / delete-redownload / Uusimaa timing as maps allow.
6. SP-031 R3 + SP-041 H1–H6 (Helsinki UX) on D1 and D2.
7. SP-061 routing walks (include off-route Prefer after SP-089).
8. SP-069 milestones / share / haptics / nav.
9. SP-079 opt-in + **traffic capture** on at least one device; opt-out; delete.
10. SP-087 public vs Pro GPX surfaces.
11. Friends-absent eyeball (**SPD-085**) on the public APK.
12. Fill evidence log. Agent does not mark Accepted.

## Non-goals

- Executing any of the above in this Phase 10 coding slice.
- Quantitative Spike 1 / H2 battery / CS1 / L1–L9 (SP-094).
- Spike 7 / SP-054 city-scale routing measurement (H7 Measure).
- Adding ABL. Implementing friends UI. Retargeting privacy URLs (SP-093 residual).
- Inventing a weaker walk script than the carried plans.
- Substituting a non-polygon city for Helsinki R3.
- Marking Phase 10 exit met or this work item Accepted.
