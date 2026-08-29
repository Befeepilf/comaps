# SP-094 — Validation plan (protocol only; not executed in this slice)

**Work item:** [SP-094](../work-items/SP-094-battery-rendering-lifecycle.md)
**Plan authored by:** Agent
**Plan date:** 2026-08-29
**Branch:** `cursor/sp-094-battery-protocol-6383` (lands on `street-pixels`)

**This Phase 10 coding slice does not execute this plan.** Do not run
Spike 1, the H2 battery protocol, cold-start timing, or lifecycle
walks on a handset here. Do not fabricate FPS, battery %, memory, or
lifecycle pass/fail numbers. A later work item executes the catalogue
on hardware and fills
[`SP-094-evidence-log.md`](SP-094-evidence-log.md).

## ID collision warning

Three different “H” series exist. Do **not** mix them.

| Series | What it is | This plan |
| --- | --- | --- |
| Phase 10 locks **H1–H10** | Launch-governance decisions **SPD-077–086** | H1 = device matrix (**SPD-077**). H2 = rendering bar + battery *protocol* (**SPD-078**). Not walk scripts. |
| SP-041 scenarios **H1–H6** | Helsinki *UX* walks (badge / tap / city zoom / completed chrome / §31 empty / no country-world %) | **Out of this item.** Those walks are SP-095 Device-verify residual. |
| This plan **R / Bat / CS / L** | Spike 1 rendering, H2 battery sessions, cold start, data-loss lifecycle | The executable protocol below. |

Phase 10 **H2** is the battery/rendering *lock* (**SPD-078**). SP-041
**H2** is a Helsinki tap-to-detail UX walk. They are unrelated.

## Approved decisions

| ID | Decision |
| --- | --- |
| This slice | Protocol documentation only. Device execution **Residual** (product-owner lock 2026-08-29). Agent does **not** mark SP-094 or Phase 10 Accepted. |
| Rendering bar | Spike 1 unchanged (**SPD-078** / SP-033): p95 ≥30 FPS at zoom 14–16 with a city loaded; overlay memory uplift &lt;150 MB. Fail vs bar → report; do **not** add LOD in the executing WI unless H2 / **SPD-078** is revised. |
| Battery | Protocol lock only. **No numeric %/hour ceiling** (**SPD-078**). Duration ≥2 h is the SP-088 H2 / WI operationalization of SPD-078 “multi-hour”, not a %/hour bar. Maintainer accepts, waives with store copy, or opens a new SPD *after* numbers exist. |
| Cold start | Time to first interactive frame is **recorded, not gated**, unless a later SPD adds a number. |
| Device matrix | **D1** Pixel-class required. **D2** one aggressive-OEM skin **is required** because **SPD-077** (Phase 10 H1) locks D1 + D2. Optional **D3** a second API level if D1/D2 are the same generation. |
| D2 OEM *functional* screen-off continuity | **Not this item.** That walk script is SP-095 residual. Battery Session A still *records* FGS survival on D1 and D2 as a measurement, not as SP-095’s OEM-kill script. |
| ABL | Stay absent (**SPD-082**). Do not add `ACCESS_BACKGROUND_LOCATION` to force a battery or FGS pass. |
| Helsinki / overlay | Spike 1 needs a loaded city **and** street-pixel overlay on. Helsinki / Uusimaa-class needs `.spa` on device (SP-053). If `.spa` is missing, Spike 1 is **Blocked** — do **not** fake FPS on an empty overlay. |
| Build type | Prefer **release** or **beta** APK when later executed. Same APK / git SHA on every device and both battery sessions. Record SHA, `versionName`, `versionCode`, flavor. |
| Schema | No schema change in this item. Observed loss is a defect in an earlier phase; file it as discovered-follow-up / owning SP. Do not “fix” storage format here unless the maintainer splits a Fix WI. |
| Brand / Help URLs | Residual elsewhere (SP-093 / **SPD-080**). Do not retarget Help URLs in this item. |
| Fabrication | Forbidden. Empty number cells until a handset run exists. |

## Scope

When a later WI executes this plan: measure street-pixel rendering
against Spike 1, measure battery under the H2 protocol (recording vs
control), record cold-start time, and prove there is no critical
exploration-data-loss path across the lifecycle matrix.

Evidence-only on the executing branch except defects routed to an
owning WI. Optimisation beyond meeting the locked bars is an explicit
Phase 10 non-goal.

## Device matrix (**SPD-077**)

| Slot | Model | OS / skin | Battery saver / exemptions | GPU / driver | Notes |
| --- | --- | --- | --- | --- | --- |
| D1 | Pixel-class already used in this project (Pixel 3a and/or Pixel 10a) | *(fill on walk)* | *(fill; keep identical for Bat-A and Bat-B)* | *(fill)* | Required for Spike 1, battery pair, cold start, lifecycle |
| D2 | One aggressive-OEM skin (Xiaomi / HyperOS, Samsung with aggressive sleep, or Huawei) | *(fill)* | *(fill; same rule)* | *(fill)* | Required by **SPD-077**. Not optional in the later handset WI. |
| D3 | Optional second API level (Android 10–12 vs 14–15) if D1/D2 are the same generation | | | | Nice-to-have |

**Build for walks (later execution):** same APK / git SHA on every
device. Prefer release or beta, not debug, for battery and FPS.

**Walker / attestor:** named person. “Tested, works” is not a record.

## Evidence rules (later execution)

Every evidence-log row must record:

| Field | Rule |
| --- | --- |
| Who | Named walker / attestor |
| Device | Slot + model + (if known) serial / marketing name. Do not invent ids in the protocol-only slice. |
| OS / skin | Version and OEM skin |
| Build | Type (release / beta / debug), flavor, git SHA, `versionName` / `versionCode` |
| Procedure | Scenario id from this plan, including city/MWM, overlay on/off, zoom, saver settings |
| Numbers | FPS p95, memory uplift MB, battery %/hour and mAh if available, cold-start ms, rematch duration — only from the walk. Empty until then. |
| Result | **Pass** / **Fail** / **Residual** / **Blocked** |
| GPU / driver | Required on Spike 1 rows |

Helsinki Spike 1 without `.spa`: **Blocked**, not Fail, and not a
fabricated Pass on an empty overlay.

Unit tests (rematch, interruption, deletion, week-boundary) may be
cited as **pointers**. They do **not** substitute for device
lifecycle rows.

## Scenario catalogue

Run on **D1 and D2** unless noted. One evidence row per
(scenario × device).

### Block R — Rendering / Spike 1 (**SPD-078** / SP-033)

Preconditions: city MWM installed (Helsinki / Uusimaa-class);
street-pixel overlay **on**; `.spa` present for Helsinki
administrative overlay (SP-053). If `.spa` is missing: **Blocked**.

| ID | Scenario | Pass / measurement |
| --- | --- | --- |
| R1 | City loaded, overlay on, pan and zoom at zoom **14–16** for a sustained sample (minutes, not a single fling) | Record p95 FPS (or equivalent frame-time p95). **Pass bar:** p95 ≥30 FPS. Record GPU/driver. Record the FPS method in notes (`dumpsys gfxinfo`, FrameMetrics, systrace, or equivalent). Do not invent a method-specific pass bar. |
| R2 | Overlay memory uplift vs overlay off (same city, same zoom band, same build) | Record app memory before overlay and with overlay on (method in notes: `dumpsys meminfo` PSS / Graphics, RSS, or equivalent — same method both sides). **Pass bar:** overlay memory uplift &lt;150 MB. |
| R3 | Fail vs bar | **Fail** the Spike 1 row; report. Do **not** add LOD, drop overlay density, or retune nside in the executing WI. LOD is a new WI only if **SPD-078** / H2 is revised. |

SP-033 qualitative Pixel 3a (Accepted 2026-08-07) is **not**
quantitative Spike 1. Do not copy qualitative “OK” into R1/R2 number
cells.

### Block Bat — Battery H2 protocol (**SPD-078**)

Two sessions on the **same device**, **same** battery-saver /
exemption / adaptive-battery settings, **same** APK. Screen off for
both. Load-bearing H2 bullets (SP-088): Session A recording active,
**no in-app navigation (routing)**, ≥2 h; Session B recording off,
**app not force-stopped**, ≥2 h. **No navigation** means no turn-by-turn
routing, not “remain stationary”. Walking with screen off is allowed
and is what Bat-pix needs to prove collection continuity.

| ID | Scenario | Pass / measurement |
| --- | --- | --- |
| Bat-A | Session A: **recording on**, screen off, **no in-app navigation**, ≥**2 hours** (SP-088 H2) | Record start/end battery %, duration, %/hour, mAh if the OEM exposes it (dumpsys / AccuBattery / similar — method in notes). Record whether the location FGS survived the whole window. Record whether pixels continued (new greens after reopen vs start snapshot). |
| Bat-B | Session B: **control** — app installed, **recording off**, screen off, **no in-app navigation**, **app not force-stopped**, ≥**2 hours** (SP-088 H2 / **SPD-078** control) | Same metrics as Bat-A. Subtract conceptually: recording cost ≈ A − B on that device. |
| Bat-cmp | Compare A vs B on that device | Record both %/hour figures and the delta. **No numeric %/hour ceiling in this plan.** Maintainer accept / waive / new SPD only after numbers exist. |
| Bat-FGS | FGS survival during Bat-A | Survived whole window / killed / interrupted. Interruption UX on reopen must match Phase 2 (toast; prior pixels kept; **no gap fill**). Do not add ABL to force survival (**SPD-082**). |
| Bat-pix | Pixels continued during Bat-A | Yes / no / unknown (if the walker was stationary, record that — stationary screen-off cannot prove collection continuity; prefer a real walk or a known moving route). |

Repeat the Bat-A + Bat-B **pair** on D1 and on D2. Do not reuse D1
numbers for D2.

SP-014 scenario B12 was “record battery only, no Phase 2 pass bar”.
This block is that protocol, locked and extended by **SPD-078**. It
still has **no** %/hour pass bar.

Aggressive-OEM *kill* as a functional walk remains SP-095. Bat-FGS
is the measurement note from the multi-hour session, not SP-095’s
OEM-kill script.

### Block CS — Cold start

| ID | Scenario | Pass / measurement |
| --- | --- | --- |
| CS1 | Cold start with a large city loaded (Helsinki / Uusimaa-class MWM present; process not in memory) | Force-stop (or reboot) so the process is not in memory, then launch. Record time to **first interactive frame** (stopwatch or systrace; define the mark in notes: first pannable frame, not splash). **Recorded, not gated**, unless a later SPD adds a number. |

### Block L — Data-loss / lifecycle matrix

No interpolated exploration across a pause, interruption, or rejected
sample (product invariant). No schema change in this item.

| ID | Scenario | Pass condition |
| --- | --- | --- |
| L1 | **Upgrade** from a prior Street Pixels build with existing `.pix` | Explored greens survive; file migrates or remains readable; no wipe. Prefer a build old enough to carry a real `.pix` (headered; legacy headerless if a fixture exists). |
| L2 | **Map update rematch** (Phase 3) + user-visible **§27.3** message | Surviving cells stay green. If the explored fraction drops because the denominator grew, toast uses `street_pixels_more_to_explore` (“…Your progress is still saved.”) and never claims personal progress was deleted. Record rematch wall-clock on large `.pix` (Phase 3 residual duration). Pointers (not a substitute): `Rematch_*` in `libs/map/street_pixels_tests/rematch_tests.cpp`. |
| L3 | **Force stop** during recording | App info → Force stop (or `adb shell am force-stop` of the app id) during an **active** recording, then reopen. Interrupt toast on reopen; pixels from before the kill remain; **no gap fill**; session force-finished per SP-013. Pointers: `InterruptedSession_PixelsBeforeInterruptionIntact`, `InterruptedSession_AfterEffects_NoInterpolate_DiscCollected`. |
| L4 | **Low-memory kill** during recording or rematch | Not the same as L3 force-stop. Use recents-kill, `adb shell am kill`, or fill RAM until the process dies. Same permanence as L3 / rematch interrupt: no exploration wipe; no gap fill; rematch recovers or rolls back (`Rematch_InterruptBeforeRenameKeepsOld`). |
| L5 | **Device restart** with an active session | Reboot the handset while recording is active (FGS running). Same as L3: interrupt UX; prior pixels; no gap fill; breadcrumb consumed (`RecordingSession_BreadcrumbPersistenceAcrossRestart` is a pointer only). |
| L6 | **Time-zone change** / weekly boundary (**SPD-060**) | Two clauses, both required for Pass: (1) weekly city week is Monday 00:00 in the **city IANA zone** when known, else **UTC**; (2) changing the **phone** time zone must not move the week bucket. **Never** the device’s local zone. Local timestamps on tracks/UI stay local-display. Pointers: `WeeklyCityWeek_DeviceTzIgnored`, `WeeklyCityWeek_UtcMondayBoundary`, `WeeklyCityLive_MondayBoundarySeparateWeeks`, `WeeklyCityLive_TzChangesWeekIdVsUtc`. **Until the IANA Fix WI:** `WeekBoundsFromUnix` ignores the IANA argument (see work-item follow-up). Do **not** mark device L6 Pass on clause (2) alone. Record UTC-only vs a stored city IANA as Fail or Residual against clause (1), not as Pass. |
| L7 | **Storage nearly full** during pixel derive / migration | Fill free space (large file on shared storage / `fallocate`) until the derive or migration cannot complete. App does not wipe `.pix` to recover space. Failure is visible (error / incomplete), progress that was already durable remains. Optional: kill mid large derive (SP-022 C2). |
| L8 | **Delete competition profile** | Local exploration (`.pix` greens, personal completion) **intact**. Recency / identity / upload queue clear as designed. Pointer: `CompetitionDeletion_SuccessClearsRecencyKeepsPix`. |
| L9 | **Clear app data** wipes as the privacy policy claims | Everything the policy says is on-device is gone (exploration, tracks, identity, DBs). **Policy/terms landing is SP-093 residual** — this row stays Residual until (a) this walk runs **and** (b) landed policy sentences exist to check against. Do not retarget Help URLs here. |

## Automated pointers (optional in the executing WI)

None new required. If the tree is built, record counts; do not weaken
tests.

```bash
# pointers only — do not treat green unit tests as device Pass
# omim --filter is ECMAScript std::regex (unescaped | is alternation; \| matches a literal pipe)
./tools/unix/build_omim.sh -d street_pixels_tests street_pixels_areas_tests
# or existing binaries:
# $BIN/street_pixels_tests --data_path=... --user_resource_path=... --filter='Rematch_'
# $BIN/street_pixels_tests ... --filter='InterruptedSession_'
# $BIN/street_pixels_tests ... --filter='RecordingSession_Breadcrumb'
# $BIN/street_pixels_tests ... --filter='CompetitionDeletion_'
# $BIN/street_pixels_areas_tests --data_path=... --user_resource_path=... --filter='WeeklyCityWeek_|WeeklyCityLive_Monday|WeeklyCityLive_Tz'
```

Filtered pointer runs above do not load Eligibility. A **full**
`street_pixels_tests` run without `data/classificator.txt` aborts
Eligibility; record that as **environment**, not a product Fail.

This protocol-only slice does **not** require those commands.

## Mapping to Phase 10 exit (still residual)

| Exit # | Criterion | This plan | Status in this slice |
| --- | --- | --- | --- |
| 6 | Battery during active recording measured and accepted | Block Bat | Protocol documented; **execution Residual**. Maintainer accept/waive only after numbers. |
| 7 | Rendering on the release build meets recorded criteria | Block R | Spike 1 bar locked; **execution Residual**. |
| 8 | No critical exploration-data-loss path | Block L | **Execution Residual**. |

Do **not** mark Phase 10 exit met.

## Execution order (later handset WI)

1. Confirm release/beta APK SHA; install on D1 (then D2).
2. Confirm Helsinki / Uusimaa MWM and `.spa`. If `.spa` missing, Block R is **Blocked**.
3. CS1 cold start (before warming the process for other work).
4. R1–R2 Spike 1 (overlay on; record GPU/driver). R3 is fail-vs-bar
   disposition, not a separate walk.
5. L1 upgrade (may be first install of the walk APK over a prior build).
6. Bat-A then Bat-B on D1 (≥2 h each; same saver settings).
7. L2–L9 as opportunity allows (L6 before/after a Monday boundary if scheduling permits).
8. Repeat Bat pair + critical L rows + R1/R2 on D2.
9. Fill evidence log; maintainer accept/waive battery; agent does not mark Accepted.

## Non-goals

- Executing any of the above in this Phase 10 coding slice.
- SP-041 H1–H6 Helsinki UX walks (SP-095).
- SP-095 aggressive-OEM functional screen-off continuity script.
- Performance work, LOD, filter loosening, ABL, Help URL retarget, brand.
- Inventing a %/hour ceiling.
