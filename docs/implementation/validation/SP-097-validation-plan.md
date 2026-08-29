# SP-097 — Validation plan (Phase 10 / §34 verification pack)

**Work item:** [SP-097](../work-items/SP-097-phase10-launch-requirement-verification.md)
**Plan authored by:** Agent
**Plan date:** 2026-08-29
**Branch:** `cursor/sp-097-launch-verification-6383` (lands on `street-pixels`)
**Parent SHA (`street-pixels`):** `c9336737a3e085275e7806317774c98ea2808542`
  (`Merge branch 'cursor/sp-096-risk-register-6383'`)

**Automated suites + evidence mapping are in scope.** Device/manual
hardware observations are **Residual** (product-owner lock 2026-08-29;
**SPD-077**, **SPD-078**, **SPD-083**, **SPD-086**). Brand writing
(app name, listing copy, privacy/terms URLs) is **Residual**
(**SPD-080**, **SPD-084**; SP-093). Agent does **not** mark Phase 10
exit met and does **not** mark this work item Accepted.

Evidence log: [`SP-097-evidence-log.md`](SP-097-evidence-log.md).

## ID collision warning

| Series | What it is | This plan |
| --- | --- | --- |
| Phase 10 locks **H1–H10** | Launch-governance **SPD-077–086** | H10 / **SPD-086** = recorded local suites are the V1 gate |
| SP-041 scenarios **H1–H6** | Helsinki *UX* walks | Out of this item. Residual SP-095 |
| This plan **§34 groups** | Product spec §34 bullets | The catalogue below |

Do **not** copy Pixel 3a SP-014 results as Phase 10 **D2** close.

## Approved decisions

| ID | Decision |
| --- | --- |
| This slice | Mapping + H10 automated gate. No handset. No brand rewrite. No ABL. No analytics sink. |
| Device | Residual SP-094 / SP-095. Empty FPS / battery / OEM cells. |
| Brand | Residual SP-093 / **SPD-080** / **SPD-084**. Help stays `https://comaps.app/privacy/` and `terms/`. |
| Backend | Explorer checkout here is friends-only. Do not fake a competition schema. §26 #5 residual Ops (SP-096). |
| Tests | Never weaken, skip, delete, or narrow. Eligibility runs if `data/classificator.txt` exists; do not `--suppress=Eligibility` to fake a pass. |
| Lint | Record errors/warnings. Do not fix in this WI. |
| Fabrication | Forbidden. Only executed output. |

## How to evidence (methods)

| Method | When to use |
| --- | --- |
| **Automated SHA** | This branch’s executed `street_pixels_tests`, payload-shape filter, smoke, lint, clang-format |
| **Prior WI** | SP-014, SP-022, SP-031, SP-041, SP-061, SP-069, SP-079, SP-087 logs; SP-089–096 WIs |
| **Residual SP-094/095** | Battery, Spike 1, lifecycle, H1 matrix, OEM, Helsinki, traffic capture |
| **Brand SP-093** | Privacy/terms URLs, listing copy, app name |
| **Ops SP-096** | Signed APK, competition backend, N&lt;3 API, EU region |

## Automated gate commands (README §8.1 + this WI)

Build directory in this environment is `/workspace/omim-build-debug`
(not `../omim-build-debug` from the workspace root). README §8.1’s
`../omim-build-debug` is the upstream default when the repo is a
sibling of the build dir.

```
# Smoke (H10). Dies if a listed binary is missing.
./tools/unix/run_tests.sh -b /workspace/omim-build-debug -s smoke

# Street Pixels suite (not part of smoke). Requires generated classificator.
./omim-build-debug/street_pixels_tests \
  --data_path=/workspace/data --user_resource_path=/workspace/data

# Payload-shape (SP-091 / §34 analytics + no-location upload)
./omim-build-debug/street_pixels_tests \
  --data_path=/workspace/data --user_resource_path=/workspace/data \
  --filter='ProductAnalytics_ReleaseUploadPayloadsHaveNoLocation'

cd android && ./gradlew -Pandroidauto=true lint
./tools/unix/clang-format.sh
```

`data/classificator.txt` and `data/types.txt` are **gitignored
generated** files (root `.gitignore`). Generate via
`tools/unix/generate_drules.sh` (also `configure.sh` `DRULES_FILES`).
Do not invent or commit them.

Backend tests: only if the explorer checkout contains a `competition/`
app. This explorer (`main`) is friends-only (SP-096). Do not run
friends pytest as a stand-in.

## Public-build code confirmations (not a device walk)

Inspect, do not install:

- No purchase / billing surface (`BillingClient` / Play Billing absent).
- No ungated GPX (`explorer_pro` capabilities default false; freeze at
  `OrganicMaps` init; `GpxSettingsVisibility` gated).
- No friends surface (`FriendSettingsVisibility.friendsCapabilityEnabled()`
  returns false; add-friend filters removed).
- No city allowlist / pilot-only runtime gate.
- No known *client* path reveals another user’s live or exact location
  (upload allow-list + copy deny-list + payload-shape). Server residual
  if no competition app.

## §34 catalogue — Core map and exploration

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| C1 | Eligible OSM route filtering | Automated: `Eligibility_*` in `street_pixels_tests` (needs classificator). Prior SP-020. |
| C2 | Street pixels generated deterministically | Automated: HEALPix / path-sampling tests. Prior SP-008/011/019. |
| C3 | Red and green persist locally | Automated: `.pix` / ever-live / rematch. Prior SP-015–018, SP-022. Device permanence residual SP-095. |
| C4 | 25-metre collection radius | Automated: `kExploreRadiusMeters` + radius tests. Prior SP-008. |
| C5 | Imported vs live distinguishable | Automated: `EverLive_*`, historical-import isolation. Prior SP-016, SP-081/082. |
| C6 | Area percentages for installed map version | Automated: area completion cache. Prior SP-034/041. Device Helsinki residual SP-095. |
| C7 | Versioned country-specific admin config | Automated: `CountryConfig_*`. Prior SP-025/031. |
| C8 | Personal exploration wherever compatible data | Code: no city allowlist. Pipeline worldwide. Device residual SP-095. |
| C9 | No city allowlist / pilot-only runtime | Code inspection this SHA. `spa_jsonl` Helsinki ids are a spot-check, not an allowlist. |

## §34 catalogue — Recording

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| R1 | Start, pause, resume, finish | Automated: `RecordingSession_*`, `PauseResume_*`, `CollectionGate_*`. Prior SP-006/010/014. Device residual SP-095. |
| R2 | Background where permission permits | Residual SP-095 / **SPD-082** (ABL absent). Do not copy Pixel 3a as D2. |
| R3 | Screen off | Residual SP-095. Protocol SP-094 Session A is measurement, not OEM close. |
| R4 | Session state clearly visible | Implementation SP-012/090. Device eyeball residual SP-095. |
| R5 | Interrupted sessions no false connecting lines | Automated: `InterruptedSession_*`, `SegmentInterpolation_Barrier_*`. Prior SP-011/013. |
| R6 | Tracks inspect and delete locally | Automated: gps-track / bookmark tests. Device residual SP-095. |

## §34 catalogue — GPS integrity

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| G1 | Poor-accuracy samples rejected | Automated: `LiveSampleAcceptance_Accuracy26_Rejected` (25 m). Prior SP-009. |
| G2 | Implausible speed rejected | Automated: `LiveSampleAcceptance_ImpliedSpeed55Kmh_Rejected`. |
| G3 | Long jumps rejected | Automated: `LiveSampleAcceptance_Teleport_Rejected` (200 m). |
| G4 | Normal valid samples interpolated safely | Automated: `SegmentInterpolation_WithinCaps_*`, walking/cycling sequences. Prior SP-011. |
| G5 | Signal loss does not paint a straight line | Automated: barriers after rejection / interruption. |
| G6 | Pause and resume do not create connecting segments | Automated: `SegmentInterpolation_Barrier_AfterPause_*`, `PauseResume_TrackBoundary_*`. |

## §34 catalogue — Progress experience

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| P1 | First-use guidance | SP-090 §10 script. Device click-through residual SP-095. |
| P2 | Area focus predictable | Automated: focus-selection tests. Prior SP-036/041. Device residual SP-095. |
| P3 | 25 / 50 / 100% milestones | Automated: `AreaMilestone_*`. Prior SP-065/069. Device residual SP-095. |
| P4 | Completed areas clear visual | Automated overlay + SP-089 check glyph. Device residual SP-095. |
| P5 | City-level aggregate progress | Automated: city rollup. Prior SP-039/041. Device residual SP-095. |
| P6 | No achievement-history screen required | Code: none added (spec §35). Pass by absence. |

## §34 catalogue — Routing

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| T1 | Standard routing works | Smoke `routing_tests` / prior SP-061. Device residual SP-095. |
| T2 | Prefer-unexplored works | Automated: Prefer + seekbar tests. Prior SP-056/061. Device residual SP-095. |
| T3 | Avoid-explored impossible routes clearly; no silent ignore | Automated: Avoid no-route + SP-089 Prefer control. Prior SP-057/058. Device residual SP-095. |

## §34 catalogue — Privacy and competition

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| K1 | Competition is opt-in | Automated: `IdentityStore_*` consent. Prior SP-071/079. Device residual SP-095. |
| K2 | Consent separate from location permission | Code: `ExploreConsentDialogFragment` vs `track_recording_location_rationale`. SP-090. |
| K3 | No raw GPS uploaded | Automated: payload-shape + competition allow-list. `ShouldAttemptStatsUpload()` false. |
| K4 | Aggregate uploads delayed and batched | Automated: 900 s + 900 s jitter. Prior SP-074. |
| K5 | Exact-location sharing absent | Client copy deny-list + no feature. Server residual if no competition app. |
| K6 | Nearby-user discovery absent | Public friends surface hidden (SP-092 / **SPD-085**). Explorer `Friendship` still exists; not a public APK surface. |
| K7 | Pseudonymous identity creation | Automated: `IdentityStore_*`. Prior SP-071. |
| K8 | Nickname restrictions and reporting | Client tests SP-077. Server residual Ops (this explorer). |
| K9 | Rename within stated limits | Client tests SP-077. Server residual Ops. |
| K10 | Users can delete competition data | Client tests SP-077/089 (recency drop). Server residual Ops. |
| K11 | Ownership calculations | Automated: `CompetitionOwnership_*`. Prior SP-072. |
| K12 | Server-side decay | Residual Ops. No `competition/` app in explorer `main` here (SP-096 §26 #5). |
| K13 | Areas can become unclaimed | Client scoring + snapshot flags. Server residual Ops. |
| K14 | Sparse ranking states preserve privacy | Client boss-line hide is not protection. Server N&lt;3 residual Ops. |
| K15 | Weekly city rankings exclude imports | Automated: `WeeklyCityLive_ImportOnlyDoesNot*`, isolation tests. Prior SP-073/082. |

## §34 catalogue — Offline and map updates

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| O1 | Manual map updates work | Automated rematch. Prior SP-017/022. Device residual SP-095. |
| O2 | Users warned percentages may change | Automated §27.3 toast signal. Prior SP-021. Device residual SP-095. |
| O3 | Statistics recalculated after updates | Automated rematch denominator. Prior SP-021/022. |
| O4 | Competition uploads include map-data versions | Payload-shape allow-list includes `map_data_version`. Prior SP-074. |
| O5 | Offline competition updates queue then sync | Automated upload queue. Device/server residual SP-095/096. |

## §34 catalogue — Sharing

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| S1 | Completion cards without routes / exact / home / timestamps | Automated deny-list. Prior SP-067/069. Device residual SP-095. |
| S2 | Cards work without a competition profile | Automated: nickname omitted when empty. Prior SP-067. |
| S3 | Share-card at 100% area completion | Automated: `CompletionCardShare_PrepareFailsWithoutHundredPercent`. Prior SP-068. Device residual SP-095. |

## §34 catalogue — Explorer Pro and monetization

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| E1 | Feature gates / entitlement abstraction for later paid activation | Automated: `ExplorerPro_*`, `GpxGate_*`. Prior SP-005/080–087. |
| E2 | Public builds no non-functional purchase action | Code: no Billing. Capabilities default false; freeze. Prior SP-087/092. |
| E3 | Imported tracks cannot affect competition regardless of flags | Automated isolation matrix. Prior SP-082/087. |
| E4 | No Play Billing / purchase / restore | Code inspection. **SPD-010**. |

## §34 catalogue — Release governance

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| L1 | Privacy policy describes local vs uploaded | Residual SP-093 / **SPD-080**. Help still CoMaps URLs. |
| L2 | Competition consent text matches behaviour | Residual SP-093 landing. In-app `explore_consent_message` snapshot in SP-093. |
| L3 | Terms cover public nicknames and rankings | Residual SP-093 / **SPD-080**. |
| L4 | Competition-profile deletion operational | Client Pass; server Residual Ops. |
| L5 | Nickname moderation and administrative reset operational | Residual Ops (no competition app here). Client SP-077. |
| L6 | Android store permissions and background-location disclosures accurate | SP-092 inventory + `play-data-safety.md`. ABL absent **SPD-082**. Listing brand residual **SPD-084**. |
| L7 | Analytics contain no raw location data | Automated payload-shape this SHA. Local uint64 only (**SPD-081**). |

## §34 catalogue — Quality

| ID | Spec bullet | How to evidence |
| --- | --- | --- |
| Q1 | Rendering performance acceptable | Residual SP-094 Spike 1. Do not invent FPS. |
| Q2 | Battery during active recording acceptable | Residual SP-094 / **SPD-078**. No %/hour ceiling. |
| Q3 | Foreground haptics can be disabled | Automated: `ExplorationHaptic_*_ToggleOff_*`. Prior SP-066. Device residual SP-095. |
| Q4 | No critical exploration-data loss | Automated rematch/interrupt. Device lifecycle residual SP-094 L1–L9. |
| Q5 | No known path reveals another user’s live or exact location | Client: payload-shape + deny-list + friends hidden. Server residual Ops. |

## Phase 10 exit criteria 1–11 (mapping only)

Do **not** mark exit met. Fill pass / fail / residual in the evidence log.

1. Every §34 line verified with recorded evidence (device residual).
2. Every §31 state implemented and observed (device observation residual; SP-090 copy).
3. Settings match §30 (URL/app-name residual).
4. Analytics match §32, no location (SP-091 + payload-shape; no sink).
5. Privacy/terms/consent/store match behaviour (brand residual).
6. Battery measured and accepted (protocol residual).
7. Rendering meets Spike 1 (protocol residual).
8. No critical data-loss across lifecycle (protocol residual).
9. No known path reveals another user’s live/exact location.
10. Every audit risk has a stated position (SP-096 table).
11. Store signing produces an installable artefact (residual Ops).

## Out of scope

- Handset walks, Spike 1 numbers, battery %/hour.
- Brand rewrite, ABL add, analytics sink.
- Fixing lint or glaze/`platform_tests` compile in this WI.
- Marking Phase 10 Complete or this item Accepted.
