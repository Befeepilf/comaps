# SP-098 — Independent map-pipeline architecture decisions

**Phase:** 11 — Independent map build and serve
**Status:** Planned (awaiting product-owner lock of P1–P10)
**Branch:** `cursor/phase-11-map-pipeline-b3d3`
**Depends on:** None for this docs item. SP-042–048 Accepted; SP-049–051
  tools in tree (**SPD-027–039**).
**Unblocks:** SP-099–104 (coding must not guess P1–P10)
**Investigation note:**
  [`notes/SP-098-map-pipeline-architecture.md`](../notes/SP-098-map-pipeline-architecture.md)

---

## Objective

Record accepted decisions for Street Pixels **map origin**, hardware split,
Option B orchestration versus Option A collectors, first-country grain,
signing, `MAP_SERIES`, Phase 10/S4 relationship, coastlines, optional mapgen
feeds, and the operator entrypoint — so SP-099+ do not encode guesses.

Docs / `DECISIONS.md` only in the implementation of this work item after lock.
No production binary changes here. **Do not append Accepted SPDs until the
product owner locks P1–P10.**

---

## Motivation

The client already downloads `{leaf}.mwm` and advertised `{leaf}.spa` from
one Custom Maps / default-host URL (**SPD-035**). What is missing is an
origin we operate: CoMaps CDN must not carry Street Pixels map traffic, and
the 256 GiB planet builder is not the hardware we have.

Without locks, later items will either hit CoMaps “just for World.mwm”,
run `maps_generator` on the 8 GiB VPS, or expand into Option A.

---

## In-scope behavior

- Re-verify current code locations in
  `phases/phase-11-independent-map-build-and-serve.md` (snapshot 2026-08-29
  in the investigation note).
- After product-owner lock: append the matching **SPD-087+** entries, strike
  **OQ-40–OQ-49**, annotate SP-099–104 and the phase file.
- Record the SPD-003 interpretation (format vs CoMaps CDN). Do not edit the
  product spec.
- Do not mark this work item or Phase 11 exit Accepted unilaterally.

## Out-of-scope behavior

- Implementing derive, CLI, `private.h`, VPS, or the Finland run (SP-099–104).
- Option A mapgen collectors.
- Editing the product spec or technical audit.
- Changing SP-050 layout or weakening Ed25519 (**SPD-035**, **SPD-036**).
- Baking a LAN Custom Maps URL into a build type (D12).

---

## Recommended locks → P1–P10 (not Accepted)

Product owner should accept or amend. Recommended positions:

| Ref | Question | Recommended position | Why | OQ |
| --- | --- | --- | --- | --- |
| P1 | May stock builds use CoMaps map CDNs? | **No.** `DEFAULT_URLS_JSON` / `METASERVER_URL` / `configure.sh` World fetch must not use CoMaps map hosts. OSM extracts from Geofabrik / OSM, not CoMaps. **SPD-003** = compatible **MWM format**, not their CDN. | Street Pixels is a different app. Community mirrors of CoMaps still load their CDN. | OQ-40 |
| P2 | Build vs serve hardware | Generate on ≥32 GiB (`NODE_STORAGE: map`, cap threads). VPS (8 GiB) **serves** the SP-050 tree only. | ini documents `mem`+256 GiB for planet only. | OQ-41 |
| P3 | Option A this phase? | **No.** Orchestrate Option B + `maps_generator`. Option A stays unallocated. | **SPD-033** / **SPD-038**; collectors are huge. Glue is the seamless operator path. | OQ-42 |
| P4 | First grain | Eight `Finland_*` leaves + extract-sourced `World.mwm`. Further countries are the same CLI, later runs. No city allowlist in the app. | FI policy and borders exist; Helsinki is the device-walk leaf. | OQ-43 |
| P5 | Signing | Generate Street Pixels Ed25519 keys. Public half → `COUNTRIES_TXT_SIGNATURE_HEX`. Channel A on the public origin. Channel B stays debug-only (**SPD-037**). | Stock phones ignore unsigned countries. | OQ-44 |
| P6 | `MAP_SERIES` | Keep `2026.06.28` unless generator/app compatibility requires a bump. | Client already keys URLs on this epoch. | OQ-45 |
| P7 | Phase 10 / S4 | Phase 11 **does not** block Phase 10 exit. Recommended: public S4 must not ship CoMaps map URLs. | Phase 10 adds no features. Hosting is a different gate. | OQ-46 |
| P8 | Coastline | Skip `Coastline` / omit `WorldCoasts` when extract coasts fail. Document missing ocean fill. Planet-quality coasts are residual (rent-a-box). | Continuous worldwide coastline is a planet job. | OQ-47 |
| P9 | Hotels, isolines, SRTM, subway, UGC, Wikipedia | **Off** in the operator default ini. | Extra hosts, RAM, and CoMaps-adjacent feeds. Not required for collection. | OQ-48 |
| P10 | Orchestration | One build-host CLI + rsync. No generate daemon on the VPS. Reuse assemble/serve; do not add `/spa/`. | Seamless = one entrypoint, same HTTP contract. | OQ-49 |

### P1 — Independent map origin (load-bearing)

**Recommended.** Stock Street Pixels must not request
`*.comaps.app` / `*.comaps.tech` / listed CoMaps map peers for `.mwm`,
`.spa`, `countries.txt`, or `meta/maps.json`. Debug
`prepare_spa_debug_root` must not be the production countries source.

Custom Maps URL remains a user Advanced override (D12). The **default**
host list is `DEFAULT_URLS_JSON`.

**Reject.** Shipping CoMaps URLs “until our CDN is ready” in a public APK.
Using community CoMaps mirrors.

### P3 — Glue, not Option A (load-bearing)

**Recommended.** Seamless means the operator runs one command that calls
existing tools in order. It does not mean `StageMwm` writes `.spa`.

**Reject.** Expanding this phase into PlaceProcessor / classificator collectors.

### P7 — Not a Phase 10 blocker (load-bearing)

**Recommended.** SP-089+ stay gated on Phases 1–9 as today. Phase 11 may
run in parallel. If P7’s S4 hosting gate is accepted, update README §5
release slices in the SP-098 **lock** commit, not before.

**Reject.** Making Phase 10 wait on Finland mapgen. Making Finland the only
installable region in the client.

---

## Acceptance criteria

1. Maintainer locks P1–P10 (or records amendments).
2. Matching SPD entries Accepted; OQ-40–OQ-49 struck.
3. Phase 11 / SP-099–104 annotated; README graph does not make Phase 10
   depend on Phase 11.
4. No production code in this WI’s implementation commits.
5. Maintainer decides acceptance.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review against **SPD-003**, **SPD-033**, **SPD-035–037**, D12,
  and the 8 GiB / 32 GiB hardware constraint.

## Failure and rollback considerations

- Do not treat recommended positions as Accepted before lock.
- Do not weaken countries signatures to make a custom origin easier.
- Do not commit spa-bearing `data/countries.txt` before the stock URL serves
  blobs (**SPD-037**).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-11-map-pipeline-b3d3` |
| Decision ids | *none until lock* |
| Docs touched | `DECISIONS.md` §15 OQ-40–OQ-49; this file; phase-11; README index |
| Implemented by | Agent (planning) |
| Accepted by | — |
| Accepted date | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Pix derive | SP-099 |
| Operator CLI | SP-100 |
| Identity / keys / configure | SP-101 |
| VPS origin | SP-102 |
| Finland run | SP-103 |
| Exit evidence | SP-104 |
| Option A | Still unallocated |
| Policies beyond FI | After Phase 11 exit |
