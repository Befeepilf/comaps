# SP-049 — `.spa` publish-layout and LAN advertisement decisions

**Phase:** 4 residual / pre-production packaging (device enabler for Phase 5 /
Phase 10 Helsinki walks; **not** a Phase 5 exit gate — **SPD-033**)
**Status:** Planned
**Branch:** `cursor/spa-local-download-plans-5ca4` (plans); implementation TBD
**Depends on:** SP-042–048 Accepted (**SPD-027–034**)
**Unblocks:** SP-050–053

---

## Objective

Record accepted decisions for (1) the **only** on-disk / URL publish layout for
leaf `.spa` beside MWMs, and (2) how a **local-network** mirror advertises spa
meta to the client without inventing a second download protocol — so SP-050–053
do not encode guesses.

Docs / `DECISIONS.md` only in the implementation of this WI. No production
binary changes in this item.

---

## Motivation

Client download (SP-046) and meta parse (SP-045) are Accepted. Ops publish and
LAN serve are residual. Phase 5 device testing cannot `adb push` `.spa` into
app-private storage. Without locked layout + advertisement rules, a “local
server” risks becoming a debug-only path that diverges from CDN.

---

## Proposed decisions (for maintainer lock → SPD-035+)

### D8 — Single publish layout (CDN ≡ LAN)

**Recommended: Accept.**

Publish tree and HTTP URL paths are identical for production CDN and local
mirror:

```text
{root}/
  meta/
    maps.json                          # map-series → latest version (+ status)
  maps/
    {MAP_SERIES}/                      # e.g. 2026.06.28 from private.h
      {dataVersion}/                   # e.g. 260714 from countries "v"
        countries.txt
        countries.txt.sig              # required for production update apply
        {mwmLeafId}.mwm
        {mwmLeafId}.spa                # iff leaf has exploration sidecar
        … other leaves …
```

Client already builds relative URLs as
`maps/{MAP_SERIES}/{dataVersion}/{UrlEncode(fileName)}` (`GetFileDownloadUrl`)
and meta as `meta/maps.json` (`SERVER_MAPS_FILE`). Do **not** invent
`/spa/…`, query params, or alternate extensions.

**Reject:** separate debug URL scheme; serving `.spa` from a different base than
maps; embedding `.spa` inside the `.mwm`.

### D9 — Advertisement remains `countries.txt` only (**SPD-028**)

**Recommended: Affirm.**

Presence of both `"spa"` and `"spa_sha1_base64"` on a leaf is the only
advertisement signal (`HasRemoteSpa()`). The local server does not teach the
client about spa via headers, directory listing, or a side manifest.

### D10 — Countries update on LAN uses the production signature path

**Recommended: Accept (production-first).**

Applying a new `countries.txt` from a custom/LAN server still requires a valid
`countries.txt.sig` verified with `COUNTRIES_TXT_SIGNATURE_HEX`. Do **not**
weaken Ed25519 verification when `CustomMapServerUrl` is set (community custom
servers would inherit a security hole).

**Consequence:** LAN mirrors that need a *new* countries file (spa ads, version
bump) must sign with the real publish key (maintainer ops), same as CDN.

### D11 — Temporary advertisement without CDN publish (debug support)

**Recommended: Accept as a bounded, non-default channel.**

Until CDN publishes spa-bearing `countries.txt`, device testing may use **one**
of these channels, in preference order:

| Priority | Channel | Production code path? | Landing rule |
| --- | --- | --- | --- |
| 1 | Signed countries on LAN (D10) with spa meta + blobs | Yes | Preferred for maintainer walks |
| 2 | Rebuild APK with spa fields injected into **bundled** `data/countries.txt` for FI leaves only, same `"v"` / MWM hashes; serve `.spa` (and optionally `.mwm`) from LAN custom server | Yes (SP-046 fetch) | **Do not merge** spa ads into `street-pixels` `data/countries.txt` until CDN (or equivalent) will serve matching blobs — otherwise stock CDN users advertise missing spa → IncompleteSpa |
| 3 | WritableDir countries override via signed update only | Yes | Same as 1 |

**Reject as V1 approach:** unsigned countries apply; ADB push into map dirs;
debug JNI “install spa from path”; making Spa mandatory for Map OnDisk.

### D12 — Custom server never a build default

**Recommended: Affirm** (existing android.mdc / SP-004 posture).

LAN URL is entered in Advanced → Custom Maps server (or equivalent). No
flavor, debug build type, or `BuildConfig` may default to a private-network
address.

### D13 — Track placement

**Recommended: Accept.**

This track is **Phase 4 residual / pre-production packaging** continued (same
as SP-042–048), and is the **device enabler** for Phase 5 / Phase 10 Helsinki
walks. It is **not** a Phase 5 exit criterion and does not reopen Phase 5
coding (SP-033–040).

---

## In-scope behavior (this WI)

1. Append **SPD-035–039** (or fewer, if maintainer collapses) to
   `DECISIONS.md` once locks land — Status Accepted with product-owner lock
   date.
2. Create/index SP-050–053 work items (plans already drafted under this
   planning branch).
3. Point phase-04 residual note + README packaging track at this continuation.
4. Cross-link
   [`notes/spa-local-download-current-state.md`](../notes/spa-local-download-current-state.md).

---

## Out-of-scope behavior

- Implementing assemble tool, HTTP server, or client changes (SP-050–052).
- Running device walks (SP-053).
- Editing product spec or technical audit.
- Weakening SPD-016 / SPD-030 / SPD-031.
- Marking Accepted unilaterally.

---

## Acceptance criteria

1. Maintainer locks D8–D13 (or records alternate choices with rationale).
2. Matching SPD entries in `DECISIONS.md` with Status Accepted.
3. README / phase-04 residual index this continuation; SPD-033 still holds
   (not Phase 5 exit).
4. No production code in this WI’s implementation commits.
5. Maintainer decides acceptance.

---

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer review against SPD-027–034 and the scoped-storage device constraint.

## Failure and rollback considerations

- Do not invent spa-diffs (SPD-029).
- Do not skip countries signature for custom servers (D10).
- Do not commit spa advertisements to bundled countries before blobs exist on
  the URL the stock app will hit (D11).

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Assemble tool | SP-050 |
| LAN HTTP server | SP-051 |
| Countries advertise channels | SP-052 |
| Device playbook | SP-053 |
