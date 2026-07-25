# Street Pixels build baseline

**Branch:** `street-pixels`  
**Work item:** SP-001  
**Recorded:** 2026-07-25  
**Host:** macOS 26.5 (Build 25F71), arm64  

---

## Toolchain versions

| Component | Version / path |
| --- | --- |
| macOS | 26.5 (25F71), arm64 |
| Apple clang | 21.0.0 (clang-2100.1.1.101) |
| Xcode | Not installed; Command Line Tools only (`xcodebuild` unavailable) |
| CMake | 4.4.0 |
| Ninja | 1.13.2 |
| JDK | OpenJDK 21.0.11 (Eclipse Adoptium) |
| Python | 3.14.6 |
| Gradle | 8.14.4 (Kotlin 2.0.21) |
| `ANDROID_HOME` (shell) | `/opt/homebrew/share/android-commandlinetools` |
| Android SDK (used by build) | `/Users/mo/Library/Android/Sdk` (written by `set_up_android.py`) |
| Android NDK (installed) | 28.2.13676358, 29.0.14206865 |
| Android SDK CMake | 3.22.1 |
| Android SDK setup | `./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk` |

---

## Build fixes applied (after initial recording)

Three commits on `street-pixels` unblock compilation:

| Commit | Change |
| --- | --- |
| `1472774241` | `[platform]` add `#include <cstddef>` to `vibration.hpp` |
| `f6c7b22333` | `[android]` close `populateIncomingRows()` before `maybeHandlePendingAddFriend()` |
| `8624015f7c` | `[cmake]` healpix libsharp host flags, disable OpenMP on Apple, skip cfitsio UTILS |

**Desktop environment note:** If Android SDK `cmake` 3.22.1 is on `PATH`, desktop
configure fails (`Unknown CMake command "block"`). Use:

```bash
export CMAKE=/opt/homebrew/bin/cmake
export PATH="/opt/homebrew/bin:$PATH"   # ensure SDK cmake is not first
```

---

## Command sequence and results

### 1. Submodule init

```bash
git submodule update --init --recursive --depth 1
```

**Result:** Exit 0 after ~2 minutes.  
**Issue:** Nested submodule `3party/protobuf/protobuf` arrived with all files staged as
deleted (empty tree). Required manual recovery before any C++ build:

```bash
cd 3party/protobuf/protobuf && git reset --hard HEAD && git clean -fd
```

### 2. Configure

```bash
./configure.sh
```

**First attempt (documented path, no env vars):** Exit 0 wall-clock ~130 s, but world-map
download failed:

```
Downloading world map...
--2026-07-25 02:26:10--  https://mapgen-fi-1.comaps.app/maps/260603/World.mwm
...
2026-07-25 02:26:11 ERROR 404: Not Found.
ln: World.mwm: File exists
```

`data/countries.txt` references map version `260603`; that version is not on the CDN
(`curl -sI` returns 404 for 260603, 200 for 251123).

**Second attempt (`./tools/unix/build_omim.sh -d`, which re-runs configure):** Exit 1 in
3.3 s — same 404 + `ln: World.mwm: File exists`, build aborted (`set -e`).

**Workaround used for subsequent steps:** Place map files under
`data/world_mwm/260603/` (copied from a checkout that already had `251123` maps) and set
`SKIP_MAP_DOWNLOAD=1` for build invocations. Full `./configure.sh` (without skip) then
completed in ~72 s (exit 0) after protobuf submodule reset.

**Root cause (affects any fresh clone today):** `configure.sh` lines 111–112 parse as
`(wget … && rm -f World.mwm); ln -s …` — `ln` runs even when `wget` fails; if
`data/World.mwm` already exists, `ln` errors and aborts configure.

### 3. Desktop debug build

```bash
export SKIP_MAP_DOWNLOAD=1
export CMAKE=/opt/homebrew/bin/cmake
/usr/bin/time -p ./tools/unix/build_omim.sh -d
```

**Initial attempt (before fixes):** Exit 1, 38.76 s — `vibration.hpp` missing `size_t`.

**After fixes — full `-d` build:** Exit 1. Smoke test binaries build successfully
(see §4); full desktop target set still fails on some non-smoke targets
(`storage_integration_tests` unity-build error observed). Disk space dropped to ~118 MiB
during iteration; freeing `omim-build-debug` intermediates and Android build cache
restored ~3.7 GiB.

**Smoke-target-only build (used for §4):**

```bash
export SKIP_MAP_DOWNLOAD=1 CMAKE=/opt/homebrew/bin/cmake
./tools/unix/build_omim.sh -d base_tests coding_tests generator_tests indexer_tests \
  map_tests mwm_tests platform_tests routing_tests search_tests
```

**Result:** Exit 0, **94.73 s** real.

### 4. Smoke suite

```bash
export CMAKE=/opt/homebrew/bin/cmake
/usr/bin/time -p ./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke
```

**Result:** Exit 1, **220.30 s** real. Script summary: **4 / 9** test binaries passed
entirely.

| Target | Built | Pass/fail | Notes |
| --- | --- | --- | --- |
| base_tests | Yes | **Pass** | `All tests passed.` |
| coding_tests | Yes | **Pass** | `All tests passed.` |
| generator_tests | Yes | **Pass** | `All tests passed.` |
| indexer_tests | Yes | **Fail** | `categories_test.cpp::LoadCategories` — `TEST(cat.m_synonyms.size() == 8) 3 8` |
| map_tests | Yes | **Fail** | `kmz_unarchive_test.cpp::Multi_KML_KMZ_UnzipTest` — unexpected `./data/bookmarks/doc.kml` |
| mwm_tests | Yes | **Pass** | `All tests passed.` |
| platform_tests | Yes | **Fail** | Multiple downloader tests — `HttpRequest error: -1004` (no network server) |
| routing_tests | Yes | **Fail** | 6 failing cases (e.g. `road_access_test`, `road_penalty_test`, `routing_test`) |
| search_tests | Yes | **Fail** | `bookmarks_processor_tests.cpp::BookmarksProcessorTest_Smoke`, `ranking_tests.cpp::NameScore_Smoke` |

**Example verbatim failures:**

```
indexer_tests/categories_test.cpp:41 TEST(cat.m_synonyms.size() == 8) 3 8

map_tests/kmz_unarchive_test.cpp:67 TEST(matched) Unexpected file path: ./data/bookmarks/doc.kml

platform_tests: HttpRequest error: -1004

search_tests: 2 tests failed (BookmarksProcessorTest_Smoke, NameScore_Smoke)
Some tests FAILED.
4 / 9 passed.
```

### 5. Android webDebug APK

```bash
./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk
export PATH=$HOME/Library/Android/Sdk/cmake/3.22.1/bin:$PATH
cd android
/usr/bin/time -p ./gradlew assembleWebDebug
```

**SDK setup:** Exit 0. Wrote `android/local.properties` → `sdk.dir=/Users/mo/Library/Android/Sdk`.

**Gradle build (after Java fix):** Exit 0. Wall-clock **141.7 s** real on incremental rebuild
(~464 s first cold build before fix).  
**Flavor / build type:** `web` + `debug` (`assembleWebDebug`).  
**APK path:** `android/app/build/outputs/apk/web/debug/CoMaps-26072405-web-debug.apk` (190 MB).

**Initial attempt (before fix):** Exit 1 — `MyAccountDialogFragment.java:381` missing `}`.

### 6. Physical device validation

```bash
adb install -r android/app/build/outputs/apk/web/debug/CoMaps-26072405-web-debug.apk
```

| Field | Value |
| --- | --- |
| Device | Google Pixel 3a |
| OS | LineageOS 22.2 |
| Flavor / build type | `web` + `debug` (`assembleWebDebug`) |
| Map render | **Confirmed** — map tiles load |
| Recorded | 2026-07-25 (maintainer manual validation) |

---

## Documented-command corrections

| Documented command | Issue | Working adjustment (this run) |
| --- | --- | --- |
| `./configure.sh` | Map version `260603` 404 on CDN; `ln` runs after failed `wget` | Provide `data/world_mwm/260603/*.mwm` manually; use `SKIP_MAP_DOWNLOAD=1` for builds |
| `cd build && ctest -L "omim-test" …` (README §8.1) | Default `build_omim.sh` output is `../omim-build-debug`, not `build/` | Use `cd ../omim-build-debug && ctest …` (§8.1 updated) |
| `git submodule update --init --recursive --depth 1` | `3party/protobuf/protobuf` can checkout empty | `cd 3party/protobuf/protobuf && git reset --hard HEAD` |
| `./tools/unix/build_omim.sh -d` on macOS after Android build | Android SDK `cmake` 3.22.1 on `PATH` breaks desktop configure | `export CMAKE=/opt/homebrew/bin/cmake` and keep SDK cmake off `PATH` |

---

## CTEST_EXCLUDE_REGEX vs smoke suite (SP-002 input)

Forgejo `.forgejo/workflows/linux-check.yaml` `CTEST_EXCLUDE_REGEX` excludes:

`generator_integration_tests`, `opening_hours_integration_tests`, …, `map_tests`,
`search_tests`, `routing_tests`, `generator_tests`, `base_tests`, `indexer_tests`,
`platform_tests`, …

| Smoke target | In CI exclude regex? |
| --- | --- |
| base_tests | Yes |
| coding_tests | **No** |
| generator_tests | Yes |
| indexer_tests | Yes |
| map_tests | Yes |
| mwm_tests | **No** |
| platform_tests | Yes |
| routing_tests | Yes |
| search_tests | Yes |

Seven of nine smoke targets are excluded from CI. Local run (after fixes): **4 pass, 5
fail** at the binary level. See [README §8.2](README.md) for disposition — none
are Street Pixels V1 merge gates. SP-002 adds `street_pixels_tests` as the
executable C++ gate instead of repairing these suites.

---

## Smoke suite disposition

Recorded 2026-07-25 on `street-pixels`. Full policy: `docs/implementation/README.md`
§8.2.

| Target | Result | Root cause (summary) | V1 gate? | Next action |
| --- | --- | --- | --- | --- |
| `base_tests` | Pass | — | No | None |
| `coding_tests` | Pass | — | No | None |
| `generator_tests` | Pass | — | No | None |
| `mwm_tests` | Pass | — | No | None |
| `indexer_tests` | Fail | `categories_test.cpp::LoadCategories` — `m_synonyms.size()` 3 vs 8 | No | Fork drift; separate hygiene work if desktop CI is ever restored |
| `map_tests` | Fail | `Multi_KML_KMZ_UnzipTest` — unzip emits `doc.kml` not `BACRNKMZdoc` | No | Fork drift; separate hygiene work |
| `platform_tests` | Fail | Downloader tests without test server (`-1004`) | No | Expected without `REQUIRE_SERVER` infrastructure |
| `routing_tests` | Fail | Six cases (`road_access_test`, `road_penalty_test`, `routing_test`, …) | No | Fork drift; separate hygiene work |
| `search_tests` | Fail | `BookmarksProcessorTest_Smoke`, `NameScore_Smoke` | No | Fork drift; separate hygiene work |

**Street Pixels V1 regression gate (from SP-002):** `street_pixels_tests` plus
per-work-item targets and Android device acceptance — not this smoke matrix.

---

## Summary

On `street-pixels` (macOS 26.5 arm64, toolchains above):

- **Android `assembleWebDebug`:** succeeds after Java brace fix; APK at path above.
- **Desktop smoke binaries:** build in ~95 s with `CMAKE=/opt/homebrew/bin/cmake`.
- **Smoke suite:** executed; 4/9 binaries pass. **Dispositioned** — not a V1 gate
  (see § Smoke suite disposition and README §8.2). SP-002 adds `street_pixels_tests`.
- **Full desktop `-d` build:** not fully green; **not a V1 blocker**.
- **Physical device map smoke:** **pass** — Pixel 3a, LineageOS 22.2, map loads (`webDebug`).
- **`./configure.sh`:** still requires map workaround on fresh clone (CDN 260603 404).

Smoke failures are dispositioned in baseline.md and README §8.2; repair is out of
scope for SP-002.
