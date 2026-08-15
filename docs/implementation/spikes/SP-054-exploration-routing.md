# SP-054 spike note — exploration-aware routing

**Work item:** [SP-054](../work-items/SP-054-routing-spike.md)  
**Date:** 2026-08-15  
**Branch:** `cursor/sp-054-routing-spike-35cf`  
**Scope:** Desktop synthetic graph measurement. No production behavior or user-facing routing change.

## Outcome

The test-only routing harness selected the expected route in every connected
mode and returned an explicit `NoPath` for the forced-cut Avoid case. Avoid
uses true `RoadAccess::Type::No` exclusion only when `exploredRatio == 1.0`.
The explored ratio represents `IsExplored()` under SPD-040.

The connected Avoid route was 300 m, a 100 m / 50% detour from the 200 m
standard route. Avoid no-route frequency was 0/101 timed runs for the connected
fixture and 101/101 for the forced-cut fixture.

## Fixtures

| Fixture | Route | Physical length | Explored ratio |
| --- | --- | ---: | --- |
| connected A | 0→1→4 | 200 m | 1.0, 1.0 |
| connected B | 0→2→4 | 300 m | 0.25, 0.25 |
| connected C | 0→3→4 | 800 m | 0.0, 0.0 |
| forced-cut A | 0→1→3 | 200 m | 0.0, 1.0 |
| forced-cut B | 0→2→3 | 240 m | 0.5, 1.0 |

Prefer applies `1 + strength / 100 * 9 * exploredRatio`. At strength 50 the
connected weighted costs are A=1100, B=637.5, and C=800. At strength 100 they
are A=2000, B=975, and C=800. The harness also verifies that ratio 1.0 is
excluded and ratio 0.999 is not.

## Results

Each row follows one warmup, a lookup-count reset, and 101 timed `FindPath`
runs. Every timed run was required to match the warmup status, path, and
physical length.

| Fixture | Mode | Status | Length | Median | p95 | Median extra vs standard | Lookup calls |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| connected | standard | OK | 200 m | 99 µs | 105 µs | 0 µs | 0 |
| connected | prefer 50 | OK | 300 m | 108 µs | 114 µs | +9 µs | 1,616 |
| connected | prefer 100 | OK | 800 m | 100 µs | 103 µs | +1 µs | 1,414 |
| connected | Avoid | OK | 300 m | 85 µs | 88 µs | -14 µs | 1,010 |
| forced-cut | standard | OK | 200 m | 83 µs | 87 µs | 0 µs | 0 |
| forced-cut | Avoid | NoPath | NA | 58 µs | 60 µs | -25 µs | 606 |

Raw result lines:

```text
SP054_RESULT fixture=connected mode=standard status=OK length_m=200 median_us=99 p95_us=105 extra_us=0 lookup_calls=0
SP054_RESULT fixture=connected mode=prefer_50 status=OK length_m=300 median_us=108 p95_us=114 extra_us=9 lookup_calls=1616
SP054_RESULT fixture=connected mode=prefer_100 status=OK length_m=800 median_us=100 p95_us=103 extra_us=1 lookup_calls=1414
SP054_RESULT fixture=connected mode=avoid status=OK length_m=300 median_us=85 p95_us=88 extra_us=-14 lookup_calls=1010
SP054_RESULT fixture=forced_cut mode=standard status=OK length_m=200 median_us=83 p95_us=87 extra_us=0 lookup_calls=0
SP054_RESULT fixture=forced_cut mode=avoid status=NoPath length_m=NA median_us=58 p95_us=60 extra_us=-25 lookup_calls=606
```

The synthetic desktop result is within the Spike 7 extra-latency bar: the
largest positive median and p95 extra was 9 µs, below 2 seconds. This does not
establish city-scale or device performance.

## Lookup-cost note

Across 101 timed calls, Prefer 50 made 16 provider lookups per `FindPath`,
Prefer 100 made 14, connected Avoid made 10, and forced-cut Avoid made 6.
`FindPath` validates bidirectional and unidirectional A* in the test utility, so
these counts cover both searches. The provider is an in-memory map; per-leaf
`.pix` lookup and cache cost remain unmeasured.

## Validation

The default sibling build directory was not writable in the Cloud VM. The
supported `-p` override placed the build under `/workspace/omim-build-debug`.
The repository initially lacked generated classification resources; after
`generate_drules.sh` generated them, the full routing binary completed.

```bash
./tools/unix/build_omim.sh -d -p /workspace routing_tests
set -o pipefail
./tools/unix/run_tests.sh -b /workspace/omim-build-debug \
  -f "StreetExplorationRoutingSpike_" 2>&1 | tee /tmp/sp054-routing-spike.log
/workspace/omim-build-debug/routing_tests \
  --data_path=/workspace/data --user_resource_path=/workspace/data
```

The focused run executed 3/3 spike tests and passed. The full
`routing_tests` binary executed 275 tests and reported `All tests passed`.
The originally specified anchored filter
`^StreetExplorationRoutingSpike_` matched no tests because this test framework
filters names in `filename.cpp::TestName` form; the unanchored test-name filter
above is the executed evidence.

## Residuals

- City-scale routing on an installed MWM with per-leaf `.pix` lookups remains a
  Phase 10 residual.
- Device routing latency, storage behavior, and battery impact remain a Phase
  10 residual.
