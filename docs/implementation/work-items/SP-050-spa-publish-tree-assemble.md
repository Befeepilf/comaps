# SP-050 — Assemble CDN-identical `.spa` publish tree

**Phase:** 4 residual / pre-production packaging (LAN + CDN)
**Status:** Planned
**Depends on:** SP-049 Accepted (layout D8, maps.json D14); SP-044 / SP-045 tools Accepted
**Unblocks:** SP-051 (serve), SP-052 (advertise), SP-053 (device)

---

## Objective

Ship a **maintainer-runnable** assemble tool that builds the publish directory
locked in SP-049 / D8: `meta/maps.json`, `maps/{MAP_SERIES}/{dataVersion}/`
containing leaf `.mwm`, leaf `.spa`, `countries.txt` with spa meta, and
optionally `countries.txt.sig`.

This is the **production** publish tree. CDN upload and LAN serve both consume
its output. Debug support is validation + dry-run + inventory logging — not a
second layout.

---

## Motivation

Pieces already exist in isolation:

- `spa_emit_tool` → `{leaf}.spa` (SP-044)
- `inject_spa_meta.py` → spa fields on leaves (SP-045)
- Official MWMs live on CDN under the same URL shape
- `sign_file` exists in `maps_generator.utils.file`

Nothing stitches them into one tree that a static HTTP server (or
`upload_to_cdn.sh`) can consume. Manual stitching will drift from
`MAP_SERIES` / version / hash rules and break SP-046 SHA checks.

---

## In-scope behavior

### 1. CLI (recommended location)

`tools/python/street_pixels/assemble_spa_publish_tree.py` (or
`tools/python/post_generation/assemble_spa_publish_tree.py`) with a thin
`tools/unix/maps/assemble_spa_publish_tree.sh` wrapper if useful.

**Inputs (all explicit — no silent defaults to private hosts):**

| Flag | Role |
| --- | --- |
| `--countries` | Source `countries.txt` (usually `data/countries.txt`) |
| `--spa-dir` | Directory of `{leaf}.spa` from `spa_emit_tool` |
| `--mwm-dir` | Directory of `{leaf}.mwm` for the **same** `dataVersion` (downloaded or mapgen output) |
| `--out` | Output publish root |
| `--map-series` | Must match app `MAP_SERIES` (e.g. `2026.06.28`) |
| `--data-version` | Countries `"v"` / output version dir (e.g. `260714`) |
| `--publish-version` | Optional Channel A meta-only bump (D10). If set and ≠ `--data-version`, rewrite output countries `"v"` to this value, write `maps.json` `latest` to this value, and place MWMs/spa under `maps/{series}/{publish-version}/`. MWM `"s"`/`sha1_base64` unchanged. Default: same as `--data-version`. |
| `--leaves` | Optional allowlist (default: every leaf that has a `.spa` in `--spa-dir`) |
| `--secret-key` | Optional path to Ed25519 PEM; if set, write `countries.txt.sig` |
| `--include-mwm` | Copy/link matching `.mwm` into version dir (default true for LAN full mirror) |
| `--spa-only` | Only place `.spa` + patched countries (for spa-refetch when MWMs already on device) |
| `--dry-run` | Print planned actions; write nothing |

**Outputs under `--out`:**

```text
meta/maps.json
maps/{map_series}/{data_version}/countries.txt
maps/{map_series}/{data_version}/countries.txt.sig   # if --secret-key
maps/{map_series}/{data_version}/{leaf}.spa          # for each advertised leaf
maps/{map_series}/{data_version}/{leaf}.mwm          # if --include-mwm
```

### 2. Assembly steps (deterministic)

1. Load countries JSON; assert source `"v"` == `--data-version` and
   `"map_series"` == `--map-series` (fail closed on mismatch — do not rewrite
   series casually). Effective publish version =
   `--publish-version` if set else `--data-version`.
2. Run `inject_spa_meta` logic on a **copy**: advertise iff `{spa-dir}/{id}.spa`
   exists; omit both keys otherwise; strip stale keys.
3. If publishing a bumped version, set countries `"v"` to the publish version
   (D10); leave MWM size/hash fields untouched.
4. For each advertised leaf: verify spa file size matches injected `"spa"`;
   recompute SHA-1 and assert equals `"spa_sha1_base64"`. Prefer also reading
   spa header `map_data_version` and warn/fail if it disagrees with the MWM
   version being paired (SPD-034 / client VersionMismatch).
5. If `--include-mwm`: for each leaf that will be served (advertised spa leaves
   and/or an optional MWM allowlist), require `{mwm-dir}/{id}.mwm` exists; copy
   or hardlink into the **publish** version dir; verify size/hash against
   countries `"s"` / `"sha1_base64"` (**fail closed** on mismatch — wrong MWM
   version is worse than missing spa).
6. Write `meta/maps.json` with CDN field names (D14):

   ```json
   {
     "map-series": {
       "2026.06.28": { "latest": 260715, "status": "active" }
     }
   }
   ```

   (`"latest"` = publish version; `"status": "active"` not `"current"`.)
7. Optionally `sign_file(countries.txt, secret_key)`.
8. Emit an inventory JSON next to the tree (not served by default): leaf id,
   mwm bytes, spa bytes, advertised yes/no, publish version — for debug and
   SP-053 evidence.

### 3. Debug support (on the production tree)

- `--verbose` logs every file copied and every hash check.
- `--verify-only` re-reads an existing `--out` and re-checks hashes / layout
  without writing.
- Fail with non-zero exit and a single actionable error message (missing leaf,
  hash mismatch, series mismatch).

### 4. Docs

- Operator recipe in this WI + short pointer from
  `docs/DEPLOY_OWN_MAP_SERVER.md` (Street Pixels `.spa` section) — do not replace
  upstream community tools; document that they must serve this layout for spa.
- Note: binaries are **not** committed; `/tmp` or maintainer disk only.

---

## Out-of-scope behavior

- HTTP serving (SP-051).
- Client changes / signature bypass (SP-052 / D10).
- Option A mapgen collectors.
- Producing leaf `.pix` or running `spa_emit_tool` (upstream of `--spa-dir`;
  SP-044 residual if `.pix` missing).
- Uploading to production CDN hosts (may reuse `upload_to_cdn.sh` later; not
  this WI’s acceptance).
- Worldwide spa emit beyond FI seed.

---

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `tools/python/post_generation/inject_spa_meta.py` | Reuse inject + hash |
| `tools/python/maps_generator/utils/file.py` `sign_file` | Optional signature |
| `tools/spa_emit_tool/` | Upstream of `--spa-dir` |
| `libs/platform/downloader_utils.cpp` `GetFileDownloadUrl` | URL shape contract |
| `libs/storage/storage.cpp` `ParseServerMapsAndGetLatestVersion` | `meta/maps.json` shape |
| `private.h` `MAP_SERIES` | Must match |
| `data/countries.txt` | Source meta; spa ads only in **output** copy |

---

## Acceptance criteria

1. Tool builds D8 layout for Finland leaves given spa dir + matching MWMs.
2. Injected spa size/hash match on-disk `.spa`; MWM size/hash match when
   included.
3. Series mismatch vs countries fails closed; `--publish-version` bump rewrites
   `"v"` + `maps.json` `latest` and places files under the new version dir.
4. `meta/maps.json` uses `"map-series"` and `"status": "active"` (D14).
5. `--spa-only` mode produces countries + spa without requiring MWM copies.
6. With `--secret-key`, `.sig` verifies with the corresponding public key.
7. `--verify-only` / `--dry-run` work; inventory written.
8. Python unit tests cover inject+layout happy path, hash mismatch failure, and
   publish-version bump.
9. Docs recipe recorded; agent does not mark Accepted.

---

## Test plan

| Case | Expect |
| --- | --- |
| Temp dirs: 1 fake `.spa` + matching countries leaf + fake `.mwm` | Layout + meta written; hashes match |
| Wrong MWM hash | Non-zero exit; no partial corrupt tree (or tree rolled back) |
| Missing `.spa` for leaf in allowlist | No spa keys; or fail if `--require-all-spa` |
| Series mismatch vs countries | Fail |
| `--publish-version` bump | Output `"v"` and maps.json `latest` = bump; MWMs under new dir; MWM hashes unchanged |
| `--spa-only` | No `.mwm` required |
| Idempotent re-run | Same inventory hashes |

---

## Implementation notes / constraints

- Prefer hardlink when same filesystem to save disk; fall back to copy.
- URL-encode is a **client** concern (`UrlEncode(fileName)`); on-disk names are
  plain `{leaf}.mwm` / `{leaf}.spa` (spaces in leaf ids are literal in the
  filename today — verify against CDN naming for Helsinki leaf).
- Do not rewrite `"s"` / `"sha1_base64"` for MWMs; only add spa fields.
- Production-first: the same command output is what CDN should eventually
  host.

## Failure and rollback

- Never write placeholder spa meta.
- Never “fix” MWM hashes to match whatever file is present.
- Signing after inject only; never inject after sign.

## Discovered follow-up

| Item | Owner |
| --- | --- |
| Serve tree on LAN | SP-051 |
| Bundle vs signed advertise policy | SP-052 |
| CDN upload automation for `.spa` | ops / later WI |
| Leaf `.pix` acquisition for emit | SP-044 residual |
