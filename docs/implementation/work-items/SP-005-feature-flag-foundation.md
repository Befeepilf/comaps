# SP-005 — Feature-flag and entitlement foundation

**Phase:** 1 — Baseline and guardrails
**Status:** Not started
**Branch:** `street-pixels`

---

## Objective

Introduce two independent, testable concepts — "is this capability available in
this build" and "does this user hold the entitlement" — so that later phases can
ship incomplete or paid surfaces disabled, and so that public V1 exposes no
non-functional purchase action.

## Motivation

Product spec §29 requires the application to distinguish feature availability
from user entitlement, and §34 requires that public builds do not present a
non-functional purchase action. Neither concept exists in the tree: there is no
entitlement abstraction anywhere, and no Android billing.

What does exist is ad hoc: Gradle product flavors (`google`, `web`, `fdroid`,
`huawei`), build types (`debug`, `release`, `beta`), BuildConfig fields such as
`EXPLORE_API_BASE_URL`, and `-P` gradle properties like
`enableVulkanDiagnostics` and `enableTrace` that reach CMake. None of that is
reachable from shared C++ as a capability query.

Establishing the shape now is cheap. Retrofitting it across the GPX and
track-management surfaces in Phase 9 is not, and doing it late invites gates
that are checked inconsistently.

## In-scope behavior

- A shared capability query in `libs/` answering "is capability X available in
  this build", with values fixed at build time.
- A shared entitlement query answering "does this user hold entitlement Y",
  backed in V1 by a stub that always reports "not entitled".
- A single composition point where the two combine, so that no caller can
  accidentally check only one.
- Definitions for the capabilities Phase 9 needs: GPX import, GPX export,
  advanced track management. Defined, defaulted off in public configuration,
  and not yet applied to any call site.
- Build wiring so the values are set per Android build configuration.
- Tests covering the full two-by-two matrix.

## Out-of-scope behavior

- Applying gates to any existing feature. Phase 9 does that. This work item
  changes no user-visible behaviour.
- Any billing integration, purchase flow, or store entitlement validation.
  Deferred by SPD-010.
- A remote or server-driven flag system. Flags are build-time in V1.
- A general-purpose experimentation framework.
- Flags for incomplete Street Pixels features other than the Pro capabilities.
  If Phase 2 or later needs one, it adds it using this foundation.
- Removing or hiding the friends feature. See OQ-6; that is a product decision,
  not a foundation task, though this foundation is what would implement it.

## Relevant product requirements

- §7 Explorer Pro: capabilities gated by entitlement; in public V1 Pro features
  may be disabled globally by feature flag while the entitlement architecture
  remains for later activation.
- §29 The build-availability versus user-entitlement distinction; the UI must
  not present a non-functional purchase action.
- §29.2 Explorer Pro features: GPX import, export, batch import, advanced local
  track management.
- §30 Settings may include GPX tools only when the flag is enabled and the user
  is entitled.
- §32.5 Monetisation analytics measured only when Explorer Pro is enabled in a
  build.
- §34 "Explorer Pro and monetization" launch requirements.

## Relevant source files or symbols

- `libs/map/CMakeLists.txt`, where new shared sources are registered
- `libs/map/backend_config.{hpp,cpp}` as the closest existing example of a
  small shared configuration module backed by settings
- `libs/platform/settings.hpp` and `platform::SecureStorage`, as candidate
  storage for entitlement state
- `android/sdk/build.gradle`, existing BuildConfig field pattern
- `android/sdk/src/main/java/app/organicmaps/sdk/Framework.java`, existing
  pattern for pushing build configuration into native code
- `android/sdk/src/main/java/app/organicmaps/sdk/OrganicMaps.java`, where such
  configuration is applied at startup
- `android/app/build.gradle`, product flavors and build types

## Dependencies

- SP-001.
- Ideally after SP-004, since both touch how build-time configuration reaches
  shared C++ and the two should end up consistent rather than inventing two
  patterns.

## Proposed implementation approach

1. Define the capability enumeration in shared code, limited to the Pro
   capabilities the product spec names.
2. Implement the availability query with build-time values, following whichever
   pattern SP-004 settles on for injecting build configuration into `libs/`.
3. Define the entitlement interface with a single V1 implementation that always
   reports "not entitled". Make it structurally obvious that it is a stub and
   that it has no path to granting entitlement.
4. Provide one composition function — capability available **and** user
   entitled — and make it the only public way to ask the question. Do not expose
   the two queries separately in a way that invites checking only one.
5. Wire Android build configuration so public configurations have all Pro
   capabilities off, and an internal configuration can turn them on.
6. Write the matrix tests.
7. Apply the gate to nothing. The diff should contain no change to existing
   behaviour.

## Acceptance criteria

1. Shared code can answer "is capability X available in this build".
2. Shared code can answer "does the user hold entitlement Y", returning "no" in
   V1.
3. A single composition point exists and is the intended call site for gating.
4. All Pro capabilities are unavailable in public Android configurations.
5. The stub entitlement source has no path to granting entitlement.
6. No existing user-visible behaviour changed. GPX import and export remain as
   they are today; Phase 9 gates them.
7. No purchase action exists anywhere in the UI.
8. The two-by-two matrix is covered by tests.

## Required automated tests

In the SP-002 target:

- Capability unavailable and not entitled: gate denies.
- Capability unavailable and entitled: gate denies.
- Capability available and not entitled: gate denies.
- Capability available and entitled: gate allows.
- The V1 entitlement source always reports not entitled, including after any
  settings manipulation the test can perform.
- Public build configuration reports every Pro capability unavailable.

## Required manual validation

- Build a public-configured APK and confirm no GPX or Explorer Pro entry point
  appeared, and none disappeared either — behaviour must be unchanged.
- Build an internal configuration with capabilities enabled and confirm the
  query reports available while entitlement still denies.
- Confirm no purchase, upgrade, or pricing UI is reachable, including through
  settings, menus, and deep links.

## Failure and rollback considerations

- The main risk is over-engineering. Two queries and one composition point are
  enough; a general flag framework is not needed and would be harder to reason
  about at the release-governance review.
- A second risk is a gate that is checked inconsistently later. Mitigate by
  exposing one composition point rather than two independent booleans.
- If entitlement state is persisted in the wrong place, moving it later is a
  migration. Since V1 persists nothing, choose the location deliberately and
  document it, but do not write to it.
- Rollback is a revert with no user-visible effect, because nothing is gated
  yet.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Commits | |
| Capabilities defined | |
| Availability mechanism | |
| Entitlement storage location chosen, unused in V1 | |
| Build configuration to capability mapping | |
| Matrix test output | |
| Confirmation that no behaviour changed | |
| Implemented by | |
| Independent reviewer | |
| Manual validation performed by and date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
