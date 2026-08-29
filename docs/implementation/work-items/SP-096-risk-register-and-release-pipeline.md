# SP-096 — Risk-register close-out and release pipeline

**Phase:** 10 — Android release hardening
**Status:** Close-out recorded 2026-08-29. Signed APK / ops residual.
  Brand listing residual (**SPD-084**). Competition backend unverified
  in the explorer checkout present here. Not Accepted.
**Depends on:** SP-088 H3/H8/H10 Accepted (**SPD-079**, **SPD-084**,
  **SPD-086**). SP-092 disclosures. Phase 10 implementation entry. Ops
  access for signing secrets as needed.
**Unblocks:** SP-097 exit #10 and #11
**Notes:** Risk-register table in docs. Signed APK / ops may residual.
  Brand listing residual (**SPD-084**). Do not rewrite application
  name or Play listing brand copy.

---

## Objective

Give every technical-audit §22 risk a final position (mitigated,
accepted, or realised), re-check audit §26 launch-blocking
conditions against current code, and document the release-pipeline
posture for the H3 store gate (**SPD-079**). Signed APK production
may residual if ops secrets are unavailable. Brand listing / application
name remain residual (**SPD-084**).

---

## Motivation

Phase 10 exit #10 and #11 are the risk register and store signing.
The audit date is 2026-07-20; many “confirmed now” risks (ungated
collection, wipe-on-update, empty formulas, Sentry PII, friends)
have since been worked. The register is not self-updating. Signing
and Forgejo workflows are still CoMaps-shaped (H8 / **SPD-084**:
reuse machinery; brand listing residual).

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
  signed `google` (or H3) release/beta APK **if ops access exists**.
  If secrets or console access are unavailable, record signed APK /
  ops as residual. Document applicationId and signing identity as
  currently configured. Do **not** rewrite application name or
  Play listing brand copy (**SPD-084** residual).
- Backend ops checklist: production settings module not SQLite
  (SP-075); EU region string (SPD-062); competition endpoints
  reachable from the signed app’s API base (SP-004 fail-closed).
- H10 / **SPD-086**: record the local test commands used as the V1
  gate; do not treat Forgejo C++ exclusions as a Phase 10 coding
  task.
- Client cheating risk stays **accepted** (audit; V1 clamps only).
- Sparse-area anonymity: confirm server-side N&lt;3 still holds
  against a direct API call (SP-076); if the explorer checkout is
  missing, residual Ops, do not fake.

## Out-of-scope behavior

- Marketing assets (phase-10 non-goal).
- Application name and Play/F-Droid listing brand copy
  (**SPD-084** residual).
- Narrowing `CTEST_EXCLUDE_REGEX` unless the maintainer overrides
  H10 (**SPD-086**).
- New anti-cheat.
- Publishing to the store (human). This item proves the artefact
  *can* be produced.

## Relevant product requirements

- Spec §34 Release governance / Quality.
- Audit §20, §22, §26.
- SPD-001–014, SPD-062, **SPD-079**, **SPD-084**, **SPD-086**.

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
  H6/SP-095/store caveats must reflect it. D2 execution is residual
  (**SPD-077** / **SPD-083**); do not invent a realised OEM result.
- Do not put secrets in git.

## Acceptance criteria

1. §22 table complete with positions.
2. §26 conditions mapped to evidence or open blockers.
3. Signed installable artefact recorded (hash, flavor, build type)
   **or** a residual/blocked ops reason. Brand listing is residual.
4. Agent does not mark Accepted.

## Required automated tests

- Smoke + `street_pixels_tests` recorded if this environment can
  build; otherwise the SP-097 run is the record.
- Android lint triaged (clean or warning list).

## Required manual validation

- Signed-APK production is in-scope if ops access exists. **Device
  install on D1 is residual** (may share with SP-095 when that item
  executes). Do not execute a hardware walk in this item; do not
  fabricate a handset.

## Failure and rollback considerations

- Signing failure is a launch blocker; do not substitute an
  unsigned APK as exit #11. Unavailable ops may residual the
  signed-APK proof rather than fake it.
- If API base in the signed APK is a developer host, that is an
  SP-004 regression — stop.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-096-risk-register-6383` (parent `street-pixels` `30eb240904d3a61aa5b36f4ddfa62e459d92352d`) |
| Risk table | [`notes/SP-096-risk-register-close-out.md`](../notes/SP-096-risk-register-close-out.md) §22 |
| §26 table | same note, §26 |
| Artefact | **Residual Ops.** `android/app/release.keystore`, `secure.properties` / `secure.properties.release`, and `google-play.json` absent. Lint configure printed `secure.properties.release doesn't exist` / `secure.properties.test doesn't exist`. That is **not** the release-task `NO RELEASE signing keys found` line. No signed `google` APK produced. No hash. Unsigned/debug-signed APK is not exit #11. |
| Test output | Original SP-096 full run aborted (`classificator.txt` absent at 14:18 UTC): **393** Running / **392** OK / **1** FAILED (`Eligibility_IncludesCommonHighways`). Independent review: `--filter=SampleAcceptance` is regex overlap (20 = 5 manager + 15 live). Unique focused **93/93**. Full `street_pixels_tests` after generated classificator files **499/499** (`All tests passed.`). Smoke **not run**. `:sdk:lintDebug` **FAILED** — **5 errors, 24 warnings** (4× `MissingPermission` VIBRATE in `Utils.java`; 1× `WrongConstant` in `RecordingSessionDebug.java`; corroborated from `lint-results-debug.txt`). |
| Independent review | 2026-08-29 — §22 19/19 vs tree; SampleAcceptance overlap; sparse-area chrome wording; lint/keystore messages; full suite 499/499. Not Accepted. |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Signed `google` release/beta APK not produced (secrets absent) | Residual Ops. Restore Forgejo `RELEASE_KEYSTORE` / `SECURE_PROPERTIES` / `GOOGLE_PLAY_JSON` and materialise the file Gradle actually reads (`secure.properties.release`). Do not substitute an unsigned APK as Phase 10 exit #11. |
| Forgejo writes `android/app/secure.properties`; Gradle applies `secure.properties.release` | Residual Ops / workflow. Align restore path with Gradle or document the ops copy step. Not a listing-brand rewrite. |
| Application name / Play/F-Droid listing still CoMaps (GPX advertised) | Residual **SPD-084**. Do not rewrite in SP-097 coding. |
| Explorer `main` (`e13a124`) has no `competition/` app; SQLite default; no `prod.py` | Residual Ops. Friends API is not a substitute. Blocks §26 condition 5 / SP-097 until a production competition backend is inspectable. |
| Sparse-area N&lt;3 not verified against a direct API call | Residual Ops (same missing backend). Boss-line chrome hide is not protection; ranking rows are not nickname-stripped. |
| Exact EU region string | Residual Ops (**SPD-062**). |
| H10 smoke / lint clean / generated classificator on a clean tree | `data/classificator.txt` is gitignored generated. Original abort was real. Review re-run **499/499** after local generate. Smoke still **not run**. `:sdk:lintDebug` 5 errors / 24 warnings (pre-existing, not fixed here). Leave smoke + lint clean to SP-097. Do not weaken tests. |
| Forgejo restore writes `android/app/release.keystore`; `android/app/.gitignore` lists `comaps-release.keystore` only | Residual Ops / gitignore. Do not commit the keystore. |
| Renderer / battery / OEM | Residual SP-094 / SP-095. Not realised. Do not invent D2 kills. |
