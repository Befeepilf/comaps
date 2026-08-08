# SP-048 — Sidecar shipping validation and incomplete / retry signaling

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** Accepted
**Accepted by:** Maintainer
**Accepted date:** 2026-08-08
**Branch:** `cursor/sp-042-sidecar-shipping-fe62`
**Depends on:** SP-042 Accepted (**SPD-031** Accepted); SP-046 Accepted (fail-soft);
  SP-047 Accepted (lifecycle)
**Unblocks:** Phase 4 packaging track close (maintainer acceptance of SP-042–048)

---

## Objective

When an advertised `.spa` fails after Map is OnDisk, record **incomplete** state
that can be observed and **retry** without re-downloading the MWM (**SPD-031**).
Keep areas fail-closed; do not invent rings or block map use. Storage-level API
+ log is enough — Android toast/dialog chrome is residual.

---

## Motivation

SP-046 fail-soft kept the MWM but only logged a stub. Callers (startup /
settings) need a durable incomplete set and a retry enqueue path so missing
advertised spa can be recovered without a full map redownload.

---

## In-scope behavior

1. **Incomplete signaling:** on advertised Spa download / register failure with
   Map OnDisk, `MarkSpaIncomplete(id)` persists a settings-backed set
   (`IncompleteSpa`, semicolon-separated country ids). Observable via
   `IsSpaIncomplete` / `GetIncompleteSpaCountries`.
2. **Clear on success:** successful Spa register calls `ClearSpaIncomplete`.
   Map delete also clears.
3. **Missing meta never incomplete:** `MarkSpaIncomplete` no-ops when
   `!HasRemoteSpa()`; retry reconcile drops stale / non-advertised entries.
4. **Retry API:** `RetryIncompleteSpaDownloads()` reconciles OnDisk maps that
   advertise spa but lack Spa on disk into the incomplete set, then
   `MaybeEnqueueRemoteSpa` without re-downloading MWM.
5. **Auto-retry hook:** `RestoreDownloadQueue` (Framework startup) calls
   `RetryIncompleteSpaDownloads` even when the download queue is empty.
6. **Tests:** fail-soft → marked; retry → spa + cleared; missing meta → never
   incomplete; restore auto-retry; fail-closed precondition (no spa path) +
   existing `ExplorationSidecar_MissingIsEmptySafe`.
7. Work item + README + phase-04 residual note (this file).

---

## Out-of-scope behavior

- Full Android polish UX (toast / dialog) — residual Phase 10 / follow-up.
- CDN production publish of real FI `countries.txt` with spa fields (ops).
- Product spec / technical audit edits.
- Marking this WI Accepted; push of packaging track acceptance.

---

## Relevant product requirements / decisions

- **SPD-031** — advertised download failure keeps MWM; areas fail-closed;
  prefer retry and incomplete / unavailable signaling.
- **SPD-027** — omitted meta → no fetch (never incomplete).
- **SPD-020** — missing sidecar → no exploration areas.

---

## Relevant source files or symbols

| Path | Role |
| --- | --- |
| `libs/storage/storage.hpp` / `.cpp` | incomplete set, Mark/Clear/Is/Get/Retry; RestoreDownloadQueue hook |
| `libs/storage/storage_tests/spa_download_tests.cpp` | `Storage_SpaIncomplete_*` |
| `libs/street_pixels_areas/.../exploration_sidecar_tests.cpp` | `ExplorationSidecar_MissingIsEmptySafe` (fail-closed) |

---

## Acceptance criteria

1. Fail-soft Spa after Map OnDisk marks incomplete; Map stays OnDisk / not failed.
2. `RetryIncompleteSpaDownloads` downloads spa and clears incomplete.
3. No-advertise leaf never becomes incomplete (including after retry).
4. Missing spa path remains fail-closed for area load (no invented areas).
5. Work item Status **In review**; README + phase-04 residual updated.
6. Prior `Storage_SpaDownload_*` / `Storage_SpaLifecycle_*` still pass.

---

## Test plan

| Case | Expect |
| --- | --- |
| `Storage_SpaIncomplete_FailSoftMarksIncomplete` | incomplete marked; Map OnDisk; no spa file |
| `Storage_SpaIncomplete_RetryClearsAfterSpaDownload` | spa OnDisk; incomplete cleared |
| `Storage_SpaIncomplete_MissingMetaNeverIncomplete` | never incomplete |
| `Storage_SpaIncomplete_RestoreQueueAutoRetries` | RestoreDownloadQueue recovers spa |
| `ExplorationSidecar_MissingIsEmptySafe` | TryLoad → Missing, empty areas |
| Prior SpaDownload / SpaLifecycle | still OK |

Build: `./tools/unix/build_omim.sh -d -p /workspace storage_tests`.
Run: `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "Storage_SpaDownload|Storage_SpaLifecycle|Storage_SpaIncomplete"`.

Optional fail-closed: `-f ExplorationSidecar_MissingIsEmptySafe` on `street_pixels_areas_tests`.

---

## Discovered follow-ups

| Item | Owner |
| --- | --- |
| Android / settings chrome observing `IsSpaIncomplete` (toast/dialog) | Phase 10 / UX follow-up |
| CDN publish of production `countries.txt` with spa fields | ops → **SP-049–053 Planned** (LAN/CDN publish mirror) |
| Packaging track SP-042–048 maintainer acceptance | maintainer |

---

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `3263f5cfe` `[storage] Incomplete spa signaling and retry enqueue`; docs commit `[docs] Record SP-048 incomplete/retry signaling evidence` (this file) |
| Decision ids | SPD-031 (implements signaling); SPD-027 (no-advertise) |
| Test output | `./tools/unix/build_omim.sh -d -p /workspace storage_tests` OK; `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "Storage_SpaDownload\|Storage_SpaLifecycle\|Storage_SpaIncomplete"` — all eleven Spa* cases OK; `3 / 3 passed`. Fail-closed: `-f ExplorationSidecar_MissingIsEmptySafe` OK. |
| Docs touched | this file; README; phase-04 residual note; SP-046/047 follow-up pointers |
| Implemented by | Cursor Agent |
| Accepted by | Maintainer |
| Accepted date | 2026-08-08 |

### Implementation notes (2026-08-07)

- Incomplete set key: `IncompleteSpa` (semicolon-separated), mirror of
  `DownloadQueue` settings pattern.
- `RetryIncompleteSpaDownloads` reconciles disk state so upgrades from
  SP-046 log-only builds still recover without a prior Mark.
- `RestoreDownloadQueue` always calls retry (including empty queue) so
  Framework startup recovers incomplete advertised spa.
- UI chrome intentionally omitted; Storage API + LOG is the SPD-031 signal.
