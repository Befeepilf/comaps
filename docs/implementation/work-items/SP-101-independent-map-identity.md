# SP-101 — Independent map identity

**Phase:** 11 — Independent map build and serve
**Status:** In review
**Depends on:** SP-098 lock (**SPD-087**, **SPD-091**, **SPD-092**, **SPD-093**)
**Unblocks:** SP-103 (Channel A); stock APK origin

---

## Objective

Give Street Pixels its own map-signing identity and stock download hosts so
builds do not use Streifzug map CDNs or Streifzug `COUNTRIES_TXT_SIGNATURE_HEX`.

`private.h` remains gitignored. This WI documents the fields, a
**non-secret** template, `configure.sh` World bootstrap without Streifzug, and
how to generate/replace keys.

---

## Motivation

Even a perfect publish tree is unused if the APK lists Streifzug in
`DEFAULT_URLS_JSON`, or if `configure.sh` curls `mapgen-fi-1.streifzug.app`, or
if `countries.txt.sig` does not match the APK public key (**SPD-036**).

D12 still forbids a baked **Custom Maps** LAN URL. The stock list is
`DEFAULT_URLS_JSON`.

---

## In-scope behavior

- Document required `private.h` map fields for this fork:
  `DEFAULT_URLS_JSON`, `METASERVER_URL` (empty or our meta host),
  `COUNTRIES_TXT_SIGNATURE_HEX`, `MAP_SERIES` (P6).
- Committed **template** (no secrets), e.g. `private.h.street-pixels.example`,
  with placeholder HTTPS origin — not a private-range IP (SP-004).
- Keygen recipe: Ed25519 PEM pair; how `sign_file` / SP-050 `--secret-key`
  matches `COUNTRIES_TXT_SIGNATURE_HEX`.
- `configure.sh`: default or documented path that **does not** fetch Streifzug
  World. Options consistent with P1: `SKIP_MAP_DOWNLOAD=1` plus copy from
  pipeline output, **or** fetch World from **our** origin once SP-102 exists.
  Fail closed if Streifzug URL would be used.
- Egress inventory note for SP-004: map hosts become the Street Pixels
  origin; Geofabrik only on the **build** machine.
- Tests: if any compiled default URL list is testable without secrets, assert
  Streifzug map host substrings are absent in the Street Pixels configuration
  path. Do not require production keys in CI.
- Do not merge spa-bearing `data/countries.txt` until the stock URL serves
  blobs (**SPD-037**) — SP-103 publishes first.

## Out-of-scope behavior

- Brand/listing copy (**SPD-084** residual).
- Competition API host (**SPD-062**).
- Custom Maps as the only production path.
- Implementing nginx (SP-102).

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `private.h` (gitignored) | Hosts, series, signature hex |
| `private.h.street-pixels.example` | Non-secret template |
| `configure.sh` | World.mwm bootstrap via `map_identity configure-world` |
| `tools/python/street_pixels/map_identity.py` | Fail-closed resolver, Ed25519 helpers |
| `libs/storage/storage.cpp` | `MAP_SERIES`, signature verify |
| `libs/platform/downloader_utils.cpp` | `GetFileDownloadUrl` |
| `docs/implementation/notes/sp-101-map-identity.md` | Keygen + env recipe |
| `docs/implementation/work-items/SP-004-network-egress-and-api-configuration.md` | Egress inventory |

## Acceptance criteria

1. Written recipe produces a keypair and a `private.h` that verifies a
   test-signed `countries.txt`.
2. `configure.sh` path for developers does not contact Streifzug map hosts.
3. Example/template committed; secrets not committed.
4. Maintainer decides acceptance.

## Required automated tests

- Signature round-trip with a throwaway key in tests (may already exist via
  `maps_generator.utils.file` / storage tests). Add coverage if missing.
- Configure script: unit or shell test that the Streifzug `MAPS_BASE_URL` is
  not used when the Street Pixels path is selected.

## Required manual validation

- Maintainer fills real `private.h` on the build host (not in git).

## Failure and rollback considerations

- Do not point `DEFAULT_URLS_JSON` at HTTP LAN.
- Do not disable Ed25519 when the origin is “ours”.
- Do not commit production secret PEM.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-101-independent-map-identity-b3d3` |
| Commits | `6d3c4d337` `[platform] Add Street Pixels private.h example template`; `d0db1f9c1` `[tools] Fail closed World bootstrap without Streifzug`; `3521efcd8` `[docs] Document map identity keygen and configure path`; `eb6390e35` `[platform] Untrack private.h and seed from the example template`; `601468e3a` `[tools] Seed missing private.h during configure`; this `[docs]` commit |
| Template | `private.h.street-pixels.example` — `DEFAULT_URLS_JSON` `https://maps.example.invalid/`; `METASERVER_URL` `""`; `COUNTRIES_TXT_SIGNATURE_HEX` 64 zero hex chars; `MAP_SERIES` `2026.06.28` |
| Untrack | `git ls-files -- private.h` empty. `git cat-file -e HEAD:private.h` fails. Working-tree `private.h` kept local (gitignore applies). Clones copy the example. |
| Keygen | [notes/sp-101-map-identity.md](../notes/sp-101-map-identity.md) — `openssl genpkey -algorithm Ed25519`; `pkeyutl -sign -rawin` matching `sign_file` / SP-050 `--secret-key`; `public-hex` for `COUNTRIES_TXT_SIGNATURE_HEX` |
| configure.sh | `export PYTHONPATH="$REPO_ROOT/tools/python"` then `ensure-private-h` and `configure-world`. Env: `SKIP_MAP_DOWNLOAD`, `STREET_PIXELS_LOCAL_WORLD`, `STREET_PIXELS_WORLD_DIR`, `STREET_PIXELS_MAPS_BASE_URL` (HTTPS, non-Streifzug; SP-102 fills the public host). Legacy `MAPS_BASE_URL` refused when Streifzug. No mapgen-fi-1 fallback. WorldCoasts 404/missing omitted (**SPD-094**). Root `CMakeLists.txt` also copies the example when `private.h` is missing (Android Gradle). |
| Tests | `cd tools/python && PYTHONPATH=. python3 -m unittest street_pixels.tests.test_map_identity` — **25/25** OK. Combined `street_pixels.tests` (`test_map_identity` + `test_map_pipeline` + `test_prepare_spa_debug_root` + `test_serve_spa_publish_tree`) — **85/85** OK. CLI: `SKIP_MAP_DOWNLOAD=1` skip exit 0; no origin exit 1; Streifzug `MAPS_BASE_URL` refused; LAN HTTPS refused. Injecting `mapgen-fi-1.streifzug.app` into the template fails `TemplateAuditTest`. Full mapgen **not** run. |
| Implemented by | Cursor Agent (`cursoragent@cursor.com`) |
| Reviewed by | — |
| Accepted by | — |

Phase 11 exit is **not** met.

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Public origin URL / TLS | SP-102 |
| First signed FI countries | SP-103 |
| Tracked `private.h` shipped Streifzug CDNs despite `.gitignore` | Untracked (`git rm --cached`); clones copy the example; local working-tree file may still list Streifzug until the maintainer replaces it |
| `DEFAULT_CONNECTION_CHECK_IP` is still Streifzug Fastly `151.101.195.52` | Residual; connectivity check, not a map CDN |
| `prepare_spa_debug_root` default bases still include Streifzug CDNs | Residual; LAN debug helper only, not stock APK |
