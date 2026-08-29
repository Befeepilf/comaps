# SP-092 — Permissions, manifest, and store disclosures

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H3, H6, H8, H9 Accepted. Phase 10 implementation
  entry.
**Unblocks:** SP-096 signing/pipeline; SP-097 exit #5 / #11

---

## Objective

Make the Android manifest, permission rationales, Play (and any locked
flavor) listing, and data-safety answers match what the software
actually does: session-only location, no purchase action, no friends
surface in public V1, GPX not advertised as free if gated, background
location declared only if ABL is present.

---

## Motivation

The google Play `en-US` full description is still generic CoMaps and
lists GPX import/export. Help and listing still imply a tracker-free
OSM app. `add-friend` intent-filters are registered. Data-safety
questionnaires are not in this repository as Street Pixels answers.

Spec §34 Release governance requires accurate Android store
permissions and background-location disclosures. A mismatch is a
launch blocker even if the binary is correct.

---

## In-scope behavior

- Re-inventory permissions vs behaviour (location, FGS types,
  notifications, vibrate, internet). Document why each is required.
- Apply H6: keep ABL absent **or** add it with Play Console
  background-location declaration notes and session-only justification
  text (video itself is ops/console, not a git binary).
- Apply H9: public flavor must not register add-friend deep links
  and must not present friend onboarding.
- Apply H3/H8: Street Pixels store listing copy for the gated
  flavor(s). Remove or qualify GPX claims (Pro / not in public V1).
  Do not claim “does not track” if session recording exists — say
  session-only, local, not sold.
- Data-safety answers as a checked-in document under
  `docs/implementation/` (or Play metadata path if one exists) that
  a human can paste into Play Console: location (not shared except
  competition aggregates when opted in), no ads, no sale of data.
- Permission rationale strings: session recording; not bundled with
  competition (already `track_recording_location_rationale`); drop
  leftover “CoMaps” product name if H8 says the listing is Street
  Pixels — **only** if product lock says the public name changes;
  otherwise record the brand divergence.
- Confirm public builds present no purchase/restore (SPD-010).
- Confirm `FOREGROUND_SERVICE_LOCATION` matches `TrackRecordingService`
  / `NavigationService` types.

## Out-of-scope behavior

- Writing the full privacy policy (SP-093).
- Actually clicking Publish in Play Console (ops; SP-096 records
  that the pipeline produced an artefact).
- Huawei/F-Droid listings if H3 excludes them from the V1 gate.
- Adding instrumented tests.

## Relevant product requirements

- Spec §10 steps 3–4, §25.1, §29.3, §34 Explorer Pro / Release
  governance.
- SPD-010, SPD-011, SPD-061, draft SPD-079 / SPD-082 / SPD-084 /
  SPD-085.

## Relevant source files or symbols

- `android/app/src/main/AndroidManifest.xml`
- `android/app/src/google/play/listings/**`
- `android/app/src/fdroid/play/**` (if H3 includes F-Droid)
- `docs/CREDENTIALS.md`
- Rationale strings in `values/strings.xml`

## Implementation notes / constraints

- Listing copy is product-owned; this item drafts from spec §36 and
  does not invent marketing slogans.
- Do not add ABL “just in case”.
- Friends code may remain compiled; public *surface* must not.

## Acceptance criteria

1. Permission inventory table: permission → code path → store
   disclosure line.
2. Public manifest matches H6 and H9.
3. Gated flavor listing does not advertise ungated GPX or friends.
4. Data-safety document exists and matches behaviour.
5. Agent does not mark Accepted.

## Required automated tests

- JVM or lint-level assertion that public BuildConfig has Pro
  capabilities false (already SP-005/083; re-run).
- If add-friend filters are flavor-gated, a unit/manifest test or
  documented aapt dump of the public APK (device dump may wait for
  SP-095).

## Required manual validation

- Install public-configured APK: no friend deep link handling, no
  purchase, listing text reviewed by maintainer.

## Failure and rollback considerations

- Play review of ABL is outside the team (phase-10 known
  uncertainty). If H6 adds ABL and review rejects, revert permission
  and disclosure together.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Permission inventory | |
| Listing paths edited | |
| Data-safety doc | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
