# SP-095 — Device-matrix residual close-out

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H1/H6. SP-089–093 for defects/copy that the
  walks observe. `.spa` on device for Helsinki (SP-053). Phase 10
  implementation entry. Release or beta APK.
**Unblocks:** SP-097 (cites this evidence for overlapping §34 lines)

---

## Objective

Execute the carried **Device-verify** residuals from Phases 2–9 on
the H1 matrix (Pixel-class + aggressive OEM), with recorded evidence,
including screen-off recording, Helsinki area UX, routing on device,
milestones/share/haptics, competition opt-in and traffic capture, and
GPX public vs Pro surfaces.

---

## Motivation

Earlier phase exits residualled hardware walks rather than failing.
Phase 10 exists to close them. Bundling them into SP-097’s §34
checklist without a roster produces a rubber-stamp. This item is the
roster and the scripts; SP-097 maps them onto §34.

---

## In-scope behavior

Produce `docs/implementation/validation/SP-095-validation-plan.md`
and `SP-095-evidence-log.md`. Execute, do not summarise from memory.

**Roster (H1):** D1 Pixel-class; D2 aggressive OEM; D3 only if locked.

**Carried scripts (reuse earlier plans; do not invent weaker ones):**

- SP-014: screen-off continuity; OEM kill; no gap-fill; pause/resume
  barriers. ABL remains absent unless H6 changed it.
- SP-022: permanence across update/delete-redownload; rematch UX;
  Uusimaa-scale timing as available.
- SP-031 R3: Helsinki neighbourhood names (no MWM-id); rural/coastal;
  settlement fallback.
- SP-041 H1–H6: badge, focus, tap, city zoom, completed chrome, §31
  no-area empty; no country/world % UI.
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

- Quantitative Spike 1 / battery protocol (SP-094). May run on the
  same outing if logs are separate.
- Fixing defects found (owning WI or new SP-NNN).
- Fabricating walks when `adb` / device is absent — residual stays
  open; do not close H7 Device-verify rows.

## Relevant product requirements

- Spec §11.2, §16, §34 Recording / GPS / Progress / Routing /
  Privacy / Sharing / Explorer Pro.
- Validation plans SP-014, SP-022, SP-031, SP-041, SP-061, SP-069,
  SP-079, SP-087.

## Relevant source files or symbols

- Validation plans under `docs/implementation/validation/`
- Device enabler: SP-053 LAN `.spa` download

## Implementation notes / constraints

- Build type, app version/SHA, map package versions in every row.
- Worldwide product: Helsinki is the *fixture*, not an allowlist.
- Friends must not appear (SPD-061).
- Competition traffic capture is required for the upload deny-list
  on at least one device.

## Acceptance criteria

1. Plan + evidence log exist.
2. Each Device-verify residual is pass / fail / still-residual with
   device ids.
3. D2 OEM screen-off has a recorded result (closes SP-014 exit #7
   posture) or an explicit waiver SPD.
4. Agent does not mark Accepted.

## Required automated tests

- None. Do not substitute desktop suites for this item.

## Required manual validation

- The entire item.

## Failure and rollback considerations

- Failed OEM continuity: do not add ABL inside this item; return to
  H6 / SP-092.
- Missing `.spa`: Helsinki rows stay residual; do not substitute a
  city without administrative polygons and call it R3.

## Completion evidence

| Field | Value |
| --- | --- |
| Validation plan | |
| Evidence log | |
| Device roster | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
