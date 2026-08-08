# Implementation plan — production `.spa` download on the local network

**Status:** Plans only (2026-08-08). Awaiting maintainer approval before coding
(roadmap §8 steps 1–2).  
**Track:** Phase 4 residual continuation (SP-049–053).  
**Not:** Phase 5 feature work; not a Phase 5 exit criterion (**SPD-033**).

---

## Verdict

Client download of advertised `.spa` beside MWM is **already shipped**
(SP-042–048). What is missing is the **production publish tree + HTTP serve +
countries advertisement** so a phone on the LAN can use Advanced → Custom Maps
server and receive `.spa` through that same path. Android scoped storage makes
file copy unsupported as the test method.

Design rule for this track: **one CDN layout; LAN is a mirror; debug is
observability and temporary recipes, never a second protocol.**

---

## Current state (short)

| Layer | State |
| --- | --- |
| Phase 5 UI / focus / completion | SP-033–040 Accepted; SP-041 awaiting exit |
| Dense FI `.spa` emit | `spa_emit_tool` (SP-044) |
| `countries.txt` spa fields | Parse + `inject_spa_meta` (SP-045); **not** in bundled `data/countries.txt` |
| Storage download / lifecycle / retry | SP-046–048 |
| CDN / LAN publish of blobs + spa-bearing countries | **Missing** (SP-048 ops residual) |
| Community map servers | MWM-only (`DEPLOY_OWN_MAP_SERVER.md`) |

Detail:
[`notes/spa-local-download-current-state.md`](notes/spa-local-download-current-state.md).

---

## Architecture (production path)

```text
                    ┌─────────────────────────┐
  rings+policy+.pix │  spa_emit_tool (SP-044) │
  ─────────────────►│  → {leaf}.spa           │
                    └───────────┬─────────────┘
                                │
  countries.txt + MWMs          ▼
                    ┌─────────────────────────┐
                    │ assemble_spa_publish_   │  SP-050
                    │ tree (CDN ≡ LAN layout) │
                    │ meta/maps.json          │
                    │ maps/{series}/{ver}/    │
                    │   countries.txt[+sig]   │
                    │   *.mwm *.spa           │
                    └───────────┬─────────────┘
                                │
              ┌─────────────────┼─────────────────┐
              ▼                                   ▼
     CDN static host                      LAN HTTP server
     (production)                         (SP-051)
              │                                   │
              └─────────────┬─────────────────────┘
                            ▼
              App Custom Maps URL (user-set)
              → MapFilesDownloader servers list
              → Map then Spa (SP-046)
              → incomplete/retry (SP-048)
```

URL contract (already in client):

`{base}/maps/{MAP_SERIES}/{dataVersion}/{UrlEncode(file)}`  
`{base}/meta/maps.json`

---

## Work items (implement in order)

| # | ID | Deliverable | Coding? |
| --- | --- | --- | --- |
| 1 | [SP-049](work-items/SP-049-spa-distribute-layout-decisions.md) | Lock D8–D13 → SPD-035+ | Docs only |
| 2 | [SP-050](work-items/SP-050-spa-publish-tree-assemble.md) | Assemble tool + verify | Python tool |
| 3 | [SP-051](work-items/SP-051-local-map-server-spa.md) | LAN static server + health/logs | Python tool |
| 4 | [SP-052](work-items/SP-052-spa-countries-advertise-path.md) | Signed countries path; temporary non-merged inject recipe; **no sig bypass** | Docs ± small client if same-version refresh needed |
| 5 | [SP-053](work-items/SP-053-spa-lan-device-validation.md) | Device evidence gate | Evidence |

---

## Production vs debugging

| Concern | Production | Debugging support |
| --- | --- | --- |
| Layout | D8 tree | Same tree; `--verify-only` / inventory |
| Serve | Any static host / CDN | SP-051 with access logs + opt-in `/debug/*` |
| Advertise spa | Signed countries on CDN | Channel A signed LAN; Channel B local APK inject **not merged** |
| Client | SP-046–048 only | Extra logs / incomplete list; no alternate download API |
| Custom server URL | User Advanced setting | Never a build default |
| Failure | MWM usable; areas fail-closed; IncompleteSpa | Same; retry after fix |

Explicitly rejected: unsigned countries apply on custom server; ADB push as
supported ingress; debug JNI install-spa; second URL scheme for `.spa`.

---

## Advertisement blocker (read carefully)

`HasRemoteSpa()` is false today for all leaves in bundled countries. Without
advertisement, a LAN server full of `.spa` files is ignored.

**Preferred:** signed countries (version bump when meta-only) on the LAN/CDN
tree (SP-052 Channel A).  
**Temporary:** inject spa into a **local** `data/countries.txt` for an APK
rebuild; do not land that inject on `street-pixels` until CDN serves blobs.

Countries updates always verify Ed25519 (SP-049 D10).

---

## Device recipe (target after implementation)

1. Emit FI dense `.spa` (`spa_emit_tool`).
2. Assemble publish tree (SP-050) — full or `--spa-only`.
3. Serve on LAN (SP-051); note printed URL.
4. Apply advertisement (SP-052 Channel A or B).
5. Phone: Custom Maps server = that URL; download or retry Helsinki.
6. Confirm areas / Phase 5 UI (SP-053 → Phase 10 walks).

---

## Open locks for maintainer (SP-049)

Approve or amend D8–D13 before SP-050 coding:

- Single CDN≡LAN layout
- Advertisement = countries fields only
- Keep signature verification on custom servers
- Temporary bundle inject allowed but not merged early
- Custom URL never build-default
- Track stays Phase 4 residual / device enabler

---

## What this does not change

- Phase 5 exit criteria or SP-041 acceptance
- Product spec / technical audit
- Privacy invariants (no GPS upload; collection only in session)
- Making `.spa` mandatory for map install (**SPD-027** / **SPD-031**)
