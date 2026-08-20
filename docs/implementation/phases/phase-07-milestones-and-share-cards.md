# Phase 7 — Milestones and share cards

**Status:** In progress (SP-062 Accepted 2026-08-19; SP-063+ coding underway)
**Depends on:** Phase 5
**Blocks:** nothing; required for release

---

## Objective

Add the restrained reward layer: a first-100-metres goal, area milestones at
25%, 50%, and 100%, and a shareable completion card that proves an
accomplishment without exposing where the user lives or where they went.

## Product-spec references

- §10 step 6 first goal, step 8 first haptic, step 9 first goal completes.
- §18.1–§18.6 Area milestones, per-threshold behaviour, no achievement history,
  completed visual state.
- §19.1–§19.3 Shareable completion cards: contents, exclusions, anonymous
  sharing, share flow.
- §22.10 Completion does not guarantee ownership, and the two completion
  messages.
- §27.4 Previous completion after a map update.
- §28.1–§28.4 Haptics: foreground and recording only, one pulse per collecting
  update, stronger patterns for milestones, single settings toggle.
- §32.4 Growth analytics: completion card generated, share action initiated.
- §34 "Sharing" and "Progress experience" launch requirements.

## Technical-audit references

- §5 Feature-reuse matrix: completion-card generation **Not found**; only
  generic track sharing exists.
- §16 Monetization and sharing: no neighbourhood share compositor.
- §18 UI architecture implications.

## Current code locations

Re-verified 2026-08-19 against the working tree (Phase 7 work-item planning).
SP-062 re-verify (2026-08-19) confirmed this table. Extras in
[`notes/SP-062-milestone-share-architecture.md`](../notes/SP-062-milestone-share-architecture.md):
`qt/screenshoter.*` is desktop QA, not a card path;
`Framework::EnterForeground` / `EnterBackground` call
`StreetPixelsManager::SetApplicationForeground` (SP-066).

| Concern | Location | Observed state |
| --- | --- | --- |
| Collection haptic | `StreetPixelsManager::TriggerCollectionVibration` / `PlayExplorationHaptic` from `OnLocationUpdate` | One 50 ms collection pulse when ≥1 new pixel **and** SPD-054 predicate (recording ∧ foreground ∧ toggle). Same-update first-goal complete plays `FirstGoalComplete` instead. Tests intercept via `VibrationHandler(ExplorationHapticKind)`. |
| Platform vibrate | `libs/platform/vibration.cpp` → Android `Utils.vibrate` / `vibratePattern` | Desktop is a no-op. Milestone waveforms in `exploration_haptics.cpp`. |
| Foreground signal | `OrganicMaps.nativeOnTransit` → `Framework::EnterForeground` / `EnterBackground` | Forwards to `StreetPixelsManager::SetApplicationForeground`. Default **false**. Play is gated; collection itself is not. |
| Haptics setting | `prefs_interface.xml`, `Config.explorationHapticsEnabled()`, C++ `StreetPixels.ExplorationHaptics` | Interface switch. Absent key → on. |
| Area completion % | `AreaCompletionCache`, `StreetPixelsManager::GetAreaCompletion` | Explored/total including imported (SPD-026). Invalidated on collect/import/rematch. **No** fired-once state, **no** original 100% date. |
| Focused-area badge | `FocusedAreaProgress` + `MapButtonsController.mExplorationBadge` | Name + % + `m_areaCompleted` (SP-035/036/040). `m_previouslyCompleted` for §27.4 detail copy (SP-065). Not a first-100 m chip. |
| Completed chrome | SP-040 / `area_overlay` styles | Distinct completed visual (§18.6). 100% celebration is badge pulse + copy card (SP-065) plus rings-only outline from `CompletionCardModel` (SP-067); do not replace overlay chrome. |
| Area geometry | `ExplorationArea::m_rings` | Mercator outer rings available offline. V1 share geometry is SPD-046 rings-only via `CompletionCardModel`. |
| Share | `SharingUtils`, bookmark/track KML/GPX | Generic file/text share. **Not** used for neighbourhood cards. SP-065/067 Share chrome is a no-op (`R.string.share`); SP-068 opens the sheet. |
| Milestone / card / first-goal | `street_pixels::FirstGoalTracker` / JNI `FirstGoalProgress`; `street_pixels::AreaMilestonePresenter` / JNI `AreaMilestonePresentation`; `street_pixels::CompletionCardModel` | First-goal exists (SP-064). 25/50/100 presentation queue exists (SP-065). 100% compositor is rings-only `CompletionCardModel` (SP-067). Share tap still no-op. |
| Growth analytics | `StreetExplorationRoutingAnalytics` (Phase 6) | Count-only routing counters only. No card-generated / share-initiated events. |
| Routing-following | `RoutingManager::IsRoutingFollowing` | Exists; milestone UI must not interrupt it. |

**Difference from the technical audit (2026-07-20):** Phase 2 gated collection
(and therefore vibration) on an active recording session — the audit/phase
note that haptics were reachable outside a session is **stale**. SP-066 landed
the foreground gate, one-pulse-per-update rule, settings toggle, and
milestone waveforms (device feel still SP-069). SP-067 landed a
rings-only `CompletionCardModel` compositor (headless stroke + Android
Canvas from the JNI model). Share sheet remains SP-068. Phase 5 delivered area-scoped % and completed
chrome the 2026-07-25 snapshot marked missing for the badge.

## Intended outcome

- A first-goal badge that appears on first recording and completes at roughly
  100 metres of new live pixels.
- Milestones firing once per area per threshold, non-blocking, never
  interrupting active routing.
- A completion card generated at 100% that contains only the permitted content.
- An explicit share action that does not auto-open the system share sheet.
- Haptics that respect the foreground and recording conditions and a single
  settings toggle.

## Dependencies

- Phase 5, for area-scoped completion percentages and the completed-area visual
  state.
- Phase 8 only for the optional competition line on the completion card. The
  card must work fully without competition, so this is not a blocking
  dependency.

## Phase-entry investigation (2026-08-19)

### Confirmed gaps

- No per-area fired-once store and no original 100% date (§18.5 / §27.4).
- No first-100 m onboarding badge (§10 steps 6 and 9).
- No 25/50/100 celebration UI. Completed chrome exists (SP-040) but does not
  fire a card or share action.
- Haptics: SP-066 recording ∧ foreground ∧ toggle; one collection pulse;
  50/100/first-goal patterns. Device feel → SP-069.
- No completion-card share sheet. SP-067 composes `CompletionCardModel` from
  `m_rings`. `SharingUtils` would share a track if reused naively.
- No growth counters for card generated / share initiated.

### Blocking unknowns (must not be guessed in coding items)

Recorded as **M1–M10** in
[`SP-062`](../work-items/SP-062-milestone-share-architecture-decisions.md),
proposed as **OQ-9–OQ-18** (draft SPD-046–055) in `DECISIONS.md` §15.
Coding SP-063+ waits on those locks (Accepted SPDs or an explicit maintainer
deferral). M1 / OQ-9 is also the remaining Phase 7 **entry** criterion
(**unmet**).

Recommended positions (not Accepted until SP-062 / maintainer):

| Ref | Question | Recommended lock |
| --- | --- | --- |
| M1 | Stylised map on the card | Recommended deny-list-safe path: off-map from `m_rings` (outers); never a Drape screenshot. Spec also allows a non-screenshot stylised map. |
| M2 | ~100 m in pixels | Recommended: 10 new live pixels from §10 step 10 (30 ≈ 300 m). Not geodesic 100 m. Also lock newly-explored vs imported→live flip. |
| M3 | Fired-state store | New local store keyed by OSM id + threshold; date beside it |
| M4 | Re-fire after rematch | Does not re-fire; date survives |
| M5 | Several areas in one session | Queue; never interrupt following; 100% > 50% > 25% |
| M6 | Date on the card | Stored always; privacy recommendation: shown only if opted in at share time (SP-068). Spec “optional” ≠ a required toggle. |
| M7 | Competition line | Stub; card works with no nickname; Phase 8 fills §22.10 |
| M8 | First-100 m lifetime | Once per install |
| M9 | Haptics predicate | Recording ∧ foreground ∧ toggle; one pulse per update |
| M10 | Growth analytics | Count-only; no area id; Phase 10 upload residual |

### Work-item breakdown

| Order | ID | Title |
| --- | --- | --- |
| 1 | [SP-062](../work-items/SP-062-milestone-share-architecture-decisions.md) | Architecture decisions (**entry gate** for coding) |
| 2 | [SP-063](../work-items/SP-063-milestone-state-tracking.md) | Milestone state tracking |
| 3 | [SP-064](../work-items/SP-064-first-100-metres-goal.md) | First-100-metres contextual goal |
| 4 | [SP-065](../work-items/SP-065-area-milestone-presentation.md) | Area milestone presentation (25 / 50 / 100) |
| 5 | [SP-066](../work-items/SP-066-exploration-haptics-policy.md) | Exploration haptics policy |
| 6 | [SP-067](../work-items/SP-067-completion-card-compositor.md) | Completion-card compositor |
| 7 | [SP-068](../work-items/SP-068-share-flow-and-growth-analytics.md) | Share flow and growth analytics |
| 8 | [SP-069](../work-items/SP-069-phase7-end-to-end-validation.md) | Phase 7 end-to-end validation (**exit gate**) |

Gate: SP-062 must lock M1–M10 (or record maintainer deferrals) before SP-063+
coding. Phase 5 exit (SP-041) remains the product dependency for coding that
consumes area % / completed chrome; SP-062 itself is docs-only and may run
while SP-041 awaits acceptance.

No dedicated compositor spike unless SP-062 finds that source inspection
cannot choose a deny-list-safe path. Privacy argues against a live-map
screenshot without a prototype.

## Data and migration concerns

- Milestone fired-state is new persisted local data, keyed by area identifier
  and threshold. It must survive map updates and area reassignment.
- Spec §18.5 permits storing the original 100% completion date locally. Decide
  whether that lives with milestone state.
- Spec §27.4: if an area drops below 100% after a map update, the card and the
  stored date remain valid and the interface may say the area was previously
  completed. Milestone state must therefore not be reset by reconciliation.
- Whether a milestone can re-fire after a map update pushes an area back below
  and then above a threshold. Default position: it does not re-fire, because
  the achievement already happened.
- Generated card images are transient. Do not accumulate them in shared
  storage.

## Privacy and security implications

The completion card is the only image this product produces for the outside
world. Its exclusion list is a hard requirement, not a guideline.

The card must not contain: raw GPS route, home location, live location,
individual timestamps, or any other user's personal information. V1
geometry is SPD-046 rings-only: a boundary outline from `m_rings` via
`CompletionCardModel`. Spec §19.1
also allows a non-screenshot stylised map. The image is not a trace of where
the user walked.

Additional considerations:

- Anonymous sharing must work with no account and no nickname (spec §19.2).
- The card is composed off-map from permitted fields. Verify no live map
  overlay leaks the user's current position marker or a recorded track into
  the composition. Never capture Drape / `MapView`.
- Analytics record that a card was generated and that a share was initiated.
  They do not record which area.

## Automated testing strategy

- Milestone firing: crossing 25%, 50%, and 100% fires exactly once each;
  re-crossing does not re-fire; three thresholds crossed in one update fire
  correctly.
- Milestone state survives a simulated map update and area reassignment.
- First-goal completion at the correct new-live-pixel equivalent of 100 metres.
- Haptics policy as a pure predicate: recording and foreground yields a pulse;
  recording and background yields none; not recording yields none; one pulse
  per update regardless of pixel count; toggle off suppresses all.
- Card content assertion: the composed card model contains only permitted
  fields, verified against an explicit deny list rather than by inspection.
- Card composition succeeds with no nickname and no completion date.

## Manual validation strategy

- Complete a small real area and confirm the 100% celebration appears, does not
  block, and does not interrupt active navigation.
- Generate the card and inspect the image for anything on the exclusion list.
- Generate the card with competition disabled and confirm the first-person copy.
- Competition-on leading / not-leading copy (§22.10) is **Phase 8**. If
  competition is absent, record it as a residual here; do not block Phase 7
  exit on live §22.10 sentences. The stub must still avoid implying that
  personal completion was invalid.
- Confirm the share sheet does not open until the user taps Share.
- Confirm haptics behave correctly with the screen off, in the background, and
  with the toggle off.

## Entry criteria

- Phase 5 exit criteria met.
- A decision exists on how the stylised map on the card is rendered.
  **Met** — maintainer Accepted M1 / OQ-9 as **SPD-046** (2026-08-19 via SP-062).

## Exit criteria

1. Milestones fire once per area per threshold and are non-blocking.
2. Milestones never interrupt active routing or demand immediate interaction.
3. The first-100-metres goal appears and completes as specified.
4. Completion cards generate at 100% and contain no excluded content, verified
   against a deny list in an automated test and by manual inspection.
5. Cards work with no competition profile and no nickname.
6. The share action is explicit; the system share sheet does not auto-open.
7. Haptics respect the recording and foreground conditions and the single
   settings toggle.
8. Milestone state survives map updates per spec §27.4.
9. Growth analytics record card generation and share initiation with no
   location data.

## Explicit non-goals

- Achievement lists, trophy cabinets, milestone-history screens, and
  achievement points. Spec §18.5 excludes all of them.
- Additional milestone types and exploration streaks. Post-V1.
- Multiple haptic strength settings. Spec §28.4.
- Aggressive retention notifications. Spec §6.
- Sharing anything other than a 100% area completion card.
- Automatically opening the share sheet.

## Known uncertainties

Tracked as **M1–M10** in SP-062. Proposed as **OQ-9–OQ-18** (draft
SPD-046–055) in `DECISIONS.md` §15. **Not Accepted.**

- M1 / OQ-9 — stylised map render path (Phase 7 entry criterion; **unmet**).
- M2 / OQ-10 — 100 m → new-live-pixel count.
- M3 / OQ-11 — fired-state store and key.
- M4 / OQ-12 — re-fire after map update.
- M5 / OQ-13 — several areas in one session. **Closed by SPD-050** (2026-08-19 via SP-062).
- M6 / OQ-14 — completion date on the card by default vs opt-in.
- M7 / OQ-15 — competition line stub vs Phase 8. **Closed by SPD-052** (2026-08-19 via SP-062).
- M8 / OQ-16 — first-100 m once per install.
- M9 / OQ-17 — haptics predicate (foreground, one pulse, toggle).
- M10 / OQ-18 — growth analytics sink (local + Phase 10 residual).
