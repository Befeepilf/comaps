# SP-104 — Phase 11 end-to-end validation

**Phase:** 11 — Independent map build and serve
**Status:** In review
**Depends on:** SP-098 **Accepted** (**SPD-087–096**); SP-099–103 In review (not Accepted)
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
| Branch | `cursor/sp-104-phase11-exit-evidence-b3d3` |
| Evidence SHA | `48e02e7bf69d25f8ed03030a3f250c178da02137` (tree the suites ran against). This `[docs]` commit records the plan/log |
| Plan / log | [SP-104-validation-plan.md](../validation/SP-104-validation-plan.md); [SP-104-evidence-log.md](../validation/SP-104-evidence-log.md) |
| Exit 1–8 | 1 **Pass**; 2 **Pass** (on-device U residual); 3 **Residual**; 4 **Residual**; 5 **Residual**; 6 **Pass** (live origin fetch residual); 7 **Residual**; 8 **Pass**. No Fail. Incomplete Finland generate → Phase 11 exit **not met** |
| Tests | `cd tools/python && PYTHONPATH=. python3 -m unittest street_pixels.tests.test_map_pipeline street_pixels.tests.test_map_identity street_pixels.tests.test_origin_configs street_pixels.tests.test_serve_spa_publish_tree street_pixels.tests.test_prepare_spa_debug_root` — **92/92** OK (40+25+3+18+6). Optional: `street_pixels_tests --filter='PixDerive'` **6/6**; `--filter='Eligibility'` **10/10** (9 `Eligibility_*` + 1 substring). No code change. No desktop rebuild |
| Finland | **Not generated.** SP-103 dry-run only. Host **15 GiB**, **SPD-088** ≥32 GiB not met. `generator_tool` / `spa_emit_tool` missing. No leaf table |
| Option A | `generator/` links `street_pixels_areas`; no emit call sites (grep). Residual **SPD-089** |
| P1 stock path | No CoMaps map fetch recorded on template / `configure.sh` / `map_pipeline`. Debug `prepare_spa_debug_root` is not the stock path |
| Implemented by | Cursor Agent (`cursoragent@cursor.com`) |
| Accepted by | — |

Phase 11 exit is **not** met.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Device UX on these maps | Phase 10 / SP-095 residual |
| Eight-leaf FI generate + live origin curl | SP-103 / SP-102 maintainer (≥32 GiB; no VPS here) |
| On-device U vs `pix_derive_tool` | Residual (no handset / no FI MWM) |
| Next countries | New WIs after exit |
