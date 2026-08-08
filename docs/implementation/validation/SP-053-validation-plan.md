# SP-053 — Validation plan (LAN `.spa` download via the app)

**Work item:** [SP-053](../work-items/SP-053-spa-lan-device-validation.md)
**Plan authored by:** Agent
**Plan date:** 2026-08-08
**Branch:** `cursor/sp-049-053-spa-publish-fe62`

## Approved decisions

| ID | Decision |
| --- | --- |
| Layout | CDN ≡ LAN (**SPD-035** / D8) |
| Advertise | Channel A preferred; Channel B temporary (**SPD-036**, **SPD-037**) |
| Custom URL | Never build default (**D12** / SP-004) |
| Device walks | Required when hardware available; else residual with what was tried |
| Phase 5 exit | Not declared Met by this WI; handoff to Phase 10 H1–H6 when S4/S7 Met |
| ADB push `.spa` | Not a pass method |

## Scope

Evidence that a Finland leaf `.spa` arrives through Custom Maps server →
SP-046, lands beside the MWM, loads in the exploration sidecar API. Fixes
blocking S1–S8 only; no new product scope.

## Device matrix

| Slot | Model | OS / skin | Region | Notes |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a (or same class as SP-014/033) | *(fill on walk)* | Finland / Helsinki | Required if available |
| D2 | Aggressive OEM | | | Optional; else Phase 10 |

**Build for walks:** same APK / git SHA on every device. Record SHA,
`versionName`, LAN URL, Channel A vs B, `dataVersion`, `MAP_SERIES` in the
evidence log.

## Scenario catalogue

| ID | Scenario | Pass condition | Auto / manual |
| --- | --- | --- | --- |
| S1 | Assemble FI publish tree (SP-050); serve (SP-051) | Health OK; curl spa 200 + exact bytes; Range 206 | Auto (unit) + curl |
| S2 | Device sets Custom Maps URL to LAN | URL persisted; native applied | Manual |
| S3 | Advertisement present (Channel A or B) | `HasRemoteSpa` for Helsinki leaf | Manual / log |
| S4 | Download or spa-retry Helsinki leaf | `.spa` OnDisk beside `.mwm`; SHA OK | Manual |
| S5 | Fail-soft: stop server mid-spa after Map OnDisk | Map usable; IncompleteSpa set | Manual |
| S6 | Retry after server restored | Spa recovers; incomplete cleared | Manual |
| S7 | Sidecar load | Areas non-empty; DisplayName ≠ MWM id; fail-closed if deleted | Manual |
| S8 | Delete map | `.spa` removed; personal `.pix`/`.spx` retained | Manual |
| S9 | Phase 5 smoke (optional) | Badge/focus/tap spot-check — or handoff Phase 10 | Manual / residual |

## Explicit non-goals

- Declaring Phase 5 exit Met.
- Quantitative Spike 1 FPS.
- Worldwide leaves.
- Fabricating device results.

## Automated baseline (required)

Re-run and record counts:

- `python3 -m unittest post_generation.tests.test_assemble_spa_publish_tree`
- `python3 -m unittest street_pixels.tests.test_serve_spa_publish_tree`
- Storage suites when binaries available:
  `Storage_SpaDownload*` / `Storage_SpaIncomplete*` / `Storage_SpaLifecycle*`

## Evidence log

Fill rows in [SP-053-evidence-log.md](SP-053-evidence-log.md). Agent does not
mark Accepted.
