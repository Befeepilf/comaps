# Street Pixels build baseline

**Recorded:** 2026-07-25  
**Branch:** `SP-001-reproducible-android-baseline` (see note below)  
**Base commit:** `1cb5c5d1fa` (`street-pixels`)  
**Host:** macOS 26.5 (Build 25F71), arm64  
**Worktree:** `/Users/mo/dev/comaps-sp001` (clean checkout; `git status` empty at start)

### Branch name note

The work item specifies branch `street-pixels/SP-001-reproducible-android-baseline`. Git
refuses to create that name because branch `street-pixels` already exists as a leaf ref
(`fatal: cannot lock ref 'refs/heads/street-pixels/SP-001-…': 'refs/heads/street-pixels'
exists`). This baseline was recorded on `SP-001-reproducible-android-baseline` cut from
`street-pixels` at the same commit.

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
/usr/bin/time -p ./tools/unix/build_omim.sh -d
```

**Result:** Exit 1. Wall-clock **38.76 s** (real). Build dir: `../omim-build-debug`
(sibling of repo root). Ninja stopped at 155/536 objects.

**Verbatim failure:**

```
FAILED: [code=1] libs/platform/CMakeFiles/platform.dir/Unity/unity_1_cxx.cxx.o
/Users/mo/dev/comaps-sp001/libs/platform/vibration.hpp:16:74: error: unknown type name 'size_t'
   16 | void VibratePattern(uint32_t const * durations, uint32_t const * delays, size_t count);
      |                                                                          ^
/Users/mo/dev/comaps-sp001/libs/platform/vibration.cpp:63:82: error: unknown type name 'size_t'
   63 | void VibratePattern(uint32_t const * /*durations*/, uint32_t const * /*delays*/, size_t /*count*/)
      |                                                                                  ^
ninja: build stopped: subcommand failed.
real 38.76
```

### 4. Smoke suite

```bash
./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke
```

**Result:** Not executed — desktop debug build did not complete; test binaries were not
produced.

**Smoke targets (from `tools/unix/run_tests.sh`):** `base_tests`, `coding_tests`,
`generator_tests`, `indexer_tests`, `map_tests`, `mwm_tests`, `platform_tests`,
`routing_tests`, `search_tests`.

| Target | Built | Pass/fail |
| --- | --- | --- |
| base_tests | No | Not run |
| coding_tests | No | Not run |
| generator_tests | No | Not run |
| indexer_tests | No | Not run |
| map_tests | No | Not run |
| mwm_tests | No | Not run |
| platform_tests | No | Not run |
| routing_tests | No | Not run |
| search_tests | No | Not run |

Optional CTest cross-check was not run for the same reason.

### 5. Android webDebug APK

```bash
./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk
export PATH=$HOME/Library/Android/Sdk/cmake/3.22.1/bin:$PATH
cd android
/usr/bin/time -p ./gradlew assembleWebDebug
```

**SDK setup:** Exit 0. Wrote `android/local.properties` → `sdk.dir=/Users/mo/Library/Android/Sdk`.

**Gradle build:** Exit 1. Wall-clock **464.34 s** (real, ~7 m 44 s).  
**Flavor / build type:** `web` + `debug` (`assembleWebDebug`).  
**APK path:** Not produced.

**Verbatim failure:**

```
> Task :app:compileWebDebugJavaWithJavac FAILED
/Users/mo/dev/comaps-sp001/android/app/src/main/java/app/organicmaps/settings/MyAccountDialogFragment.java:381: error: illegal start of expression
  private void maybeHandlePendingAddFriend()
  ^
1 error

FAILURE: Build failed with an exception.
Execution failed for task ':app:compileWebDebugJavaWithJavac'.
BUILD FAILED in 7m 43s
real 464.34
```

The error is a missing closing brace before `maybeHandlePendingAddFriend()` in WIP explore
account code committed on `street-pixels` at `1cb5c5d1fa`.

### 6. Physical device validation

```bash
adb devices
```

**Result:** No devices attached at time of run (`List of devices attached` empty).

APK install, launch, and map-render confirmation were **not performed** — no APK was
built and no device was connected.

---

## Documented-command corrections

| Documented command | Issue | Working adjustment (this run) |
| --- | --- | --- |
| `./configure.sh` | Map version `260603` 404 on CDN; `ln` runs after failed `wget` | Provide `data/world_mwm/260603/*.mwm` manually; use `SKIP_MAP_DOWNLOAD=1` for builds |
| `cd build && ctest -L "omim-test" …` (README §8.1) | Default `build_omim.sh` output is `../omim-build-debug`, not `build/` | Use `cd ../omim-build-debug && ctest …` (§8.1 updated) |
| `git submodule update --init --recursive --depth 1` | `3party/protobuf/protobuf` can checkout empty | `cd 3party/protobuf/protobuf && git reset --hard HEAD` |
| Branch `street-pixels/SP-001-…` | Blocked by existing `street-pixels` branch ref | Use `SP-001-reproducible-android-baseline` or rename integration branch |

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

Seven of nine smoke targets are excluded from CI. Only `coding_tests` and `mwm_tests`
would run if the full smoke suite were executed in CI. Local smoke results are unknown
on this commit because the desktop build did not finish.

---

## Summary

On `street-pixels` @ `1cb5c5d1fa`, from a clean worktree on macOS arm64 with the
toolchains above:

- **Desktop debug build:** fails (C++ compile error in `libs/platform/vibration.hpp`).
- **Smoke suite:** not run (blocked by desktop build failure).
- **Android `assembleWebDebug`:** fails (Java syntax error in `MyAccountDialogFragment.java`).
- **Physical device map smoke:** not performed (no APK; no device connected).
- **`./configure.sh`:** fails on a truly fresh clone until map CDN serves `260603` or
  maps are provided manually.

No production source, CI, or test fixes were made as part of this baseline recording.
