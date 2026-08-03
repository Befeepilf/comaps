# SP-025 — Versioned country-config schema

**Phase:** 4 — Administrative-area pipeline
**Status:** Planned
**Branch:** `street-pixels`
**Depends on:** SP-024 (config format / versioning decision)

---

## Objective

Define and land a reviewable, versioned country-configuration schema that
lists which `admin_level` values (and place-boundary fallbacks) are valid for
each country and in what priority order (SPD-006).

## Motivation

A single global `admin_level` rule is not semantically valid across countries.
Assignment determinism requires a policy version paired with map-data version
(Phase 3 stamp). Configuration must be data, not hard-coded C++.

## In-scope behavior

- Schema and at least one fixture country (the SP-023 measured country).
- Policy version field; documentation of review/update process (per SP-024).
- Loader API used by generator and/or client (per SP-024 locus).
- Tests: fixture selects expected levels; unknown country falls back safely
  (settlement / no-area per SPD-007 — do not invent grids).

## Out-of-scope behavior

- Complete worldwide coverage (incremental; settlement fallback + no-area cover
  the rest).
- Generator polygon emission (SP-026).
- UI for editing config.
- City allowlists (SPD-004).

## Relevant product requirements

- §8.3, §8.4, §34 country-configuration principle; SPD-006, SPD-007.

## Relevant source files or symbols

- To be chosen under SP-024 (data path + loader). Likely under `data/` and a
  small C++/Java loader used by assignment.

## Acceptance criteria

1. Schema lands as reviewable data under version control.
2. At least one country fixture with priority order.
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
| Decision ids (SP-024) | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
