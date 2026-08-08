# SP-047 — `.spa` full-refetch on map update and delete-with-map lifecycle

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** Accepted
**Accepted by:** Maintainer
**Accepted date:** 2026-08-08
**Branch:** `cursor/sp-042-sidecar-shipping-fe62`
**Depends on:** SP-042 Accepted (**SPD-029**, **SPD-030** Accepted); SP-046 Accepted
  (`MaybeEnqueueRemoteSpa` after Map/Diff; SyncWithDisk Spa; fail-soft)
**Unblocks:** SP-048 (incomplete / retry signaling beside lifecycle)

---

## Objective

Delete leaf `.spa` with the map (**SPD-030**) and full-refetch advertised `.spa`
on map / `dataVersion` update (**SPD-029**). Personal `.pix` / `.pixr` / `.spx`
keep existing SPD-016 / SP-030 sparse rules.

---

## Motivation

SP-046 couples Map download to advertised Spa but left orphan `.spa` after
`DeleteCountry` / obsolete-version cleanup, and could keep a stale Spa OnDisk
across same-version Map/Diff replace because `MaybeEnqueueRemoteSpa` skipped
when Spa was already present.

---

## In-scope behavior

1. **`DeleteFromDiskWithIndexes(Map)`** also removes beside-MWM `.spa`
   (`Platform::RemoveFileIfExists`) so deferred `DeleteCustomCountryVersion`
   copies that never SyncWithDisk'd after Spa download still clean up.
2. Do **not** delete `.pix` / `.pixr` / `.spx` as part of map/spa cleanup.
3. **Obsolete / empty version dirs:** `FindAllLocalMapsInDirectoryAndCleanup`
   removes `.spa` in old version directories and orphan `.spa` when no MWM remains
   so `RmDir` can succeed.
4. **Update refetch:** on successful Map or Diff register, drop any existing
   `.spa` then `MaybeEnqueueRemoteSpa` (full refetch; no spa-diffs).
5. Tests covering delete-with-map, personal-file retention, Map redownload
   refetch, and obsolete-version spa removal.
6. Work item + README (this file).

---

## Out-of-scope behavior

- Full incomplete / retry UX (**SP-048**).
- Product spec / technical audit edits.
- Marking this WI Accepted; push (per task instructions).

---

## Relevant product requirements / decisions

- **SPD-029** — no spa-diffs in V1; full `.spa` on map / dataVersion update.
- **SPD-030** — delete `.spa` with the map; personal files unchanged.
- **SPD-016** / SP-018 — explored archive survives map delete.
- **SPD-027** / **SPD-031** — advertise + fail-soft (SP-046; unchanged here).

---

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `libs/storage/storage.cpp` | `DeleteFromDiskWithIndexes`; Map/Diff success → drop spa + enqueue |
| `libs/platform/local_country_file_utils.cpp` | Obsolete/orphan `.spa` cleanup |
| `libs/storage/storage_tests/spa_download_tests.cpp` | `Storage_SpaLifecycle_*` |
| `libs/platform/platform_tests/local_country_file_tests.cpp` | Comment: Map-only `DeleteFromDisk` |

---

## Acceptance criteria

1. `DeleteCountry(Map)` removes leaf `.spa`; planted `.pix` / `.pixr` / `.spx`
   remain.
2. Same-version Map redownload drops stale spa and re-fetches advertised spa.
3. Obsolete version dir spa is removed when maps are registered / cleaned.
4. Existing `Storage_SpaDownload_*` cases still pass.
5. Work item Status **In review**; README indexes SP-047.

---

## Test plan

| Case | Expect |
| --- | --- |
| `Storage_SpaLifecycle_DeleteCountryRemovesSpaKeepsPersonal` | spa gone; pix/pixr/spx kept |
| `Storage_SpaLifecycle_MapRedownloadRefetchesSpa` | stale marker gone; spa size = advertised |
| `Storage_SpaLifecycle_ObsoleteVersionRemovesSpa` | old version spa removed |
| Prior `Storage_SpaDownload_*` | still OK |

Build: `./tools/unix/build_omim.sh -d -p /workspace storage_tests`.
Run: `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "Storage_SpaDownload|Storage_SpaLifecycle"`.

---

## Discovered follow-ups

| Item | Owner |
| --- | --- |
| Incomplete / retry signaling when advertised spa missing after fail-soft | **SP-048** (Accepted 2026-08-08) |
| `LocalCountryFile::DeleteFromDisk(Map)` remains Map-only (storage lifecycle owns Spa) | intentional; document only |

---

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `83c0f5ea7` `[storage] Delete .spa with map and refetch on update`; docs commit `[docs] Record SP-047 spa lifecycle work item evidence` (this file) |
| Decision ids | SPD-029, SPD-030 (implements) |
| Test output | `./tools/unix/build_omim.sh -d -p /workspace storage_tests` OK; `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "Storage_SpaDownload\|Storage_SpaLifecycle"` — all seven SpaDownload/SpaLifecycle cases OK; `3 / 3 passed`. |
| Docs touched | this file; README; phase-04 residual note |
| Implemented by | Cursor Agent |
| Accepted by | Maintainer |
| Accepted date | 2026-08-08 |

### Implementation notes (2026-08-07)

- Spa delete uses `RemoveFileIfExists(GetPath(Spa))` beside Map delete so
  Framework deferred deregister (`DeleteCustomCountryVersion`) cannot leave
  orphans when the `LocalCountryFile` copy never saw Spa in `m_files`.
- Map/Diff register success always clears Spa before `MaybeEnqueueRemoteSpa`
  so Diff-in-place and same-version redownload cannot keep stale geometry.
- Personal artifacts are not under `MapFileType` delete for this path; tests
  plant WritableDir `.pix` / `.pixr` / `.spx` and assert survival.
