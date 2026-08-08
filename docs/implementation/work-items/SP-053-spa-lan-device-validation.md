# SP-053 — LAN device validation: download `.spa` via the app

**Phase:** 4 residual / pre-production packaging; **enables** Phase 5 / Phase 10
Helsinki device walks
**Status:** Planned
**Depends on:** SP-049–052 implemented (or Channel B documented and usable)
**Unblocks:** Phase 10 Helsinki area UX walks that need live `.spa` on device

---

## Objective

Prove on real hardware that a Finland leaf `.spa` arrives **only** through the
production download path (Custom Maps server → SP-046), lands beside the MWM,
loads in the exploration sidecar API, and unblocks Phase 5 UI walks — without
ADB file copies into app-private storage.

Evidence-only discipline (SP-014 / SP-022 / SP-031 / SP-041 pattern). Agent
does not mark Accepted or Phase 5 exit Met.

---

## Motivation

SP-041 residualled device walks to Phase 10 partly because `.spa` was not on
device. Scoped storage makes sideload unreliable. This WI is the acceptance
gate for the LAN distribute track and the precondition checklist for those
walks.

---

## In-scope behavior

### 1. Validation plan + evidence log

Create:

- `docs/implementation/validation/SP-053-validation-plan.md`
- `docs/implementation/validation/SP-053-evidence-log.md`

### 2. Scenario catalogue (minimum)

| ID | Scenario | Pass |
| --- | --- | --- |
| S1 | Assemble FI publish tree (SP-050); serve (SP-051) | Health OK; curl spa 200 |
| S2 | Device sets Custom Maps URL to LAN (never build-default) | URL persisted; native applied |
| S3 | Advertisement present (Channel A or B from SP-052) | `HasRemoteSpa` for Helsinki leaf (log or debug) |
| S4 | Download or spa-retry Helsinki leaf | `.spa` OnDisk beside `.mwm`; SHA OK |
| S5 | Fail-soft: stop server mid-spa after Map OnDisk | Map remains usable; IncompleteSpa set |
| S6 | Retry after server restored | Spa recovers; incomplete cleared |
| S7 | Sidecar load | Areas non-empty; DisplayName not MWM id; fail-closed if deleted |
| S8 | Delete map | `.spa` removed; personal `.pix`/`.spx` retained (**SPD-030**) |
| S9 | Phase 5 smoke (optional same session) | Badge/focus/tap spot-check on Helsinki — or explicit handoff to Phase 10 H1–H6 |

### 3. Device matrix

| Slot | Device | Notes |
| --- | --- | --- |
| D1 | Pixel 3a (or same class as SP-014/033) | Required if available |
| D2 | Aggressive OEM | Optional; else Phase 10 |

Record: OS, build type, APK `versionName`, git SHA, LAN URL, Channel A vs B,
`dataVersion`, `MAP_SERIES`.

### 4. Explicit non-goals

- Declaring Phase 5 exit Met.
- Quantitative Spike 1 FPS (still Phase 10).
- Worldwide leaves.

---

## Out-of-scope behavior

- New features beyond fixes blocking S1–S8.
- Weakening tests.
- Fabricating device results.

---

## Acceptance criteria

1. Validation plan + evidence log exist; executed rows filled with real output.
2. S4 + S7 pass on D1 (or residual explicitly: no device / no signing key /
   blocked LAN — with what was tried).
3. S5–S6 pass or residual with reason.
4. No ADB push of `.spa` used as the pass method.
5. Pointer from Phase 10 carried residuals / SP-041 R1 that `.spa` on-device
   precondition is Met or still blocked.
6. Maintainer decides acceptance.

---

## Required automated tests

- None new required beyond SP-050–052; re-run
  `Storage_SpaDownload*` / `Storage_SpaIncomplete*` / `Storage_SpaLifecycle*`
  and record counts.

## Required manual validation

- Full S1–S8 on device when hardware + LAN available.

## Failure and rollback

- Failed S4 blocks claiming the distribute track ready for Phase 5 walks.
- Report owning WI (050–052); do not invent sideload workaround as success.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Phase 5 H1–H6 Helsinki walks | Phase 10 (SP-041 R1) once S4/S7 Met |
| CDN production cutover | ops after this track |
| Incomplete spa Android chrome | Phase 10 |
