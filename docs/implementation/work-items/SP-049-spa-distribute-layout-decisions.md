# SP-049 — `.spa` publish-layout and LAN advertisement decisions

**Phase:** 4 residual / pre-production packaging (device enabler for Phase 5 /
Phase 10 Helsinki walks; **not** a Phase 5 exit gate — **SPD-033** / **SPD-038**)
**Status:** Accepted
**Accepted by:** Maintainer
**Accepted date:** 2026-08-08
**Branch:** `cursor/sp-049-053-spa-publish-fe62`
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

## Locked decisions (D8–D14 → SPD-035–039)

Product-owner locks accepted 2026-08-08:

| Lock | Choice | SPD |
| --- | --- | --- |
| D8 | Single CDN≡LAN publish layout | **SPD-035** |
| D9 | Advertisement = `countries.txt` spa fields only | Affirm **SPD-028** (no new SPD) |
| D10 | Keep signature; Channel A meta-only version bump | **SPD-036** |
| D11 | Temporary bundled countries spa inject; not merged early | **SPD-037** |
| D12 | Custom URL never a build default | Affirm SP-004 (in SPD-035 / SPD-038) |
| D13 | Track = Phase 4 residual / device enabler; not Phase 5 exit; not Option A | **SPD-038** (refs **SPD-033**) |
| D14 | `meta/maps.json`: `map-series`, `latest`, `status` `active`\|`EOL` | **SPD-039** |

### D8 — Single publish layout (CDN ≡ LAN)

**Accepted** → **SPD-035**.

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

**Affirmed** — no new SPD (recorded in SPD-035 consequences).

Presence of both `"spa"` and `"spa_sha1_base64"` on a leaf is the only
advertisement signal (`HasRemoteSpa()`). The local server does not teach the
client about spa via headers, directory listing, or a side manifest.

### D10 — Countries update: keep signature; Channel A uses meta-only version bump

**Accepted** → **SPD-036**.

Applying a new `countries.txt` from a custom/LAN server still requires a valid
`countries.txt.sig` verified with `COUNTRIES_TXT_SIGNATURE_HEX`. Do **not**
weaken Ed25519 verification when `CustomMapServerUrl` is set (community custom
servers would inherit a security hole).

**Same-version skip (code fact):** `Storage::RunCountriesCheckAsyncSaveOnly`
skips when `maps.json` `latest <= m_currentVersion`. Spa-only meta changes
therefore **do not apply** without either a version bump or a new client
same-version refresh path.

**Channel A rule:** when only spa advertisement (and optional other meta)
changes, bump countries `"v"` and `meta/maps.json` `"latest"` together, keep
MWM `"s"` / `"sha1_base64"` unchanged, resign, and serve MWMs under the **new**
version directory (copy/link the same MWM bytes). This mirrors a real map
publish and needs no client change.

**Reject:** unsigned countries apply; “set latest == current and hope spa
ads appear.”

Optional later (not required for this track): a narrowly scoped client
affordance to re-fetch countries at the current version when Custom Map Server
is set **and** signature still verifies — track as a follow-up if bumping is
operationally painful; do not block SP-050–053 on it.

### D11 — Temporary advertisement without CDN publish (debug support)

**Accepted** → **SPD-037**.

Until CDN publishes spa-bearing `countries.txt`, device testing may use **one**
of these channels, in preference order:

| Priority | Channel | Production code path? | Landing rule |
| --- | --- | --- | --- |
| 1 | Signed countries on LAN (D10) with spa meta + blobs + version bump | Yes | Preferred for maintainer walks |
| 2 | Rebuild APK with spa fields injected into **bundled** `data/countries.txt` for FI leaves only, same `"v"` / MWM hashes; serve `.spa` (and optionally `.mwm`) from LAN custom server | Yes (SP-046 fetch) | **Do not merge** spa ads into `street-pixels` `data/countries.txt` until CDN (or equivalent) will serve matching blobs — otherwise stock CDN users advertise missing spa → IncompleteSpa |
| 3 | WritableDir countries override via signed update only | Yes | Same as 1 |

**Reject as V1 approach:** unsigned countries apply; ADB push into map dirs;
debug JNI “install spa from path”; making Spa mandatory for Map OnDisk.

### D12 — Custom server never a build default

**Affirmed** (existing android.mdc / SP-004 posture; in SPD-035 / SPD-038).

LAN URL is entered in Advanced → Custom Maps server (or equivalent). No
flavor, debug build type, or `BuildConfig` may default to a private-network
address.

### D13 — Track placement

**Accepted** → **SPD-038** (references **SPD-033**).

This track is **Phase 4 residual / pre-production packaging** continued (same
as SP-042–048), and is the **device enabler** for Phase 5 / Phase 10 Helsinki
walks. It is **not** a Phase 5 exit criterion and does not reopen Phase 5
coding (SP-033–040). It is **not** Option A mapgen collectors.

### D14 — `meta/maps.json` field contract matches CDN

**Accepted** → **SPD-039**.

`ParseServerMapsAndGetLatestVersion` reads:

- top-level `"map-series"` (hyphen), object keyed by series string
- per series: `"latest"` (integer), `"status"` (string; `"EOL"` sets EOL flag)

Assemble / LAN trees must use `"status": "active"` for non-EOL series (CDN
convention), not invented values like `"current"`. Unknown non-EOL status
strings still work today (only `"EOL"` is special-cased) but matching CDN
avoids drift.

---

## In-scope behavior (this WI)

1. Append **SPD-035–039** to `DECISIONS.md` — Status Accepted with product-owner
   lock date 2026-08-08. Mapping: D8→SPD-035 layout, D9 affirm SPD-028,
   D10→SPD-036 signature+version-bump, D11→SPD-037 temporary inject channel,
   D12 affirm SP-004 posture, D13→SPD-038 track placement, D14→SPD-039
   maps.json contract.
2. Index SP-050–053 work items (plans already drafted).
3. Point phase-04 residual note + README packaging track at this continuation.
4. Cross-link
   [`notes/spa-local-download-current-state.md`](../notes/spa-local-download-current-state.md).

---

## Out-of-scope behavior

- Implementing assemble tool, HTTP server, or client changes (SP-050–052).
- Running device walks (SP-053).
- Option A mapgen collectors / StageMwm (still unallocated residual).
- Editing product spec or technical audit.
- Weakening SPD-016 / SPD-030 / SPD-031.

---

## Acceptance criteria

1. Maintainer locks D8–D14 (or records alternate choices with rationale).
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
- Do not assume same-version `maps.json` latest applies spa ads (D10 bump).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-049-053-spa-publish-fe62` |
| Decision ids | SPD-035, SPD-036, SPD-037, SPD-038, SPD-039 (D9 → SPD-028; D12 → SP-004) |
| Product locks | D8–D14 Accepted 2026-08-08 |
| Docs touched | `DECISIONS.md`; `README.md`; this file |
| Implemented by | Agent |
| Accepted by | Maintainer |
| Accepted date | 2026-08-08 |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Assemble tool | SP-050 |
| LAN HTTP server | SP-051 |
| Countries advertise channels | SP-052 |
| Device playbook | SP-053 |
| Optional same-version signed countries refresh client affordance | Follow-up only if D10 bumps are painful |
| Option A mapgen collectors | Unallocated residual (not this track) |
| Leaf `.pix` / offline derive for dense emit | SP-044 residual; blocks FI spa blobs upstream of SP-050 |
