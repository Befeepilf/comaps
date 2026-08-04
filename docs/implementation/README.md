# Street Pixels — Implementation Roadmap

**Document status:** Living project index
**Scope of this document:** Android public V1
**Last structural update:** 2026-07-25

This file is the index for Street Pixels implementation work. It intentionally
contains no implementation detail. Detail belongs in `phases/` and
`work-items/`.

---

## 1. Project documents and their authority

| Document | Authority | Rules |
| --- | --- | --- |
| `docs/STREET_PIXELS_PRODUCT_SPEC.md` | **Product source of truth.** Defines what the product is and what V1 must do. | Never changed as part of an implementation work item. Contradictions are reported, not silently resolved. |
| `docs/street-pixels-technical-audit.md` | **Dated implementation baseline** (audit date 2026-07-20). Describes the code as inspected on that date. | Not authoritative for current code. Where code and audit disagree, the code wins and the difference is recorded in the relevant phase file. |
| `docs/implementation/README.md` (this file) | **Roadmap and index.** Phase order, dependencies, entry/exit criteria, conventions, status. | Updated only after a work item merges, or when a phase boundary changes. |
| `docs/implementation/DECISIONS.md` | **Decision log.** Confirmed product and architecture decisions. | Append-only in practice. Superseding a decision adds a new entry and marks the old one superseded. |
| `docs/implementation/phases/*.md` | **Phase plans.** Objective, boundaries, dependencies, testing strategy, uncertainties. | Updated when a phase's understanding changes. |
| `docs/implementation/work-items/*.md` | **Work items.** One reviewable branch or pull request each. | Updated during and after implementation with evidence. |
| `.cursor/rules/*.mdc` | **Agent guardrails.** Scoped behavioural rules. | Link to these documents; do not duplicate the product spec. |
| `docs/CONTRIBUTING.md`, `docs/PR_GUIDE.md`, `docs/CPP_STYLE.md`, `docs/JAVA_STYLE.md` | **Upstream repository conventions.** | Followed unless a Street Pixels document explicitly overrides them. |

Where a phase file states a "current code location", that statement is a
snapshot. Re-verify before implementing.

---

## 2. Confirmed Android V1 scope

Public V1 targets **Android only**. The shared C++ core must remain suitable
for later iOS support, but no iOS work is a V1 release requirement.

In scope for public Android V1 (see product spec §5 and §34):

- Offline street-pixel map with red (unexplored) and green (explored) states.
- Explicit user-started recording sessions with start, pause, resume, finish.
- Background and screen-off recording where platform permission allows.
- GPS validation and safe interpolation; no false exploration across gaps.
- Permanent personal exploration state that survives map-data updates.
- Deterministic administrative exploration areas from a versioned,
  country-configured polygon pipeline, with settlement fallback.
- Area and city completion percentages.
- 25%, 50%, and 100% area milestones.
- Shareable 100% completion cards.
- Exploration-aware routing: prefer-unexplored **and** hard avoid-explored.
- Opt-in competition: pseudonymous identity, nickname, area ownership,
  server-side decay, weekly city leaderboard, aggregate-only uploads.
- Worldwide availability wherever compatible CoMaps map data exists.

---

## 3. Explicit post-V1 scope

The following are **not** V1 release requirements and must not be added to a
V1 exit criterion:

- **iOS public release**, StoreKit, iOS permission flows, iOS UI parity gates.
- **Public Explorer Pro purchasing**: Google Play Billing, purchase flow,
  purchase restoration, pricing, store entitlement validation.
- Country and world exploration percentages.
- Country and global public leaderboards.
- Cross-device accounts and profile recovery.
- Friends, groups, live locations, nearby-user discovery.
- Server-side anti-cheat and attested client statistics.
- Automatic map updates and server-side map-version normalization.

Explorer Pro **architecture** (feature flag plus entitlement abstraction) is in
V1 scope. Explorer Pro **purchasing** is not.

---

## 4. Implementation phases

| # | Phase | File | Status |
| --- | --- | --- | --- |
| 1 | Baseline and guardrails | [`phases/phase-01-baseline-and-guardrails.md`](phases/phase-01-baseline-and-guardrails.md) | Not started |
| 2 | Recording and collection correctness | [`phases/phase-02-recording-and-collection-correctness.md`](phases/phase-02-recording-and-collection-correctness.md) | Complete (OEM screen-off residual → Phase 10) |
| 3 | Exploration storage and map-update reconciliation | [`phases/phase-03-exploration-storage-and-reconciliation.md`](phases/phase-03-exploration-storage-and-reconciliation.md) | Complete (device-walk residual → Phase 10) |
| 4 | Administrative-area pipeline | [`phases/phase-04-administrative-area-pipeline.md`](phases/phase-04-administrative-area-pipeline.md) | In progress (SP-026 Accepted; SP-027 In review) |
| 5 | Area progress and map interaction | [`phases/phase-05-area-progress-and-map-interaction.md`](phases/phase-05-area-progress-and-map-interaction.md) | Not started |
| 6 | Exploration-aware routing | [`phases/phase-06-exploration-aware-routing.md`](phases/phase-06-exploration-aware-routing.md) | Not started |
| 7 | Milestones and share cards | [`phases/phase-07-milestones-and-share-cards.md`](phases/phase-07-milestones-and-share-cards.md) | Not started |
| 8 | Competition | [`phases/phase-08-competition.md`](phases/phase-08-competition.md) | Not started |
| 9 | GPX and feature gating | [`phases/phase-09-gpx-and-feature-gating.md`](phases/phase-09-gpx-and-feature-gating.md) | Not started |
| 10 | Android release hardening | [`phases/phase-10-android-release-hardening.md`](phases/phase-10-android-release-hardening.md) | Not started |

Phase order is unchanged from the originally proposed sequence. Repository
inspection did not reveal a dependency requiring phases to be moved or split.
Two adjustments were made **inside** phase boundaries and are recorded in the
relevant phase files:

- Phase 1 gains a Street Pixels test-harness work item, because no street-pixel
  tests exist and the existing `map_tests` target is excluded from the CI test
  run. Without it, the validation policy in §8 is not executable.
- Phase 2 must **introduce** live-sample interpolation before it can forbid
  interpolation across pauses and interruptions, because the current live
  collection path performs no interpolation at all.

### 4.1 Phase dependencies

```text
Phase 1  Baseline and guardrails
  │
  ├──> Phase 2  Recording and collection correctness
  │       │
  │       └──> Phase 3  Exploration storage and map-update reconciliation
  │               │
  │               ├──> Phase 4  Administrative-area pipeline
  │               │       │
  │               │       └──> Phase 5  Area progress and map interaction
  │               │               │
  │               │               ├──> Phase 7  Milestones and share cards
  │               │               └──> Phase 8  Competition
  │               │
  │               └──> Phase 6  Exploration-aware routing
  │
  └──> Phase 9  GPX and feature gating   (needs Phase 3 source flags)

Phase 10 Android release hardening   (needs every other phase)
```

Notes on the graph:

- Phase 3 is a hard prerequisite for Phases 4, 6, 8, and 9 because they all
  depend on a per-pixel `live` versus `imported` distinction and on a stable
  map-data version stamp.
- Phase 6 depends on Phase 3 only for the live/imported source flag; it does
  **not** depend on administrative areas and can run in parallel with Phase 4.
- Phase 8 depends on Phase 4 for area identifiers. If Phase 4 fails its exit
  criteria, Phase 8 cannot start; escalate to a product decision rather than
  substituting a different area model.
- Phase 9 can start as soon as Phase 3 lands. It is sequenced late only because
  it is not on the critical path.

### 4.2 Phase entry and exit criteria

Detailed criteria live in each phase file. The summary below is the version the
roadmap tracks.

| # | Entry criteria | Exit criteria |
| --- | --- | --- |
| 1 | Product spec and technical audit read; repository builds are attempted and the result is recorded. | Documented reproducible Android and desktop builds; a Street Pixels test target that runs in CI; telemetry defaults consistent with "private by default"; no developer-only network endpoint reachable from a release build; feature-flag and entitlement abstraction exists with all Pro flags off. |
| 2 | Phase 1 exit criteria met. | Pixel collection happens only in an active, non-paused recording session; session state machine with start/pause/resume/finish/discard; spec-conforming sample acceptance and interpolation with pause and interruption barriers; interrupted sessions detected and reported without gap filling; automated tests cover the acceptance pipeline; documented device validation exists. |
| 3 | Phase 2 exit criteria met. | Per-pixel ever-live vs imported-only bit and map-data version persisted; map updates rematch explored HEALPix identifiers instead of deleting them; path sampling unified at 15 m (SPD-019); migration is crash-safe and recoverable; reconciliation covered by automated tests. |
| 4 | Phase 3 exit criteria met; the area-pipeline spike has a recorded outcome. | Country-configured administrative polygons available to the client for at least one full country; deterministic pixel-to-area assignment; settlement fallback works; assignment is reproducible for a fixed map-data and policy version. |
| 5 | Phase 4 exit criteria met. | Focused-area behaviour matches the spec; area and city completion percentages are correct for the installed map version; area selection and completed-area visual state work; rendering performance validated at city scale. |
| 6 | Phase 3 exit criteria met; routing spike outcome recorded. | Prefer-unexplored exposed for walking and cycling; avoid-explored implemented with an explicit fallback offer; no silent abandonment of the selected rule. |
| 7 | Phase 5 exit criteria met. | 25/50/100 milestones fire once per area per threshold; completion card generated with no route, home, live location, or per-visit timestamp; share action is explicit. |
| 8 | Phase 4 exit criteria met; competition formulas resolved in `DECISIONS.md`. | Opt-in consent recorded with policy version and timestamp; aggregate-only uploads batched with delay and jitter; ownership, eligibility, decay, unclaimed and contested states work; weekly city leaderboard excludes imports; sparse-area anonymity enforced server-side; profile deletion works. |
| 9 | Phase 3 exit criteria met; Phase 1 feature-flag foundation in place. | GPX import marks pixels `imported` and never touches recency or competition queues; export gated by build flag plus entitlement; public builds present no non-functional purchase action. |
| 10 | All other phases at exit. | Every item in product spec §34 is verified with recorded evidence; store disclosures accurate; battery and rendering acceptable; no known path reveals another user's live or exact location. |

### 4.3 Current phase status

**Active phase: Phase 4 — Administrative-area pipeline.**

| Work item | Status |
| --- | --- |
| SP-001–014 | Accepted (Phases 1–2) |
| SP-015–022 | Accepted (Phase 3; device walks → Phase 10) |
| SP-023 | Accepted — Finland size/coverage spike |
| SP-024 | Accepted — SPD-020–025; entry store/locus Met |
| SP-025 | Accepted — country-config schema + loader |
| SP-026 | Accepted — `.spa` format + library + fixture tests |
| SP-027 | In review — offline `.spa` client load/verify API |
| SP-028–031 | Planned — assignment, settlement, rematch, validation |

Phase 4 entry criteria for polygon store and assignment locus are **Met**
(SPD-020, SPD-021). SP-024 Accepted 2026-08-03.

## 5. Release slices

A release slice is a coherent build that can be handed to someone. Slices are
not calendar milestones.

| Slice | Contents | Audience | Gate |
| --- | --- | --- | --- |
| **S1 — Correctness build** | Phases 1–3 | Maintainer only | Recording gate holds; no exploration is collected outside a session; map updates preserve exploration. |
| **S2 — Progress build** | Phases 4–5 | Maintainer plus a small internal group | Real administrative areas with correct percentages; acceptable rendering at city scale. |
| **S3 — Feature-complete beta** | Phases 6–9 | Closed beta | Routing, milestones, share cards, competition, and GPX behind flags all functional; competition backend reachable. |
| **S4 — Public Android V1** | Phase 10 | Public release | All product spec §34 launch requirements verified. |

Explorer Pro purchasing is not part of any V1 slice. iOS is not part of any V1
slice.

---

## 6. Branch, commit, and work-item conventions

**Work-item identifiers** are `SP-NNN`, allocated sequentially and never reused.
The identifier is stable even if the title changes.

**Integration branch:** all Street Pixels implementation work lands on `street-pixels`.
There is one checkout; do not create per-work-item branches or git worktrees unless
the maintainer explicitly asks for an isolated experiment.

**Scope per work item:** implement one work item at a time with a clean working tree
(no unrelated local modifications). Commits for a work item stack on `street-pixels`
in reviewable order — build fixes separate from docs when the work item requires it.

**Commits** follow `docs/PR_GUIDE.md` and match upstream CoMaps history on this
fork:

| Part | Rule |
| --- | --- |
| Subject | `[subsystem] Imperative summary` — e.g. `[routing] Align smoke tests with current penalty weights` |
| Prefixes | Lowercase name in brackets; several allowed: `[map][android]` |
| Mood | Imperative: Fix, Add, Align — not Fixed, Added, Aligned |
| Body | Optional; explain why; include `Work item: SP-NNN` when applicable |
| Sign-off | Required: `git commit -s` (`docs/CONTRIBUTING.md`) |
| Split | One idea per commit; docs vs code vs generated data in separate commits |

**Common prefixes:** `[android]`, `[map]`, `[routing]`, `[search]`, `[indexer]`,
`[generator]`, `[platform]`, `[drape]`, `[cmake]`, `[tools]`, `[docs]`,
`[strings]`, `[styles]`, `[ci]`.

- English only.
- Every commit compiles; whitespace and formatting changes are separate from
  logical changes.

Include the work-item identifier in the commit body or pull-request
description, for example `Work item: SP-007`.

**Pull requests** carry the work-item title, link the work-item file, and state
what and why rather than how.

**Cursor checkpoints are not version control.** A checkpoint is not a commit,
not a branch, and not evidence. Every work item ends with real Git history.

---

## 7. Rules for updating roadmap status

1. Status in this file is changed only by the human maintainer, and only after
   a work item is merged into the integration branch.
2. An agent may propose a status change in its report. It must not edit the
   status table itself unless explicitly instructed in that session.
3. Allowed work-item status values: `Not started`, `Planned`, `In progress`,
   `In review`, `Merged`, `Blocked`, `Superseded`.
4. Allowed phase status values: `Not started`, `In progress`, `Blocked`,
   `Exit criteria met`.
5. A phase moves to `Exit criteria met` only when every exit criterion in its
   phase file has recorded evidence. "Looks done" is not evidence.
6. When a work item merges, update in the same pass: the work item's status and
   completion-evidence fields, the phase file if its uncertainties changed, and
   `DECISIONS.md` if a decision was made or superseded.
7. Discovered follow-up work is recorded in the originating work item's
   discovered-follow-up field and, if it needs its own branch, becomes a new
   `SP-NNN` file. It is never silently folded into the current branch.

---

## 8. Validation policy

Every work item follows the same sequence. Steps are not skipped and not
reordered.

1. **Plan without editing.** Read the relevant spec sections, the phase file,
   the work item, and the actual current code. Produce a plan. Change nothing.
2. **Human review and approval of the plan.** Implementation does not start
   before approval.
3. **Implement on `street-pixels` with a clean working tree.** One work item at a
   time; no unrelated local modifications.
4. **Run focused tests.** The specific automated tests named in the work item.
5. **Run relevant regression tests.** At minimum the affected library's test
   target; for shared-core changes, the smoke suite.
6. **Inspect the complete diff.** Read every changed line, including files
   touched incidentally. Unrelated changes are reverted.
7. **Independent review in a fresh session.** A reviewer without the
   implementation context reviews the diff against the work item's acceptance
   criteria.
8. **Documented manual acceptance testing.** Execute the work item's manual
   validation steps on real hardware where the item requires it, and record
   device, OS version, build type, and observed result.
9. **Merge only after acceptance criteria pass.** Failing or unexecuted
   criteria block the merge.
10. **Update roadmap and decision records after merge.** Per §7.

Additional standing rules:

- Tests are never weakened, skipped, deleted, or narrowed to make an
  implementation pass. If a test blocks the change, the disagreement is
  reported.
- Test results are never summarised from memory or expectation. Only executed
  output counts.
- An implementing agent reports implementation and validation evidence. It does
  not declare a work item accepted.
- Work does not continue into the next work item automatically.

### 8.1 Commands this repository actually provides

| Purpose | Command | Source |
| --- | --- | --- |
| First-time configure | `./configure.sh` | `docs/INSTALL.md` |
| Desktop debug build with tests | `./tools/unix/build_omim.sh -d` | `docs/UNIT_TESTING.md` |
| Build one test target | `./tools/unix/build_omim.sh -d map_tests` | `docs/UNIT_TESTING.md` |
| Run a test suite | `./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke` | `tools/unix/run_tests.sh` |
| Run one test by name filter | `./tools/unix/run_tests.sh -b ../omim-build-debug -f "<regex>"` | `tools/unix/run_tests.sh` |
| CTest directly | `cd ../omim-build-debug && ctest -L "omim-test" --output-on-failure` | `docs/INSTALL_DESKTOP.md` (default `build_omim.sh` output dir; upstream doc says `cd build`) |
| Android debug APK | `cd android && ./gradlew assembleWebDebug` | `docs/INSTALL.md` |
| Android lint | `cd android && ./gradlew -Pandroidauto=true lint` | `.github/workflows/android-check.yaml` |
| C++ formatting check | `./tools/unix/clang-format.sh` | `.github/workflows/code-style-check.yaml` |

Known CI gap: Forgejo `linux-check.yaml` excludes most C++ unit suites via
`CTEST_EXCLUDE_REGEX`, and `.github/workflows/` has no C++ test job. For V1,
focused tests (including `street_pixels_tests`) are validated locally per
`docs/implementation/README.md` §8. Post-V1 generic C++ test CI is tracked as
follow-up from SP-002.

---

## 9. First recommended work items

Implement in this order. Each links to a file with full detail.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 1 | [SP-001](work-items/SP-001-reproducible-android-baseline.md) | Reproducible Android and desktop build baseline | 1 | **Accepted** — known-good build commands and baseline recorded. |
| 2 | [SP-002](work-items/SP-002-street-pixels-test-harness.md) | Street Pixels test harness | 1 | **Accepted** — lean `street_pixels_tests` target; local validation gate for Phase 2. |
| 3 | [SP-003](work-items/SP-003-privacy-and-telemetry-baseline.md) | Privacy and telemetry baseline | 1 | **Accepted** 2026-07-26 — Sentry private-by-default defaults, log scrub, manifest guard. |
| 4 | [SP-004](work-items/SP-004-network-egress-and-api-configuration.md) | Network egress inventory and API base configuration | 1 | **Accepted** 2026-07-26 — fail-closed API base, egress inventory; build and `street_pixels_tests` green. |
| 5 | [SP-005](work-items/SP-005-feature-flag-foundation.md) | Feature-flag and entitlement foundation | 1 | **Accepted** 2026-07-27 — capability + entitlement stub, Pro flags off, matrix tests green. |
| 6 | [SP-006](work-items/SP-006-recording-session-state-model.md) | Shared recording-session state model | 2 | **Accepted** 2026-07-27 — `RecordingSession` state machine + settings breadcrumb; no collection gate yet. |
| 7 | [SP-007](work-items/SP-007-pixel-collection-recording-gate.md) | Pixel-collection recording gate | 2 | **Accepted** 2026-07-27 — gate in `StreetPixelsManager::OnLocationUpdate`; 47/47 `street_pixels_tests` green. |

| 8 | [SP-008](work-items/SP-008-collection-radius-alignment.md) | Align collection radius with the specified 25 metres | 2 | **Accepted** 2026-07-27 — `kExploreRadiusMeters` 25 m; 51/51 `street_pixels_tests` green. |
| 9 | [SP-009](work-items/SP-009-live-sample-acceptance-filter.md) | Live sample acceptance filter | 2 | **Accepted** 2026-08-02 — `LiveSampleAcceptanceFilter` wired into collection; 71/71 `street_pixels_tests` green. |
| 10 | [SP-010](work-items/SP-010-pause-and-resume-semantics.md) | Pause and resume semantics | 2 | **Accepted** 2026-08-02 — track boundaries + filter reset on pause/resume; D2 live drape deferred; bus test → SP-014. |
| 11 | [SP-011](work-items/SP-011-segment-interpolation-with-barriers.md) | Segment interpolation with pause and interruption barriers | 2 | **Accepted** 2026-08-02 — 10 m segment sampling + barriers; 98/98 `street_pixels_tests`; device → SP-014. |
| 12 | [SP-012](work-items/SP-012-android-recording-controls.md) | Android recording controls and foreground-service integration | 2 | **Accepted** 2026-08-02 — Record Track drives shared session + FGS pause/resume; ABL not added; device matrix → SP-014. |
| 13 | [SP-013](work-items/SP-013-interrupted-session-recovery.md) | Interrupted-session detection and recovery | 2 | **Accepted** 2026-08-02 — cold-start force-finish + 60 s mid-session gap; 10 InterruptedSession tests; device → SP-014. |
| 14 | [SP-014](work-items/SP-014-recording-end-to-end-validation.md) | Recording end-to-end validation | 2 | **Accepted** 2026-08-03 — Pixel 3a checks pass; SP014-1 FAB fix; aggressive OEM / exit #7 → Phase 10. |

Phase 2 work items SP-001–014 are accepted. Validation plan:
[`validation/SP-014-validation-plan.md`](validation/SP-014-validation-plan.md).
Evidence: [`validation/SP-014-evidence-log.md`](validation/SP-014-evidence-log.md).
Phase 2 residual (aggressive-OEM screen-off continuity, smoke re-run, ABL
decision) tracked in Phase 10.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 15 | [SP-015](work-items/SP-015-pixel-file-format-and-map-version.md) | Pixel-file format version and map-data version header | 3 | Accepted — versioned `.pix` header + legacy migrate |
| 16 | [SP-016](work-items/SP-016-exploration-source-flag-store.md) | Per-pixel ever-live bit in `.pix` | 3 | Accepted — 1 ever-live bit; +0 B (SPD-015) |
| 17 | [SP-017](work-items/SP-017-crash-safe-map-update-rematch.md) | Crash-safe rematch on map update | 3 | Accepted — replaces wipe-on-download |
| 18 | [SP-018](work-items/SP-018-exploration-survives-map-delete.md) | Explored state survives map delete and redownload | 3 | Accepted — compact archive (SPD-016) |
| 19 | [SP-019](work-items/SP-019-derivation-sampling-alignment.md) | Unify path sampling at 15 m | 3 | Accepted — live/track → 15 m (SPD-019) |
| 20 | [SP-020](work-items/SP-020-eligibility-policy-alignment.md) | Eligibility vs spec §13 | 3 | Accepted — tighten + divergence register; OQ-5 closed |
| 21 | [SP-021](work-items/SP-021-denominator-recalc-and-update-messaging.md) | Denominator recalculation and §27.3 messaging | 3 | Accepted — rematch fraction toast; progress preserved |
| 22 | [SP-022](work-items/SP-022-exploration-storage-end-to-end-validation.md) | Exploration storage end-to-end validation | 3 | Accepted — plan + suite baseline; device walks → Phase 10 |

Phase 3 complete 2026-08-03. Decisions: SPD-015 (ever-live bit), SPD-016
(delete archive), SPD-017 (`nside` locked), SPD-018 (`.pixf` dead), SPD-019
(15 m sampling). Validation plan:
[`validation/SP-022-validation-plan.md`](validation/SP-022-validation-plan.md).
Evidence: [`validation/SP-022-evidence-log.md`](validation/SP-022-evidence-log.md).
Phase 3 residual (Pixel 3a / Uusimaa device walks, rematch timing on large
`.pix`) tracked in Phase 10.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 23 | [SP-023](work-items/SP-023-admin-polygon-size-spike.md) | Spike: admin polygon retention size and coverage | 4 | **Accepted** 2026-08-03 — Finland measurements; SP-024 inputs agreed |
| 24 | [SP-024](work-items/SP-024-area-pipeline-architecture-decisions.md) | Area-pipeline architecture decisions | 4 | **Accepted** 2026-08-03 — SPD-020–025 |
| 25 | [SP-025](work-items/SP-025-country-config-schema.md) | Versioned country-config schema | 4 | **Accepted** 2026-08-04 — `data/street_pixels/` JSON + `street_pixels_config` loader |
| 26 | [SP-026](work-items/SP-026-generator-true-polygons.md) | Generator: emit true closed exploration polygons | 4 | **Accepted** 2026-08-04 — per-MWM `.spa` + `street_pixels_areas` |
| 27 | [SP-027](work-items/SP-027-client-polygon-runtime-api.md) | Client runtime polygon API | 4 | **In review** — offline TryLoad/verify façade over `.spa` (SPD-020/021/025) |
| 28 | [SP-028](work-items/SP-028-pixel-to-area-assignment.md) | Deterministic pixel-to-area assignment | 4 | Planned — consume/verify precomputed subdivision map (SPD-021) |
| 29 | [SP-029](work-items/SP-029-settlement-fallback-and-no-area.md) | Settlement fallback and no-area state | 4 | Planned — true municipal rings (SPD-025); SPD-007 |
| 30 | [SP-030](work-items/SP-030-assignment-persistence-and-rematch.md) | Persist assignments and rematch hooks | 4 | Planned — sparse + rematerialize (SPD-022) |
| 31 | [SP-031](work-items/SP-031-area-pipeline-end-to-end-validation.md) | Area-pipeline end-to-end validation | 4 | Planned — Phase 4 exit gate; no numeric floor yet (SPD-024) |

Phase 4 entry investigation (2026-08-03) recorded in
[`phases/phase-04-administrative-area-pipeline.md`](phases/phase-04-administrative-area-pipeline.md).
Architecture decisions SPD-020–025 Accepted under SP-024 (2026-08-03).
SP-028 = subdivision assignment; SP-029 = settlement fallback / no-area.

Detailed work items exist for Phases 1–4. Later phases are broken down after
their entry criteria are met, and after any spike that phase depends on has a
recorded outcome.
