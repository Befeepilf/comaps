# SP-094 — Battery, rendering, and lifecycle measurement

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H1/H2 Accepted. SP-089 glyph fix if H7 includes
  it (re-measure overlay after). Phase 10 implementation entry. A
  release-configured APK.
**Unblocks:** SP-097 exit #6, #7, #8

---

## Objective

Measure street-pixel rendering against the Spike 1 bar, measure
battery under the H2 protocol during active recording vs control, and
prove there is no critical exploration-data-loss path across app
upgrade, map update, force stop, low-memory kill, device restart,
time-zone change, and near-full storage.

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

- **Rendering (Spike 1):** city loaded (Helsinki / Uusimaa-class),
  overlay on, pan/zoom at zoom 14–16, p95 FPS and overlay memory
  uplift. Device D1 required; D2 if H1 says so. Record GPU/driver.
  Fail vs bar → report; do not add LOD here unless H2 is revised.
- **Battery (H2 protocol):** Session A recording screen-off ≥2 h;
  Session B control recording-off screen-off ≥2 h; same device and
  saver settings. Record %/hour, FGS survival, whether pixels
  continued. Maintainer accepts/waives numbers.
- **Cold start** with a large city: time to first interactive frame
  (recorded; gated only if H2 added a number).
- **Data-loss matrix:**
  - Upgrade from a prior Street Pixels build with existing `.pix`
  - Map update rematch (Phase 3); user-visible §27.3 message
  - Force stop during recording (Phase 2 interruption; no gap fill)
  - Low-memory kill
  - Device restart with an active session
  - Time-zone change (weekly week boundary SPD-060; local timestamps)
  - Storage nearly full during pixel derive / migration
  - Delete competition profile: local exploration intact
  - Clear app data: everything gone as the policy claims (SP-093)
- Evidence log: who, device, OS, build type, procedure, numbers,
  pass/fail/residual.
- No schema change. Loss is a defect in an earlier phase; file it
  as discovered-follow-up / owning SP, do not “fix” storage format
  in this measurement item unless the maintainer splits a Fix WI.

## Out-of-scope behavior

- Aggressive-OEM *functional* screen-off continuity as a walk
  script (SP-095). This item may share the battery session with
  that walk if the log separates FPS/battery from “samples still
  arrive”.
- Performance work beyond the bar.
- Fabricating device numbers in an agent environment without a
  handset.

## Relevant product requirements

- Spec §11.2, §27.3, §33.12, §34 Quality / Offline.
- SP-033 Spike 1; SPD-016–019; SPD-060; draft SPD-077 / SPD-078.

## Relevant source files or symbols

- `StreetPixelRenderer` / overlay
- `.pix` / rematch / `CleanupStreetPixels` absence
- `RecordingSession` interruption
- Weekly store timezone

## Implementation notes / constraints

- Prefer a release (or beta) APK, not debug, for battery and FPS.
- Helsinki walks need `.spa` on device (SP-053). If download is
  still blocked, record Blocked and do not fake FPS on an empty
  overlay.
- Map-update timing on large `.pix` (Phase 3 residual) is a
  duration measurement in this log.

## Acceptance criteria

1. Spike 1 numbers recorded on D1 (and D2 if locked) or an explicit
   blocked reason.
2. H2 battery protocol executed; maintainer accept/waive recorded.
3. Every lifecycle case has pass / fail / residual with evidence.
4. No critical loss left without an owning Fix work item.
5. Agent does not mark Accepted.

## Required automated tests

- None new required. Rematch / interruption unit tests remain
  green if the tree is built in this environment; record counts if
  run.

## Required manual validation

- All in-scope measurements are manual on H1 devices.

## Failure and rollback considerations

- FPS below bar: do not silently drop overlay density. Escalate
  (LOD is a new WI, not this one).
- Battery rejected: store copy / OEM caveats, or a session-UX
  change in a new WI — not a filter loosening.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Evidence log | `docs/implementation/validation/SP-094-evidence-log.md` (create in this item) |
| Spike 1 | |
| Battery | |
| Lifecycle table | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
