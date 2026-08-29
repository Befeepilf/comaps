# SP-095 — Device-matrix residual close-out

**Phase:** 10 — Android release hardening
**Status:** Residual
**Depends on:** SP-088 H1/H6 Accepted (**SPD-077**, **SPD-082**).
  SP-089–092 for defects/copy that the walks would observe. `.spa` on
  device for Helsinki (SP-053). Phase 10 implementation entry. Release or
  beta APK.
**Unblocks:** SP-097 (would cite this evidence for overlapping §34
  lines; those lines stay residual until a handset run exists)
**Notes:** **Residual** (device walks). Product-owner lock 2026-08-29:
  do not execute the H1 matrix, OEM continuity, Helsinki walks,
  traffic capture, or other hardware walks in SP-089–097. The
  Device-verify *classification* is locked (**SPD-083**); execution
  is residual.

---

## Objective

Execute the carried **Device-verify** residuals from Phases 2–9 on
the H1 matrix (Pixel-class + aggressive OEM), with recorded evidence,
including screen-off recording, Helsinki area UX, routing on device,
milestones/share/haptics, competition opt-in and traffic capture, and
GPX public vs Pro surfaces.

**This work is residual.** Do not execute hardware walks in this
Phase 10 coding slice.

---

## Motivation

Earlier phase exits residualled hardware walks rather than failing.
Phase 10 exists to close them. Bundling them into SP-097’s §34
checklist without a roster produces a rubber-stamp. This item is the
roster and the scripts; SP-097 maps them onto §34.

Product-owner lock 2026-08-29 residualised on-device testing.

---

## In-scope behavior

**None in Phase 10 coding.** Recorded residual work, for a later WI:

Produce `docs/implementation/validation/SP-095-validation-plan.md`
and `SP-095-evidence-log.md`. Execute, do not summarise from memory.

**Roster (H1 / SPD-077):** D1 Pixel-class; D2 aggressive OEM; D3 only if locked.

**Carried scripts (reuse earlier plans; do not invent weaker ones):**

- SP-014: screen-off continuity; OEM kill; no gap-fill; pause/resume
  barriers. ABL remains absent (**SPD-082**) unless a later SPD changes it.
- SP-022: permanence across update/delete-redownload; rematch UX;
  Uusimaa-scale timing as available.
- SP-031 R3: Helsinki neighbourhood names (no MWM-id); rural/coastal;
  settlement fallback.
- SP-041 validation scenarios H1–H6 (Helsinki device walks: badge,
  focus, tap, city zoom, completed chrome, §31 no-area empty; no
  country/world % UI). These are SP-041 evidence-log ids, **not**
  Phase 10 locks H1–H6.
- SP-061: Prefer/Avoid on walk/bike; no-route Prefer control;
  mid-nav stability; off-route after SP-089 if that Fix landed.
- SP-069: 25/50/100, first-100 m, card deny-list eyeball, explicit
  share, haptics predicate, nav not interrupted.
- SP-079: opt-in vs §20.2; traffic capture of upload; opt-out zero
  upload; offline queue; N&lt;3 nicknames if a sparse fixture exists;
  delete profile / local intact; no presence copy.
- SP-087: public APK no GPX tools / no purchase; Pro-internal tools
  work; share-sheet GPX refused when gated; import does not move
  weekly/ownership.

Map screenshots that show a live position are location data — do not
put them in Sentry or a public evidence log. Text logs + device
metadata are the record.

## Out-of-scope behavior

- All hardware walks in this Phase 10 coding slice.
- Quantitative Spike 1 / battery protocol (SP-094; also device
  execution residual).
- Fixing defects found (owning WI or new SP-NNN).
- Fabricating walks when `adb` / device is absent.

## Relevant product requirements

- Spec §11.2, §16, §34 Recording / GPS / Progress / Routing /
  Privacy / Sharing / Explorer Pro.
- Validation plans SP-014, SP-022, SP-031, SP-041, SP-061, SP-069,
  SP-079, SP-087.
- **SPD-077**, **SPD-082**, **SPD-083**.

## Relevant source files or symbols

- Validation plans under `docs/implementation/validation/`
- Device enabler: SP-053 LAN `.spa` download

## Implementation notes / constraints

- Do not execute this item in SP-089–097.
- Build type, app version/SHA, map package versions in every row
  *when a later WI executes*.
- Worldwide product: Helsinki is the *fixture*, not an allowlist.
- Friends must not appear (SPD-061 / **SPD-085**).
- Competition traffic capture is required for the upload deny-list
  on at least one device *when executed*.

## Acceptance criteria

Not applicable in this Phase 10 coding slice. Residual until a later
work item executes the H1 matrix.

When that later item runs:

1. Plan + evidence log exist.
2. Each Device-verify residual is pass / fail / still-residual with
   device ids.
3. D2 OEM screen-off has a recorded result (closes SP-014 exit #7
   posture) or an explicit waiver SPD.
4. Agent does not mark Accepted.

## Required automated tests

- None. Do not substitute desktop suites for this item.

## Required manual validation

- The entire item. **Execution is residual.**

## Failure and rollback considerations

- Failed OEM continuity: do not add ABL inside this item; return to
  H6 / SP-092 / a new SPD (**SPD-082** keeps ABL absent).
- Missing `.spa`: Helsinki rows stay residual; do not substitute a
  city without administrative polygons and call it R3.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | Residual |
| Evidence log | Residual |
| Device roster | Residual (SPD-077 matrix defined; not executed) |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| H1 matrix, OEM continuity, Helsinki, traffic capture | Residual (this item); not SP-089–097 coding |
