# SP-046 — Client leaf download fetches advertised `.spa` beside MWM

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** In review
**Branch:** `cursor/sp-042-sidecar-shipping-fe62`
**Depends on:** SP-042 In review (**SPD-027**, **SPD-031** Accepted); SP-045 In review
  (`HasRemoteSpa` / size / hash advertisement)
**Unblocks:** SP-047 (lifecycle beside download); SP-048 (incomplete / retry signaling)

---

## Objective

When a leaf map downloads or updates, the client **also fetches** the leaf
`.spa` **if and only if** `countries.txt` advertises one (`HasRemoteSpa()`).
Sequential download (queue is per `CountryId`): Spa after Map. Failures keep
the MWM usable (**SPD-031**). Progress / subtree size include advertised spa
bytes.

---

## Motivation

SP-045 exposes advertisement meta. Production shipping needs the storage
download path to couple Map + advertised Spa without making Spa mandatory for
map install (**SPD-027** / **SPD-031**).

---

## In-scope behavior

1. **`MapFileType::Spa`** before `Count`; `DebugPrint`; `GetFileName` → `.spa`.
2. **`SyncWithDisk`:** keep Diff/Map exclusive; detect Spa independently.
3. After successful Map register (and Diff Ok → Map), **`MaybeEnqueueRemoteSpa(id)`**
   if `HasRemoteSpa()` and spa not already on disk.
4. Sequential download — Spa after Map (queue per CountryId).
5. **`OnDownloadFinished`:** use `GetSpaSha1()` for Spa integrity.
6. **`RegisterDownloadedFiles` Spa arm:** rename to `GetPath(Spa)`; SyncWithDisk;
   on success refresh (`m_didDownload`); on failure **fail-soft** if Map OnDisk
   (do not `OnMapDownloadFailed` for the leaf map); incomplete stub/log for SP-048.
7. **`CountryStatusEx`:** if Map OnDisk at current version and only Spa is
   queued/downloading → report **OnDisk** (do not demote usable map).
8. Progress / download sizes: include `GetRemoteSpaSize()` when advertised
   (`QueuedCountry` Spa size; `CountrySizeInBytes` / `CalculateProgress` via
   `GetRemoteDownloadSize`; fold into `GetSubtreeMwmSizeBytes`).
9. **`GetFilePathByUrl`:** detect `.spa` → Spa.
10. Work item + README (this file).

---

## Out-of-scope behavior

- DeleteCountry Spa deletion / full-refetch lifecycle (**SP-047**).
- Full incomplete / retry UX (**SP-048**) — log stub only.
- Product spec / technical audit edits.
- Marking this WI Accepted; push (per task instructions).

---

## Relevant product requirements / decisions

- **SPD-027** — couple leaf download to advertised `.spa`; omit → no fetch.
- **SPD-031** — advertised download failure keeps MWM; areas fail-closed.
- **SPD-028** — spa size/hash meta (SP-045); size UI may include spa when present.

---

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `libs/platform/country_defines.hpp` / `.cpp` | `MapFileType::Spa` |
| `libs/platform/country_file.cpp` | `GetFileName` → `.spa` |
| `libs/platform/local_country_file.cpp` | `SyncWithDisk` Spa independent |
| `libs/platform/downloader_utils.cpp` | `GetFilePathByUrl` `.spa` |
| `libs/storage/queued_country.cpp` | Spa `GetDownloadSize` |
| `libs/storage/storage_helpers.hpp` / `.cpp` | `GetRemoteDownloadSize` |
| `libs/storage/storage.hpp` / `.cpp` | enqueue, SHA, register, status, progress |
| `libs/storage/country_tree.cpp` | fold spa into `GetSubtreeMwmSizeBytes` |
| `libs/platform/platform_tests/*` | GetFileName / SyncWithDisk / GetFilePathByUrl |
| `libs/storage/storage_tests/spa_download_tests.cpp` | advertise / no-advertise / fail-soft / sizes |
| `libs/storage/storage_tests/country_tree_spa_meta_tests.cpp` | subtree size with spa |

---

## Acceptance criteria

1. Advertised leaf: Map then Spa both land on disk after `DownloadCountry(Map)`.
2. No advertise: Spa never queued; Map alone OnDisk.
3. Spa download fail with Map OnDisk: Map remains registered; not DownloadFailed.
4. `CountryStatusEx` stays OnDisk while Spa-only download runs after Map.
5. Progress / `CountrySizeInBytes` / subtree size include spa when advertised.
6. Platform path helpers and SyncWithDisk cover Spa.
7. Affected platform/storage tests pass (executed output recorded).

---

## Test plan

| Case | Expect |
| --- | --- |
| `GetFileName` / `GetPath` Spa | `.spa` extension |
| `SyncWithDisk` Spa | independent of Diff/Map |
| `GetFilePathByUrl` `.spa` | `MapFileType::Spa` ready path |
| Fake downloader + advertise | Map then Spa both OnDisk |
| No advertise | Spa never OnDisk |
| Spa download denied after Map | Map OnDisk; not failed country |
| Restore queue with Map OnDisk, Spa missing | Advertised Spa enqueued/downloaded; Map stays OnDisk |
| `CountrySizeInBytes` / subtree | map + spa when advertised |

Build: `./tools/unix/build_omim.sh -d storage_tests platform_tests` (or filtered
targets). Run: `./tools/unix/run_tests.sh -b … -f "CountryFile_Smoke|LocalCountryFile_|Downloader_GetFilePathByUrl|CountryTree_SpaMeta|Storage_SpaDownload"`.

---

## Discovered follow-ups

| Item | Owner |
| --- | --- |
| DeleteCountry / update full-refetch deletes or replaces `.spa` | **SP-047** |
| Incomplete / retry UX when advertised spa missing after fail-soft | **SP-048** |
| `DeleteFromDisk(Spa)` not called from Map delete — orphan `.spa` possible until SP-047 | **SP-047** |
| Spa-only progress while Map already local (re-fetch) does not add map bytes to downloaded offset when Map type was not just downloaded in-session | residual / low priority |

---

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `f7b637a25` `[platform] Add MapFileType::Spa and path sync`; `837954e2f` `[storage] Download advertised .spa beside leaf MWM`; `3af50a455` `[docs] Record SP-046 spa download work item evidence`; `7e4384355` `[storage] Resume advertised .spa enqueue after Map OnDisk` |
| Decision ids | SPD-027, SPD-031 (implements); SPD-028 size fold-in |
| Test output | Re-run after restore-queue fix: `./tools/unix/build_omim.sh -d -p /workspace storage_tests` OK; `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "Storage_SpaDownload"` — `Storage_SpaDownload_AdvertisedMapThenSpa`, `Storage_SpaDownload_NoAdvertiseNeverQueuesSpa`, `Storage_SpaDownload_FailKeepsMap`, `Storage_SpaDownload_RestoreQueueEnqueuesSpaWhenMapOnDisk` all OK; `3 / 3 passed`. |
| Docs touched | this file; README |
| Implemented by | Cursor Agent |
| Accepted by | — |
| Accepted date | — |

### Implementation notes (2026-08-07)

- `MaybeEnqueueRemoteSpa` after successful Map / Diff register only; starts pending
  downloads immediately so Spa is not stranded behind a fresh countries check.
- Spa register / download failure with Map OnDisk logs incomplete stub for SP-048
  and does **not** call `OnMapDownloadFailed`.
- `GetRemoteSize()` / MWM `"s"` semantics unchanged; download estimates use
  `GetRemoteDownloadSize` (MWM + advertised spa).
- `GetSubtreeMwmSizeBytes` folds advertised spa bytes (Android `totalSize`).
- `CountryStatusEx` reports OnDisk while only Spa is queued/downloading after Map.
- `DeleteCountry` Spa cleanup remains **SP-047**.
- **Review fix:** `RestoreDownloadQueue` calls `MaybeEnqueueRemoteSpa` when the leaf
  Map is already OnDisk at `m_currentVersion` (Spa-only pending queue was stranded
  by `DownloadNode`'s OnDisk early-return). `DownloadNode` OnDisk path also resumes
  advertised Spa for leaves so other callers cannot strand (**SPD-027**).
