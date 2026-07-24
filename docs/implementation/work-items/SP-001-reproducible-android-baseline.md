# SP-001 — Reproducible Android and desktop build baseline

**Phase:** 1 — Baseline and guardrails
**Status:** Not started
**Branch:** `street-pixels/SP-001-reproducible-android-baseline`

---

## Objective

Produce and record a verified command sequence that builds a debug Android APK
and a desktop debug build with tests, from a clean checkout of this fork, on the
maintainer's machine.

## Motivation

The technical audit did not complete a build in its pass; it verified only that
tooling exists. Every later work item's validation depends on being able to
build and run something. If the first build failure is discovered during
SP-007, the failure will be misattributed to the change under review.

This work item also establishes the baseline the whole project measures
against: what currently compiles, what currently passes, and what is already
broken before Street Pixels work begins.

## In-scope behavior

- Verifying and, where necessary, correcting the build instructions in
  `docs/implementation/README.md` §8.1 so that every command works as written.
- Recording the exact toolchain versions used: Xcode or clang, CMake, JDK,
  Android SDK, NDK, Gradle, Ninja, Python.
- Recording the current state of the test suites: which targets build, which
  pass, and which are already failing on this branch before any Street Pixels
  work.
- A short `docs/implementation/baseline.md` capturing the above, or the same
  content added to the roadmap if it is short enough to belong there.
- Minimal build-script or configuration fixes if a documented command is
  actually broken.

## Out-of-scope behavior

- Changing any C++, Java, or Kotlin production source.
- Changing CMake structure, Gradle flavors, or dependency versions.
- Adding new build targets. SP-002 does that.
- Fixing pre-existing test failures. They are recorded, not repaired.
- Changing CI configuration. SP-002 does that.
- Upgrading toolchains.

## Relevant product requirements

Indirect. No product-spec requirement is implemented here. The work exists to
make the validation policy in `docs/implementation/README.md` §8 executable.

## Relevant source files or symbols

Read and possibly annotate; modify only if demonstrably broken:

- `configure.sh`
- `docs/INSTALL.md`, `docs/INSTALL_DESKTOP.md`, `docs/UNIT_TESTING.md`
- `tools/unix/build_omim.sh`, `tools/unix/run_tests.sh`
- `tools/android/set_up_android.py`
- `android/gradlew`, `android/app/build.gradle`, `android/sdk/build.gradle`
- `.github/workflows/android-check.yaml`
- `.forgejo/workflows/linux-check.yaml`, in particular its
  `CTEST_EXCLUDE_REGEX`

## Dependencies

None. This is the first work item.

## Proposed implementation approach

1. From a clean clone of the `street-pixels` branch, run `./configure.sh` and
   record the outcome.
2. Build the desktop debug configuration with tests using
   `./tools/unix/build_omim.sh -d`. Record duration and any failures verbatim.
3. Run `./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke` and record
   the per-target result. Do not fix failures; record them.
4. Set up the Android SDK per `tools/android/set_up_android.py` and build a
   debug APK. Record which flavor was used and why.
5. Install the APK on a physical device and confirm it launches and shows a map.
6. Compare `CTEST_EXCLUDE_REGEX` in `linux-check.yaml` against the local
   results to see which exclusions reflect genuinely failing suites and which
   look stale. Record the finding; SP-002 acts on it.
7. Write the baseline document. Where a documented command did not work, record
   the command that did.
8. If a build script needed a fix, keep that fix in its own commit, separate
   from documentation.

## Acceptance criteria

1. A documented command sequence builds a debug Android APK from a clean
   checkout, and the sequence has been executed end to end.
2. A documented command sequence builds the desktop debug configuration with
   tests, and it has been executed.
3. The APK installs on a physical device and shows a map.
4. Toolchain versions are recorded.
5. The current pass and fail state of the smoke suite is recorded per target,
   with verbatim failure output for anything failing.
6. Any correction to the documented commands is captured in the baseline
   document.
7. No production source file is modified.

## Required automated tests

None are added by this work item. The output is the recorded result of the
existing suites.

If a build-script fix is made, it is validated by re-running the affected
command from a clean state.

## Required manual validation

- Full sequence executed from a clean checkout, not from an already-configured
  tree.
- APK installed on a physical Android device; record model, OS version, and
  flavor.
- Map renders and the app is interactive.
- Record wall-clock build times so later work items can judge whether a build
  is hanging or merely slow.

## Failure and rollback considerations

- Nothing user-facing changes, so there is nothing to roll back for users.
- If `configure.sh` or a Gradle build fails for environmental reasons, record
  the failure and the workaround rather than changing the repository to suit
  one machine.
- If a documented command is broken for everyone, the fix belongs in this work
  item. If it is broken only locally, the workaround belongs in the baseline
  document and not in the repository.
- Risk of scope creep is high: it is tempting to fix failing tests. Do not.

## Completion evidence

Fill in before requesting review.

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Toolchain versions | |
| Desktop build result | |
| Smoke suite result per target | |
| Android build command and flavor | |
| Test device model and OS version | |
| Documented-command corrections | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

Record anything found that is real but out of scope. Each entry either becomes
a new `SP-NNN` work item or is explicitly dropped with a reason.

| Finding | Proposed disposition |
| --- | --- |
| | |
