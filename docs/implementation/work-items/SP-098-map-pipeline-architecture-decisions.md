# SP-098 — Independent map-pipeline architecture decisions

**Phase:** 11 — Independent map build and serve
**Status:** Accepted
**Accepted by:** Product owner
**Accepted date:** 2026-08-29
**Branch:** `cursor/phase-11-map-pipeline-b3d3`
**Depends on:** None for this docs item. SP-042–048 Accepted; SP-049–051
  tools in tree (**SPD-027–039**).
**Unblocks:** SP-099–104 (coding must not guess the locks listed here)
**Investigation note:**
  [`notes/SP-098-map-pipeline-architecture.md`](../notes/SP-098-map-pipeline-architecture.md)

---

## Objective

Record accepted decisions for Street Pixels **map origin**, hardware split,
Option B orchestration versus Option A collectors, first-country grain,
signing, `MAP_SERIES`, Phase 10/S4 relationship, coastlines, optional mapgen
feeds, and the operator entrypoint — so SP-099+ do not encode guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

The client already downloads `{leaf}.mwm` and advertised `{leaf}.spa` from
one Custom Maps / default-host URL (**SPD-035**). What is missing is an
origin we operate: Streifzug CDN must not carry Street Pixels map traffic, and
the 256 GiB planet builder is not the hardware we have.

Without locks, later items will either hit Streifzug “just for World.mwm”,
run `maps_generator` on the 8 GiB VPS, or expand into Option A.

---

## Product-owner lock 2026-08-29

Product owner accepted recommended P1–P8 and P10 as **SPD-087–094** and
**SPD-096**, with this override:

- **P9** mapgen extras default **on** (**SPD-095**). Reason: the app is
  primarily still a map tool; gamification and exploration are secondary.

**Status of this WI:** Accepted (lock recorded). Phase 11 coding (SP-099+)
may proceed. Do **not** mark Phase 11 exit met.

---

## In-scope behavior

- Re-verify current code locations in
  `phases/phase-11-independent-map-build-and-serve.md` (snapshot 2026-08-29
  in the investigation note).
- Append **SPD-087–096**. Strike **OQ-40–OQ-49**. Annotate SP-099–104 and
  the phase file. Update README §5 S4 hosting gate (**SPD-093**).
- Record the SPD-003 interpretation (format vs Streifzug CDN). Do not edit the
  product spec.
- Do not mark Phase 11 exit Accepted unilaterally.

## Out-of-scope behavior

- Implementing derive, CLI, `private.h`, VPS, or the Finland run (SP-099–104).
- Option A mapgen collectors (**SPD-089**).
- Editing the product spec or technical audit.
- Changing SP-050 layout or weakening Ed25519 (**SPD-035**, **SPD-036**).
- Baking a LAN Custom Maps URL into a build type (D12).

---

## Locked decisions → SPD-087–096

| Ref | Question | Accepted position | SPD |
| --- | --- | --- | --- |
| P1 | May stock builds use Streifzug map CDNs? | **No.** Own origin; SPD-003 = format. | **SPD-087** |
| P2 | Build vs serve | ≥32 GiB builder; 8 GiB VPS serve-only. | **SPD-088** |
| P3 | Option A this phase? | **No.** Glue Option B. | **SPD-089** |
| P4 | First grain | Eight `Finland_*` + extract World. No allowlist. | **SPD-090** |
| P5 | Signing | Street Pixels Ed25519; Channel A public; B debug-only. | **SPD-091** |
| P6 | `MAP_SERIES` | Keep `2026.06.28` unless compatibility requires a bump. | **SPD-092** |
| P7 | Phase 10 / S4 | Not a Phase 10 blocker. S4 must not ship Streifzug map URLs. | **SPD-093** |
| P8 | Coastline | Skip if extract coasts fail; document missing water. | **SPD-094** |
| P9 | Hotels, isolines, SRTM, subway, UGC, Wikipedia | **On** by default (map tool first). | **SPD-095** |
| P10 | Orchestration | One build-host CLI + rsync; reuse SPD-035. | **SPD-096** |

### P1 — Independent map origin (load-bearing)

**Accepted** → **SPD-087**.

Stock Street Pixels must not request Streifzug map peers for `.mwm`, `.spa`,
`countries.txt`, or `meta/maps.json`. Custom Maps remains user-set (D12).

**Reject.** Shipping Streifzug URLs “until our CDN is ready.” Community mirrors.

### P3 — Glue, not Option A (load-bearing)

**Accepted** → **SPD-089**.

**Reject.** PlaceProcessor / classificator collectors in this phase.

### P7 — Not a Phase 10 blocker; S4 hosting gate (load-bearing)

**Accepted** → **SPD-093**.

SP-089+ stay gated on Phases 1–9. Phase 11 may run in parallel. Public S4
must not ship Streifzug map URLs.

**Reject.** Making Phase 10 wait on Finland mapgen. Finland-only client.

### P9 — Extras on (override)

**Accepted** → **SPD-095**.

Operator defaults enable hotels, isolines, SRTM, subway, UGC, and
Wikipedia/description stages. Phone origin is still **SPD-087**: extras are
build-host datasets. Missing independent source → skip that feed with a
warning, not a Streifzug map-CDN fetch.

---

## Acceptance criteria

1. Maintainer locks P1–P10 (or records amendments). **Met** 2026-08-29
   (P9 override).
2. Matching SPD entries Accepted; OQ-40–OQ-49 struck.
3. Phase 11 / SP-099–104 annotated; README graph does not make Phase 10
   depend on Phase 11; S4 hosting gate recorded.
4. No production code in this WI’s implementation commits.
5. Maintainer decides acceptance of this lock record.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Product-owner lock against **SPD-003**, **SPD-033**, **SPD-035–037**, D12,
  and the 8 GiB / 32 GiB hardware constraint. **Done** this session.

## Failure and rollback considerations

- Do not weaken countries signatures.
- Do not commit spa-bearing `data/countries.txt` before the stock URL serves
  blobs (**SPD-037**).
- Do not fetch Streifzug map hosts to satisfy **SPD-095**.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-11-map-pipeline-b3d3` |
| Decision ids | SPD-087–096 (P9 override SPD-095) |
| Docs touched | `DECISIONS.md`; this file; phase-11; README; SP-099–104 depends |
| Implemented by | Agent |
| Accepted by | Product owner |
| Accepted date | 2026-08-29 |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Pix derive | SP-099 |
| Operator CLI (extras **on**) | SP-100 |
| Identity / keys / configure | SP-101 |
| VPS origin | SP-102 |
| Finland run | SP-103 |
| Exit evidence | SP-104 |
| Option A | Still unallocated (**SPD-089**) |
| Policies beyond FI | After Phase 11 exit |
| Extra feeds with no independent source | SP-100 skip+warn; not Streifzug map CDN |
