# Street Pixels — Implementation Roadmap

**Document status:** Living project index
**Scope of this document:** Android public V1
**Last structural update:** 2026-08-28

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
| 4 | Administrative-area pipeline | [`phases/phase-04-administrative-area-pipeline.md`](phases/phase-04-administrative-area-pipeline.md) | Exit criteria met (device residual → Phase 10) |
| 5 | Area progress and map interaction | [`phases/phase-05-area-progress-and-map-interaction.md`](phases/phase-05-area-progress-and-map-interaction.md) | In progress (phase-entry planning 2026-08-07) |
| 6 | Exploration-aware routing | [`phases/phase-06-exploration-aware-routing.md`](phases/phase-06-exploration-aware-routing.md) | Not started |
| 7 | Milestones and share cards | [`phases/phase-07-milestones-and-share-cards.md`](phases/phase-07-milestones-and-share-cards.md) | In progress (SP-069 In review; exit awaiting maintainer) |
| 8 | Competition | [`phases/phase-08-competition.md`](phases/phase-08-competition.md) | SP-070 Accepted; SP-071 in progress; SP-072–074 Accepted |
| 9 | GPX and feature gating | [`phases/phase-09-gpx-and-feature-gating.md`](phases/phase-09-gpx-and-feature-gating.md) | In progress (SP-081–087 Accepted; Phase 9 exit awaiting maintainer) |
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
| 6 | Phase 3 exit criteria met; routing spike outcome recorded. | Prefer-unexplored exposed for walking and cycling (with strength seekbar); avoid-explored excludes fully explored edges and falls back via an explicit Prefer switch; no silent abandonment of the selected rule. |
| 7 | Phase 5 exit criteria met. | 25/50/100 milestones fire once per area per threshold; completion card generated with no route, home, live location, or per-visit timestamp; share action is explicit. |
| 8 | Phase 4 exit criteria met; competition formulas resolved in `DECISIONS.md`. | Opt-in consent recorded with policy version and timestamp; aggregate-only uploads batched with delay and jitter; ownership, eligibility, decay, unclaimed and contested states work; weekly city leaderboard excludes imports; sparse-area anonymity enforced server-side; profile deletion works. |
| 9 | Phase 3 exit criteria met; Phase 1 feature-flag foundation in place. | GPX import marks pixels `imported` and never touches recency or competition queues; export gated by build flag plus entitlement; public builds present no non-functional purchase action. |
| 10 | All other phases at exit. | Every item in product spec §34 is verified with recorded evidence; store disclosures accurate; battery and rendering acceptable; no known path reveals another user's live or exact location. |

### 4.3 Current phase status

**Active phase: Phase 5 — Area progress and map interaction.**

Phase 4 is **complete** (Exit criteria met 2026-08-07) with residuals: R3
device walks → Phase 10; narrowed R1 production `.spa` shipping → **Phase 4
residual / pre-production packaging** (SP-042 / SPD-027–033; follow-ons
SP-043–048 — **not** Phase 5, **not** Phase 10 device work). Evidence:
[`validation/SP-031-evidence-log.md`](validation/SP-031-evidence-log.md).

| Work item | Status |
| --- | --- |
| SP-001–014 | Accepted (Phases 1–2) |
| SP-015–022 | Accepted (Phase 3; device walks → Phase 10) |
| SP-023 | Accepted — Finland size/coverage spike |
| SP-024 | Accepted — SPD-020–025; entry store/locus Met |
| SP-025 | Accepted — country-config schema + loader |
| SP-026 | Accepted — `.spa` format + library + fixture tests |
| SP-027 | Accepted — offline `.spa` client load/verify API |
| SP-028 | Accepted — consume/verify precomputed subdivision map |
| SP-029 | Accepted — settlement fallback / no-area (SPD-007/025) |
| SP-030 | Accepted — sparse `.spx` + rematerialize (SPD-022) |
| SP-031 | Accepted — Phase 4 exit validation (R3 → Phase 10) |
| SP-032 | Accepted — offline `spa_emit_tool` + shipping-encoder FI sizes |
| SP-042 | **Accepted** 2026-08-08 — sidecar shipping decisions (SPD-027–033); Phase 4 R1 packaging track |
| SP-043 | **Accepted** 2026-08-08 — blob contract freeze (**SPD-034**; `format_version` 2) |
| SP-044 | **Accepted** 2026-08-08 — production leaf `.spa` emit (Option B offline batch; closes R1 emit) |
| SP-045 | **Accepted** 2026-08-08 — optional `spa` / `spa_sha1_base64` in `countries.txt` (**SPD-028**) |
| SP-046 | **Accepted** 2026-08-08 — client leaf download fetches advertised `.spa` beside MWM (**SPD-027**, **SPD-031**) |
| SP-047 | **Accepted** 2026-08-08 — `.spa` full-refetch on update + delete-with-map (**SPD-029**, **SPD-030**) |
| SP-048 | **Accepted** 2026-08-08 — incomplete / retry signaling (**SPD-031**) |
| SP-049–053 | SP-049 **Accepted** 2026-08-08 (D8–D14 → **SPD-035–039**); SP-050–053 **In review** — LAN/CDN `.spa` publish mirror (device S2–S8 residual; not Phase 5 exit) |
| SP-033 | **Accepted** 2026-08-07 — qualitative Pixel 3a OK; quantitative Spike 1 → Phase 10 |
| SP-034 | **Accepted** 2026-08-07 — area completion cache + SPD-026 |
| SP-035 | **Accepted** 2026-08-07 — focused-area badge binding (map-centre stub → SP-036) |
| SP-036 | **Accepted** 2026-08-07 — Focus-selection engine (§12.5) |
| SP-037 | **Accepted** 2026-08-07 — Area boundary rendering and completion shading |
| SP-038 | **Accepted** 2026-08-07 — Area tap selection and focused-area detail surface |
| SP-039 | **Accepted** 2026-08-07 — City-scale aggregation and summary badge |
| SP-040 | **Accepted** 2026-08-07 — Completed-area visual + no-area empty state |
| SP-041 | Implemented, awaiting acceptance — Phase 5 end-to-end validation (**exit gate**) |

**Next: maintainer acceptance of SP-041 / Phase 5 exit** (evidence recorded;
device + Spike 1 quantitative residuals → Phase 10). Phase 5 entry
investigation:
[`phases/phase-05-area-progress-and-map-interaction.md`](phases/phase-05-area-progress-and-map-interaction.md).
Validation:
[`validation/SP-041-validation-plan.md`](validation/SP-041-validation-plan.md),
[`validation/SP-041-evidence-log.md`](validation/SP-041-evidence-log.md).

Phase 6 work-item planning (2026-08-15) may proceed in parallel: Phase 6
depends on Phase 3 only, not on Phase 5. SPD-040–045 are recorded. SP-054
is Accepted (desktop synthetic; city-scale/device residual → Phase 10).
Coding SP-056+ may proceed. See
[`phases/phase-06-exploration-aware-routing.md`](phases/phase-06-exploration-aware-routing.md).

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
| 27 | [SP-027](work-items/SP-027-client-polygon-runtime-api.md) | Client runtime polygon API | 4 | **Accepted** 2026-08-04 — offline TryLoad/verify façade over `.spa` |
| 28 | [SP-028](work-items/SP-028-pixel-to-area-assignment.md) | Deterministic pixel-to-area assignment | 4 | **Accepted** 2026-08-06 — consume/verify precomputed subdivision map |
| 29 | [SP-029](work-items/SP-029-settlement-fallback-and-no-area.md) | Settlement fallback and no-area state | 4 | **Accepted** 2026-08-06 — true municipal rings; SPD-007; client settlement PIP |
| 30 | [SP-030](work-items/SP-030-assignment-persistence-and-rematch.md) | Persist assignments and rematch hooks | 4 | **Accepted** 2026-08-07 — sparse `.spx` + rematerialize |
| 31 | [SP-031](work-items/SP-031-area-pipeline-end-to-end-validation.md) | Area-pipeline end-to-end validation | 4 | **Accepted** 2026-08-07 — Phase 4 exit; R3 device walks → Phase 10; narrowed R1 mapgen → pre-production |
| 32 | [SP-032](work-items/SP-032-phase4-residual-spa-emit.md) | Phase 4 residual: offline `.spa` emit | 4 | **Accepted** 2026-08-07 — `spa_emit_tool`; FI ~1.93 MiB / Helsinki ~0.44 MiB; 11/11 spot-check |

Phase 4 **Exit criteria met** 2026-08-07. Residuals: R3 device walks → Phase
10; narrowed R1 production `.spa` shipping → **Phase 4 residual /
pre-production packaging** (not Phase 5; not Phase 10 device work).
Investigation and architecture:
[`phases/phase-04-administrative-area-pipeline.md`](phases/phase-04-administrative-area-pipeline.md);
SPD-020–025 Accepted under SP-024. Validation:
[`validation/SP-031-validation-plan.md`](validation/SP-031-validation-plan.md),
[`validation/SP-031-evidence-log.md`](validation/SP-031-evidence-log.md).

### Phase 4 residual / pre-production packaging (`.spa` shipping)

Closes narrowed R1 (production mapgen emit + CDN leaf download / packaging).
**Not** a Phase 5 exit gate and **not** a Phase 10 device residual
(**SPD-033**). Decisions: **SPD-027–033** under SP-042.

| Order | ID | Title | Phase | Why |
| --- | --- | --- | --- | --- |
| 42 | [SP-042](work-items/SP-042-sidecar-shipping-decisions.md) | Sidecar shipping decisions | 4 residual | **Accepted** 2026-08-08 — SPD-027–033; product locks D1–D7 (2026-08-07) |
| 43 | [SP-043](work-items/SP-043-spa-blob-contract-freeze.md) | Freeze production `.spa` blob contract (`nside` / universe-order / `format_version`) | 4 residual | **Accepted** 2026-08-08 — **SPD-034** (`format_version` 2) |
| 44 | [SP-044](work-items/SP-044-production-spa-emit.md) | Production leaf `.spa` emit (Phase 4 R1; Option B offline batch) | 4 residual | **Accepted** 2026-08-08 — Option A mapgen collectors deferred |
| 45 | [SP-045](work-items/SP-045-countries-spa-meta.md) | Add optional `spa` / `spa_sha1_base64` leaf fields to `countries.txt` publish | 4 residual | **Accepted** 2026-08-08 — **SPD-028**; parse + publish inject; download = SP-046 |
| 46 | [SP-046](work-items/SP-046-spa-download-beside-mwm.md) | Client leaf download fetches advertised `.spa` beside MWM | 4 residual | **Accepted** 2026-08-08 — **SPD-027**, **SPD-031**; delete lifecycle = SP-047 |
| 47 | [SP-047](work-items/SP-047-spa-lifecycle-update-delete.md) | `.spa` full-refetch on map update and delete-with-map lifecycle | 4 residual | **Accepted** 2026-08-08 — **SPD-029**, **SPD-030** |
| 48 | [SP-048](work-items/SP-048-sidecar-shipping-validation.md) | Sidecar shipping validation and incomplete / retry signaling | 4 residual | **Accepted** 2026-08-08 — **SPD-031**; packaging track SP-042–048 |

**Continuation — LAN / CDN publish mirror (device enabler; plans 2026-08-08):**
closes SP-048 ops residual (serve advertised `.spa` on a production-shaped
HTTP tree). Needed because Android scoped storage blocks copying `.spa` onto
device for Phase 5 / Phase 10 Helsinki walks. Still **not** a Phase 5 exit gate
(**SPD-033**). Current-state note:
[`notes/spa-local-download-current-state.md`](notes/spa-local-download-current-state.md).

| Order | ID | Title | Phase | Why |
| --- | --- | --- | --- | --- |
| 49 | [SP-049](work-items/SP-049-spa-distribute-layout-decisions.md) | Publish-layout + LAN advertisement decisions | 4 residual | **Accepted** 2026-08-08 — D8–D14 locked (**SPD-035–039**; D9→SPD-028; D12→SP-004); production layout ≡ LAN; Channel A version bump |
| 50 | [SP-050](work-items/SP-050-spa-publish-tree-assemble.md) | Assemble CDN-identical publish tree (MWM + `.spa` + countries + meta) | 4 residual | **In review** — assemble tool + unit tests; human acceptance pending |
| 51 | [SP-051](work-items/SP-051-local-map-server-spa.md) | Local-network HTTP server for that layout | 4 residual | **In review** — Range GETs; `/health`; debug opt-in; human acceptance pending |
| 52 | [SP-052](work-items/SP-052-spa-countries-advertise-path.md) | Countries advertisement path (signed bump vs temporary bundle inject) | 4 residual | **In review** — Channel A/B recipes; no signature bypass; human acceptance pending |
| 53 | [SP-053](work-items/SP-053-spa-lan-device-validation.md) | LAN device validation — download `.spa` via the app | 4 residual | **In review** — plan + evidence; S1 Pass; S2–S8 residual (no device) |

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 33 | [SP-033](work-items/SP-033-city-scale-rendering-performance-spike.md) | Spike: city-scale street-pixel rendering performance | 5 | **Accepted** 2026-08-07 — qualitative Pixel 3a OK; quantitative Spike 1 → Phase 10 |
| 34 | [SP-034](work-items/SP-034-area-scoped-completion-computation.md) | Area-scoped completion computation and cache | 5 | **Accepted** 2026-08-07 — `AreaCompletionCache` + SPD-026 |
| 35 | [SP-035](work-items/SP-035-primary-progress-badge-focused-area.md) | Primary progress badge bound to focused area | 5 | **Accepted** 2026-08-07 — DisplayName + SP-034 %; map-centre stub → SP-036 |
| 36 | [SP-036](work-items/SP-036-focus-selection-engine.md) | Focus-selection engine (§12.5) | 5 | **Accepted** 2026-08-07 — five §12.5 rules + recording>pan; city zoom stub → SP-039 |
| 37 | [SP-037](work-items/SP-037-area-boundary-rendering-and-shading.md) | Area boundary rendering and completion shading by zoom | 5 | **Accepted** 2026-08-07 — additive overlay; keep one-circle-per-cell (SP-033) |
| 38 | [SP-038](work-items/SP-038-area-tap-selection-and-detail-surface.md) | Area tap selection and focused-area detail surface | 5 | **Accepted** 2026-08-07 — polygon PIP + detail sheet; sticky explicit focus |
| 39 | [SP-039](work-items/SP-039-city-scale-aggregation-and-summary-badge.md) | City-scale aggregation and summary badge | 5 | **Accepted** 2026-08-07 — city rollup explored/total (not avg %) |
| 40 | [SP-040](work-items/SP-040-completed-area-and-no-area-states.md) | Completed-area visual state and no-area empty state | 5 | **Accepted** 2026-08-07 — §18.6 completed chrome + §31 empty |
| 41 | [SP-041](work-items/SP-041-phase5-end-to-end-validation.md) | Phase 5 end-to-end validation | 5 | Implemented, awaiting acceptance — exit evidence; device/Spike1 → Phase 10 |

Phase 5 entry investigation (2026-08-07) recorded in
[`phases/phase-05-area-progress-and-map-interaction.md`](phases/phase-05-area-progress-and-map-interaction.md).
SP-033–SP-040 **Accepted** 2026-08-07. SP-041 exit evidence recorded (awaiting
maintainer Phase 5 exit decision). Quantitative Spike 1 → Phase 10.
SPD-026 locks personal completion.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 54 | [SP-054](work-items/SP-054-routing-spike.md) | Spike: exploration-aware routing measurement | 6 | **Accepted** 2026-08-15 — Spike 7 desktop synthetic; city-scale/device residual → Phase 10 |
| 55 | [SP-055](work-items/SP-055-routing-architecture-decisions.md) | Routing architecture decisions | 6 | **In review** — SPD-040–045 (OQ-2 closed; Prefer/Avoid; seekbar; fully-explored exclusion; Prefer fallback) |
| 56 | [SP-056](work-items/SP-056-prefer-unexplored-walk-bike.md) | Prefer-unexplored on walking and cycling surfaces | 6 | **Accepted** 2026-08-15 — Prefer + seekbar on walk/bike (SPD-041/045) |
| 57 | [SP-057](work-items/SP-057-avoid-explored-engine.md) | Avoid-explored engine (strict pass + distinct no-route) | 6 | **Accepted** 2026-08-15 — exclude `exploredRatio == 1` (SPD-042) |
| 58 | [SP-058](work-items/SP-058-avoid-fallback-and-warning.md) | Avoid warning, no-route UX, Prefer+strength fallback | 6 | **Accepted** 2026-08-16 — no min-connection search (SPD-042) |
| 59 | [SP-059](work-items/SP-059-mid-navigation-avoid-stability.md) | Mid-navigation stability when the route becomes explored | 6 | **Accepted** 2026-08-17 — skip traffic rebuild while following Avoid (SPD-043) |
| 60 | [SP-060](work-items/SP-060-routing-mode-analytics.md) | Count-only routing-mode analytics | 6 | **Accepted** 2026-08-18 — local counters; upload residual Phase 10 (SPD-044) |
| 61 | [SP-061](work-items/SP-061-phase6-end-to-end-validation.md) | Phase 6 end-to-end validation | 6 | **In progress** — exit gate |

Phase 6 entry investigation (2026-08-15) recorded in
[`phases/phase-06-exploration-aware-routing.md`](phases/phase-06-exploration-aware-routing.md).
SP-054 **Accepted** 2026-08-15. SP-055 **In review** (SPD-040–045 recorded;
OQ-2 closed). SP-056 **Accepted** 2026-08-15. SP-057 **Accepted** 2026-08-15.
SP-058 **Accepted** 2026-08-16. SP-059 **Accepted** 2026-08-17. SP-060 **Accepted** 2026-08-18.
SP-061 **Planned**. Spike 7 desktop synthetic is recorded;
city-scale/device residual → Phase 10. Coding SP-056+ may proceed.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 62 | [SP-062](work-items/SP-062-milestone-share-architecture-decisions.md) | Milestone and share-card architecture decisions | 7 | **Accepted** — SPD-046–055 (2026-08-19) |
| 63 | [SP-063](work-items/SP-063-milestone-state-tracking.md) | Milestone state tracking | 7 | **In review** — fire-once + §27.4 date |
| 64 | [SP-064](work-items/SP-064-first-100-metres-goal.md) | First-100-metres contextual goal | 7 | **Accepted** — 10 newly explored live pixels (2026-08-19) |
| 65 | [SP-065](work-items/SP-065-area-milestone-presentation.md) | Area milestone presentation (25 / 50 / 100) | 7 | **Accepted** — non-blocking 25/50/100 (2026-08-20) |
| 66 | [SP-066](work-items/SP-066-exploration-haptics-policy.md) | Exploration haptics policy | 7 | **Accepted** — recording ∧ foreground ∧ toggle (2026-08-20) |
| 67 | [SP-067](work-items/SP-067-completion-card-compositor.md) | Completion-card compositor | 7 | **Accepted** — deny-list model + rings outline (2026-08-20) |
| 68 | [SP-068](work-items/SP-068-share-flow-and-growth-analytics.md) | Share flow and growth analytics | 7 | **Accepted** — explicit image share; count-only (2026-08-20) |
| 69 | [SP-069](work-items/SP-069-phase7-end-to-end-validation.md) | Phase 7 end-to-end validation | 7 | **In review** — exit gate |

Phase 7 work-item planning (2026-08-19) recorded in
[`phases/phase-07-milestones-and-share-cards.md`](phases/phase-07-milestones-and-share-cards.md).
SP-062 **Accepted** (SPD-046–055, 2026-08-19). SP-063 **In review**.
SP-064 **Accepted** (2026-08-19). SP-065 **Accepted** (2026-08-20). SP-066 **Accepted** (2026-08-20). SP-067 **Accepted** (2026-08-20). SP-068 **Accepted** (2026-08-20). SP-069 **In review**. Phase 7 **In progress**; stylised-map entry criterion
**met** (SPD-046). Phase 5 exit (SP-041) is still the product prerequisite
for milestone UI coding.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 70 | [SP-070](work-items/SP-070-competition-architecture-decisions.md) | Competition architecture decisions | 8 | **Accepted** — SPD-057–066 (2026-08-23; unique nicknames) |
| 71 | [SP-071](work-items/SP-071-consent-and-competition-identity.md) | Consent record and competition identity | 8 | **In progress** — re-prompt; unique nickname; hide friends |
| 72 | [SP-072](work-items/SP-072-recency-and-ownership-scoring.md) | Recency store and ownership scoring | 8 | **Accepted** (2026-08-25) — SPD-057 / SPD-058 / SPD-063 |
| 73 | [SP-073](work-items/SP-073-weekly-city-new-live-pixels.md) | Weekly city new-live-pixel counting | 8 | **Accepted** (2026-08-25) — SPD-060 |
| 74 | [SP-074](work-items/SP-074-competition-upload-queue.md) | Competition upload queue | 8 | **Accepted** (2026-08-26) — 15 min + jitter; §25.2 allow-list |
| 75 | [SP-075](work-items/SP-075-backend-competition-core.md) | Backend competition core | 8 | **Accepted** (2026-08-26) — `competition/` ingest + decay |
| 76 | [SP-076](work-items/SP-076-backend-reads-and-sparse-anonymity.md) | Backend reads and sparse-area anonymity | 8 | **Accepted** (2026-08-26) — server-side N<3 nickname hide |
| 77 | [SP-077](work-items/SP-077-nickname-moderation-and-deletion.md) | Nickname moderation and profile deletion | 8 | **Accepted** (2026-08-26) — filter, report, 7-day rename, delete |
| 78 | [SP-078](work-items/SP-078-competition-ui-and-card-copy.md) | Competition UI, card copy, 30-pixel hint | 8 | **Accepted** (2026-08-26) — §22.10; SPD-066 |
| 79 | [SP-079](work-items/SP-079-phase8-end-to-end-validation.md) | Phase 8 end-to-end validation | 8 | In progress — exit gate |

Phase 8 work-item planning (2026-08-23) recorded in
[`phases/phase-08-competition.md`](phases/phase-08-competition.md).
SP-070 **Accepted** (SPD-057–066; unique nicknames per product-owner
override of spec §20.4). SP-071 **In progress**. SP-072 **Accepted**
(2026-08-25). SP-073 **Accepted** (2026-08-25). SP-074 **Accepted**
(2026-08-26). SP-075 **Accepted** (2026-08-26). SP-076 **Accepted** (2026-08-26). SP-077 **Accepted** (2026-08-26). SP-078 **Accepted** (2026-08-26). SP-079 in progress.

| Order | ID | Title | Phase | Why first |
| --- | --- | --- | --- | --- |
| 80 | [SP-080](work-items/SP-080-gpx-feature-gating-architecture-decisions.md) | GPX and feature-gating architecture decisions | 9 | **Planned** — G1–G10 / draft SPD-067–076; coding waits on G1, G5, G6, G7 |
| 81 | [SP-081](work-items/SP-081-dedicated-historical-import-pipeline.md) | Dedicated historical-import pipeline | 9 | **Accepted** (2026-08-28) — `ImportHistoricalTrack`; catch-all replay retired |
| 82 | [SP-082](work-items/SP-082-competition-isolation-historical-import.md) | Competition isolation on historical import | 9 | **Accepted** (2026-08-28) — `ImportHistoricalTrack` store assertions; four-gate matrix |
| 83 | [SP-083](work-items/SP-083-apply-pro-gate-to-gpx-surfaces.md) | Apply Pro gate to GPX surfaces | 9 | **Accepted** (2026-08-28) — call-site gate; debug entitle freeze; KML/KMZ stays |
| 84 | [SP-084](work-items/SP-084-gpx-settings-surface.md) | GPX settings surface | 9 | **Accepted** (2026-08-28) — settings rows on Enabled; G8 info on Available |
| 85 | [SP-085](work-items/SP-085-historical-import-robustness.md) | Historical-import robustness | 9 | **Accepted** (2026-08-28) — malformed reject; 10k/50k RSS; no chunking |
| 86 | [SP-086](work-items/SP-086-explorer-pro-monetisation-analytics.md) | Explorer Pro monetisation analytics | 9 | **Accepted** (2026-08-28) — count-only; Available gate; upload → Phase 10 |
| 87 | [SP-087](work-items/SP-087-phase9-end-to-end-validation.md) | Phase 9 end-to-end validation | 9 | **Accepted** (2026-08-28) — evidence recorded; Phase 9 exit awaiting maintainer |

Phase 9 work-item planning (2026-08-28) recorded in
[`phases/phase-09-gpx-and-feature-gating.md`](phases/phase-09-gpx-and-feature-gating.md).
Entry criteria Phase 3 + SP-005 are met. SP-081 **Accepted** (2026-08-28)
using recommended G1/G5 locks (OQ-20–OQ-29 still Open). SP-082 **Accepted**
(2026-08-28). SP-083 **Accepted** (2026-08-28). SP-084 **Accepted**
(2026-08-28). SP-085 **Accepted** (2026-08-28). SP-086 **Accepted**
(2026-08-28). SP-087 **In review** (evidence recorded; Phase 9 exit
awaiting maintainer). G1–G10 still Open. Phase 9 is **not** Exit criteria
met.

Detailed work items exist for Phases 1–9. Phase 10 is broken down after
other phases meet exit.
