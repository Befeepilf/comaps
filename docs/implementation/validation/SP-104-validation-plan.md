# SP-104 — Validation plan (Phase 11 exit evidence)

**Work item:** [SP-104](../work-items/SP-104-phase11-end-to-end-validation.md)
**Plan authored by:** Cursor Agent (`cursoragent@cursor.com`)
**Plan date:** 2026-08-30
**Branch:** `cursor/sp-104-phase11-exit-evidence-b3d3`
**Evidence:** [SP-104-evidence-log.md](SP-104-evidence-log.md)

This is Phase 11 **exit evidence**, not a feature. Agent does **not** mark
SP-104 or Phase 11 Accepted. Phase 11 exit is **not** met.

---

## Honesty lock (this environment)

SP-103 already recorded: eight-leaf Finland generate **did not run**.
This Cloud Agent VM is **~15 GiB RAM, 4 CPUs, no swap**. **SPD-088**
requires a **≥32 GiB** builder. Incomplete Finland run → Phase 11 exit
**not met**. This plan does not hide that, start mapgen, or invent leaf
tables / VPS curls / APK downloads.

There is **no VPS** and **no handset** here. Live origin fetch and
device download are residual.

---

## What this Cloud Agent run executes

| ID | Action | Pass condition |
| --- | --- | --- |
| E1 | Host / SHA facts | Record branch, `HEAD`, RAM, binaries |
| E2 | Exit 1 | P1–P10 present as **SPD-087–096** in `DECISIONS.md`; SP-098 **Accepted** |
| E3 | Exit 2 | `pix_derive_tool` exists; shared `DeriveStreetPixelsUniverse`; no highway-proxy U in production path. On-device U compare is residual, not invented |
| E4 | Layout / ads / sig / D12 | SPD-035 tree; SPD-028 ads not merged into git `data/countries.txt`; SPD-036 verify not skipped; D12 template `maps.example.invalid` (no LAN Custom Maps default) |
| E5 | Exit 8 / Option A | `generator/` links `street_pixels_areas` but has **no emit call sites** (grep). Residual **SPD-089** |
| E6 | Focused Python tests | Re-run SP-099/100/101/051 suites listed below. Executed output only. Do not weaken |
| E7 | Optional C++ filters | If `omim-build-debug/street_pixels_tests` exists: `PixDerive` / `Eligibility`. Do **not** start a full desktop rebuild |
| E8 | Stock-path P1 | No Streifzug map fetch on the recorded stock path. Any such fetch → **Fail** exit 1 / P1 |

## What this Cloud Agent run does not execute

- Eight-leaf Finland `maps_generator` / `map_pipeline` without `--dry-run`
- PBF download; `.mwm` / `.spa` / `.pix` production
- Channel A signing (no production keys)
- Channel B inject (not the public path — **SPD-037**)
- VPS rsync / live `curl` of `meta/maps.json` or Helsinki objects
- APK download / hosts-file traffic capture / spec §34 device matrix
- Fallback to Streifzug MWMs; highway-proxy U

SP-103 dry-run evidence is reused, not re-executed as a fake generate.

---

## Phase 11 exit criteria → expected scoring (verify against tree)

Suggested at plan time; evidence log records **Pass / Fail / Residual**
after tree checks and executed tests.

| # | Criterion | Expected |
| --- | --- | --- |
| 1 | P1–P10 as SPD-087–096 | **Pass** if DECISIONS + SP-098 Accepted hold |
| 2 | Offline MWM→`.pix` derive; no highway proxy | **Pass** for tool existence / shared derive; on-device U residual |
| 3 | Operator command produces FI tree | **Residual** (SP-103 dry-run only) |
| 4 | VPS serves tree + Range | **Residual** (snippets only; no live origin) |
| 5 | Signed countries with spa on stock path | **Residual** (no Channel A keys, no generate) |
| 6 | `configure.sh` without Streifzug | **Pass** for documented fail-closed path; live origin fetch residual SP-102 |
| 7 | Evidence log of real Finland artifacts | **Residual** (SP-103) |
| 8 | Option A out; FI-only policies OK | **Pass** for Option A out |

Incomplete Finland generate is enough, by itself, for Phase 11 exit
**not met**.

---

## Required automated tests

```text
cd /workspace/tools/python
PYTHONPATH=. python3 -m unittest \
  street_pixels.tests.test_map_pipeline \
  street_pixels.tests.test_map_identity \
  street_pixels.tests.test_origin_configs \
  street_pixels.tests.test_serve_spa_publish_tree \
  street_pixels.tests.test_prepare_spa_debug_root
```

Optional, existing binary only:

```text
/workspace/omim-build-debug/street_pixels_tests \
  --data_path=/workspace/data --user_resource_path=/workspace/data \
  --filter='PixDerive'
/workspace/omim-build-debug/street_pixels_tests \
  --data_path=/workspace/data --user_resource_path=/workspace/data \
  --filter='Eligibility'
```

---

## Acceptance

Maintainer decides Phase 11 exit. Agent leaves **Accepted by** empty.
SP-104 status after evidence: **In review**. Phase 11 stays **In
progress**; exit **not met**.
