# SP-079 — Evidence log (Phase 8 exit)

**Plan:** [SP-079-validation-plan.md](SP-079-validation-plan.md)
**Branch:** `cursor/sp-079-phase8-end-to-end-validation-f95c`
**Status:** Evidence recorded — Phase 8 exit **awaiting maintainer** (agent does not mark exit Met)

## Build / automated baseline

| Field | Value |
| --- | --- |
| Date | 2026-08-26 |
| Client git SHA (suite run tip) | `b8fec313eacf28641e66b57b947a2c106cb6c804` (`[docs] Add SP-079 Phase 8 validation plan` on `cursor/sp-079-phase8-end-to-end-validation-f95c`; parent `dcad9abd2` `[docs] Accept SP-078 and start SP-079`) |
| Explorer git SHA | `a2875770bc72b68917b58356d17adfb39af2ea10` on `cursor/sp-077-nickname-moderation-deletion-f95c` (not `main`) |
| Build | `./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests street_pixels_areas_tests` → `/home/ubuntu/omim-build-debug/` (no dedicated `sp079_build.log`; binaries mtime 2026-08-26 19:55; suites started 19:55:47 against those binaries) |
| `data/classificator.txt` at run time | Present (`-rw-r--r--` 35452 bytes, 2026-08-25) |
| `adb devices` | `adb: command not found` — no handset |
| `IdentityStore_|FriendsManager_|ExploreStatsUpload_` | **31/31** All tests passed |
| `BackendConfig_|ExploreStatsUpload_|FriendsManager_` | **29/29** All tests passed |
| `CompetitionUpload_` | **22/22** All tests passed |
| `CompetitionOwnership_` | **18/18** All tests passed |
| `CompetitionDeletion_` | **2/2** All tests passed |
| `CompetitionSnapshot_` | **5/5** All tests passed |
| `CompetitionHint_` | **7/7** All tests passed (does **not** include HintCopy) |
| `CompetitionHintCopy_` | **2/2** All tests passed |
| `CompetitionCard_|CompetitionRanking_|CompetitionSparse_|CompetitionChrome_|CompetitionCopy_|CompetitionHintCopy_` | **15/15** All tests passed |
| `WeeklyCityLive_` (`street_pixels_tests`) | **12/12** All tests passed |
| `Competition` | **97/97** All tests passed |
| `street_pixels_areas_tests --filter='OwnershipScoring_|LiveRecency_|WeeklyCity'` | **37/37** All tests passed |
| Full `street_pixels_areas_tests` | **151/151** All tests passed |
| Full `street_pixels_tests` | **423/423** All tests passed (Eligibility ran; no abort) |
| Explorer `uv run pytest -q` | **108 passed**, 4 warnings (Pydantic class-based config deprecation) |
| Smoke / APK | Not run (agent desktop suites only) |
| Device walks / traffic capture | Deferred → Phase 10 |

### Suite command transcripts (counts)

Executed. `grep -c '^OK$'` matches `grep -c '^Running '` on each client log. Do not treat these as guesses.

```text
$ git -C /workspace rev-parse HEAD
b8fec313eacf28641e66b57b947a2c106cb6c804

$ git -C /workspace branch --show-current
cursor/sp-079-phase8-end-to-end-validation-f95c

$ adb devices || true
adb: command not found

$ git -C /home/ubuntu/explorer-src/explorer rev-parse HEAD
a2875770bc72b68917b58356d17adfb39af2ea10

$ git -C /home/ubuntu/explorer-src/explorer branch --show-current
cursor/sp-077-nickname-moderation-deletion-f95c

$ ls -la /workspace/data/classificator.txt
-rw-r--r-- 1 ubuntu ubuntu 35452 Aug 25 14:02 /workspace/data/classificator.txt

$ ./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests street_pixels_areas_tests
# no dedicated sp079_build.log; independent reviewer did not re-run the build
$ ls -la /home/ubuntu/omim-build-debug/street_pixels_tests /home/ubuntu/omim-build-debug/street_pixels_areas_tests
-rwxr-xr-x 1 ubuntu ubuntu 234387984 Aug 26 19:55 /home/ubuntu/omim-build-debug/street_pixels_tests
-rwxr-xr-x 1 ubuntu ubuntu  28307592 Aug 26 19:55 /home/ubuntu/omim-build-debug/street_pixels_areas_tests
# suites started 19:55:47 against those binaries (see per-suite START lines)

BIN=/home/ubuntu/omim-build-debug
DATA_ARGS="--data_path=/workspace/data --user_resource_path=/workspace/data"

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='IdentityStore_|FriendsManager_|ExploreStatsUpload_'
# grep -c '^OK$' → 31
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='BackendConfig_|ExploreStatsUpload_|FriendsManager_'
# grep -c '^OK$' → 29
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionUpload_'
# grep -c '^OK$' → 22
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionOwnership_'
# grep -c '^OK$' → 18
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionDeletion_'
# grep -c '^OK$' → 2
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionSnapshot_'
# grep -c '^OK$' → 5
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionHint_'
# grep -c '^OK$' → 7
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionHintCopy_'
# grep -c '^OK$' → 2
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='CompetitionCard_|CompetitionRanking_|CompetitionSparse_|CompetitionChrome_|CompetitionCopy_|CompetitionHintCopy_'
# grep -c '^OK$' → 15
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='WeeklyCityLive_'
# grep -c '^OK$' → 12
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS --filter='Competition'
# grep -c '^OK$' → 97
All tests passed.

$ "$BIN/street_pixels_areas_tests" $DATA_ARGS --filter='OwnershipScoring_|LiveRecency_|WeeklyCity'
# grep -c '^OK$' → 37
All tests passed.

$ "$BIN/street_pixels_areas_tests" $DATA_ARGS
# grep -c '^OK$' → 151
All tests passed.

$ "$BIN/street_pixels_tests" $DATA_ARGS
# grep -c '^OK$' → 423
# Eligibility_IncludesCommonHighways … ParkingAisleAndBuswayStillIncluded each OK
All tests passed.

$ cd /home/ubuntu/explorer-src/explorer && uv run pytest -q
108 passed, 4 warnings in 1.04s
```

Logs: `/opt/cursor/artifacts/sp079_baseline.log`, `sp079_suite_summary.log`, `sp079_identity_friends_upload.log`, `sp079_backend_config.log`, `sp079_competition_upload.log`, `sp079_competition_ownership.log`, `sp079_competition_deletion.log`, `sp079_competition_snapshot.log`, `sp079_competition_hint.log`, `sp079_street_pixels_hintcopy.log`, `sp079_competition_card_ranking_sparse.log`, `sp079_weekly_city_live.log`, `sp079_competition_all.log`, `sp079_areas_ownership_recency_weekly.log`, `sp079_areas_full.log`, `sp079_street_pixels_full.log`, `sp079_explorer_pytest.log`.

## Device roster

| Slot | Model | OS | Region | Walker |
| --- | --- | --- | --- | --- |
| D1 | Google Pixel 3a | — | Finland / Helsinki | Deferred Phase 10 (`adb` absent) |
| D2 | Aggressive OEM | — | — | Deferred Phase 10 |
| D3 | optional | — | — | Deferred Phase 10 |

Map screenshots remain **forbidden**. No walks, packet captures, or fabricated traffic were produced.

## Scenario results

| Scenario | Device / agent | Result | Notes |
| --- | --- | --- | --- |
| A1–A11 Consent off / boolean not consent / opt-out zero HTTP / friends hidden | agent | **Pass** | IdentityStore + CompetitionUpload + FriendsManager + Hint skipped-when-consented; `StreetPixels.CompetitionMapMode : 0` in suite output |
| A12 / M1 Opt-in walk vs §20.2 | — | **Residual** | No handset. Phase 10. SP-071 not accepted |
| B1–B5 Consent record version + timestamp + seed-on-grant | agent | **Pass** | IdentityStore grant tests + `CompetitionOwnership_GrantHandlerSeedsWithoutExplicitCall` |
| B6 Device consent record eyeball | — | **Residual** | Phase 10. SP-071 not accepted |
| C1–C15 Client nickname / claim / no friends headers | agent | **Pass** | IdentityStore **28** in N1 |
| C16–C26 Backend register uniqueness (SPD-059) | agent | **Pass** | explorer `test_register_*` in **108 passed** |
| C27 No email/password (code review) | agent | **Pass** (code) | Competition API `auth=None`; JSON `profile_id` |
| D1–D12 Client payload allow/deny, no `/stats/upload` | agent | **Pass** | CompetitionUpload **22/22** |
| D13–D17 Backend ingest schema reject | agent | **Pass** | `test_ingest_*` / `test_stats_upload_*` |
| D18 / M2 Traffic capture | — | **Residual** | Phase 10 |
| E1–E9 Cadence, jitter, offline flush, no retry-storm | agent | **Pass** | CompetitionUpload cadence/offline tests |
| E10–E11 / M3–M4 Opt-out walk, offline queue on device | — | **Residual** | Phase 10 |
| F1–F38 Ownership / eligibility / contested / unclaimed / card lines | agent | **Pass** | OwnershipScoring + LiveRecency + CompetitionOwnership + server clamp/contested + CompetitionCard |
| F39 Boss haptic | — | **Residual** | Out (SPD-054). Do not fail exit 6 |
| F40 Highway `Eligibility_*` | agent | **Pass** (env) | File present; 9 Eligibility tests OK in full suite. Not Phase 8 exit 6 |
| F41 `QueryCompetitionOwnership` loaded-span only | — | **Residual** | SP-072 / SP-074 |
| G1–G9 Server decay + contested-after-decay | agent | **Pass** | `test_decay_*` + contested-after-decay |
| G10 / M6 Decay without opening the app | — | **Residual** | Phase 10 |
| H1–H27 Weekly client + server week membership | agent | **Pass** | WeeklyCityLive **12** map + store/week in areas **37** filter; weekly pytest |
| H28 `city_timezones.py` empty dict | — | **Residual** | SP-076; SPD-060 keep |
| H29 Weekly GET JNI not wired | — | **Residual** | Later client WI / Phase 10. Not wired on this branch |
| H30 Ever-live-flip vs newly-explored-only §24.1 | — | **Residual** | SP-073 product lock |
| H31 Weekly crash window drops increment | — | **Residual** | SP-073 |
| I1–I14 Sparse N=0..4 server + client parse/copy | agent | **Pass** | area/weekly snapshot pytest + CompetitionSparse/Ranking/Snapshot |
| I15 / M5 `N < 3` on device | — | **Residual** | Phase 10 |
| J1–J20 Nickname filter / report / 7-day / admin reset | agent | **Pass** | IdentityStore report/rename + `test_nickname_moderation` + `test_reports` |
| J21 Client 7-day-gates after admin reset | — | **Residual** | SP-077 follow-up still open |
| J22 HTTP 409 `rename_limited` mapped to Collision | — | **Residual** | SP-077 |
| J23 V1 blocked-list small whole-token set | — | **Residual** | Expand later |
| J24 Unicode script-table vs server | — | **Residual** | Server remains authority |
| K1–K17 Delete/leave client + server | agent | **Pass** | CompetitionDeletion **2/2** + IdentityStore delete/leave + `test_profile_lifecycle` |
| K18 Failed `POST /leave` has no retry queue | — | **Residual** | SP-077 |
| K19 Revoke does not delete `live_recency.db` rows | — | **Residual** | SP-072 |
| K20 / M7 Delete on device; local exploration intact | — | **Residual** | Phase 10 |
| L1–L12 Copy deny-list, never nearby, chrome stale/offline, hint, schema | agent | **Pass** | CompetitionCopy/Sparse/Chrome/Hint/HintCopy; ingest extra lat/lon 422 |
| L13 `friends_signup_*` nickname toasts | — | **Residual** | Later copy cleanup; SPD-061 |
| L14 / M8 Presence eyeball | — | **Residual** | Phase 10. Map screenshots forbidden |
| M1–M8 Manual / device | — | **Residual** | `adb` absent. Phase 10 |
| N1–N14 Client suites | agent | **Pass** | Counts in baseline table |
| N15 Explorer pytest | agent | **Pass** | **108 passed**, 4 warnings |
| N16 Throttle smoke | agent | **Pass** | in explorer **108** |
| N17 Prod settings reject SQLite | agent | **Pass** (unit) | Deploy remains ops / Phase 10 |

## Phase 8 exit criteria table

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Competition is off by default and requires active confirmation separate from location permission | **Pass (automated + code review) + Residual (SP-071 not accepted; device walk Phase 10)** | A1–A11; N1; N2; M1 → Phase 10. Do not treat as Fail. Do not call SP-071 Accepted |
| 2 | The consent record includes the privacy-policy version and a timestamp | **Pass (automated + code review) + Residual (SP-071 not accepted; device walk Phase 10)** | B1–B5; N1; M1 → Phase 10 |
| 3 | Pseudonymous identity and nickname creation work with no email or password | **Pass (automated + code review) + Residual (SP-071 not accepted; device walk Phase 10)** | C1–C27; N1; N15. SPD-059 unique nicknames |
| 4 | Uploads contain only the spec §25.2 fields; the backend rejects anything else at the schema level | **Pass (automated) + Residual (device traffic capture Phase 10)** | D1–D17; N3; N15; M2 → Phase 10 |
| 5 | Upload cadence is at most once per 15 minutes plus jitter, with offline queueing | **Pass (automated) + Residual (device opt-out / offline queue Phase 10)** | E1–E9; N3; M3; M4 → Phase 10 |
| 6 | Ownership, eligibility, boss selection, contested and unclaimed states work | **Pass (automated) + Residual (boss haptic out SPD-054; QueryCompetitionOwnership loaded-span; device)** | F1–F38; N4; N12; N13; N15. Highway `Eligibility_*` not this exit (ran green because classificator present). Do not fail for SPD-054 |
| 7 | Server-side decay works between uploads | **Pass (automated) + Residual (decay-without-app device Phase 10)** | G1–G9; N15; M6 → Phase 10 |
| 8 | The weekly city leaderboard excludes revisits and imports and resets weekly | **Pass (automated) + Residual (Weekly GET JNI; city_timezones empty dict; SP-073 crash window / ever-live-flip)** | H1–H27; N10; N12; N15. JNI not wired on this branch |
| 9 | Sparse-area anonymity is enforced server-side | **Pass (automated) + Residual (`N < 3` device Phase 10)** | I1–I14; N6; N9; N15; M5 → Phase 10 |
| 10 | Nickname validation, filtering, reporting, administrative reset, and the seven-day rename limit work | **Pass (automated) + Residual (SP-077: client 7-day after admin reset; 409 rename_limited→Collision; small blocked-list; unicode table)** | J1–J20; N1; N15 |
| 11 | Profile and aggregate deletion works and leaves local exploration intact | **Pass (automated) + Residual (leave retry queue; live_recency rows on revoke; device delete Phase 10)** | K1–K17; N5; N15; M7 → Phase 10 |
| 12 | No surface reveals another user's live location, exact location, or presence | **Pass (automated) + Residual (friends_signup_* copy; presence eyeball Phase 10)** | L1–L12; N7; N8; N9; M8 → Phase 10. Map screenshots forbidden |

## Residuals

| ID | Finding | Disposition |
| --- | --- | --- |
| R1 | No handset: opt-in walk, traffic capture, opt-out zero upload, offline queue, `N < 3`, decay-without-app, delete+local intact, presence eyeball | Phase 10. Map screenshots remain forbidden. |
| R2 | SP-071 still In progress / not accepted | Maintainer; residual on exits 1–3 |
| R3 | friends_signup_* nickname toasts | Later copy cleanup; SPD-061 |
| R4 | Weekly GET not JNI-wired | Later client WI / Phase 10 |
| R5 | Boss haptic out | SPD-054 |
| R6 | CompetitionHint_ misses CompetitionHintCopy_ | Mitigated by extra filter (`sp079_street_pixels_hintcopy.log` **2/2**) |
| R7 | Client 7-day-gates after admin reset | SP-077 follow-up still open |
| R8 | HTTP 409 rename_limited mapped to Collision | Residual on SP-077 |
| R9 | Failed POST /leave has no retry queue | Residual on SP-077 |
| R10 | V1 blocked-list small whole-token set | Residual |
| R11 | Unicode script-table vs server | Server remains authority |
| R12 | city_timezones.py empty dict | SP-076; SPD-060 keep |
| R13 | Postgres production deploy | Ops / Phase 10 |
| R14 | Exact EU region string | SPD-062 ops |
| R15 | QueryCompetitionOwnership loaded-span only | SP-072 / SP-074 |
| R16 | Revoke does not delete live_recency.db rows | SP-072 |
| R17 | Weekly crash window drops increment | SP-073 |
| R18 | Ever-live-flip vs newly-explored-only §24.1 | SP-073 product lock |
| R19 | /stats/upload leftover client symbol | Leave unused |
| R20 | classificator.txt Eligibility abort if absent | **Not triggered this run.** File present; Eligibility 9 tests OK inside full `street_pixels_tests` **423/423**. Keep as environment residual if a later environment lacks the file. Do not weaken Eligibility. |
| R21 | Phase 8 exit Met? | Maintainer only |

Explorer pytest warnings: four `PydanticDeprecatedSince20` class-based `config` messages from the venv. Not a product fail.

Dirty tree left unstaged (never committed): `3party/healpix/healpix`, `data/area_milestones.db`, `data/live_recency.db`, `data/street_stats.db`, `data/weekly_city_live.db`.

## Phase 8 exit recommendation (agent)

Automated Blocks A–L (named client + explorer suites) are green on client SHA `b8fec313e` and explorer SHA `a287577`. Exit criteria 1–12 are **Pass** on shared-core / backend coverage with honest **residuals**: SP-071 not accepted (exits 1–3), device/manual M1–M8 → Phase 10 (R1), and the pre-filled owning-item residuals R3–R19. R20 (Eligibility abort) did not trigger because `data/classificator.txt` was present.

**Maintainer decides** whether Phase 8 exit is Met with residuals, or blocked pending SP-071 acceptance and/or device walks. Agent does **not** mark SP-079 or Phase 8 Accepted. Agent does **not** set phase Status to Exit criteria met.
