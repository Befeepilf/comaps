# SP-099 — Offline leaf MWM → `.pix` derive

**Phase:** 11 — Independent map build and serve
**Status:** In review
**Depends on:** SP-098 lock (**SPD-089**; **SPD-095** extras on does not affect this)
**Unblocks:** SP-100, SP-103

---

## Objective

Ship a maintainer CLI that, given `{mwmLeafId}.mwm` from **this** generator
revision, writes `{mwmLeafId}.pix` whose ascending NEST universe **U** matches
what the client would derive on first open (`DeriveStreetPixelsFromFeatures`,
15 m, `IsExplorable`). Explored / ever-live bits are **empty**. That `.pix` is
the only production U source for `spa_emit_tool`.

---

## Motivation

`spa_emit_tool --mode=production` requires `--pix_dir`. SP-044 recorded that
full FI dense emit is blocked without it. Highway-proxy U must not ship.
Linking the full `map` library into emit was avoided; a **small derive tool**
is the remaining packaging residual.

---

## In-scope behavior

- New tool under `tools/` (name e.g. `pix_derive_tool`) that:
  - Opens one or many leaf `.mwm` (directory or explicit list).
  - Derives U with the **same** eligibility and 15 m sampling as
    `StreetPixelsManager::DeriveStreetPixelsFromFeatures` (**SPD-019**).
  - Writes headered `.pix` via the existing `street_pixels_file` writer
    (`SaveRematchedUniverse` or equivalent) with empty explored/ever-live.
  - Stamps `map_data_version` from the MWM / `--map_data_version`.
- Fail closed on missing MWM, corrupt MWM, or empty U (production emit
  already refuses empty U).
- Sequential leaves; do not hold all universes in RAM.
- Automated tests: tiny fixture MWM or existing test MWM; U non-empty;
  `ScanUniverseAscending` order; explored count 0; second run overwrite
  is deterministic.
- Docs: operator flags; pointer from SP-044 residual and phase-11.

Prefer extracting a shared derive helper over copy-paste if that is smaller
than linking `StreetPixelsManager`. If linking `map` is required, say so in
the PR and keep the CLI tiny. **Never** invent a second eligibility table.

## Out-of-scope behavior

- Writing `.spa` (still `spa_emit_tool`).
- Option A / `StageMwm`.
- Explored-bit migration or rematch (Phase 3).
- Highway proxy.
- `private.h` / CDN URLs.

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `libs/map/street_pixels_manager.cpp` `DeriveStreetPixelsFromFeatures` | Client algorithm to match |
| `libs/map/street_pixels_file.*` | `.pix` serdes |
| `tools/spa_emit_tool/` | Consumer of `--pix_dir` |
| `libs/street_pixels_areas/sample_centres.hpp` | Already scans `.pix` without linking `map` |

## Acceptance criteria

1. CLI builds with the desktop generator/test toolchain.
2. For a fixture (or documented small MWM), output `.pix` has `assign`-ready
   U: `ScanUniverseAscending` length matches derive; explored empty.
3. Tests fail if sampling is not 15 m or eligibility diverges (reuse or wrap
   existing eligibility tests — do not weaken them).
4. Production emit can run on derived `.pix` in a smoke (tiny U or one leaf).
5. Maintainer decides acceptance.

## Required automated tests

- Derive round-trip on a fixture; empty explored; version header.
- Empty/corrupt MWM → non-zero exit.
- Existing `street_pixels_tests` / `street_pixels_areas_tests` still pass.

## Required manual validation

- Optional: one real Helsinki (or smaller) MWM on the build host; record `|U|`
  vs a device-open of the same MWM if a device is available. If no device,
  record fixture-only and residual the on-device U compare to SP-103/104.

## Failure and rollback considerations

- Do not ship proxy U.
- Do not write ever-live / explored from packaging.
- Do not change on-device derive to “match a simpler tool”.

## Operator flags (`pix_derive_tool`)

Links the `map` library. Derive is `DeriveStreetPixelsUniverse` (shared with
the client). The manager wrapper still returns empty on empty `countryId`; the
helper does not.

```text
pix_derive_tool --mwm_dir /path/to/mwms --out_dir /path/to/pix
pix_derive_tool --mwm /path/to/{leaf}.mwm --out_dir /path/to/pix
pix_derive_tool --mwm_dir /path/to/mwms --mwm extra.mwm leftover.mwm --out_dir /path/to/pix
```

| Flag | Meaning |
| --- | --- |
| `--mwm_dir` | Directory of leaf `.mwm` (skips `World` / `WorldCoasts` in the directory listing) |
| `--mwm` | Single `.mwm` path (can combine with `--mwm_dir` and leftover argv files) |
| leftover argv | Extra `.mwm` paths |
| `--out_dir` | Output directory for `{leaf}.pix` (required; empty → exit **5**) |
| `--map_data_version` | YYMMDD stamp; **0** (default) reads from the MWM header |

Fail-closed exit codes: missing MWM **1**, unreadable/corrupt **2**, empty U **3**,
write failure **4**, bad `--out_dir` **5**, `World` / `WorldCoasts` (filename or
non-Country `DataHeader`) **NotALeaf** exit **1**. No silent empty `.pix`.
`--mwm_dir` skips World files; an explicit `--mwm World.mwm` fails closed and
does not write. Sequential leaves; one universe in RAM at a time. A failed
leaf does not stop later leaves; the process exit code is the **first** error.
Explored / ever-live bits are empty (`SaveUnexploredIds`). Production emit:
`spa_emit_tool --mode=production --pix_dir` on this output. Full FI rings +
Helsinki MWM emit is **SP-103**.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-099-offline-pix-derive-b3d3` |
| Commits | `c3dc852e7` `[map] Extract shared street-pixel universe derive helper`; `e0f5ca0c4` `[tools] Add pix_derive_tool for offline MWM to .pix`; `a1be562dd` `[map] Fail closed on corrupt MWM without debug abort`; `210b6907e` `[tools] Report pix_derive failures on stderr`; `4e595186e` `[docs] Record SP-099 pix_derive_tool evidence`; `61378b7e5` `[map] Refuse World MWM and open each leaf once`; `db9f1d21b` `[map] Tighten pix-derive tests for World, empty out_dir, and 15 m`; `221693411` `[tools] Exit 5 on empty pix_derive --out_dir`; `4949e0b1d` `[map] Probe MWM TOC offset before opening FilesContainerR`; this `[docs]` commit |
| Tool | `tools/pix_derive_tool` — links `map` + `gflags`; shared `DeriveStreetPixelsUniverse` / `IsExplorableFeature` / `kPathSamplingStepMeters` (15) |
| Fixture | `data/minsk-pass.mwm` — \|U\| **24069**, 3718 streets, `map_data_version=210811` |
| Test output | `/workspace/omim-build-debug/street_pixels_tests --data_path=/workspace/data --user_resource_path=/workspace/data` — **505/505** OK (includes `PixDerive_*` 6/6 and existing `Eligibility_*` 9/9) |
| CLI smoke | Empty `--out_dir` exit **5**. Missing MWM exit **1**. Corrupt MWM exit **2**, no `.pix`. Explicit `World.mwm` exit **1** `NotALeaf`, no `.pix`. Fixture `--mwm data/minsk-pass.mwm --out_dir /tmp/sp099_review_cli` exit **0**, `leaf=minsk-pass \|U\|=24069 map_data_version=210811` |
| Build | `SKIP_MAP_DOWNLOAD=1 ./tools/unix/build_omim.sh -d -n 3 -p /workspace pix_derive_tool street_pixels_tests` (default `../omim-build-debug` was not writable here) |
| Implemented by | Cloud agent (`befeepilf@protonmail.com`) |
| Accepted by | — |

On-device U compare vs this fixture \|U\| is residual to SP-103/104 (no device
in this environment). Phase 11 exit is **not** met.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Wire into operator CLI | SP-100 |
| Full `spa_emit_tool --mode=production` with FI rings + Helsinki MWM | SP-103 (no Helsinki MWM / rings in this env; do not invent proxy U) |
| On-device first-open U vs `pix_derive_tool` on the same leaf MWM | SP-103/104 |
| Debug `CHECK` in `DatSectionHeader::Read` can still abort if TOC tags exist but the DAT version byte is garbage | Residual; do not change indexer in this WI. TOC offset probe covers the garbage-file case. |
