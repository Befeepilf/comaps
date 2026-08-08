# SP-052 — Countries advertisement path for LAN / pre-CDN `.spa`

**Phase:** 4 residual / pre-production packaging
**Status:** Planned
**Depends on:** SP-049 Accepted (D10–D11); SP-050 / SP-051 for tree + server
**Unblocks:** SP-053 device validation; honest CDN cutover later

---

## Objective

Make `HasRemoteSpa()` true on a test device for Finland leaves **without**
weakening production countries signature verification, so SP-046 downloads
`.spa` from the LAN server.

Production-first: the long-term path is signed `countries.txt` on CDN/LAN.
Debug support is a **temporary, non-merged** bundled inject recipe and
observability — not an unsigned update backdoor.

---

## Motivation

Gaps that block LAN download today:

1. Bundled `data/countries.txt` has no spa fields → SP-046 never enqueues Spa.
2. Downloaded countries require Ed25519 `.sig` → unsigned LAN countries are
   rejected (`Storage::RunCountriesCheckAsyncSaveOnly`).
3. Committing spa ads into git before CDN hosts blobs would mark IncompleteSpa
   for stock users (**SPD-031** fail-soft, but noisy / wrong).

---

## In-scope behavior

### Channel A — Production / maintainer (preferred)

1. SP-050 assemble with `--secret-key` → signed countries with spa meta.
2. SP-051 serve tree; `meta/maps.json` `latest` ≥ device current version (or
   equal with care — see notes).
3. Device Custom Maps URL → check updates → persist countries → download /
   retry spa.

**Client code changes for Channel A:** none expected if version/series match
existing update path. Implementation work is ops + docs + maybe a small
“force check updates” UX note.

If `latest == current` and spa ads are the only change, confirm whether the
update path applies same-version countries. Today
`dataVersion <= m_currentVersion` skips update. **Plan requirement:** either

- bump a **local-only** patch version in the assembled countries `"v"` and
  `maps.json` latest (still signed; MWMs unchanged hashes), **or**
- add an explicit, narrowly scoped client affordance: when Custom Map Server is
  set, allow re-fetch of countries at the **current** version if signature
  verifies (production-safe; no signature skip).

**Recommended production-first choice:** prefer the **version bump** in the
LAN/CDN publish (countries `"v"` and maps.json) when only meta changes —
mirrors how real map publishes work. Document the bump policy in SP-049/D10
follow-through. Client “same-version refresh” is optional and must still
verify signatures.

### Channel B — Temporary bundled inject (debug support, non-default)

For walks when the publish signing key is unavailable in the session:

1. Run `inject_spa_meta` against a **local checkout copy** of
   `data/countries.txt` using the SP-050 spa dir (same `"v"`, same MWM hashes).
2. Build a **local** Android APK that embeds that countries file.
3. Serve `.spa` (spa-only tree) from LAN; set Custom Maps URL.
4. With Map already OnDisk: `RetryIncompleteSpaDownloads` / re-open app so
   `MaybeEnqueueRemoteSpa` runs; or delete+redownload leaf.

**Hard rules:**

- Do **not** merge spa-advertising `data/countries.txt` to `street-pixels`
  until CDN (or the URL stock builds use) serves matching `.spa` blobs.
- Do not change `GetRemoteSize()` / MWM hash fields.
- Document the local inject as a maintainer recipe in SP-053, not as a CI
  default.

### Channel C — Rejected

- Skip Ed25519 when custom server is set.
- ADB push `.spa` into app storage as the supported path.
- Debug-only download API that bypasses `Storage`.

### Observability (debug on production path)

- Ensure existing logs are enough to diagnose: custom server URL, countries
  load source (bundled vs writable), `HasRemoteSpa`, Spa enqueue, SHA fail,
  `MarkSpaIncomplete`.
- Optional: settings/debug screen listing `GetIncompleteSpaCountries()` —
  polish may residual to Phase 10; storage API already exists (SP-048).

---

## Out-of-scope behavior

- Redesigning metaserver.
- Worldwide spa ads.
- Android toast polish (Phase 10 residual from SP-048).
- Weakening SPD-031 fail-soft.

---

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `libs/storage/storage.cpp` | Countries check, version compare, signature, `MaybeEnqueueRemoteSpa`, `RetryIncompleteSpaDownloads` |
| `libs/platform/country_file.*` | `HasRemoteSpa` |
| `tools/python/post_generation/inject_spa_meta.py` | Channel B |
| `private.h` | `COUNTRIES_TXT_SIGNATURE_HEX`, `MAP_SERIES` |
| `data/countries.txt` | Must stay spa-free in git until CDN cutover |

---

## Acceptance criteria

1. Written recipe for Channel A (signed, version policy clear) verified by
   maintainer or agent with key access.
2. Written recipe for Channel B (local APK inject) that explicitly forbids
   merging spa ads early.
3. If same-version countries refresh is implemented, it still verifies
   Ed25519; tests cover accept/reject.
4. No signature bypass lands.
5. Automated tests for any client change; docs for no-client Channel A.
6. Agent does not mark Accepted.

---

## Test plan

| Case | Expect |
| --- | --- |
| Signed countries newer `v` with spa meta | Applied; `HasRemoteSpa` true for FI leaves |
| Tampered countries / bad sig | Rejected; bundled countries unchanged |
| Channel B APK + spa-only server + Map OnDisk | Spa downloads; areas load |
| No advertise | Spa never queued (existing SP-046 tests) |

---

## Implementation notes / constraints

- Confirm leaf filename encoding for spaces (`Finland_Southern Finland_Helsinki.spa`)
  against `UrlEncode` in downloader.
- `map_data_version` inside `.spa` headers should align with the MWM /
  policy expectations the client checks on load (SP-027 / SPD-034) — assemble
  tool should stamp / verify consistency (cross-check in SP-050 verify).
- Production cutover checklist (later ops): emit FI spa → assemble → upload CDN
  → publish signed countries with spa → only then merge ads into repo countries
  if the repo ships the same version.

## Failure and rollback

- If a bad countries with spa ads ships without blobs: IncompleteSpa +
  fail-closed areas (**SPD-031**) — retry after blobs appear; do not invent
  geometry.
- Revert Channel B local countries by rebuilding from clean tree.

## Discovered follow-up

| Item | Owner |
| --- | --- |
| Device playbook + evidence | SP-053 |
| CDN cutover PR merging spa ads into `data/countries.txt` | ops after blobs live |
| Settings UI for incomplete spa | Phase 10 |
