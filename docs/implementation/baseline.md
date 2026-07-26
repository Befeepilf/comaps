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

**Initial result (before harness/test updates):** Exit 1, **220.30 s** real. Script
summary: **4 / 9** test binaries passed entirely (see git history for per-target
failures).

**After `run_tests.sh` harness fixes and test expectation updates (rebased main,
2026-07-25):** Exit 0, **211.45 s** real. **9 / 9** passed.

| Target | Built | Pass/fail | Notes |
| --- | --- | --- | --- |
| base_tests | Yes | **Pass** | |
| coding_tests | Yes | **Pass** | |
| generator_tests | Yes | **Pass** | |
| indexer_tests | Yes | **Pass** | Expectation updates for localized type names, trie node threshold, category sort order |
| map_tests | Yes | **Pass** | KMZ path assertions; `cm.at` URLs now `UrlType::Incorrect` |
| mwm_tests | Yes | **Pass** | |
| platform_tests | Yes | **Pass** | Local test server; locale-aware distance string; downloader cleanup polling |
| routing_tests | Yes | **Pass** | Penalty constants; `LoopGraph` weight |
| search_tests | Yes | **Pass** | Bookmark query and ranking score expectations |

**Harness changes in `tools/unix/run_tests.sh`:**

- Pass `--data_path` / `--user_resource_path` pointing at repo `data/`
- `export TZ=UTC`, `LC_ALL=C`, `LANG=C`
- Start/stop `tools/python/test_server` before `platform_tests`

**Test fixes (expectations aligned to current classificator, branding, and locale
behaviour on rebased upstream main):** `indexer_tests`, `map_tests`,
`platform_tests`, `routing_tests`, `search_tests` — see `git diff` on
`street-pixels`.

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

## Known CI gap (unchanged; SP-002 scope)

Forgejo `.forgejo/workflows/linux-check.yaml` `CTEST_EXCLUDE_REGEX` excludes most
smoke targets. SP-002 added `street_pixels_tests` with **local** validation only;
generic C++ test CI is deferred until after public Android V1.

Excluded targets include:
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

Seven of nine smoke targets are excluded from CI. Local run (after harness and test
updates on rebased main): **9 / 9 pass** in ~211 s.

---

## Summary

On `street-pixels` (macOS 26.5 arm64, toolchains above):

- **Android `assembleWebDebug`:** succeeds after Java brace fix; APK at path above.
- **Desktop smoke binaries:** build in ~95 s with `CMAKE=/opt/homebrew/bin/cmake`.
- **Smoke suite:** **9/9** binaries pass (`211.45 s` real) after harness and test updates.
- **Full desktop `-d` build:** not fully green (non-smoke targets still fail).
- **Physical device map smoke:** **pass** — Pixel 3a, LineageOS 22.2, map loads (`webDebug`).
- **`./configure.sh`:** still requires map workaround on fresh clone (CDN 260603 404).

SP-001 accepted 2026-07-25. SP-002 accepted 2026-07-26 (`street_pixels_tests`,
local gate). SP-003 accepted 2026-07-26 (Sentry privacy defaults, device
validation on Pixel 3a / webBeta). Full desktop `-d` remains partially red
(non-smoke targets); recorded in §3.

### 7. Telemetry defaults (SP-003)

Android Sentry meta-data (all flavors/types share
`android/app/src/main/AndroidManifest.xml`):

| Setting | Value | Notes |
| --- | --- | --- |
| `send-default-pii` | `false` | No default PII |
| `attach-screenshot` | `false` | Map screenshot is location data |
| `attach-view-hierarchy` | `false` | Hierarchy can expose map/location UI |
| `traces.sample-rate` | `0.1` | 10% of transactions |
| `traces.profiling.session-sample-rate` | `0.1` | 10% of sessions; `lifecycle=trace` |
| `traces.profiling.start-on-app-start` | `false` | No always-on launch profiling |
| `logs.enabled` | `false` | Do not ship app logs to Sentry |
| `traces.user-interaction.enable` | `true` | Breadcrumbs only; residual risk accepted |

Guard: `./tools/unix/check_sentry_privacy.sh` (also wired into GitHub and Forgejo
`android-check` as a hard-fail job).

Release-configured validation build used for SP-003 packaging inspection:
`cd android && ./gradlew -Parm64 assembleWebBeta` →
`app/build/outputs/apk/web/beta/CoMaps-*-web-beta.apk`.
Device crash/logcat confirmation still pending (see SP-003 evidence).
