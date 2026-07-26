# SP-002 — Street Pixels test harness

**Phase:** 1 — Baseline and guardrails
**Status:** Accepted (2026-07-26)
**Branch:** `street-pixels`

---

## Objective

Create a fast, dependency-light C++ test target for Street Pixels logic that
Phase 2 work items can extend with focused, locally runnable assertions.

## Motivation

No street-pixel test exists anywhere in the repository. Searches across
`libs/map/map_tests/` and every `libs/*_tests/` directory find no reference to
`StreetPixelsManager`, street pixels, or HEALPix outside the vendored
`3party/healpix` library.

The obvious host, `libs/map/map_tests`, is declared
`omim_add_test(map_tests ... REQUIRE_QT REQUIRE_SERVER)`, so it needs a Qt
event loop and a test server for what will mostly be pure arithmetic on
synthetic GPS samples. Worse, `map_tests` is one of the targets excluded by
`CTEST_EXCLUDE_REGEX` in `.forgejo/workflows/linux-check.yaml`, and
`.github/workflows/` contains no C++ test job at all. A test added to
`map_tests` today would not run in CI.

Phase 2 is largely unverifiable without this. Its acceptance criteria are
statements about GPS sequences, session transitions, and threshold boundaries —
exactly the things that need cheap, fast, repeatable tests.

## In-scope behavior

- A new test target, `libs/map/street_pixels_tests/`, registered through
  `omim_add_test_subdirectory` from `libs/map/CMakeLists.txt`.
- The target avoids `REQUIRE_QT` and `REQUIRE_SERVER` if the code under test
  permits; if it does not, that fact is recorded and the reason documented.
- At least one meaningful assertion against existing behaviour, so the target
  is not empty. Suitable candidates: the exploration-multiplier arithmetic in
  `StreetPixelsManager`, or the `df::StreetPixel` bit accessors.
- Small test helpers for building synthetic `location::GpsInfo` sequences and
  synthetic pixel sets, so that Phase 2 work items do not each invent their own.
- Local validation: the target builds and passes on the maintainer machine via
  `build_omim.sh` and `ctest`.

## Out-of-scope behavior

- Fixing existing failing test suites or un-excluding them. SP-001 records
  their state; changing it is separate work.
- Adding tests for behaviour that does not exist yet. Phase 2 work items bring
  their own tests.
- Android instrumented tests.
- Backend tests.
- Refactoring `StreetPixelsManager` to be more testable. If a refactor turns out
  to be necessary, it is recorded as discovered follow-up and scoped as its own
  work item, because a refactor of that file must not ride along with harness
  setup.
- Coverage measurement.
- CI automation for C++ unit tests. Deferred until after public Android V1;
  local execution is the gate for V1.

## Relevant product requirements

Indirect. This implements the validation policy in
`docs/implementation/README.md` §8, steps 4 and 5 (local focused tests).

## Relevant source files or symbols

- `cmake/OmimTesting.cmake`: `omim_add_test`, with options `REQUIRE_QT`,
  `REQUIRE_SERVER`, `NO_PLATFORM_INIT`, `BOOST_TEST`; `omim_add_test_subdirectory`
- `libs/map/CMakeLists.txt` lines 134–137, where test subdirectories are added
- `libs/map/map_tests/CMakeLists.txt` as the pattern to follow and diverge from
- `libs/map/street_pixels_manager.{hpp,cpp}`
- `libs/drape_frontend/street_pixel.{hpp,cpp}`: `GetPixelId`, `IsExplored`,
  `SetExplored`
- `libs/platform/location.hpp`: `location::GpsInfo`
- `tools/unix/run_tests.sh`

## Dependencies

- SP-001, so that a working build and the current suite state are known.

## Proposed implementation approach

1. Create `libs/map/street_pixels_tests/` with a `CMakeLists.txt` calling
   `omim_add_test(street_pixels_tests ${SRC})` with the smallest set of options
   that links. Try without `REQUIRE_QT` and `REQUIRE_SERVER` first.
2. Register it from `libs/map/CMakeLists.txt` next to the existing
   `omim_add_test_subdirectory` calls.
3. Link only what is needed. If linking `map` pulls in Qt or platform
   initialisation, prefer testing the smallest unit that can be linked
   independently and record what could not be reached.
4. Add a test-support header with helpers for synthetic GPS sequences and
   synthetic pixel sets.
5. Add at least one real assertion. The multiplier formula
   `1.0 + strength * 9.0 * exploredRatio` and the `StreetPixel` bit accessors
   are both fully deterministic and need no fixtures.
6. Validate locally with `build_omim.sh -d street_pixels_tests` and
   `ctest -L omim-test -R '^street_pixels_tests$'`.

## Acceptance criteria

1. `libs/map/street_pixels_tests` builds through the standard desktop build.
2. It runs through `ctest -L omim-test` and is included in the full
   `run_tests.sh` suite (`find … '*_tests'`).
3. It contains at least one assertion that would fail if the behaviour it tests
   changed.
4. Test helpers for synthetic GPS sequences and pixel sets are available for
   later work items.
5. The target does not require a Qt event loop or a test server, or the reason
   it must is documented.
6. No existing test target's behaviour changed.
7. No production source file is modified.

## Required automated tests

The work item is the tests. Specifically, at minimum:

- `StreetPixel` bit accessors: setting explored preserves the identifier; the
  identifier mask excludes the high bit; a maximal valid HEALPix identifier at
  `nside = 1048576` round-trips.
- Exploration multiplier: ratio 0 yields 1.0; ratio 1 at maximum strength yields
  10.0; ratio 0.5 at maximum strength yields 5.5; strength 0 yields 1.0
  regardless of ratio.

## Required manual validation

- Run the target locally and confirm it completes in a small number of seconds.
- Confirm it appears in the ctest list with the `omim-test` label.

## Failure and rollback considerations

- Adding a test target cannot affect the shipped application; the worst case is
  a build-configuration problem, and `SKIP_TESTS=ON` already excludes tests
  from Android builds.
- If the target cannot be built without Qt, do not force it. Record the
  constraint and accept the heavier target rather than restructuring
  `StreetPixelsManager` inside this work item.
- Rollback is a branch revert with no user-visible effect.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | `eac83c5eab` `[map] Add street_pixels_tests for pixel bits and multiplier` |
| Test target path | `libs/map/street_pixels_tests/` |
| `omim_add_test` options used | `NO_PLATFORM_INIT` (no `REQUIRE_QT`, no `REQUIRE_SERVER`) |
| Local run output | `ctest -L omim-test -R '^street_pixels_tests$'`: 8/8 cases passed, 2026-07-26 |
| Target runtime | ~0.07 s (ctest), 8 test cases |
| CI | Deferred post-V1; not required for acceptance |
| Implemented by | Maintainer session 2026-07-26 |
| Independent reviewer | — |
| Manual validation performed by and date | Maintainer, 2026-07-26 (macOS arm64, CMake 4.4.0) |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Multiplier tests duplicate `1.0 + strength * 9.0 * exploredRatio` from `GetSegmentExplorationWeightMultiplier` rather than calling production code (manager needs DataSource/road geometry). | Extract a pure helper in a later work item (likely Phase 6 routing) and point tests at it; do not refactor in SP-002. |
| C++ unit-test CI (Forgejo `linux-check` exclusions, no GitHub C++ job). | Post-V1: one generic desktop test workflow; not a per-feature pipeline. Out of V1 scope per maintainer decision 2026-07-26. |
