# Phase 7 — Milestones and share cards

**Status:** Not started
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

Verified 2026-07-25 against the working tree.

| Concern | Location | Observed state |
| --- | --- | --- |
| Haptic on collection | `libs/map/street_pixels_manager.cpp` `OnLocationUpdate` | Vibration fires from the collection path. Not conditioned on foreground state, and today not conditioned on a session either. |
| Haptics setting | — | No single "Exploration haptics" toggle found |
| Milestones | — | Not found |
| Share compositor | — | Not found. Generic bookmark and track sharing exists via the KML and GPX pipeline. |
| Completion date storage | — | Not found |
| Progress badge | `android/app/.../MwmActivity.java` map buttons | Present but not area-scoped and not milestone-aware |

**Difference from the technical audit:** none material. Note that the haptic
path is currently reachable outside a recording session, which Phase 2 fixes as
a side effect of the collection gate; the foreground condition in spec §28.1 is
this phase's responsibility.

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

## Proposed work-item breakdown

Not yet decomposed. Likely shape:

1. Milestone state tracking: which thresholds have fired for which area, and
   local persistence.
2. First-100-metres contextual goal.
3. Milestone presentation at 25%, 50%, and 100%, including the non-blocking and
   non-interrupting rules.
4. Haptics policy: foreground plus recording condition, one pulse per
   collecting update, milestone patterns, settings toggle.
5. Completion-card compositor.
6. Share flow and growth analytics.

**Marked for phase-specific Plan Mode investigation.** Card composition needs a
decision about whether the stylised map is rendered through Drape, drawn from
polygon geometry directly, or composed on the Android side. That cannot be
settled from source inspection alone.

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
individual timestamps, or any other user's personal information. The stylised
map is a boundary outline, not a trace of where the user walked.

Additional considerations:

- Anonymous sharing must work with no account and no nickname (spec §19.2).
- The card is a screenshot-like artefact of the user's map. Verify no map
  overlay leaks the user's current position marker or a recorded track into the
  composition.
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
- Generate the card with competition enabled in both the leading and
  not-leading cases, and confirm the copy matches spec §22.10 and does not
  imply the completion was invalid.
- Confirm the share sheet does not open until the user taps Share.
- Confirm haptics behave correctly with the screen off, in the background, and
  with the toggle off.

## Entry criteria

- Phase 5 exit criteria met.
- A decision exists on how the stylised map on the card is rendered.

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

- How the stylised area map is rendered, and whether it can be produced without
  a visible map screenshot.
- What "the equivalent of approximately 100 metres of new live street pixels"
  converts to in pixel count, given the fixed HEALPix cell size.
- Whether milestone state belongs in `street_stats.db` or a settings-like
  store.
- Whether a milestone should re-fire after a map update drops and restores an
  area above a threshold.
- How the celebration behaves if several areas cross thresholds during one
  session.
- Whether the completion date should be shown by default or opted into, since
  it is a weak temporal signal about the user.
