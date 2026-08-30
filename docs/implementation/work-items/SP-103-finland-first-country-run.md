# SP-103 — Finland first-country pipeline run

**Phase:** 11 — Independent map build and serve
**Status:** In review
**Depends on:** SP-098 lock (**SPD-090**, **SPD-094**, **SPD-095**); SP-099–102
**Unblocks:** SP-104

---

## Objective

Execute the operator pipeline on a **Geofabrik Finland extract** (or pinned
equivalent), produce eight leaf `.mwm` + dense `.spa`, assemble, sign
(Channel A if P5 keys exist), publish to the origin, and record evidence that
**no Streifzug map host** was used.

This is the first production-shaped dataset, not a CI test.

---

## Motivation

Tools without a measured FI run leave SP-044’s eight-leaf residual open.
Phase 5/10 Helsinki walks need real `.spa` beside MWM.

---

## In-scope behavior

- Run SP-100 with **SPD-090** grain; `NODE_STORAGE: map`; **SPD-094** /
  **SPD-095** defaults (extras on; skip coasts if they fail).
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
3. Evidence states Streifzug map hosts were not used (build + publish).
4. Origin `meta/maps.json` `latest` equals published `"v"`.
5. Maintainer decides acceptance.

## Required automated tests

- None (offline batch). Do not weaken unit tests.

## Required manual validation

- The run itself. Device download may be SP-104 or residual if no handset.

## Failure and rollback considerations

- Do not fall back to Streifzug MWMs to “finish” the tree.
- Do not use highway-proxy U if derive fails — fail the WI.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-103-finland-first-run-b3d3` |
| Evidence SHA | `56980a8ce2a07899556080100c1a8eee8f91f463` (dry-run / tests / host facts). This `[docs]` commit records them |
| Plan / log | [SP-103-validation-plan.md](../validation/SP-103-validation-plan.md); [SP-103-evidence-log.md](../validation/SP-103-evidence-log.md) |
| Host | Cloud Agent VM: **15 GiB RAM**, 4 CPUs, **no swap** (`MemTotal` 16398384 kB). Disk overlay 252 G, 227 G avail. **SPD-088** ≥32 GiB **not** met |
| Binaries | `pix_derive_tool` present (`/workspace/omim-build-debug/pix_derive_tool`, sha256 `fd919692d7a4417ce4dc8be34037ee6483f54ad9ecf76fb7f664aa385d7c8dc2`). `generator_tool` **missing**. `spa_emit_tool` **missing** |
| Tests | `cd tools/python && PYTHONPATH=. python3 -m unittest street_pixels.tests.test_map_pipeline` — **40/40** OK. No code change |
| Dry-run | `map_pipeline --dry-run --pbf https://download.geofabrik.de/europe/finland-latest.osm.pbf --out /tmp/sp103 --countries 'World,Finland_*'` — exit 0; **no PBF download**; `/tmp/sp103` not created; eight `Finland_*` + `World`; `WorldCoasts` omitted; `NODE_STORAGE: map`; no Streifzug hosts in printed plan / rendered ini |
| PBF | URL recorded. Checksum sidecar fetched only: `ab51ec4bf46b4b3c87941e6bdce385ff  finland-latest.osm.pbf` (`https://download.geofabrik.de/europe/finland-latest.osm.pbf.md5`, `X-Derived-From: europe/finland-260828.osm.pbf.md5`, 2026-08-30T01:17:16Z). **PBF not downloaded.** Not committed |
| Leaf table | **Not produced.** Eight-leaf generate **did not run** (RAM below 32 GiB, **SPD-088**). Do not invent sizes |
| Channel A | No production keys in git. Example `COUNTRIES_TXT_SIGNATURE_HEX` is zeros. `private.h` and secret PEM absent on disk. Signing **not executed**. Channel B **not** used and **not** the public path |
| Publish / curl | **Not executed** (no VPS). Residual SP-102/103 maintainer |
| Generate | **Not executed.** Residual: maintainer ≥32 GiB MacBook. No Streifzug MWM fallback. No highway-proxy U |
| Implemented by | Cursor Agent (`cursoragent@cursor.com`) |
| Accepted by | — |

Phase 11 exit is **not** met.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Eight-leaf FI generate + dense `.spa` on ≥32 GiB builder (`generator_tool` + `spa_emit_tool` same revision as APK) | Residual — maintainer MacBook (**SPD-088**). This Cloud Agent host must not try mapgen |
| Channel A sign if P5 keys exist on the builder | Residual — no production keys in git |
| Publish + `curl` `meta/maps.json` and Helsinki objects | Residual SP-102/103 maintainer (no VPS here) |
| Helsinki `VerifyDenseAssignments` + leaf size table | After generate; do not invent |
| Exit checklist | SP-104 |
| Next ISO policy | After Phase 11 |
