# SP-042 — Sidecar shipping decisions (Phase 4 residual)

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** Accepted
**Accepted by:** Maintainer
**Accepted date:** 2026-08-08
**Branch:** `cursor/sp-042-sidecar-shipping-fe62`
**Depends on:** SP-032 Accepted; Phase 4 exit criteria met (narrowed R1);
  product-owner locks D1–D7 (2026-08-07)
**Unblocks:** SP-043–048 (sidecar shipping implementation track)

---

## Objective

Record accepted shipping decisions for production `.spa` download, advertisement
in `countries.txt`, update/delete lifecycle, failure behaviour, blob-contract
freeze gate, and track placement — so follow-on items SP-043–048 do not encode
guesses. Docs and `DECISIONS.md` only.

## Motivation

Phase 4 exit used an offline emit harness (SP-032). Production mapgen emit and
CDN leaf download of `.spa` remain pre-production (narrowed R1). Product owner
locked D1/D2/D7 and accepted recommended D3–D6 on 2026-08-07. Those locks must
land as SPD-027–033 before coding download, meta, or CDN packaging.

## In-scope behavior

- Append **SPD-027–033** to `docs/implementation/DECISIONS.md` (Accepted).
- Create this work-item file; index SP-042 and stub one-liners for SP-043–048
  in README / phase-04 / phase-10 (no full stub work-item files yet).
- Map product locks → decisions:

  | Lock | Choice | SPD |
  | --- | --- | --- |
  | D1 | A — couple leaf download to advertised `.spa`; MWM usable if missing | SPD-027 |
  | D2 | A — optional `spa` / `spa_sha1_base64` in `countries.txt` | SPD-028 |
  | D3 | A — no spa-diffs in V1; full `.spa` on map / dataVersion update | SPD-029 |
  | D4 | A — delete `.spa` with the map; personal files unchanged | SPD-030 |
  | D5 | A — advertised download failure keeps MWM; areas fail-closed | SPD-031 |
  | D6 | A — freeze production blob contract before CDN publish | SPD-032 |
  | D7 | A — Phase 4 residual / pre-production packaging track | SPD-033 |

- Clarify: this track is **not** a Phase 5 exit gate and **not** a Phase 10
  device-walk residual.

## Out-of-scope behavior

- Implementing download, parser, mapgen emit, or CDN packaging (SP-043–048).
- Creating full work-item files for SP-043–048 (README one-line index only).
- Editing product spec or technical audit.
- Marking this work item or Phase 4 R1 as Accepted/Complete unilaterally.
- Inventing grids, mandatory `.spa` for map install, or spa-diffs.

## Subsequent work items (stubs — index only)

| ID | Title |
| --- | --- |
| SP-043 | Freeze production `.spa` blob contract (`nside` / universe-order / `format_version`) |
| SP-044 | Production leaf `.spa` emit (Phase 4 R1; Option B offline batch — see work item) |
| SP-045 | Add optional `spa` / `spa_sha1_base64` leaf fields to `countries.txt` publish |
| SP-046 | Client leaf download fetches advertised `.spa` beside MWM |
| SP-047 | `.spa` full-refetch on map update and delete-with-map lifecycle |
| SP-048 | Sidecar shipping validation and incomplete / retry signaling |

## Relevant product requirements / decisions

- Product spec §3.5, §3.6, §8.6, §27, §31.
- SPD-007, SPD-016, SPD-017, SPD-020–022, SPD-024.
- Phase 4 R1 (narrowed); SP-026 / SP-028 / SP-030 / SP-032 notes.

## Acceptance criteria

1. SPD-027–033 present in `DECISIONS.md` with Status Accepted and product-owner
   lock context where relevant (2026-08-07).
2. This work-item file exists; Status **In review** after docs land (not
   Accepted by agent).
3. README indexes Phase 4 residual / pre-production packaging track (SP-042 +
   SP-043–048 one-liners); clarifies not Phase 5 / not Phase 10 device work.
4. phase-04 Residuals / R1 note points at SP-042 / SPD-027–033 and follow-ons
   SP-043–048; still pre-production packaging.
5. phase-10 Phase 4 R1 row cross-refs SP-042 / SPD-033; still not a Phase 10
   device item.
6. No production code changes; no product-spec / audit edits.
7. Maintainer decides acceptance; agent does not mark Accepted.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review of SPD-027–033 against locks D1–D7.

## Failure and rollback considerations

- Do not weaken SPD-016 permanence when recording SPD-030.
- Do not move sidecar shipping into Phase 5 exit or Phase 10 device residuals.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `904920824` |
| Decision ids | SPD-027, SPD-028, SPD-029, SPD-030, SPD-031, SPD-032, SPD-033 |
| Product locks | D1–D7 = A (2026-08-07); D3–D6 recommended locks accepted for implementation |
| Docs touched | `DECISIONS.md`; `README.md`; `phases/phase-04-…`; `phases/phase-10-…`; this file |
| Implemented by | Agent |
| Accepted by | Maintainer |
| Accepted date | 2026-08-08 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Field-level `.spa` header / `nside` / universe-order freeze | SP-043 |
| Production leaf `.spa` emit (Option B; Option A residual) | SP-044 (closes narrowed R1 emit for FI packaging) |
| `countries.txt` optional spa fields | SP-045 |
| Client advertised `.spa` download beside MWM | SP-046 |
| Full refetch on update + delete-with-map | SP-047 |
| Incomplete / retry signaling validation | SP-048 |
