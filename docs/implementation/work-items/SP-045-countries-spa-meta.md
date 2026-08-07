# (this commit)SP-045 — Add optional `spa` / `spa_sha1_base64` leaf fields to `countries.txt` publish

**Phase:** 4 residual / pre-production packaging (not Phase 5; not Phase 10 device)
**Status:** In review
**Branch:** `cursor/sp-042-sidecar-shipping-fe62` (implementation); planning draft was
  `cursor/sp-045-countries-spa-meta-cf0b`
**Depends on:** SP-042 In review (**SPD-028** Accepted); SP-044 In review (real leaf
  `.spa` blobs to advertise); `data/countries.txt` / storage country tree
**Unblocks:** SP-046 (advertisement signal + size/hash for download); SP-048
  packaging checks that meta matches published blobs

---

## Objective

Extend `countries.txt` leaf meta with optional **`"spa"`** (payload bytes) and
**`"spa_sha1_base64"`** (SHA-1 of the leaf `.spa`, base64), parallel to existing
**`"s"`** / **`"sha1_base64"`** for MWMs (**SPD-028**). Parse and expose them on
`platform::CountryFile`. Omit both when the leaf has no sidecar. Presence of
both fields is the advertisement signal for SP-046 (**SPD-027**).

Do **not** implement HTTP download (SP-046) or delete lifecycle (SP-047).

---

## Motivation

Map download already trusts leaf `"s"` / `"sha1_base64"`. Sidecar shipping needs
the same advertisement pattern without forcing every leaf to ship exploration
geometry. Older clients must keep loading `countries.txt` when new keys appear.

---

## Exploration findings (2026-08-07 tree)

### 1. How `countries.txt` is parsed

| Symbol / path | Role |
| --- | --- |
| `COUNTRIES_FILE` (`defines.hpp`) | `"countries.txt"` |
| `Storage::LoadCountriesFile` → `LoadCountriesFromFile` | Bundled (`fr`) vs writable (`w`) pick |
| `LoadCountriesFromBuffer` / `LoadCountriesImpl` | `libs/storage/country_tree.cpp` |
| `LoadGroupImpl` | Per-node JSON walk; builds `CountryTree` |
| `StoreCountries::InsertToCountryTree` | Builds `Country` + `platform::CountryFile` when `mapSize != 0` |
| `storage::Country::GetFile()` | Leaf `CountryFile`; groups keep empty file |

Leaf size/hash parse today (`LoadGroupImpl`):

```350:358:libs/storage/country_tree.cpp
  int nodeSize;
  FromJSONObjectOptionalField(node, "s", nodeSize);
  ASSERT_LESS_OR_EQUAL(0, nodeSize, ());

  string nodeHash;
  FromJSONObjectOptionalField(node, "sha1_base64", nodeHash);

  // We expect that mwm and routing files should be less than 2GB.
  Country * addedNode = store.InsertToCountryTree(id, nodeSize, nodeHash, depth, parent);
```

Subtree attrs: leaf `mwmSize = nodeSize` (`"s"` only); groups sum children into
`Country::m_subtreeMwmSizeBytes` via `SetSubtreeAttrs`.

### 2. How `"s"` / `"sha1_base64"` are stored and used

| Symbol | Role |
| --- | --- |
| `platform::CountryFile` (`libs/platform/country_file.hpp`) | `m_mapSize`, `m_sha1`; `GetRemoteSize()`, `GetSha1()` |
| Ctor `CountryFile(name, size, sha1)` | Set at insert when `mapSize != 0` |
| `Storage::GetCountryFile` / `CountrySizeInBytes` | Remote side = `GetRemoteSize(countryFile)` (MWM only; diffs may override via `storage::GetRemoteSize`) |
| `Storage::GetNodeAttrs` → `NodeAttrs::m_mwmSize` | `GetSubtreeMwmSizeBytes()` (sums `"s"`) |
| Download SHA verify | `Storage` finish path uses `GetCountryFile(id).GetSha1()` vs `coding::SHA1::CalculateBase64(path)` for **MWM** only (`storage.cpp` ~1090) |

`MapFileType` today: `Map`, `Diff`, `Pix` — **no `Spa`** yet (SP-046 adds download
type / path; out of scope here).

### 3. How `countries.txt` is generated / published

| Path | Role |
| --- | --- |
| `tools/python/post_generation/hierarchy_to_countries.py` | Builds tree from hierarchy; `fill_last` sets `"s"` / `"sha1_base64"` from `{name}.mwm` only |
| `CountryDict.order` | `["id", "n", "v", "map_series", "c", "s", "sha1_base64", "rs", "g"]` — no spa keys yet |
| `StageCountriesTxt` (`tools/python/maps_generator/generator/stages_declaration.py`) | Calls `hierarchy_to_countries`, optional `inject_promo_ids`, writes `env.paths.countries_txt_path`, optional `sign_file` |
| Sample leaf | e.g. `Finland_Southern Finland_Helsinki` with `"s"` + `"sha1_base64"` only (`data/countries.txt`) |

SP-044 Option B emits `{mwmLeafId}.spa` into a **publish tree beside** mapgen,
not inside `hierarchy_to_countries`. Prefer a **post-step patch tool** over
rewiring mapgen collectors.

### 4. Android / UI size display

Yes — remote sizes shown in downloader UI come from subtree **`"s"`** sums:

- JNI `MapManager.cpp` sets `CountryItem.totalSize` from `attrs.m_mwmSize`
- UI: `DownloaderAdapter`, `OnmapDownloader`, `PlacePageView`, routing download
  dialogs, etc. format `country.totalSize`

**SP-045 decision (minimal):** do **not** fold `"spa"` into
`m_subtreeMwmSizeBytes`, `GetRemoteSize()`, or Android `totalSize`. Keep MWM
size semantics unchanged. Expose `GetRemoteSpaSize()` / `GetSpaSha1()` for
SP-046 (download queue / progress may add spa bytes then). SPD-028’s “may
include” is deferred to SP-046 or an explicit UI follow-up — record in
discovered-follow-ups.

### 5. Forward-compat (unknown keys)

Parser is **key-whitelist by read**, not schema-strict: jansson
`json_object_get` / `FromJSONObjectOptionalField` only touch requested keys;
unread keys are ignored. An older client that never reads `"spa"` /
`"spa_sha1_base64"` already tolerates them. SP-045 must **verify with a test**
(load buffer containing unknown keys + spa fields; assert MWM fields unchanged
and spa accessors populated only when present).

---

## In-scope behavior

### Client parse / expose

1. Extend `platform::CountryFile` with optional spa size + spa SHA-1 (default
   empty / 0 when absent).
2. Accessors: `GetRemoteSpaSize()`, `GetSpaSha1()`, and a clear advertisement
   helper e.g. `HasRemoteSpa() const` (**both** size present with `> 0` **and**
   non-empty hash — see rules below).
3. `LoadGroupImpl`: `FromJSONObjectOptionalField(node, "spa", …)` and
   `"spa_sha1_base64"`; pass into `InsertToCountryTree` / `CountryFile`.
4. Update `StoreInterface::InsertToCountryTree` and both store classes
   (`StoreCountries`, `StoreFile2Info`) for the new parameters (File2Info may
   ignore spa).
5. Absent fields → spa size 0, empty hash, `HasRemoteSpa() == false`.
6. Do not invent placeholders. Do not change MWM `"s"` / hash behaviour.

### Advertisement / consistency rules (parser)

| Published leaf JSON | Client behaviour |
| --- | --- |
| Both `"spa"` and `"spa_sha1_base64"` present, spa > 0, hash non-empty | `HasRemoteSpa() == true`; expose size/hash |
| Both omitted | `HasRemoteSpa() == false` (normal worldwide uncovered leaf) |
| Only one of the two present, or spa == 0 with hash, or hash empty with spa > 0 | **Fail-closed for advertisement:** treat as no spa (`HasRemoteSpa() == false`); `LOG(LWARNING, …)` once per leaf. Do not partial-advertise. Do not fail the whole `countries.txt` load. |

Rationale: SPD-028 says omit **both**; SP-046 must not fetch on a half-signal.

### Publish-side

Add a small post-generation tool (preferred over deep `maps_generator` changes):

**Recommended path:** `tools/python/post_generation/inject_spa_meta.py`

- CLI via `post_generation inject_spa_meta` (mirror `inject_promo_ids`).
- Inputs: `--countries` (countries.json/txt), `--spa-dir` (directory of
  `{mwmLeafId}.spa`), `--output`.
- Walk leaves (reuse `_get_nodes` pattern from `inject_promo_ids.py`).
- If `{spa_dir}/{id}.spa` exists: set `"spa"` = file size, `"spa_sha1_base64"` =
  base64(SHA-1 digest) — same hashing style as `get_mwm_hash` in
  `hierarchy_to_countries.py`.
- If missing: ensure both keys are **absent** (pop if re-running).
- Extend `CountryDict.order` to include `"spa"`, `"spa_sha1_base64"` after
  `"sha1_base64"` when that helper is used for dumps.
- Optional thin hook in `StageCountriesTxt`: if `env` / settings provides an
  spa publish dir, call inject **before** `sign_file`. If no dir, skip (no
  placeholders). Offline SP-044 publish can run the CLI without full mapgen.

Do **not** require every worldwide leaf to have a `.spa` for publish to succeed.

### Docs / index

- This work-item file; README + phase-04 index → Planned (draft) / link.
- No product-spec or audit edits.

---

## Out-of-scope behavior

- HTTP / queue download of `.spa` (**SP-046**).
- Delete-with-map / full-refetch lifecycle (**SP-047**).
- Incomplete / retry UX (**SP-048**).
- Adding `MapFileType::Spa` or download URL path helpers (SP-046).
- Folding spa bytes into Android `totalSize` / `GetSubtreeMwmSizeBytes` /
  `GetRemoteSize()` (follow-up; see discovered-follow-ups).
- Embedding spa emit into `hierarchy_to_countries` MWM scan (Option A mapgen).
- Editing `docs/STREET_PIXELS_PRODUCT_SPEC.md` or the technical audit.
- Marking this WI Accepted.

---

## Relevant product requirements / decisions

- **SPD-028** — optional `"spa"` / `"spa_sha1_base64"`; omit when no sidecar.
- **SPD-027** — presence of fields = advertisement; omit = no fetch; MWM still
  usable.
- SPD-020 (sidecar grain = MWM leaf); SPD-034 / SP-043 (blob contract); SP-044
  (emit publish tree).

---

## Relevant source files or symbols

### Parse / storage (must change)

| Path | Symbols |
| --- | --- |
| `libs/platform/country_file.hpp` / `.cpp` | `CountryFile`; add `m_spaSize`, `m_spaSha1`; `GetRemoteSpaSize`, `GetSpaSha1`, `HasRemoteSpa`; ctor overload / defaults |
| `libs/platform/platform_tests/country_file_tests.cpp` | Accessor smoke |
| `libs/storage/country_tree.cpp` | `LoadGroupImpl`; `StoreInterface::InsertToCountryTree`; `StoreCountries` / `StoreFile2Info` |
| `libs/storage/country.hpp` | unchanged unless SetFile path needs spa (prefer ctor on `CountryFile`) |
| `libs/storage/storage_tests/storage_tests.cpp` **or** new `country_tree_spa_meta_tests.cpp` | Parse with/without spa; unknown-key forward-compat; partial-field fail-closed |

### Publish (must add / light touch)

| Path | Symbols |
| --- | --- |
| `tools/python/post_generation/inject_spa_meta.py` | **new** — patch leaves from spa dir |
| `tools/python/post_generation/__main__.py` | register `inject_spa_meta` subcommand |
| `tools/python/post_generation/hierarchy_to_countries.py` | `CountryDict.order` add spa keys (key order only; optional if inject dumps plain dict) |
| `tools/python/maps_generator/generator/stages_declaration.py` | Optional `StageCountriesTxt` hook before sign |
| Python unit test under `tools/python/post_generation/` or `tools/python/.../tests/` | inject with/without matching `.spa` |

### Reference only (do not change for this WI)

| Path | Why |
| --- | --- |
| `libs/storage/storage.cpp` | `CountrySizeInBytes`, SHA verify, `GetNodeAttrs` — MWM-only until SP-046 |
| `android/.../MapManager.cpp`, downloader UI | `totalSize` ← `m_mwmSize` |
| `data/countries.txt` | Do not commit production spa ads without a real publish; tests use fixtures |
| `defines.hpp` `COUNTRIES_FILE` | unchanged |

---

## Minimal code-change sketch (implementation; not done in planning)

```cpp
// country_file.hpp — additive
MwmSize GetRemoteSpaSize() const { return m_spaSize; }
std::string const & GetSpaSha1() const { return m_spaSha1; }
bool HasRemoteSpa() const { return m_spaSize > 0 && !m_spaSha1.empty(); }
// private: MwmSize m_spaSize = 0; std::string m_spaSha1;

// LoadGroupImpl — after sha1_base64 read
MwmSize spaSize = 0;
FromJSONObjectOptionalField(node, "spa", spaSize);
string spaHash;
FromJSONObjectOptionalField(node, "spa_sha1_base64", spaHash);
if ((spaSize > 0) != !spaHash.empty()) {
  LOG(LWARNING, ("Inconsistent spa meta for", id, "- ignoring"));
  spaSize = 0;
  spaHash.clear();
}
Country * addedNode = store.InsertToCountryTree(id, nodeSize, nodeHash, spaSize, spaHash, depth, parent);
// StoreCountries: CountryFile{id, mapSize, mapSha1} then set spa fields, or extended ctor
```

Publish CLI sketch:

```text
post_generation inject_spa_meta \
  --countries /path/countries.txt \
  --spa-dir /path/publish_tree \
  --output /path/countries.txt
```

---

## Acceptance criteria

1. Parser loads leaves **without** spa fields; `HasRemoteSpa() == false`;
   `GetRemoteSize()` / `GetSha1()` unchanged.
2. Parser loads leaves **with** both fields; `GetRemoteSpaSize()` /
   `GetSpaSha1()` match JSON; `HasRemoteSpa() == true`.
3. Forward-compat: buffer with unknown extra keys still loads; MWM fields OK.
4. Partial spa meta does not advertise (`HasRemoteSpa() == false`) and does not
   abort countries load.
5. Publish tool sets both fields iff `{id}.spa` exists under `--spa-dir`; omits
   both otherwise; idempotent re-run strips stale spa keys when file removed.
6. Android / subtree MWM size display behaviour unchanged (no spa folded into
   `m_mwmSize`).
7. Automated tests green for platform + storage (+ Python inject unit test).
8. This WI Status **In review** after implementation lands; agent does not mark
   Accepted.

---

## Required automated tests

| Case | Where |
| --- | --- |
| `CountryFile` spa accessors default / set | `country_file_tests.cpp` |
| `LoadCountriesFromBuffer` without spa | storage tests (extend `kCountriesTxt` leaf or dedicated buffer) |
| Same with both spa fields on one leaf | assert size/hash/`HasRemoteSpa` |
| Unknown keys (`"future_key": 1`) alongside normal leaf | load succeeds |
| Partial `"spa"` only / hash only / spa 0 | `HasRemoteSpa() == false` |
| `inject_spa_meta` with temp `.spa` + sample JSON | Python |

Do not weaken existing storage size assertions that sum `"s"` only.

## Required manual validation

- Maintainer skim: SPD-028 field names match publish tool output.
- Optional: run inject against SP-044 FI publish tree + a countries.txt copy;
  spot-check one Finland leaf has spa size ≈ file size.

## Failure and rollback considerations

- Never write placeholder spa size/hash for missing sidecars.
- Never fail countries load solely because spa meta is inconsistent — log and
  clear advertisement.
- If publish inject runs after signing, signatures break — keep inject **before**
  `sign_file` in any StageCountriesTxt hook.
- Do not change `GetRemoteSize()` meaning without an explicit UI/download WI.

## Discovered follow-ups

| Item | Owner |
| --- | --- |
| Include spa bytes in download estimates / Android `totalSize` when advertised | **SP-046** (or tiny follow-up after download wiring) |
| `MapFileType::Spa` + path / URL + SHA verify on spa file | **SP-046** |
| CDN packaging validation that every advertised leaf has a matching blob | **SP-048** |
| Option A: emit spa size/hash inside mapgen beside `.mwm` | post-SP-044 Option A residual |
| Optional `StageCountriesTxt` inject hook when mapgen gains an spa publish-dir setting | follow-up (CLI is sufficient for SP-044 offline publish) |

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-042-sidecar-shipping-fe62` |
| Commits | `e88658b2e` `[platform][storage] Parse optional spa size and sha1 in CountryFile`; `641b31f41` `[tools] Add inject_spa_meta for countries.txt publish`; this docs commit `[docs] Record SP-045 countries spa meta evidence` |
| Decision ids | SPD-028 (implements); SPD-027 (advertisement contract) |
| Test output | `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "CountryFile_Smoke\|CountryTree_SpaMeta"` — `CountryFile_Smoke` OK; six `CountryTree_SpaMeta_*` OK (partial meta logs `Inconsistent spa meta … - ignoring`); `python3 tools/python/post_generation/tests/test_inject_spa_meta.py` — 3/3 OK. Note: full `platform_tests` unity build currently fails on pre-existing `glaze_test.cpp` / `glz::expected`; `CountryFile_Smoke` was validated by temporarily excluding `glaze_test.cpp` from that target’s CMakeLists (restored; not committed). |
| Docs touched | this file; README; phase-04 index |
| Implemented by | Cursor Agent |
| Accepted by | — |
| Accepted date | — |

### Implementation notes (2026-08-07)

- `platform::CountryFile` stores `m_spaSize` / `m_spaSha1` with `GetRemoteSpaSize()`,
  `GetSpaSha1()`, `HasRemoteSpa()`; extended ctor when both map and spa meta are set.
- `LoadGroupImpl` parses optional `"spa"` / `"spa_sha1_base64"`; inconsistent pairs
  clear both and log once per leaf (load continues).
- **Spa bytes are not folded into `GetSubtreeMwmSizeBytes` / Android `totalSize`**
  in this WI — deferred to SP-046 (documented in code comment + discovered-follow-ups).
- Publish: `tools/python/post_generation/inject_spa_meta.py` +
  `post_generation inject_spa_meta` CLI; `CountryDict.order` includes spa keys.
- **Maps_generator `StageCountriesTxt` hook not added** — no existing env spa
  publish-dir setting; CLI covers SP-044 offline publish without inventing new
  mapgen config (low-risk criterion not met).
