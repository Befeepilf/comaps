# SP-099 — Offline leaf MWM → `.pix` derive

**Phase:** 11 — Independent map build and serve
**Status:** Planned
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

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | — |
| Implemented by | — |
| Accepted by | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Wire into operator CLI | SP-100 |
