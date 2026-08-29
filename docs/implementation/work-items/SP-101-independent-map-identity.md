# SP-101 — Independent map identity

**Phase:** 11 — Independent map build and serve
**Status:** Planned
**Depends on:** SP-098 lock (P1, P5, P6, P7)
**Unblocks:** SP-103 (Channel A); stock APK origin

---

## Objective

Give Street Pixels its own map-signing identity and stock download hosts so
builds do not use CoMaps map CDNs or CoMaps `COUNTRIES_TXT_SIGNATURE_HEX`.

`private.h` remains gitignored. This WI documents the fields, a
**non-secret** template, `configure.sh` World bootstrap without CoMaps, and
how to generate/replace keys.

---

## Motivation

Even a perfect publish tree is unused if the APK lists CoMaps in
`DEFAULT_URLS_JSON`, or if `configure.sh` curls `mapgen-fi-1.comaps.app`, or
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
- `configure.sh`: default or documented path that **does not** fetch CoMaps
  World. Options consistent with P1: `SKIP_MAP_DOWNLOAD=1` plus copy from
  pipeline output, **or** fetch World from **our** origin once SP-102 exists.
  Fail closed if CoMaps URL would be used.
- Egress inventory note for SP-004: map hosts become the Street Pixels
  origin; Geofabrik only on the **build** machine.
- Tests: if any compiled default URL list is testable without secrets, assert
  CoMaps map host substrings are absent in the Street Pixels configuration
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
| `configure.sh` | World.mwm fetch |
| `libs/storage/storage.cpp` | `MAP_SERIES`, signature verify |
| `libs/platform/downloader_utils.cpp` | `GetFileDownloadUrl` |
| `docs/implementation/work-items/SP-004-network-egress-and-api-configuration.md` | Egress inventory |

## Acceptance criteria

1. Written recipe produces a keypair and a `private.h` that verifies a
   test-signed `countries.txt`.
2. `configure.sh` path for developers does not contact CoMaps map hosts.
3. Example/template committed; secrets not committed.
4. Maintainer decides acceptance.

## Required automated tests

- Signature round-trip with a throwaway key in tests (may already exist via
  `maps_generator.utils.file` / storage tests). Add coverage if missing.
- Configure script: unit or shell test that the CoMaps `MAPS_BASE_URL` is
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
| Branch | — |
| Implemented by | — |
| Accepted by | — |

## Discovered follow-up

| Finding | Disposition |
| --- | --- |
| Public origin URL / TLS | SP-102 |
| First signed FI countries | SP-103 |
