# SP-094 — Battery, rendering, and lifecycle measurement

**Phase:** 10 — Android release hardening
**Status:** Protocol documented 2026-08-29. **Device execution:
  Residual.** Not Accepted.
**Depends on:** SP-088 H1/H2 Accepted (**SPD-077**, **SPD-078**).
  SP-089 glyph fix if H7 includes it (re-measure overlay after;
  measurement on device is residual). Phase 10 implementation entry.
  A release-configured APK is required only when device execution is
    no longer residual.
**Unblocks:** SP-097 exit #6, #7, #8 (those remain residual until a
  handset run exists)
**Notes:** **Device execution residual.** H2 / Spike 1 / lifecycle
  protocol recorded in
  [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md).
  Evidence log is Residual (not executed). Do not execute Spike 1,
  battery protocol, or lifecycle walks on a handset in this Phase 10
  coding slice.

---

## Objective

Measure street-pixel rendering against the Spike 1 bar, measure
battery under the H2 protocol during active recording vs control, and
prove there is no critical exploration-data-loss path across app
upgrade, map update, force stop, low-memory kill, device restart,
time-zone change, and near-full storage.

**Device execution is residual** (product-owner lock 2026-08-29).
This item may record the **SPD-078** protocol in docs. Do not execute
hardware walks here.

---

## Motivation

Spec §34 Quality requires acceptable rendering, acceptable battery,
and no critical exploration-data loss. Spike 1 quantitative numbers
were deferred (SP-033 / SP-041 R2). Battery has no spec number —
H2 supplies the protocol. Lifecycle cases are listed in the phase
file and never had a combined evidence log.

These are measurements. Optimisation beyond meeting the locked bars
is an explicit Phase 10 non-goal.

---

## In-scope behavior

**Docs only in this Phase 10 coding slice.** Record the H2 protocol
(**SPD-078**). Do not execute the measurements below on a handset.

When device execution is no longer residual, the intended measurements
are:

- **Rendering (Spike 1):** city loaded (Helsinki / Uusimaa-class),
  overlay on, pan/zoom at zoom 14–16, p95 FPS and overlay memory
  uplift. Device D1 required; D2 if H1 says so (**SPD-077**). Record GPU/driver.
  Fail vs bar → report; do not add LOD here unless H2 is revised.
- **Battery (H2 protocol):** Session A recording screen-off ≥2 h;
  Session B control recording-off screen-off ≥2 h; same device and
  saver settings. Record %/hour, FGS survival, whether pixels
  continued. Maintainer accepts/waives numbers.
- **Cold start** with a large city: time to first interactive frame
  (recorded; gated only if a later SPD added a number).
- **Data-loss matrix:**
  - Upgrade from a prior Street Pixels build with existing `.pix`
  - Map update rematch (Phase 3); user-visible §27.3 message
  - Force stop during recording (Phase 2 interruption; no gap fill)
  - Low-memory kill
  - Device restart with an active session
  - Time-zone change (weekly week boundary SPD-060; local timestamps)
  - Storage nearly full during pixel derive / migration
  - Delete competition profile: local exploration intact
  - Clear app data: everything gone as the policy claims (SP-093
    **residual**)
- Evidence log: who, device, OS, build type, procedure, numbers,
  pass/fail/residual.
- No schema change. Loss is a defect in an earlier phase; file it
  as discovered-follow-up / owning SP, do not “fix” storage format
  in this measurement item unless the maintainer splits a Fix WI.

## Out-of-scope behavior

- Aggressive-OEM *functional* screen-off continuity as a walk
  script (SP-095 **residual**).
- Performance work beyond the bar.
- Fabricating device numbers in an agent environment without a
  handset.
- Executing Spike 1, battery protocol, or lifecycle walks on a
  handset in this Phase 10 coding slice.

## Relevant product requirements

- Spec §11.2, §27.3, §33.12, §34 Quality / Offline.
- SP-033 Spike 1; SPD-016–019; SPD-060; **SPD-077**, **SPD-078**.

## Relevant source files or symbols

- `StreetPixelRenderer` / overlay
- `.pix` / rematch / `CleanupStreetPixels` absence
- `RecordingSession` interruption
- Weekly store timezone

## Implementation notes / constraints

- Prefer a release (or beta) APK, not debug, for battery and FPS
  *when device execution is no longer residual*.
- Helsinki walks need `.spa` on device (SP-053). If download is
  still blocked, record Blocked and do not fake FPS on an empty
  overlay.
- Map-update timing on large `.pix` (Phase 3 residual) is a
  duration measurement in this log *when executed*.
- Product-owner lock 2026-08-29: do not execute these on a handset
  in this slice.

## Acceptance criteria

1. Protocol documented (**SPD-078**). Spike 1 numbers on D1 (and D2
   if locked) remain residual until a handset run exists.
2. H2 battery protocol documented; execution residual. Maintainer
   accept/waive recorded only after numbers exist.
3. Every lifecycle case stays residual (or pass/fail if a later
   handset run exists).
4. No critical loss left without an owning Fix work item, when
   observed.
5. Agent does not mark Accepted. Do not fabricate a handset.

## Required automated tests

- None new required. Rematch / interruption unit tests remain
  green if the tree is built in this environment; record counts if
  run.

## Required manual validation

- All in-scope measurements are manual on H1 devices (**SPD-077** D1 + D2).
  Protocol: [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md).
  **Execution is residual** in this Phase 10 coding slice.

## Failure and rollback considerations

- FPS below bar: do not silently drop overlay density. Escalate
  (LOD is a new WI, not this one).
- Battery rejected: store copy / OEM caveats, or a session-UX
  change in a new WI — not a filter loosening.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-094-battery-protocol-6383` |
| Protocol recorded | [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md) (2026-08-29). This slice did **not** execute the plan. |
| Evidence log | [`validation/SP-094-evidence-log.md`](../validation/SP-094-evidence-log.md) — every scenario **Residual** (device; not executed) |
| Spike 1 | Residual (device execution) |
| Battery | Residual (device execution); protocol locked **SPD-078** (no %/hour ceiling) |
| Lifecycle table | Residual (device execution) |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Spike 1 (R1–R3), H2 battery (Bat-A/B), cold start (CS1), and lifecycle L1–L9 remain unexecuted on hardware | Residual (this item). Later handset WI executes [`validation/SP-094-validation-plan.md`](../validation/SP-094-validation-plan.md). Do not invent OEM realised results. |
| Helsinki Spike 1 needs `.spa` on device (SP-053) | If missing at execution: Blocked — do not fake FPS on an empty overlay. |
| Clear-app-data vs policy claims (L9) also needs landed Street Pixels policy/terms | Coupled residual with SP-093 / **SPD-080**. Do not retarget Help URLs here. |
| `WeekBoundsFromUnix` ignores the IANA argument and always UTC-falls-back (`(void)ianaTz` in `libs/street_pixels_areas/weekly_city_week.cpp`). Store can persist tz (`SetCityIanaTz`) but query still uses UTC (`WeeklyCityLive_TzChangesWeekIdVsUtc` asserts that). Contradicts **SPD-060** “Monday 00:00 in the city IANA zone when known”. | Recorded, not fixed in this docs item. Owning follow-up is a later Fix WI (SP-073 family / new SP), not LOD or battery work. L6 device walk will observe UTC-only until then. |
