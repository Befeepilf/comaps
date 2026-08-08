# Current state — shipping `.spa` onto device for Phase 5 testing

**Authored:** 2026-08-08  
**Scope:** Why Phase 5 device walks need a LAN download path, and what already
exists vs what is still missing.  
**Authority:** Snapshot of the working tree and accepted SPDs / work items. Not
a product decision.

---

## 1. Why this exists

Phase 5 (area progress / map interaction) is functionally implemented
(SP-033–040 Accepted; SP-041 evidence recorded, awaiting maintainer exit).
Device walks (Helsinki badge / focus / tap / city / completed / no-area) are
residualled to Phase 10 because the on-device area pipeline needs the leaf
`.spa` sidecar beside the MWM.

Android scoped / app-private storage blocks a reliable `adb push` of
`{mwmLeafId}.spa` into the map directory the storage layer owns. The supported
ingress path is the **same production download path** already implemented for
CDN packaging (SP-046): advertise in `countries.txt`, fetch `.spa` beside the
leaf MWM over HTTP.

Therefore local-network testing must use a **production-shaped publish tree**
served over HTTP on the LAN, pointed at via Advanced → Custom Maps server —
not a one-off debug copy hack.

---

## 2. What is already done (do not re-implement)

| Piece | Status | Notes |
| --- | --- | --- |
| Blob contract `format_version` 2 | **SPD-034** / SP-043 Accepted | `nside=1048576`, AscendingNest |
| Dense FI leaf emit | **SP-044** Accepted | `tools/spa_emit_tool` `--mode=production` |
| Optional `spa` / `spa_sha1_base64` parse + inject | **SP-045** Accepted | `inject_spa_meta.py`; `HasRemoteSpa()` |
| Client Map→Spa download | **SP-046** Accepted | `MapFileType::Spa`; fail-soft MWM |
| Update full-refetch + delete-with-map | **SP-047** Accepted | **SPD-029**, **SPD-030** |
| Incomplete / retry | **SP-048** Accepted | **SPD-031**; settings-backed set |
| Custom map server URL | Upstream CoMaps | `pref_custom_map_download_url` → `SetCustomMapServerUrl`; skips metaserver; cleartext permitted |
| Download URL shape | Existing | `{server}/maps/{MAP_SERIES}/{dataVersion}/{UrlEncode(file)}` |
| Meta update shape | Existing | `{server}/meta/maps.json` then countries + `.sig` |

Decisions: **SPD-027–034**. Packaging track SP-042–048 is Accepted and is
**not** a Phase 5 exit gate (**SPD-033**).

---

## 3. What is still missing (the gap)

SP-048 explicitly residualled **"CDN publish of production `countries.txt` with
spa fields"** to ops. There is **no** in-repo tool that:

1. Assembles a CDN-identical directory for a chosen `dataVersion` /
   `MAP_SERIES` containing leaf `.mwm`, leaf `.spa`, `countries.txt` (with spa
   meta), `countries.txt.sig` (when signing key available), and `meta/maps.json`.
2. Serves that directory over HTTP on the LAN with the exact URL prefixes the
   client already requests.
3. Documents the end-to-end device recipe for Phase 5 / Phase 10 Helsinki walks
   without scoped-storage file copies.

Community servers (`comaps-map-distributor`, `comaps-server` in
`docs/DEPLOY_OWN_MAP_SERVER.md`) mirror official CDN MWMs only — they do not
know about `.spa` or spa meta.

Bundled `data/countries.txt` (`v` = `260714`, `map_series` = `2026.06.28`) has
**no** `"spa"` / `"spa_sha1_base64"` fields on Finland leaves. Without
advertisement, SP-046 never queues Spa even if blobs sit on a custom server.

Countries **updates** require Ed25519 signature verification against
`COUNTRIES_TXT_SIGNATURE_HEX` (`private.h`). An unsigned LAN `countries.txt`
will not be applied by the production update path. Same-version
`maps.json` `latest` is also skipped — Channel A must bump `"v"` / `latest`
(SP-049 D10).

**Upstream of assemble:** dense FI `.spa` emit needs leaf `.pix` for universe
**U** (SP-044 residual). This distribute track does not implement mapgen
Option A or `.pix` derive.

---

## 4. Production-first principle

| Prefer | Avoid |
| --- | --- |
| One publish layout used by CDN and LAN | A second “debug download” protocol |
| Real `HasRemoteSpa` → Map then Spa queue | Sideloading into WritableDir via ADB |
| Fail-soft / incomplete / retry already in SP-046/048 | Making `.spa` mandatory for map install |
| Debug = observability on the production path | Debug = alternate client code paths in release |
| Custom server URL stays user-set, never a build default | Hardcoding LAN IPs (android.mdc) |

Debug support means: request logging, health/listing, verify CLI, incomplete-spa
visibility, and documented failure modes — layered on the production layout and
client code.

---

## 5. Follow-on work items

Overview plan:
[`spa-local-download-implementation-plan.md`](spa-local-download-implementation-plan.md).

| ID | Title |
| --- | --- |
| [SP-049](../work-items/SP-049-spa-distribute-layout-decisions.md) | Publish-layout + LAN advertisement decisions |
| [SP-050](../work-items/SP-050-spa-publish-tree-assemble.md) | Assemble CDN-identical publish tree (incl. `.spa`) |
| [SP-051](../work-items/SP-051-local-map-server-spa.md) | Local-network HTTP server for that layout |
| [SP-052](../work-items/SP-052-spa-countries-advertise-path.md) | Countries advertisement path (signed update vs temporary bundle inject) |
| [SP-053](../work-items/SP-053-spa-lan-device-validation.md) | Device validation playbook — download `.spa` via app on LAN |

Implementation does **not** start until the maintainer approves these plans
(roadmap §8 steps 1–2).
