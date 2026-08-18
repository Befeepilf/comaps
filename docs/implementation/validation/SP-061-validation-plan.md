# SP-061 — Validation plan (Phase 6 exit)

**Work item:** [SP-061](../work-items/SP-061-phase6-end-to-end-validation.md)
**Plan authored by:** Agent
**Plan date:** 2026-08-18
**Branch:** `cursor/sp-061-phase6-validation-35cf`

## Approved decisions

| ID | Decision |
| --- | --- |
| Device walks | Deferred to **Phase 10** (SP-014 / SP-022 / SP-031 / SP-041 pattern). Automated exit coverage remains mandatory. |
| Spike 7 city-scale | Deferred to **Phase 10**. SP-054 recorded a desktop synthetic; city-scale MWM+`.pix` is residual. |
| GPS off-route Prefer dialog | **Phase 10. Do not implement** on this branch. Off-route rebuild uses `nullptr` `removeRouteCallback` (`RoutingManager::CheckLocationForRouting`). Residual R3. |
| Analytics upload / debug readout | **Phase 10**. Local uint64 counters only (SPD-044). |
| `routing_integration_tests` | Not required for this gate (world dataset / `REQUIRE_SERVER`). Residual R8. |
| Phase 6 Accepted | Maintainer decides after reviewing evidence. Agent does **not** mark Phase 6 exit Met unilaterally. |

## Scope

Evidence-only. No production behaviour changes on this branch except defect
fixes that block suites (prefer fix on owning SP-054–060). Map each Phase 6
exit criterion (1–7) to pass / fail / residual with pointers into the evidence
log.

Phase 6 modules under test: SP-054 (spike residual), SP-055 (locks), SP-056
(Prefer walk/bike), SP-057 (Avoid engine), SP-058 (warning + fallback UX),
SP-059 (mid-nav freeze), SP-060 (count-only analytics).

Do **not** implement: GPS off-route Prefer dialog, analytics upload, debug
readout, device walks, city-scale spike, pixel→routing notify, AA toast-as-switch,
min-connection search, car Avoid, §17.4 modes.

## Device matrix

| Slot | Model | OS / skin | Region under test | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | *(fill on walk)* | Finland / Helsinki | Same class as SP-014 / SP-033 |
| D2 | Aggressive OEM | | | Deferred Phase 10 |
| D3 | optional | | | Nice-to-have |

**Build for walks:** same APK / git SHA on every device. Record SHA and
`versionName` in the evidence log when walks run.

## Scenario catalogue

### Block A — Prefer walk/bike (exit 1)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| A1 | Max strength + full ratio → 10× | `ExplorationMultiplier_MaxStrengthFullRatioYieldsTen` | 1 |
| A2 | Formula edges | `ExplorationMultiplier_RatioZeroYieldsOne`, `_MaxStrengthHalfRatioYieldsFivePointFive`, `_ZeroStrengthYieldsOneRegardlessOfRatio` | 1 |
| A3 | Prefer fully explored max strength | `ExplorationWeight_PreferFullyExploredMaxStrength` | 1 |
| A4 | Unexplored / Neither / Avoid multiplier | `ExplorationWeight_PreferUnexploredIsOne`, `_NeitherIsOneWhenExplored`, `_AvoidStoredDoesNotChangeMultiplier` | 1 |
| A5 | Prefer still uses soft multiplier | `StreetExplorationAvoid_PreferStillUsesSoftMultiplier` | 1 |
| A6 | Options persist / migrate (10 tests) | `StreetExplorationRoutingOptions_*` (10 UNIT_TESTs in `street_exploration_routing_options_tests.cpp`) | 1 |
| A7 | Walk/bike bind Avoid + include | `WalkingOptionsFragment` / `CyclingOptionsFragment` call `bindWithAvoid`; layouts include `include_street_exploration_prefer.xml` | 1 |
| A8 | Seekbar visible iff Prefer | `StreetExplorationPreferBinder`: `strengthContainer` visible only when Prefer is checked | 1 |
| A9 | Imported-only counts like live | `ExplorationWeight_ImportedOnlyCountsLikeLive` | 1 |
| A10 | Cross-leaf `.pix` + eviction | `ExplorationWeight_OverlayCountryDiffersButSegmentPixInstalled`, `_LeafPixEvictedAfterFileReplace` | 1 |
| A11 | Missing `.pix` → 1.0 | `ExplorationWeight_MissingPixIsOne` | 1 |
| A12 | Device Prefer walk/bike | Residual → Phase 10 | 1 |

### Block B — Avoid engine (exit 2)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| B1 | Mixed path kept; fully explored unused | `StreetExplorationAvoid_MixedPathKeptFullyExploredUnused` | 2 |
| B2 | Exclude iff fully explored | `ExplorationWeight_AvoidExcludesFullyExplored`, `_AvoidDoesNotExcludeMixed` | 2 |
| B3 | Imported-only excludes like live | `ExplorationWeight_AvoidImportedOnlyExcludesLikeLive` | 2 |
| B4 | Overlay mismatch still uses segment MWM `.pix` | `ExplorationWeight_AvoidUsesSegmentMwmPixWhenOverlayDiffers` | 2 |
| B5 | Car estimator does not exclude | `StreetExplorationAvoid_CarEstimatorDoesNotExclude`; `EdgeEstimator::AppliesAvoidExclusion` is Pedestrian \|\| Bicycle only | 2 |
| B6 | Spike connected + exclusion boundary | `StreetExplorationRoutingSpike_Connected`, `_AvoidExclusionBoundary` | 2 |
| B7 | Device Avoid possible route | Residual → Phase 10 | 2 |

### Block C — No-route + fallback (exit 3)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| C1 | All paths fully explored is distinct code | `StreetExplorationAvoid_AllPathsFullyExploredIsDistinctCode` | 3 |
| C2 | Convert Avoid no-path | `ConvertResult_AvoidNoPathIsAvoidExploredNoRoute` | 3 |
| C3 | Car / inactive Avoid no-path is `RouteNotFound` | `ConvertResult_CarOrInactiveAvoidNoPathIsRouteNotFound` | 3 |
| C4 | ToString distinct | `ToString_AvoidExploredNoRoute` | 3 |
| C5 | Spike forced cut | `StreetExplorationRoutingSpike_ForcedCut` | 3 |
| C6 | Java code 17 | `ResultCodes.AVOID_EXPLORED_NO_ROUTE = 17`; `RoutingBuildErrorTest` | 3 |
| C7 | Prefer fallback keeps strength | `StreetExplorationRoutingOptionsTest` preferFallback (5 tests) | 3 |
| C8 | No-route dialog | `RoutingErrorDialogFragment` + `ResultCodesHelper` case 17 | 3 |
| C9 | Driving-options error does not steal 17 | `RoutingController` / `RoutingBuildError.isDrivingOptionsBuildError` excludes code 17 | 3 |
| C10 | No min-connection search | No min-connection second search in routing/Android (SPD-042) | 3 |
| C11 | Device SP-058 script | Residual → Phase 10 | 3 |
| C12 | GPS off-route Prefer dialog | Residual. `CheckLocationForRouting` passes `nullptr` `removeRouteCallback`. **Do not implement.** | 3 |

### Block D — Warning (exit 4)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| D1 | Warning copy exact | `dialog_routing_avoid_explored_warning_message` = `This can produce very long routes or no available route.` | 4 |
| D2 | Warning before save; dismiss reverts | `StreetExplorationPreferBinder.bindWithAvoid`: dialog before `MODE_AVOID` save; `OnDismissListener` unchecks if not confirmed | 4 |
| D3 | `bind()` hides avoid container | `StreetExplorationPreferBinder.bind()` sets avoid container `GONE` (car) | 4 |
| D4 | Device SP-058 warning steps | Residual → Phase 10 | 4 |

### Block E — Mid-nav (exit 5)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| E1 | Research after painting remaining abandons path | `AvoidFollowStability_ResearchAfterPaintingRemainingAbandonsPath` | 5 |
| E2 | Traffic rebuild skipped while following Avoid | `TestTrafficRebuildSkippedWhileFollowingAvoidRoute` | 5 |
| E3 | Traffic rebuild runs when not following Avoid | `TestTrafficRebuildRunsWhenNotFollowingAvoidRoute` | 5 |
| E4 | Off-route / explicit rebuild still runs | `TestOffRouteRebuildStillRunsWhileFollowingAvoidRoute` | 5 |
| E5 | Early-return policy | `RoutingSession::RebuildRouteOnTrafficUpdate` returns when `IsFollowing() && IsOnRoute() && WasBuiltUnderAvoid()` | 5 |
| E6 | Pixel collection does not notify routing | Residual. No collection→routing notify in this tree. | 5 |
| E7 | Device Avoid follow | Residual → Phase 10 | 5 |

### Block F — Analytics (exit 6)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| F1 | Counters start / increment / neither | `StreetExplorationRoutingAnalytics_DefaultZero`, `_RecordPreferUsed`, `_RecordAvoidUsed`, `_NeitherDoesNotIncrement` | 6 |
| F2 | Fallback counter is distinct | `_RecordAvoidFallbackPrefer`, `_FallbackIsNotPreferUsed` | 6 |
| F3 | Persist / isolate | `_PersistRoundTrip`, `_ResetIsolatesTests` | 6 |
| F4 | Snapshot has no location keys | `StreetExplorationRoutingAnalytics_SnapshotHasNoLocationKeys` | 6 |
| F5 | AssignRoute increments | `TestAssignRouteIncrementsExplorationAnalytics` | 6 |
| F6 | Settings keys integers only | `settings::Set` / `TryGet` uint64 for the three analytics keys | 6 |
| F7 | Dialog records fallback | `RoutingErrorDialogFragment` calls `recordAvoidFallbackPrefer` on Prefer button | 6 |
| F8 | Not Sentry | Routing analytics are local settings counters, not a Sentry sink | 6 |
| F9 | Upload / debug readout | Residual → Phase 10 | 6 |
| F10 | AA toast not SPD-042 switch | Residual. Android Auto `CarToast` is trip-finished / unable-to-calc, not Avoid→Prefer switch. | 6 |

### Block G — Existing tests (exit 7)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| G1 | Full `routing_tests` | All pass; OK == Running | 7 |
| G2 | Full `street_pixels_tests` | All pass; OK == Running | 7 |
| G3 | Full `routing_common_tests` | All pass; OK == Running | 7 |
| G4 | `routing_integration_tests` | Not run (not required) | 7 |

### Block H — Non-goals

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| H1 | Car no Avoid | `DrivingOptionsFragment` calls `bind` not `bindWithAvoid` | — |
| H2 | §17.4 modes not added | Modes remain Neither / Prefer / Avoid only | — |
| H3 | No min-connection | Same as C10 | — |
| H4 | No Sentry sink | Same as F8 | — |
| H5 | No country allowlists | No city/country runtime restriction in Phase 6 routing | — |
| H6 | Prefer/Avoid not Pro-gated | No `explorer_pro::Capability` on prefer/avoid | — |
| H7 | iOS / public Explorer Pro purchasing | Out of Android V1 scope | — |

### Block I — Manual / device (all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| I1 | SP-058 strings exactly | `dialog_routing_avoid_explored_warning_message` and no-route / Prefer-button copy as locked | 3, 4 |
| I2 | Device Prefer walk/bike | Residual → Phase 10 | 1 |
| I3 | Device Avoid possible route | Residual → Phase 10 | 2 |
| I4 | Device SP-058 no-route + warning script | Residual → Phase 10 | 3, 4 |
| I5 | Device Avoid follow / GPS off-route | Residual → Phase 10 | 5 |
| I6 | All other device walks | Residual → Phase 10 | 1–5 |

### Block J — Automated suites (feeds all exits)

| ID | Scenario | Pass condition | Exit # |
| --- | --- | --- | --- |
| J1 | `routing_tests` full re-run | All pass; grep counts recorded | 2, 5, 6, 7 |
| J2 | `street_pixels_tests` full re-run | All pass; grep counts recorded | 1, 2, 7 |
| J3 | `routing_common_tests` full re-run | All pass; grep counts recorded | 7 |
| J4 | Android compile + JUnit | `:app:compileFdroidDebugJavaWithJavac` (native skipped); `:sdk:testDebugUnitTest` `RoutingBuildErrorTest` + `StreetExplorationRoutingOptionsTest` | 3, 4, 7 |

## Exit criteria mapping (fill in evidence log)

| # | Criterion | Evidence blocks |
| --- | --- | --- |
| 1 | Prefer-unexplored reachable and functional for walking and cycling | A + J2 + I → Pass + Residual device |
| 2 | Avoid-explored produces a route when one exists that skips fully explored edges | B + J1/J2 + I → Pass + Residual device |
| 3 | Distinct no-route + Prefer+seekbar control; never silent abandon (except C12) | C except C12 + J4 + I → Pass + Residual device + C12 |
| 4 | §17.3 warning shown before Avoid is used | D + I → Pass code + Residual device |
| 5 | Mid-navigation behaviour defined, implemented, stable | E + I → Pass skip-rebuild + Residual device/GPS |
| 6 | Routing analytics record mode usage with no location data | F → Pass local + Residual upload |
| 7 | Existing routing tests pass | G + J → Pass iff suites green |

## Non-goals for this gate

- GPS off-route Prefer dialog (C12 / R3).
- Analytics upload or in-app debug readout (F9 / R4 / R5).
- Pixel collection notifying routing (E6 / R2).
- Android Auto toast-as-switch (F10 / R6).
- City-scale Spike 7 (R1).
- Min-connection second search; car Avoid; §17.4 modes.
- Marking Phase 6 **Accepted** without maintainer decision.
- Weakening tests.
- Editing `docs/implementation/README.md`.
