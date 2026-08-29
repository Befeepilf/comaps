# SP-096 — Risk-register close-out and release pipeline

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H3/H8/H10. SP-092 disclosures. Phase 10
  implementation entry. Ops access for signing secrets as needed.
**Unblocks:** SP-097 exit #10 and #11

---

## Objective

Give every technical-audit §22 risk a final position (mitigated,
accepted, or realised), re-check audit §26 launch-blocking
conditions against current code, and prove the release pipeline
produces a signed, installable artefact for the H3 store gate.

---

## Motivation

Phase 10 exit #10 and #11 are the risk register and store signing.
The audit date is 2026-07-20; many “confirmed now” risks (ungated
collection, wipe-on-update, empty formulas, Sentry PII, friends)
have since been worked. The register is not self-updating. Signing
and Forgejo workflows are still CoMaps-shaped (H8).

Ops items from Phase 8 (Postgres, EU region string, production
Django settings) sit here as **Ops**, not as client features.

---

## In-scope behavior

- For each audit §22 row, write: current evidence (code pointer or
  measurement), position (`mitigated` / `accepted` / `realised` /
  `n/a Android V1`), owner. iOS Always permission → n/a (SPD-002).
- Re-evaluate §26 seven launch-blocking conditions. Expected
  (verify, do not assume): (1) session gate SP-007; (2) rematch
  SP-017; (3) admin pipeline Phase 4; (4) GPS filter SP-009; (5)
  competition backend SP-075; (6) iOS n/a; (7) Sentry SP-003.
  Any still-open condition blocks SP-097.
- Release pipeline: from `docs/CREDENTIALS.md` and
  `.forgejo/workflows/android-release.yaml`, produce or record a
  signed `google` (or H3) release/beta APK. Document applicationId,
  signing identity, and that it is not an unsigned debug build.
- Backend ops checklist: production settings module not SQLite
  (SP-075); EU region string (SPD-062); competition endpoints
  reachable from the signed app’s API base (SP-004 fail-closed).
- H10: record the local test commands used as the V1 gate; do not
  treat Forgejo C++ exclusions as a Phase 10 coding task.
- Client cheating risk stays **accepted** (audit; V1 clamps only).
- Sparse-area anonymity: confirm server-side N&lt;3 still holds
  against a direct API call (SP-076); if the explorer checkout is
  missing, residual Ops, do not fake.

## Out-of-scope behavior

- Marketing assets (phase-10 non-goal).
- Narrowing `CTEST_EXCLUDE_REGEX` unless the maintainer overrides
  H10.
- New anti-cheat.
- Publishing to the store (human). This item proves the artefact
  *can* be produced.

## Relevant product requirements

- Spec §34 Release governance / Quality.
- Audit §20, §22, §26.
- SPD-001–014, SPD-062, draft SPD-079 / SPD-084 / SPD-086.

## Relevant source files or symbols

- `docs/street-pixels-technical-audit.md` §22, §26
- `.forgejo/workflows/android-release.yaml`
- `docs/CREDENTIALS.md`
- `android/app/build.gradle` flavors / signing configs
- explorer `competition/` settings

## Implementation notes / constraints

- Audit document is not edited; the close-out table lives in this
  WI / a validation note.
- “Realised” means the risk happened (e.g. OEM kill on D2); then
  H6/SP-095/store caveats must reflect it.
- Do not put secrets in git.

## Acceptance criteria

1. §22 table complete with positions.
2. §26 conditions mapped to evidence or open blockers.
3. Signed installable artefact recorded (hash, flavor, build type)
   or a blocked ops reason.
4. Agent does not mark Accepted.

## Required automated tests

- Smoke + `street_pixels_tests` recorded if this environment can
  build; otherwise the SP-097 run is the record.
- Android lint triaged (clean or warning list).

## Required manual validation

- Install the signed APK on D1 at least once (may share with
  SP-095).

## Failure and rollback considerations

- Signing failure is a launch blocker; do not substitute an
  unsigned APK as exit #11.
- If API base in the signed APK is a developer host, that is an
  SP-004 regression — stop.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Risk table | |
| §26 table | |
| Artefact | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
