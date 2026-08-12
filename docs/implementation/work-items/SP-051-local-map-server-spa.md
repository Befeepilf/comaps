# SP-051 — Local-network HTTP map server for `.spa` publish tree

**Phase:** 4 residual / pre-production packaging (LAN mirror of production CDN)
**Status:** In review
**Depends on:** SP-049 Accepted (D8, D12); SP-050 (tree to serve; In review)
**Unblocks:** SP-052 (end-to-end advertise+fetch), SP-053 (device walks)

---

## Objective

Provide a **production-compatible** static HTTP server that serves an SP-050
publish tree on the local network so Android devices can download leaf `.mwm`
and `.spa` via the existing Custom Maps server setting and SP-046 client path.

Debug support is observability (access logs, health, inventory), not a second
protocol.

---

## Motivation

`docs/DEPLOY_OWN_MAP_SERVER.md` points at community tools that only know MWMs.
Street Pixels needs `.spa` beside MWMs under the exact path the client requests.
Reusing a tiny in-repo server (or a documented `python -m http.server` recipe
bound to the publish root) keeps the mirror identical to CDN layout and
reviewable in this repository.

Android already permits cleartext (`network_security_config` base-config
`cleartextTrafficPermitted="true"`), and Custom Map Server accepts `http://`
and `https://`. LAN testing can use `http://{lan-ip}:{port}/`.

---

## In-scope behavior

### 1. Server

**Recommended primary:** small Python 3 stdlib (or tornado, already used by
`tools/python/test_server`) static file server:

- CLI: `tools/python/street_pixels/serve_spa_publish_tree.py`
- Args: `--root` (SP-050 `--out`), `--host 0.0.0.0`, `--port 8080`,
  `--log-access` (default on), `--bind-lan-only` optional helper that prints
  suggested Custom Maps URL including detected LAN IPv4.

**Behaviour:**

- Document root = publish root (so URLs are
  `{base}/maps/...` and `{base}/meta/maps.json`).
- GET only; no upload; no directory write.
- Correct `Content-Type` for binary (`.mwm`, `.spa`, `.sig`) —
  `application/octet-stream`; `countries.txt` / `maps.json` as
  `application/json` or `text/plain`.
- Optional gzip **off** by default (client downloader expects raw bytes for SHA).
- Range / resume: CoMaps downloaders often use ranged GETs for large MWMs —
  **verify during implementation** against `HttpMapFilesDownloader` / Android
  HTTP stack. If ranges are required, use a server that supports them (or
  implement Range); spa files are small enough that full GET is fine, but MWM
  downloads on the same server must not regress.

### 2. Debug / production observability

| Feature | Purpose |
| --- | --- |
| Access log: method, path, status, bytes, duration | Diagnose 404 from wrong series/version |
| `GET /health` → `200` + JSON `{ok, map_series, data_version, spa_leaf_count}` | Phone/browser sanity without listing secrets |
| `GET /debug/inventory` (optional, **off unless `--enable-debug-routes`**) | Lists advertised spa leaves from on-disk inventory; never enable by default on a shared network |
| Startup banner prints exact Custom Maps URL to paste | Reduces misconfiguration |

Debug routes must be **opt-in**. Default listen is production-static only.

### 3. Integration with Custom Maps server

Document the device steps (also expanded in SP-053):

1. Phone and server on same LAN (or USB reverse with `adb reverse` —
   document both).
2. Advanced → Custom Maps server → `http://{ip}:{port}/` (trailing slash
   normalized by `Framework.normalizeServerUrl`).
3. No build default URL (D12).

### 4. Docs

- Extend `docs/DEPLOY_OWN_MAP_SERVER.md` with a **Street Pixels** section:
  assemble (SP-050) → serve (this WI) → custom URL → download FI leaf.
- Note community Docker images need the same URL layout to grow spa support;
  this in-repo server is the supported Street Pixels path until then.

---

## Out-of-scope behavior

- TLS termination (optional later; HTTP cleartext is enough for trusted LAN).
- Authentication / VPN.
- Replacing CDN or community distributors.
- Client signature bypass.
- Bundling the server into the Android app.

---

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| SP-050 output tree | Document root |
| `libs/storage/map_files_downloader.cpp` | Custom server → `m_serversList` |
| `android/.../CustomMapServerDialog.java` | User URL entry |
| `tools/python/test_server/` | Prior art for local HTTP in-repo |
| `android/.../network_security_config.xml` | Cleartext already allowed |

---

## Acceptance criteria

1. Serving an SP-050 tree, `curl` succeeds for
   `{base}/maps/{series}/{version}/countries.txt` and one `{leaf}.spa`.
2. With Custom Maps URL set, desktop or instrumented storage path can resolve
   MakeUrlList to the LAN host (unit/integration or documented curl parity).
3. Access log shows SPA and MWM GETs distinctly.
4. `/health` works; `/debug/inventory` disabled by default.
5. Docs recipe complete; no private IP as build default.
6. Agent does not mark Accepted.

---

## Test plan

| Case | Expect |
| --- | --- |
| Serve temp tree; GET spa | 200 + exact bytes |
| Wrong path | 404 logged |
| `--enable-debug-routes` off | `/debug/inventory` 404 |
| Startup prints pasteable URL | Matches `--host`/`--port` |

Manual: one phone on LAN downloads Helsinki map (+ spa) after SP-052
advertisement is in place (SP-053).

---

## Implementation notes / constraints

- Bind `0.0.0.0` for real devices; `127.0.0.1` only works with `adb reverse`.
- Document firewall / OS permission for inbound port.
- Do not follow symlinks outside `--root` (path traversal hardening).
- Production-first: any CDN static host serving the same tree is automatically
  compatible; this WI is the LAN instance of that contract.

## Failure and rollback

- If Range requests are required and missing, fix the server — do not change
  client SHA or download semantics.
- If cleartext blocked on a future hardening change, prefer documenting HTTPS
  with a local cert over inventing a custom scheme.

## Discovered follow-up

| Item | Owner |
| --- | --- |
| Countries advertisement channels | SP-052 |
| Device evidence | SP-053 |
| Upstream community distributor spa support | post-V1 / ops |

---

## Implementation evidence (agent — not Accepted)

| Field | Value |
| --- | --- |
| Status | **In review** — human acceptance pending |
| Tool | `tools/python/street_pixels/serve_spa_publish_tree.py` |
| CLI | `python3 -m street_pixels serve_spa_publish_tree --root …` |
| Tests | `tools/python/street_pixels/tests/test_serve_spa_publish_tree.py` (15) |
| Test command | `cd tools/python && python3 -m unittest street_pixels.tests.test_serve_spa_publish_tree -v` |
| Covered | spa GET; Range 206; 404; health; debug off-by-default; path traversal; inventory leak closed; HEAD debug; `%20` leaf |
| Docs | `docs/DEPLOY_OWN_MAP_SERVER.md` Street Pixels section |
| Range | Implemented — Android `ChunkTask` requires 206 for ranged MWM chunks |
| Review fixes (2026-08-08) | Hide `/inventory.json` unless debug route; HEAD `/debug/inventory`; BrokenPipe; startup tree warn; health `ok` when meta present |
