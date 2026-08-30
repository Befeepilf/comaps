# SPA advertise channels — Channel A / Channel B recipes (SP-052)

**Status:** Recipe docs for maintainer/agent use. No client signature bypass.
**Decisions:** SPD-028, SPD-036, SPD-037 (D10–D11).
**Depends on:** SP-050 assemble, SP-051 serve.

Production-first rule: long-term path is **signed** `countries.txt` on CDN/LAN
(Channel A). Channel B is a **temporary, non-merged** local APK inject when the
signing key is unavailable in the session.

---

## Preconditions (both channels)

**Shortcut (device on CDN latest):** 

```bash
cd tools/python
PYTHONPATH=. python3 -m street_pixels prepare_spa_debug_root \
  --spa-dir /path/to/spa_emit_out \
  --out /tmp/spa_debug_root \
  --channel serve-only   # or A / B
```

This downloads public CDN `meta/maps.json` + latest `countries.txt` for
`MAP_SERIES`, then assembles the publish root. Prefer that over git
`data/countries.txt` (bundled seed often lags WritableDir after map updates).

Manual path:

1. Emit leaf `.spa` with `spa_emit_tool` (SP-044). Finland Helsinki leaf id
   typically `Finland_Southern Finland_Helsinki` (space encoded in URLs by the
   client `UrlEncode`).
2. Matching `.mwm` for the same `dataVersion` / `MAP_SERIES` the app uses
   (`MAP_SERIES` = `2026.06.28` in current private.h). Use **CDN / WritableDir**
   countries `"v"` (e.g. 260803), not necessarily bundled `data/countries.txt`.
3. SP-050 assemble tree under a local `--out` (not committed) — or the prepare
   helper above.
4. SP-051 (or equivalent) serving that `--out`.
5. Device Custom Maps URL = LAN host (user-set only; **D12**).

Do **not** merge spa-bearing `data/countries.txt` to `street-pixels` until CDN
(or the URL stock builds hit) serves matching `.spa` blobs (**SPD-037**).

---

## Channel A — signed countries + meta-only version bump (preferred)

Client code changes: **none**. Same-version `maps.json` `latest` does **not**
apply spa ads (`dataVersion <= m_currentVersion` → NoUpdate).

### Steps

1. Choose `--publish-version` **strictly greater** than the device’s current
   countries `"v"` (example: device has `260714` → publish `260715`).
2. Assemble with spa meta, MWMs under the **new** version dir, and sign:

```bash
cd tools/python
PYTHONPATH=. python3 -m post_generation assemble_spa_publish_tree \
  --countries ../../data/countries.txt \
  --spa-dir /path/to/spa_emit_out \
  --mwm-dir /path/to/matching_mwms \
  --out /tmp/spa_publish \
  --map-series 2026.06.28 \
  --data-version 260714 \
  --publish-version 260715 \
  --secret-key /path/to/ed25519_secret.pem
```

3. Confirm layout:

```text
/tmp/spa_publish/meta/maps.json
  → "map-series"."2026.06.28"."latest" == 260715
  → "status": "active"
/tmp/spa_publish/maps/2026.06.28/260715/countries.txt   # "v": 260715, spa fields
/tmp/spa_publish/maps/2026.06.28/260715/countries.txt.sig
/tmp/spa_publish/maps/2026.06.28/260715/{leaf}.mwm
/tmp/spa_publish/maps/2026.06.28/260715/{leaf}.spa
```

4. Serve:

```bash
PYTHONPATH=. python3 -m street_pixels serve_spa_publish_tree \
  --root /tmp/spa_publish --host 0.0.0.0 --port 8080
```

5. On device: set Custom Maps URL → check for map updates → accept countries
   update → download / open Finland leaf (or
   `RetryIncompleteSpaDownloads` if Map already OnDisk).

### Verification

| Check | Expect |
| --- | --- |
| `curl {base}/meta/maps.json` | `latest` = publish version, `status` active |
| `curl {base}/maps/.../countries.txt.sig` | 200 |
| Tampered countries / bad sig | Rejected; bundled countries unchanged |
| `latest == current` with spa-only countries | **NoUpdate**; spa ads not applied (documents D10) |
| Log / debug | `HasRemoteSpa` true for FI leaves after countries apply |

Signing key: same Ed25519 PEM family used by mapgen `sign_file` /
`COUNTRIES_TXT_SIGNATURE_HEX` public half. Keygen and hex extract:
[sp-101-map-identity.md](sp-101-map-identity.md). Without the secret key, use
Channel B for walks — do **not** disable verification.

---

## Channel B — temporary bundled inject (debug support only)

Use when Channel A signing is unavailable. Builds a **local** APK that embeds
spa-advertising countries; **never** merge that countries file to git early.

### Steps

1. Inject spa meta into a **local copy** (same `"v"`, same MWM hashes):

```bash
cd tools/python
mkdir -p /tmp/spa_channel_b
cp ../../data/countries.txt /tmp/spa_channel_b/countries.txt
PYTHONPATH=. python3 -m post_generation inject_spa_meta \
  --countries /tmp/spa_channel_b/countries.txt \
  --spa-dir /path/to/spa_emit_out \
  --output /tmp/spa_channel_b/countries.txt
```

2. Point the Android build at that countries file for a **local** APK only
   (copy over `data/countries.txt` in a dirty worktree, or your usual local
   override). Do not push / merge.

3. Assemble a **spa-only** tree for the server (MWMs already on device optional):

```bash
PYTHONPATH=. python3 -m post_generation assemble_spa_publish_tree \
  --countries /tmp/spa_channel_b/countries.txt \
  --spa-dir /path/to/spa_emit_out \
  --out /tmp/spa_publish_spa_only \
  --map-series 2026.06.28 \
  --data-version 260714 \
  --spa-only
```

4. Serve spa-only tree; set Custom Maps URL; with Map OnDisk, reopen app or
   trigger spa retry so `MaybeEnqueueRemoteSpa` runs; or delete+redownload leaf.

### Hard rules

- Do **not** merge spa ads into `street-pixels` `data/countries.txt` until CDN
  hosts matching blobs.
- Do not change `GetRemoteSize()` / MWM `"s"` / `"sha1_base64"`.
- Do not skip Ed25519 for custom servers.
- Do not ADB-push `.spa` into app-private storage as the supported path.
- Revert by rebuilding from a clean tree / discarding local countries.

---

## Channel C — rejected

- Skip Ed25519 when custom server is set.
- ADB push `.spa` as the pass method for SP-053.
- Debug-only download API that bypasses `Storage`.

---

## Observability

Existing logs should show: custom server URL, countries load source (bundled vs
writable), `HasRemoteSpa`, Spa enqueue, SHA fail, `MarkSpaIncomplete`. Optional
settings UI for `GetIncompleteSpaCountries()` may residual to Phase 10.

---

## Production cutover (later ops — not this WI)

1. Emit FI (then more) spa → assemble → upload CDN.
2. Publish signed countries with spa + bumped `"v"` (Channel A).
3. Only then merge ads into repo `data/countries.txt` if the repo ships that
   same version.
