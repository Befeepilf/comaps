# SP-025 — Versioned country-config schema

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 Accepted (SPD-023 locks location/versioning/keying/Finland seed)

---

## Objective

Define and land a reviewable, versioned country-configuration schema that
lists which `admin_level` values (and place-boundary fallbacks) are valid for
each country and in what priority order (SPD-006, SPD-023).

## Motivation

A single global `admin_level` rule is not semantically valid across countries.
Assignment determinism requires a policy version paired with map-data version
(Phase 3 stamp). Configuration must be data, not hard-coded C++.

## In-scope behavior

- Schema and Finland fixture under **`data/street_pixels/`** (SPD-023); exact
  filenames chosen here.
- Monotonic **`policy_version`**; ISO 3166-1 alpha-2 country keying; document
  PR-review / update process (SPD-023).
- Finland seed priority (SPD-023 / SP-023): subdivision **admin_10**, then
  **admin_9**, then **admin_11**; settlement **admin_8**; closed polygonal
  **place=*** sparse supplement only.
- Loader API used by generator (precompute, SPD-021) and client rematch.
- **No invented numeric suitability floors** (SPD-024) — suitability is
  closed/named rings at configured levels.
- Tests: fixture selects expected levels; unknown country falls back safely
  (settlement / no-area per SPD-007 — do not invent grids).

## Out-of-scope behavior

- Complete worldwide coverage (incremental; settlement fallback + no-area cover
  the rest).
- Generator polygon emission (SP-026).
- UI for editing config.
- City allowlists (SPD-004).
- Client pixel-count / area-size gates (SPD-024).

## Relevant product requirements

- §8.3, §8.4, §34 country-configuration principle; SPD-006, SPD-007, SPD-023,
  SPD-024.

## Relevant source files or symbols

- `data/street_pixels/` (SPD-023); small C++/Java loader used by generator and
  assignment rematch.

## Acceptance criteria

1. Schema lands as reviewable data under version control at `data/street_pixels/`.
2. At least one country fixture (Finland) with priority order per SPD-023.
3. Loader + unit tests green.
4. Policy version is readable for assignment keying with map-data version.

## Required automated tests

- Fixture parse / priority order.
- Unknown-country fallback behaviour (settlement / no-area — no grids).

## Failure and rollback considerations

- Schema changes after first ship must bump policy version; rematch path is
  SP-030.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Schema path | |
| Fixture country | |
| Decision ids (SP-024) | SPD-023 (primary); SPD-024 (no numeric floors) |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
