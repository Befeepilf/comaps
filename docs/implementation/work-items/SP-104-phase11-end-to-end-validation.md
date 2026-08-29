# SP-104 — Phase 11 end-to-end validation

**Phase:** 11 — Independent map build and serve
**Status:** Planned
**Depends on:** SP-098 **Accepted** (**SPD-087–096**); SP-099–103 In review or Accepted
**Unblocks:** Phase 11 exit (maintainer)

---

## Objective

Verify Phase 11 exit criteria 1–8 with recorded evidence: locks, derive,
operator CLI, identity, origin, Finland artifacts, and no CoMaps map
origin on the stock path.

Does **not** declare Phase 11 complete. Does **not** execute Phase 10
device UX walks unless a handset is available; if not, record device
download as residual (SP-053/SP-095 still exist).

---

## Motivation

Same as other phase-exit WIs: a single evidence log so hosting is not
“probably wired”.

---

## In-scope behavior

- Validation plan + evidence log under `docs/implementation/validation/`.
- Checklist mapped to phase-11 exit 1–8.
- Confirm: layout **SPD-035**; spa advertised **SPD-028**; signature
  **SPD-036**; D12 still holds (no LAN default).
- Confirm Option A still absent (`generator/` still no emit call sites).
- Optional: APK with template `private.h` pointed at the VPS downloads
  Helsinki `.mwm` then `.spa`; `HasRemoteSpa`; SHA OK.
- Traffic: hosts-file/DNS block of CoMaps map peers **or** log-based
  proof the downloader used only our origin.

## Out-of-scope behavior

- Spec §34 device matrix (Phase 10).
- Marking Phase 11 Accepted.
- Editing product spec / audit.

## Acceptance criteria

1. Every Phase 11 exit line has pass / fail / residual with evidence.
2. Failures are not hidden by skipping tests.
3. Maintainer decides phase exit.

## Required automated tests

- Re-run SP-099/100/101/051 focused tests; record output (executed only).

## Required manual validation

- Evidence from SP-103; optional device download.

## Failure and rollback considerations

- Incomplete Finland run → Phase 11 exit not met.
- Any CoMaps map fetch on the recorded stock path → Fail P1.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | — |
| Evidence log | — |
| Implemented by | — |
| Accepted by | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Device UX on these maps | Phase 10 / SP-095 residual |
| Next countries | New WIs after exit |
