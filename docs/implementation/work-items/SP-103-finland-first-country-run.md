# SP-103 — Finland first-country pipeline run

**Phase:** 11 — Independent map build and serve
**Status:** Planned
**Depends on:** SP-098 lock (P4, P8); SP-099–102
**Unblocks:** SP-104

---

## Objective

Execute the operator pipeline on a **Geofabrik Finland extract** (or pinned
equivalent), produce eight leaf `.mwm` + dense `.spa`, assemble, sign
(Channel A if P5 keys exist), publish to the origin, and record evidence that
**no CoMaps map host** was used.

This is the first production-shaped dataset, not a CI test.

---

## Motivation

Tools without a measured FI run leave SP-044’s eight-leaf residual open.
Phase 5/10 Helsinki walks need real `.spa` beside MWM.

---

## In-scope behavior

- Run SP-100 with P4 grain; `NODE_STORAGE: map`; P8/P9 defaults.
- Record: git SHA, generator binary identity, PBF source URL + checksum,
  wall time, peak RAM if reasonably available, per-leaf MWM bytes, spa
  `area_count` / `assign_count` / file bytes, Helsinki known-id spot-check.
- `VerifyDenseAssignments` on Helsinki (full or documented sample with
  fail-closed).
- Publish to the SP-102 origin; `curl` inventory.
- Evidence log under `docs/implementation/validation/` (new
  `SP-103-evidence-log.md` + short plan). Do not commit binaries.
- If Channel A keys are missing, record Channel B as **not** the public path
  and stop short of stock-APK advertisement (SPD-037).

## Out-of-scope behavior

- Option A.
- Countries beyond FI.
- Phase 10 Helsinki UX walks (may *use* these files later).
- Planet World/coasts (P8).

## Acceptance criteria

1. Eight FI `{leaf}.spa` with `format_version` 2, `assign_count == |U|`,
   `assign_count > 0`.
2. Matching eight `.mwm` in the publish version dir; hashes match countries.
3. Evidence states CoMaps map hosts were not used (build + publish).
4. Origin `meta/maps.json` `latest` equals published `"v"`.
5. Maintainer decides acceptance.

## Required automated tests

- None (offline batch). Do not weaken unit tests.

## Required manual validation

- The run itself. Device download may be SP-104 or residual if no handset.

## Failure and rollback considerations

- Do not fall back to CoMaps MWMs to “finish” the tree.
- Do not use highway-proxy U if derive fails — fail the WI.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | — |
| PBF | — |
| Leaf table | — |
| Implemented by | — |
| Accepted by | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Exit checklist | SP-104 |
| Next ISO policy | After Phase 11 |
