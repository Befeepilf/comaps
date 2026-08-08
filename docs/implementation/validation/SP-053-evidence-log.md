# SP-053 — Evidence log (LAN `.spa` download)

**Plan:** [SP-053-validation-plan.md](SP-053-validation-plan.md)
**Branch:** `cursor/sp-049-053-spa-publish-fe62`
**Status:** Automated S1 + unit baselines recorded; **device S2–S8 residual**
(no hardware / signing key in this agent session). Maintainer decides acceptance.

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-08 |
| Git SHA | tip of `cursor/sp-049-053-spa-publish-fe62` (see commits) |
| Assemble tests | **6/6** OK |
| Serve tests | **15/15** OK |
| Storage Spa* suites | **17/17** OK (`storage_tests --filter=Spa`) |
| Device walks | Residual — no phone attached to this cloud agent |

### Suite command transcripts

```text
$ cd tools/python && python3 -m unittest post_generation.tests.test_assemble_spa_publish_tree street_pixels.tests.test_serve_spa_publish_tree
Ran 21 tests in ~3s
OK

$ ./omim-build-debug/storage_tests --filter=Spa
… (17 × OK) …
All tests passed.
# grep -c '^OK$' → 17
```

### S1 curl parity (agent, temp tree)

```text
# After assemble fixture + serve on 127.0.0.1:<port>:
GET /health → 200 {"ok": true, "map_series": "...", "data_version": ..., "spa_leaf_count": N}
GET /maps/{series}/{ver}/{leaf}.spa → 200 + exact bytes
GET with Range: bytes=0-3 → 206 + Content-Range
GET /debug/inventory without --enable-debug-routes → 404
```

Covered by `street_pixels.tests.test_serve_spa_publish_tree`.

## Device roster

| Slot | Model | OS | Channel | Walker |
| --- | --- | --- | --- | --- |
| D1 | — | — | — | Residual: no device in agent environment |
| D2 | — | — | — | Deferred Phase 10 |

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| S1 Assemble + serve | agent | **Pass** | SP-050 + SP-051 unit tests |
| S2 Custom Maps URL | — | **Residual** | No device |
| S3 Advertise HasRemoteSpa | — | **Residual** | Needs Channel A key or Channel B local APK |
| S4 Spa OnDisk + SHA | — | **Residual** | Blocked on S2–S3 |
| S5 Fail-soft IncompleteSpa | — | **Residual** | No device |
| S6 Retry recover | — | **Residual** | No device |
| S7 Sidecar load | — | **Residual** | No device; also needs real FI `.spa` (SP-044 `.pix` residual) |
| S8 Delete map keeps pix | — | **Residual** | No device; Storage lifecycle unit coverage exists from SP-047 |
| S9 Phase 5 smoke | — | **Residual** | Handoff Phase 10 when S4/S7 Met |

## Residuals / handoff

| Finding | Disposition |
| --- | --- |
| No hardware on cloud agent | Maintainer / local walk fills S2–S8 |
| Leaf `.pix` for dense FI emit | SP-044 residual — blocks real Helsinki blob for S7 |
| Channel A signing key | Maintainer-held; recipe in `notes/spa-advertise-channels.md` |
| Phase 5 H1–H6 Helsinki walks | Phase 10 (SP-041 R1) once S4/S7 Met |
| Incomplete spa Android chrome | Phase 10 |

## Pointers

- Recipes: `docs/implementation/notes/spa-advertise-channels.md`
- Deploy: `docs/DEPLOY_OWN_MAP_SERVER.md` (Street Pixels section)
- Client path: SP-046–048 Accepted
