# SP-090 — Settings, empty-state, and first-run audit

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 (H4/H9 for policy and friends copy). Phase 10
  implementation entry.
**Unblocks:** SP-093 (policy URLs may land here or there); SP-095 /
  SP-097 observe the surfaces

---

## Objective

Make Android settings match product spec §30, implement every spec §31
error and empty state that is missing or wrong, and walk spec §10
first-run so a new user can record without a tutorial — without
exposing radius or internal parameters.

---

## Motivation

Phase 10 exit criteria 2 and 3 are §31 and §30. `prefs_privacy.xml`
today is search history and Play services. Spec §30 also requires
competition enable/disable, nickname, delete competition profile,
map-data management, local recording management, privacy information,
and terms / competition rules. Friends copy still describes “username
so friends can add you” (SPD-061).

§31 states are specified as UX, not as “log a warning”. Several were
residualled from Phase 5 (no-area) and Phase 6 (Avoid impossible) as
device observations; the **copy and actions** still need an audit
against the spec sentences.

---

## In-scope behavior

- Audit current settings IA against spec §30. Add missing Street
  Pixels rows; remove or hide public-V1 friend rows (H9 / SPD-061);
  confirm haptics toggle remains the single exploration-haptics
  control (SPD-054).
- Confirm the 25 m radius and HEALPix / GPS / decay / scoring
  internals are **not** ordinary settings.
- GPX rows only when capability+entitlement allow (SPD-070 / SPD-072
  / SP-084). Public builds add nothing.
- Audit each §31 state: location denied; background location denied
  (foreground recording remains); no downloaded map; poor GPS
  accuracy (no interpolation); interrupted recording; no selected
  exploration area; no local competitors (no empty leaderboard);
  no competition connectivity (queue); Avoid impossible (Prefer
  offer — SPD-042, may already be SP-089).
- Implement missing copy/actions. Reuse existing strings when they
  match the spec; do not paraphrase privacy guarantees.
- Spec §10 first-run: permission rationale is session-only and not
  bundled with competition; first recording; first pixels; no full
  tutorial required. Record gaps; fix copy/flow holes that are not
  owned by another Phase 10 item.
- English strings in this item; translations follow existing
  CoMaps process, not a launch blocker unless a user-visible English
  string is missing.

## Out-of-scope behavior

- Purchase / restore / pricing settings (SPD-010).
- Drawing the check glyph, share-card defects, weekly JNI (SP-089).
- Privacy *policy text* hosted on the web (SP-093); this item may
  add in-app rows that *open* those URLs once H4 exists.
- Device execution of the §31 matrix (SP-095 / SP-097).
- New achievement screens (spec §18.5).

## Relevant product requirements

- Spec §10, §30, §31, §34 Progress / Recording / Routing / Privacy.
- SPD-010, SPD-011, SPD-042, SPD-054, SPD-061, SPD-070, SPD-074.

## Relevant source files or symbols

- `prefs_main.xml`, `prefs_interface.xml`, `prefs_privacy.xml`,
  `DataManagementSettingsFragment`, `GpxSettingsFragment`,
  `MyAccountDialogFragment`, `ExploreConsentDialogFragment`
- Location permission rationale in `MwmActivity`
- Area empty / no-area overlay (SP-040)
- Recording interruption UI (SP-013)
- Avoid no-route UI (SP-058)

## Implementation notes / constraints

- Settings audit first (table in completion evidence): required row
  → present / missing / wrong. Then implement missing/wrong only.
- Do not add a dedicated “Street Pixels debug” screen in public
  builds.
- Background-location denied copy must not imply ABL is requested if
  H6 keeps ABL absent: explain FGS / screen-off limits honestly.

## Acceptance criteria

1. Evidence table maps every §30 bullet to a surface.
2. Evidence table maps every §31 state to copy + action, implemented
   or explicitly residualled with owner.
3. No radius or internal-parameter setting in public builds.
4. Friend settings hidden in public builds (H9).
5. Focused JVM / C++ tests for any new gating. Agent does not mark
   Accepted.

## Required automated tests

- Settings visibility: public (all Pro capabilities false) has no
  GPX rows and no friend rows.
- §31 Avoid / no-area behaviours that are already unit-tested remain
  green; add tests only for new branches.

## Required manual validation

- First-run click-through on D1 is SP-095. This item records the
  intended script.

## Failure and rollback considerations

- If a §31 state needs ABL (H6 still absent), write copy for FGS
  death rather than adding the permission here.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| §30 table | |
| §31 table | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
