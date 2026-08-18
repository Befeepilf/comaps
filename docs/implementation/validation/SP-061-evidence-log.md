# SP-061 — Evidence log (Phase 6 exit)

**Plan:** [SP-061-validation-plan.md](SP-061-validation-plan.md)
**Branch:** `cursor/sp-061-phase6-validation-35cf`
**Status:** Evidence recorded — Phase 6 exit **awaiting maintainer decision** (agent does not mark exit Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-18 |
| Git SHA (suite run tip) | `ab1c065f3f645d56616cbb490cfc25110af7fd38` (`[docs] Add SP-061 Phase 6 validation plan` on `cursor/sp-061-phase6-validation-35cf`; production tree matches SP-060 merge `8e9357abf`) |
| Build | `ninja -C /workspace/omim-build-debug routing_tests street_pixels_tests routing_common_tests` (exit 0; CMake re-run then 12 compile/link steps) |
| `routing_tests` | **306/306** All tests passed (`grep -c '^OK$'` → 306; `grep -c '^Running '` → 306) |
| `street_pixels_tests` | **222/222** All tests passed (`grep -c '^OK$'` → 222; `grep -c '^Running '` → 222) |
| `routing_common_tests` | **26/27** Some tests FAILED (`grep -c '^OK$'` → 26; `grep -c '^Running '` → 27). Failure: `vehicle_model_test.cpp::VehicleModel_CarModelValidation` — `TEST(factor == kHighwayBasedFactors.cend()) highway-living_street` at `routing_common_tests/vehicle_model_test.cpp:457`. Assertion inverted in 2026-02-11 `[tests] SmallMap fixup` (`715a28bd1`). Phase 6 did not touch `libs/routing_common/`. Test not weakened. |
| Android compile | `:app:compileFdroidDebugJavaWithJavac` (native CMake skipped) **BUILD SUCCESSFUL** in 6s; 36 actionable tasks |
| JUnit | `RoutingBuildErrorTest` **11/11** failures=0; `StreetExplorationRoutingOptionsTest` **5/5** failures=0; Gradle **BUILD SUCCESSFUL** in 6s |
| `routing_integration_tests` | Not run (not required) |
| Smoke / APK | Not run (agent desktop suites only) |
| Device walks | Deferred → Phase 10 |

### Suite command transcripts (counts)

Harness writes `Running` / `OK` to **stderr**. Counts below used `2>&1 | tee` so `grep -c '^OK$'` matches. First `routing_tests` pipe without `2>&1` captured only six `SP054_RESULT` stdout lines (`grep -c '^OK$'` → 0); combined capture of that same process still showed 306 OK and “All tests passed.” Re-run with `2>&1` is the log on disk.

```text
$ git rev-parse HEAD
ab1c065f3f645d56616cbb490cfc25110af7fd38

$ ninja -C /workspace/omim-build-debug routing_tests street_pixels_tests routing_common_tests
ninja: Entering directory `/workspace/omim-build-debug'
[12/12] Linking CXX executable street_pixels_tests
# exit 0

$ /workspace/omim-build-debug/routing_tests --data_path=/workspace/data --user_resource_path=/workspace/data 2>&1 | tee /tmp/sp061-routing_tests.log
… (306 × OK) …
All tests passed.
# grep -c '^OK$' → 306
# grep -c '^Running ' → 306
# tail -n 5:
Running position_accumulator_tests.cpp::PositionAccumulator_LongSegment
OK
Test took 0 ms

All tests passed.

$ /workspace/omim-build-debug/street_pixels_tests --data_path=/workspace/data --user_resource_path=/workspace/data 2>&1 | tee /tmp/sp061-street_pixels_tests.log
… (222 × OK) …
All tests passed.
# grep -c '^OK$' → 222
# grep -c '^Running ' → 222

$ /workspace/omim-build-debug/routing_common_tests --data_path=/workspace/data --user_resource_path=/workspace/data 2>&1 | tee /tmp/sp061-routing_common_tests.log
Running vehicle_model_test.cpp::VehicleModel_CarModelValidation
FAILED
routing_common_tests/vehicle_model_test.cpp:457 TEST(factor == kHighwayBasedFactors.cend()) highway-living_street
…
1  tests failed:
vehicle_model_test.cpp::VehicleModel_CarModelValidation
Some tests FAILED.
# grep -c '^OK$' → 26
# grep -c '^Running ' → 27

$ cd /workspace/android && ./gradlew :app:compileFdroidDebugJavaWithJavac \
    -x externalNativeBuildDebug -x externalNativeBuildRelease \
    -x configureCMakeDebug -x buildCMakeDebug
BUILD SUCCESSFUL in 6s
36 actionable tasks: 8 executed, 28 up-to-date

$ cd /workspace/android && ./gradlew :sdk:testDebugUnitTest \
    --tests app.organicmaps.sdk.routing.RoutingBuildErrorTest \
    --tests app.organicmaps.sdk.routing.StreetExplorationRoutingOptionsTest \
    -x externalNativeBuildDebug -x externalNativeBuildRelease \
    -x configureCMakeDebug -x buildCMakeDebug --rerun-tasks
BUILD SUCCESSFUL in 6s
# TEST-…RoutingBuildErrorTest.xml tests="11" failures="0"
# TEST-…StreetExplorationRoutingOptionsTest.xml tests="5" failures="0"
```

Logs copied to `/opt/cursor/artifacts/sp061_routing_tests.log`,
`sp061_street_pixels_tests.log`, `sp061_routing_common_tests.log`,
`sp061_RoutingBuildErrorTest.xml`, `sp061_StreetExplorationRoutingOptionsTest.xml`,
`sp061_test_counts.txt`.

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |
| D3 | optional | — | — | Deferred Phase 10 |

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1 Max strength full ratio → 10 | agent | **Pass** | `ExplorationMultiplier_MaxStrengthFullRatioYieldsTen` OK |
| A2 Formula edges | agent | **Pass** | `RatioZeroYieldsOne`, `MaxStrengthHalfRatioYieldsFivePointFive`, `ZeroStrengthYieldsOneRegardlessOfRatio` OK |
| A3 Prefer fully explored | agent | **Pass** | `ExplorationWeight_PreferFullyExploredMaxStrength` OK |
| A4 Unexplored / Neither / Avoid multiplier | agent | **Pass** | `PreferUnexploredIsOne`, `NeitherIsOneWhenExplored`, `AvoidStoredDoesNotChangeMultiplier` OK |
| A5 Prefer still uses soft multiplier | agent | **Pass** | `StreetExplorationAvoid_PreferStillUsesSoftMultiplier` OK |
| A6 Options 10 tests | agent | **Pass** | 10 × `StreetExplorationRoutingOptions_*` OK in `routing_tests` |
| A7 Walk/bike bindWithAvoid + include | agent | **Pass** | `WalkingOptionsFragment` / `CyclingOptionsFragment` call `bindWithAvoid`; both layouts + driving include `include_street_exploration_prefer.xml` |
| A8 Seekbar visible iff Prefer | agent | **Pass** | `StreetExplorationPreferBinder`: `strengthContainer` VISIBLE only when Prefer checked (both `bind` and `bindWithAvoid`) |
| A9 Imported-only like live | agent | **Pass** | `ExplorationWeight_ImportedOnlyCountsLikeLive` OK |
| A10 Cross-leaf + eviction | agent | **Pass** | `OverlayCountryDiffersButSegmentPixInstalled`, `LeafPixEvictedAfterFileReplace` OK |
| A11 Missing pix → 1.0 | agent | **Pass** | `ExplorationWeight_MissingPixIsOne` OK |
| A12 Device Prefer walk/bike | — | **Residual** | Phase 10 |
| B1 Mixed path kept | agent | **Pass** | `StreetExplorationAvoid_MixedPathKeptFullyExploredUnused` OK |
| B2 Exclude iff fully explored | agent | **Pass** | `AvoidExcludesFullyExplored`, `AvoidDoesNotExcludeMixed` OK |
| B3 Imported-only excludes like live | agent | **Pass** | `ExplorationWeight_AvoidImportedOnlyExcludesLikeLive` OK |
| B4 Overlay mismatch uses segment `.pix` | agent | **Pass** | `ExplorationWeight_AvoidUsesSegmentMwmPixWhenOverlayDiffers` OK |
| B5 Car does not exclude | agent | **Pass** | `StreetExplorationAvoid_CarEstimatorDoesNotExclude` OK; `AppliesAvoidExclusion` is Pedestrian \|\| Bicycle only (`edge_estimator.cpp`) |
| B6 Spike connected + boundary | agent | **Pass** | `StreetExplorationRoutingSpike_Connected` OK (after SP054_RESULT); `_AvoidExclusionBoundary` OK |
| B7 Device Avoid possible route | — | **Residual** | Phase 10 |
| C1 All paths fully explored distinct | agent | **Pass** | `StreetExplorationAvoid_AllPathsFullyExploredIsDistinctCode` OK |
| C2 Convert Avoid no-path | agent | **Pass** | `ConvertResult_AvoidNoPathIsAvoidExploredNoRoute` OK |
| C3 Car / inactive → RouteNotFound | agent | **Pass** | `ConvertResult_CarOrInactiveAvoidNoPathIsRouteNotFound` OK |
| C4 ToString | agent | **Pass** | `ToString_AvoidExploredNoRoute` OK |
| C5 Spike forced cut | agent | **Pass** | `StreetExplorationRoutingSpike_ForcedCut` OK |
| C6 Java code 17 + RoutingBuildErrorTest | agent | **Pass** | `ResultCodes.AVOID_EXPLORED_NO_ROUTE = 17`; JUnit 11/11 |
| C7 preferFallback keeps strength | agent | **Pass** | `StreetExplorationRoutingOptionsTest` 5/5 |
| C8 No-route dialog | agent | **Pass** | `RoutingErrorDialogFragment` Prefer button; `ResultCodesHelper` case 17 title/message |
| C9 Code 17 not driving-options error | agent | **Pass** | `RoutingBuildError.isDrivingOptionsBuildError` excludes 17; `RoutingController` delegates |
| C10 No min-connection | agent | **Pass** | No min-connection symbol under `libs/`; fallback is `preferFallback` (mode Prefer, same strength) |
| C11 Device SP-058 script | — | **Residual** | Phase 10 |
| C12 GPS off-route Prefer dialog | — | **Residual** | `RoutingManager::CheckLocationForRouting` passes `nullptr` `removeRouteCallback`. Not implemented (R3) |
| D1 Warning copy exact | agent | **Pass** | `values/strings.xml` + `values-en/strings.xml`: `This can produce very long routes or no available route.` |
| D2 Warning before save; dismiss reverts | agent | **Pass** | `bindWithAvoid`: dialog before `MODE_AVOID` save; dismiss unchecks if `!confirmed[0]` |
| D3 `bind()` hides avoid container | agent | **Pass** | `StreetExplorationPreferBinder.bind()` sets avoid container `GONE`; `DrivingOptionsFragment` calls `bind` not `bindWithAvoid` |
| D4 Device SP-058 warning | — | **Residual** | Phase 10 |
| E1 Research after painting remaining | agent | **Pass** | `AvoidFollowStability_ResearchAfterPaintingRemainingAbandonsPath` OK |
| E2 Traffic rebuild skipped while following Avoid | agent | **Pass** | `TestTrafficRebuildSkippedWhileFollowingAvoidRoute` OK |
| E3 Traffic rebuild when not following Avoid | agent | **Pass** | `TestTrafficRebuildRunsWhenNotFollowingAvoidRoute` OK |
| E4 Off-route rebuild still runs | agent | **Pass** | `TestOffRouteRebuildStillRunsWhileFollowingAvoidRoute` OK |
| E5 Early-return policy | agent | **Pass** | `RebuildRouteOnTrafficUpdate` returns when `IsFollowing() && IsOnRoute() && WasBuiltUnderAvoid()` |
| E6 Pixel collection does not notify routing | agent | **Residual** | No collection→`RebuildRoute` / notify in `libs/map` street-pixel sources (R2) |
| E7 Device Avoid follow | — | **Residual** | Phase 10 |
| F1 Analytics increment / neither | agent | **Pass** | `_DefaultZero`, `_RecordPreferUsed`, `_RecordAvoidUsed`, `_NeitherDoesNotIncrement` OK |
| F2 Fallback distinct | agent | **Pass** | `_RecordAvoidFallbackPrefer`, `_FallbackIsNotPreferUsed` OK |
| F3 Persist / isolate | agent | **Pass** | `_PersistRoundTrip`, `_ResetIsolatesTests` OK |
| F4 Snapshot no location keys | agent | **Pass** | `StreetExplorationRoutingAnalytics_SnapshotHasNoLocationKeys` OK |
| F5 AssignRoute increments | agent | **Pass** | `TestAssignRouteIncrementsExplorationAnalytics` OK |
| F6 Settings keys integers only | agent | **Pass** | `settings::Set` / `TryGet` `uint64_t` for three analytics keys |
| F7 Dialog records fallback | agent | **Pass** | `RoutingErrorDialogFragment` calls `StreetExplorationRoutingAnalytics.recordAvoidFallbackPrefer()` |
| F8 Not Sentry | agent | **Pass** | Counters are local settings; no Sentry API in analytics module. Sentry Gradle plugin remains crash-upload infra, not this sink |
| F9 Upload / debug readout | — | **Residual** | Phase 10 (R4, R5). `DebugPrint` exists for tests only; no in-app readout |
| F10 AA toast not SPD-042 switch | — | **Residual** | `CarToast` used for trip-finished / unable-to-calc only (R6) |
| G1 `routing_tests` | agent | **Pass** | **306/306** |
| G2 `street_pixels_tests` | agent | **Pass** | **222/222** |
| G3 `routing_common_tests` | agent | **Fail** | **26/27** — `VehicleModel_CarModelValidation` only. Not a listed Phase 6 UNIT_TEST. Not weakened |
| G4 `routing_integration_tests` | — | **Residual** | Not run (R8) |
| H1 Car no Avoid | agent | **Pass** | `DrivingOptionsFragment.bind` not `bindWithAvoid`; AA `DrivingOptionsScreen` Prefer toggle only |
| H2 §17.4 modes not added | agent | **Pass** | C++ `Neither/Prefer/Avoid`; Java `MODE_NEITHER/PREFER/AVOID` only |
| H3 No min-connection | agent | **Pass** | Same as C10 |
| H4 No Sentry sink | agent | **Pass** | Same as F8 |
| H5 No country allowlists | agent | **Pass** | No city/country routing allowlist in Phase 6 sources |
| H6 Prefer/Avoid not Pro-gated | agent | **Pass** | `explorer_pro::Capability` is GPX/track only |
| H7 iOS / public Explorer Pro purchasing | — | **Pass** | Out of Android V1 scope; not implemented here |
| I1 SP-058 strings exactly | agent | **Pass** | Warning + no-route title/message + Prefer button in `values/strings.xml` |
| I2–I6 Device walks | — | **Residual** | Phase 10 |
| J1 `routing_tests` | agent | **Pass** | 306 OK == 306 Running |
| J2 `street_pixels_tests` | agent | **Pass** | 222 OK == 222 Running |
| J3 `routing_common_tests` | agent | **Fail** | 26 OK != 27 Running |
| J4 Android compile + JUnit | agent | **Pass** | Compile SUCCESS; JUnit 11 + 5, 0 failures |

## Phase 6 exit criteria table

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Prefer-unexplored reachable and functional for walking and cycling | **Pass** (automated) + **Residual** (device) | A1–A11; J2; A12/I2 → Phase 10 |
| 2 | Avoid-explored produces a route when one exists that skips fully explored edges | **Pass** (automated) + **Residual** (device) | B1–B6; J1/J2; B7 → Phase 10 |
| 3 | Distinct no-route + Prefer+seekbar control; never silent abandon (except C12) | **Pass** (automated) + **Residual** (device) + **C12 Residual** | C1–C11; J4; C12/R3 not implemented |
| 4 | §17.3 warning shown before Avoid is used | **Pass** (code) + **Residual** (device) | D1–D3; I1; D4 → Phase 10 |
| 5 | Mid-navigation behaviour defined, implemented, stable | **Pass** (skip-rebuild tests) + **Residual** (device/GPS + no pixel notify) | E1–E5; E6/R2; E7 → Phase 10 |
| 6 | Routing analytics record mode usage with no location data | **Pass** (local counters) + **Residual** (upload/readout) | F1–F8; F9/F10 → Phase 10 |
| 7 | Existing routing tests pass | **Fail** (`routing_common_tests` 26/27) | G1/J1 **306/306**; G2/J2 **222/222**; G3/J3 **26/27** `VehicleModel_CarModelValidation`; G4 not run |

## Residuals → Phase 10 / follow-up

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | SP-054 city-scale MWM+`.pix` / Spike 7 on real data | Phase 10 |
| R2 | Pixel collection does not notify routing | Residual; do not implement as a silent rebuild |
| R3 | GPS off-route Prefer dialog not shown (`nullptr` `removeRouteCallback`) | Phase 10; do not implement on SP-061 |
| R4 | Analytics upload | Phase 10 |
| R5 | No in-app debug readout | Phase 10 |
| R6 | Android Auto toast-as-switch (SPD-042) | Residual; AA still toast for trip-finished / unable-to-calc |
| R7 | All device walks (Prefer, Avoid possible, SP-058 script, warning, Avoid follow) | Phase 10 |
| R8 | `routing_integration_tests` not run | Not required for this gate |
| R9 | `routing_common_tests` `VehicleModel_CarModelValidation` fails: `TEST(factor == kHighwayBasedFactors.cend()) highway-living_street` (line 457; inverted vs `IsValid()` on the iterator). Introduced 2026-02-11 `715a28bd1`; Phase 6 did not touch `routing_common`. | Follow-up outside SP-061. Do not weaken the test. Not an SPD production regression |

## Contradictions (reported, not silently resolved)

1. **Spec §17.3 any-explored vs SPD-042 `exploredRatio == 1`.** Product spec still describes avoiding explored streets more broadly; **SPD-042** locks V1 exclusion to fully explored edges only. Code matches SPD-042 (`IsSegmentExcluded` / Avoid tests). Spec was not edited.

2. **Spec min-connection vs SPD-042 Prefer+strength.** Spec §17.3 / §31 describe a min-connection / return-to-normal style fallback; **SPD-042** locks explicit Prefer+seekbar. Code has `preferFallback` and no min-connection search (C10). Spec was not edited.

3. **Spec no seekbar vs SPD-041 keep seekbar.** Spec §17.2 does not define a 0–100 strength control; **SPD-041** keeps the seekbar for V1. Code shows the seekbar iff Prefer (A8). Spec was not edited.

4. **Audit 2026-07-20 Prefer driving-only / Avoid missing vs current code.** Audit snapshot: Prefer on driving options, Avoid absent. Current code: Prefer+Avoid+warning on walk/bike (`bindWithAvoid`); car `bind()` hides Avoid; Avoid engine excludes `exploredRatio == 1`. Phase-06 current-code table records this; audit file was not edited.

5. **Audit Spike 7 large penalty vs SPD-042 exclusion.** Audit recommended a very large finite penalty first. **SPD-042** / SP-057 implement true exclusion of fully explored edges. Code matches SPD-042, not the audit’s first-step penalty.

6. **SPD-043 show Prefer on off-route fail vs `nullptr` callback (R3).** SPD-043 says off-route uses the SPD-042 fallback UX. `RoutingManager::CheckLocationForRouting` rebuilds with `nullptr` `removeRouteCallback`, so the Avoid no-route dialog is not shown on that path. Residual R3; not implemented here.

7. **README §4 Phase 6 “Not started” vs phase-06 In progress.** `docs/implementation/README.md` §4 still lists Phase 6 as **Not started**. `phase-06-exploration-aware-routing.md` **Status: In progress** (SP-054–060 Accepted; SP-061 in progress). README was **not** edited (instruction).

8. **README §4.2 exit summary incomplete vs phase-06 1–7.** README §4.2 Phase 6 exit is a three-clause summary (Prefer walk/bike + seekbar; Avoid exclusion; explicit Prefer switch / no silent abandon). Phase-06 file lists **seven** numbered exits including warning, mid-nav stability, analytics, and existing-test regression. README was **not** edited.

9. **SP-055 Status In review while SPD-040–045 Accepted.** `SP-055-routing-architecture-decisions.md` still says **Status: In review**. `DECISIONS.md` / phase-06 record SPD-040–045 Accepted 2026-08-15. SP-055 was not marked Accepted here.

10. **Phase-06 Confirmed gaps is a 2026-08-15 snapshot.** The “Confirmed gaps” list still describes Prefer UI on the vehicle tab, Avoid absent, enabled+strength options, etc. That was true at phase entry. Current-code table (updated in later SPs) contradicts those gap bullets. Confirmed-gaps section was **not** rewritten (instruction).

## Phase 6 exit recommendation (agent)

Blocks A–F automated rows and J1/J2/J4 are green on SHA `ab1c065f3`. Exits 1–6 are **Pass** on shared-core / Android-signal coverage with honest **device / GPS / upload residuals** (R1–R8). Exit 7 is **Fail** because `routing_common_tests` is 26/27 (`VehicleModel_CarModelValidation`, pre-Phase-6 inverted assertion). Phase 6 UNIT_TESTs in `routing_tests` and `street_pixels_tests` are green.

**Maintainer decides** whether Phase 6 exit is Met with residuals plus R9, or blocked on `routing_common_tests` / device walks. Agent does **not** mark Phase 6 or SP-061 Accepted.
