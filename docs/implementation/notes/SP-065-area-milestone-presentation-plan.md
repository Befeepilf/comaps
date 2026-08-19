# SP-065 — Area milestone presentation (implementation plan)

**Status:** Draft plan for coding; not Accepted.
**Work item:** [`SP-065-area-milestone-presentation.md`](../work-items/SP-065-area-milestone-presentation.md)
**Date:** 2026-08-19
**Locks (do not re-open):** SPD-050 (queue / following / city-summary), SPD-052 (anonymous first-person; §22.10 stub).

This note is the coding contract. Do not invent a second queue, a modal, a map screenshot, or a share sheet.

---

## 0. Status of building blocks (verified 2026-08-19)

| Piece | Actual state | SP-065 action |
| --- | --- | --- |
| `street_pixels::AreaMilestoneStore` | Exists. `EvaluateAndRecordFires` writes fired-state, appends `m_pendingCrossings`, **sorts 100 > 50 > 25**. `ConsumePendingCrossings` drains the vector. City cache is never passed in. | Consume into a **presentation** queue. Do not change fire-once policy. |
| `StreetPixelsManager::ConsumePendingAreaMilestoneCrossings()` | Thin wrapper over the store. **No UI listener.** Rebuild path (`RebuildAreaCompletionCacheUnlocked`) calls `EvaluateAreaMilestonesUnlocked` then overlay push; nothing presents. | After evaluate, ingest pending into presenter and notify. |
| `WasAreaPreviouslyCompletedBelow100(compactIndex)` | Real name (not `WasFocusedAreaPreviouslyCompletedBelow100`). | Bind into `FocusedAreaProgress` + detail sheet. |
| First-goal | `libs/map/first_goal.hpp`, JNI `FirstGoalProgress`, `OnStreetPixelsChangedListener.onFirstGoalProgressChanged`, `MapButtonsController.applyFirstGoalBadge`, chip in `map_exploration_banner.xml`. Haptic: `SetFirstGoalCompleteHandler` is C++-only (not JNI). | Copy this push-listener + banner pattern. |
| `Framework.nativeIsRoutingActive()` | Maps to `RoutingManager::IsRoutingActive()` (planning **or** following). **`nativeIsRoutingFollowing` does not exist.** Android following ≈ `RoutingController.isNavigating()` (`State.NAVIGATION`). `nativeCloseRouting` / `nativeDisableFollowing` exist and must not be called. | Gate display in Android with `isNavigating()`. Do not add following JNI unless C++ must delay (it must not). |
| `street_pixels::DisplayName` | Returns `area.m_name` only. Blank → empty. Never MWM id. `SetFocusedArea` already fail-closes on blank name. | Resolve presentation names the same way. Drop the item if blank. |
| Completed overlay | `StyleForCompletion` sets `m_completed`; drape `ExplorationAreaOverlayItem.m_completed`. No pulse/glow animator. | Do **not** replace completed chrome. 100% celebration = badge pulse + card surface. No drape rewrite. |
| City-summary | `RefreshFocusedAreaFractionUnlocked` reads `m_cityCompletionCache` when `m_citySummary`. Evaluate milestones only on **area** cache rows. | Do not enqueue from city cache or city-summary focus. |
| Share | `SharingUtils` is KML/GPX/location. `R.string.share` exists. | Surface + Share **chrome** only. No `ACTION_SEND`. No `SharingUtils`. |

---

## 1. Architecture

### 1.1 Split

**C++ owns:** what fired (already SP-063), presentation **queue order**, one-at-a-time head, DisplayName lookup, haptic **events**, competition-line stub (empty), previously-completed flag on the focused snapshot. C++ never opens UI, never captures the map, never shares.

**Android owns:** toast/banner/card chrome, badge pulse, Share button **visible and no-op**, following-safe placement (existing exploration banner, already on navigation layouts). Android never reorders the queue and never invents a second fire.

### 1.2 Queue model (SPD-050)

SP-063 pending crossings are a **fire log**, not a UI queue. `ConsumePendingCrossings` is destructive. If Android is not listening, events are lost unless C++ holds them.

Add `street_pixels::AreaMilestonePresenter` (same role as `FirstGoalTracker`):

1. After `EvaluateAreaMilestonesUnlocked`, **consume** store pending (skip already-shown this crossing = consume-once + store fired-mask).
2. Resolve `DisplayName` from the SPA file used for that rebuild (`FindAreaByCompactIndex` + `DisplayName`). **Drop** the crossing if name is empty. Never substitute country id, leaf, OSM id, or compact index.
3. Map threshold → `AreaMilestonePresentation` and **enqueue**, keeping 100 > 50 > 25 (store already sorted; presenter re-sorts on merge so a later 100% still jumps ahead of an unshown 25%).
4. Dedupe key `(osmId, threshold)` already in the queue (including the head).
5. **One at a time:** `Peek()` is the only item Android renders. `Acknowledge()` pops the head and notifies if a new head exists.
6. First-100 m stays independent (do not share this queue).

City-summary: presenter ingest is called only with area-cache crossings. Never iterate `CityCompletionCache::Rows()`.

### 1.3 Consume vs notify

Call ingest from `RebuildAreaCompletionCacheUnlocked` immediately after `EvaluateAreaMilestonesUnlocked`, while `resolver.GetFile()` is in scope (same SPA as the cache). Do **not** rely on `m_cachedFocusSpaFile` (focus-only).

Push like first-goal: `SetAreaMilestonePresentationListener(std::optional<AreaMilestonePresentation>)`. JNI hops to GUI thread. `null` means queue empty (after last Acknowledge).

Android: on head changed → render; on toast timeout / card hide → `nativeAcknowledgeAreaMilestonePresentation()`.

### 1.4 Following-routing gate

Chosen SPD-050 branch: **always non-blocking overlay**, never “wait until guidance ends”.

- C++ does not read `IsRoutingFollowing` and does **not** include `routing_manager.hpp`.
- C++ presentation path does not call `CloseRouting`, `DisableFollowMode`, `FollowRoute`, `TriggerCollectionVibration`, or any share hook.
- Android uses the existing exploration banner (already included in `map_buttons_layout_navigation.xml`, top strip, **not** `layout_nav_top` / `nav_next_turn_container`).
- When `RoutingController.get().isNavigating()`, still show 25/50 toast+pulse and the 100% **copy card** in that banner. No `AlertDialog`, no `BottomSheetDialog` for the celebration, no `nativeCloseRouting`, no `nativeDisableFollowing`, no auto-pause of recording.
- Detail bottom sheet remains tap-driven (SP-038) and is not the celebration.

Do not add `Framework.nativeIsRoutingFollowing()`.

### 1.5 100% card vs compositor

Until SP-067: copy-only surface (title + first-person line + Share chrome). No `MapView` / Drape / bitmap / placeholder PNG. Share tap is a no-op (SP-068). Animation end / Acknowledge must not share.

Competition line: empty string field on the presentation struct. Hook: `using CompetitionLineFn = std::function<std::string(uint64_t osmId)>`; default empty. Phase 8 fills §22.10. Never emit “invalid completion”.

---

## 2. Exact files

### 2.1 Add

| Path | Why |
| --- | --- |
| `libs/map/area_milestone_presentation.hpp` | Types + `AreaMilestonePresenter` (mirror `first_goal.hpp`). |
| `libs/map/area_milestone_presentation.cpp` | Queue, sort, dedupe, name drop. |
| `libs/map/street_pixels_tests/area_milestone_presentation_tests.cpp` | Gate tests. |
| `android/sdk/src/main/java/app/organicmaps/sdk/maplayer/streetpixels/AreaMilestonePresentation.java` | JNI mirror of `FirstGoalProgress`. |
| `android/app/src/main/res/layout/area_completion_card.xml` | Copy-only 100% surface + Share button. |

### 2.2 Modify

| Path | Why |
| --- | --- |
| `libs/map/CMakeLists.txt` | Add `area_milestone_presentation.cpp/.hpp` next to `first_goal.cpp`. |
| `libs/map/street_pixels_tests/CMakeLists.txt` | Add `area_milestone_presentation_tests.cpp`. |
| `libs/map/street_pixels_manager.hpp` / `.cpp` | Presenter member; ingest after evaluate; listener; haptic handler; previously-completed on focused snapshot; Acknowledge JNI target. |
| `libs/street_pixels_areas/focused_area_progress.hpp` | Add `m_previouslyCompleted`. |
| `android/sdk/src/main/java/.../FocusedAreaProgress.java` | Mirror `previouslyCompleted`. |
| `android/sdk/src/main/cpp/.../streetpixels/StreetPixelsManager.cpp` | `ToJavaAreaMilestonePresentation`, listener, Acknowledge, extend focused-area ctor. |
| `android/sdk/src/main/java/.../OnStreetPixelsChangedListener.java` | `onAreaMilestonePresentationChanged`. |
| `android/sdk/src/main/java/.../StreetPixelsManager.java` | Callback register + natives (first-goal pattern). |
| `android/app/src/main/java/.../MapButtonsController.java` | Render queue head; badge pulse; 100% card; Acknowledge; Share no-op. |
| `android/app/src/main/res/layout/map_exploration_banner.xml` | Third child: include `area_completion_card` (`visibility=gone`). |
| `android/app/src/main/java/.../FocusedAreaDetailBottomSheet.java` | Previously-completed body copy. |
| `android/app/src/main/java/app/organicmaps/MwmActivity.java` | Pass `previouslyCompleted` into `show(...)` if the tap path stays here; today tap is in `MapButtonsController` **and** `MwmActivity.onExplorationAreaTapped` — keep both in sync. |
| `android/app/src/main/res/values/strings.xml` | English strings. |
| `android/app/src/main/res/values-en/strings.xml` | Same English strings (SP-064 rule). |
| `docs/implementation/work-items/SP-065-area-milestone-presentation.md` | Notes, status **In review**, evidence placeholders. Agent does **not** mark Accepted. |
| `docs/implementation/phases/phase-07-milestones-and-share-cards.md` | Current-code table: presentation symbol now exists. |
| `docs/implementation/README.md` | Propose SP-065 **In review** in the Phase 7 table (maintainer owns merge status). |

Do **not** edit: `area_milestone_store.*` fire policy, `area_overlay` completed colors, `SharingUtils`, drape overlay shaders, iOS, SP-063 tests except if a helper must be shared (prefer duplicating the small SPA fixture like `area_milestone_manager_tests.cpp`).

No inline imports. No new comments. No formatting-only changes.

---

## 3. Proposed types / APIs

Naming follows first-goal: `FirstGoalState` / `FirstGoalProgress` / `FirstGoalTracker` / `GetFirstGoalProgress` / `SetFirstGoalProgressListener` / `SetFirstGoalCompleteHandler`.

### 3.1 C++ (`street_pixels` in `libs/map/area_milestone_presentation.hpp`)

Reuse `street_pixels::AreaMilestoneThreshold` from the store. Do not duplicate the enum.

```cpp
enum class AreaMilestoneHapticEvent : uint8_t
{
  FiftyPercent = 0,
  HundredPercent = 1,
};

struct AreaMilestonePresentation
{
  uint64_t m_osmId = 0;
  uint32_t m_compactIndex = 0;
  AreaMilestoneThreshold m_threshold = AreaMilestoneThreshold::P25;
  std::string m_displayName;       // DisplayName only; never MWM id
  std::string m_competitionLine;   // SPD-052 stub; empty until Phase 8
};

bool operator==(AreaMilestonePresentation const &, AreaMilestonePresentation const &);
std::string DebugPrint(AreaMilestonePresentation const &);

class AreaMilestonePresenter
{
public:
  using NameLookup = std::function<std::string(uint32_t compactIndex, uint64_t osmId)>;
  using CompetitionLineFn = std::function<std::string(uint64_t osmId)>;

  void SetCompetitionLineProvider(CompetitionLineFn const & fn);  // default: always {}
  void Enqueue(std::vector<AreaMilestoneCrossing> const & crossings, NameLookup const & names);
  std::optional<AreaMilestonePresentation> Peek() const;
  void Acknowledge();  // no-op if empty
  void ResetForTesting();
};
```

`Enqueue` rules:

- Call `names(compact, osmId)`. Empty → skip.
- Competition line from provider (default empty); never required for enqueue.
- Stable sort: threshold priority 100=0, 50=1, 25=2, then `osmId`, then `compactIndex` (same as store `SortCrossings`).
- Skip duplicate `(osmId, threshold)`.

### 3.2 `StreetPixelsManager`

```cpp
using AreaMilestonePresentationChangedFn =
    std::function<void(std::optional<street_pixels::AreaMilestonePresentation> const &)>;
using AreaMilestoneHapticFn = std::function<void(street_pixels::AreaMilestoneHapticEvent)>;

void SetAreaMilestonePresentationListener(AreaMilestonePresentationChangedFn const & fn);
void SetAreaMilestoneHapticHandler(AreaMilestoneHapticFn const & fn);  // SP-066; tests count calls
std::optional<street_pixels::AreaMilestonePresentation> GetCurrentAreaMilestonePresentation() const;
void AcknowledgeAreaMilestonePresentation();
void ResetAreaMilestonePresentationForTesting();
```

Wire:

- After `EvaluateAreaMilestonesUnlocked`, `ConsumePendingAreaMilestoneCrossings()`, `Enqueue` with lookup from the rebuild `SpaFile`.
- If Peek changed, notify listener (GUI hop is JNI’s job, same as first-goal).
- When the **new head** is P50 or P100, call haptic handler **once**. Do **not** call `TriggerCollectionVibration`. P25: no haptic event.
- `AcknowledgeAreaMilestonePresentation`: pop; if new head is P50/P100, emit haptic for that head (one pulse per shown milestone).
- Haptic fires on **show** (head becomes current), not on store record, so a queued 50% still vibrates when it reaches the head.

Do not add a share handler on the manager.

`FocusedAreaProgress`: add `bool m_previouslyCompleted = false`. Set in `RefreshFocusedAreaFractionUnlocked`:

- `false` when `m_citySummary` or no focus or cache invalid.
- else `WasAreaPreviouslyCompletedBelow100(compactIndex)`.

Update `operator==`. JNI ctor currently `(ZZZZZIJLjava/lang/String;D)V` — extend with a trailing `Z` for `previouslyCompleted`.

### 3.3 JNI / Java mirrors

`AreaMilestonePresentation.java` (`@Keep`):

```java
public static final int THRESHOLD_25 = 0;
public static final int THRESHOLD_50 = 1;
public static final int THRESHOLD_100 = 2;
public final int threshold;
public final long osmId;
public final int compactIndex;
@NonNull public final String displayName;
@NonNull public final String competitionLine;
```

Ctor `(I J I String String)` matching C++ field order: threshold, osmId, compactIndex, displayName, competitionLine.

`OnStreetPixelsChangedListener.onAreaMilestonePresentationChanged(@Nullable AreaMilestonePresentation presentation)`

`StreetPixelsManager`:

- `AreaMilestonePresentationCallback`
- `registerAreaMilestonePresentationCallback` / `unregister...`
- `nativeGetCurrentAreaMilestonePresentation()` → `@Nullable AreaMilestonePresentation`
- `nativeAcknowledgeAreaMilestonePresentation()`
- Register listener inside existing `nativeAddListener`; clear in `nativeRemoveListener`.

Do not JNI-wire the haptic handler in this item (same as first-goal complete handler).

### 3.4 Android UI API (no new Activity)

`MapButtonsController`:

- Field `mCompletionCard` (root of include).
- `MapButtons.completionCard` if `showButton` needs it; otherwise `UiUtils.showIf` on the include (prefer this to avoid expanding the enum if `showButton` switch would need a new case — if the enum is required for `mButtonsMap`, add `completionCard` next to `firstGoalBanner`).
- `applyAreaMilestonePresentation(@Nullable AreaMilestonePresentation p)`
- `pulseExplorationBadge(int threshold)` — 25 short scale, 50 longer scale.
- Share `OnClickListener`: empty. Do not start an Intent. Do not call `SharingUtils`.

`FocusedAreaDetailBottomSheet.show(..., boolean previouslyCompleted)` + `ARG_PREVIOUSLY_COMPLETED`. When true and not area-completed, show body `street_pixels_area_previously_completed`. When area-completed, keep `street_pixels_area_completed` on the percent row; do not add an achievement list.

---

## 4. Copy strings

Add identical English entries to `values/strings.xml` **and** `values-en/strings.xml`. Format arg is DisplayName (`%1$s`).

| Resource name | English text | Used for |
| --- | --- | --- |
| `street_pixels_area_milestone_25` | `25% of %1$s explored` | 25% toast / banner |
| `street_pixels_area_milestone_50` | `Half of %1$s explored` | 50% toast / banner |
| `street_pixels_area_milestone_100` | `%1$s fully explored` | 100% toast + card title |
| `street_pixels_area_previously_completed` | `Previously completed` | Detail sheet / optional badge note when §27.4 |
| `street_pixels_completion_card_body` | `%1$s fully explored` | 100% card body (first-person, competition off). Same sentence as title is OK; do not add nickname or “invalid”. |
| `street_pixels_completion_card_share` | `Share` | Share chrome label. Prefer this dedicated id so SP-068 can keep chrome copy even if generic `share` is reused; **or** reuse existing `share` (“Share”) and skip this id. **Decision for coding:** reuse `R.string.share` to avoid duplicate English. |

Do **not** add competition-on sentences. Do not add “completion was invalid”, “does not count”, or MWM/country ids in copy.

Badge previously-completed: keep the FAB as current `%` (already). Put §27.4 copy on the **detail sheet**, not a new screen. Optional: if `previouslyCompleted && !areaCompleted`, percent line can stay numeric; body shows `Previously completed`.

---

## 5. UI behavior

### 5.1 25% / 50%

- Non-blocking: `Toast.LENGTH_SHORT` (25) / `Toast.LENGTH_LONG` (50) **or** `Utils.showSnackbar` on the map coordinator. **Prefer Toast** (matches rematch “more to explore”; quieter than a modal). Do not use `MaterialAlertDialog`.
- Pulse `mExplorationBadge`: 25 ≈ 180–220 ms scale 1.00→1.08→1.00; 50 ≈ 350–450 ms scale 1.00→1.16→1.00. No extra haptic pattern on 25.
- After toast duration, `AcknowledgeAreaMilestonePresentation()` so the next queued item can show.
- If the focused area is a different neighbourhood than the crossing, still show the toast (copy has DisplayName). Do not steal focus.

### 5.2 100%

- Same toast/pulse as 50% **plus** show `area_completion_card` under the first-goal chip in `map_exploration_banner.xml`.
- Card: DisplayName heading, body `street_pixels_completion_card_body`, Share button using `R.string.share`. No image `ImageView` bound to a bitmap. No `MapView`.
- Share: visible; click no-op. Do not disable in a way that looks like an error; disabling the button is OK if it still reads as chrome (`setEnabled(false)` + no click). Prefer enabled + empty click so chrome is obvious.
- Dismiss: user taps outside the card **or** a small close/timeout (~4 s) then Acknowledge. Map remains interactive. Do not auto-open share on timeout.
- Overlay: existing completed chrome stays. Do not change `StyleForCompletion`. Badge may pulse on top.

### 5.3 Following

- Celebration views live in `map_exploration_banner`, already on navigation layouts, **not** over `nav_next_turn_frame` / street name.
- `showButton` for `explorationBanner` is **not** gated on `isInNavigationMode()` (unlike layers). Keep that.
- No call into `Framework.nativeCloseRouting`, `nativeDisableFollowing`, `RoutingController` stop APIs, or recording pause.

### 5.4 Multi-fire

Three crossings from one rebuild: Peek is 100% first; Acknowledge → 50%; Acknowledge → 25%. Do not drop 100%.

### 5.5 City-summary

City-summary badge % updates must not call presenter ingest. Only area-cache evaluate → consume → enqueue.

---

## 6. Haptic stubs (SP-066)

| Event | When | Implementation now |
| --- | --- | --- |
| 25% | — | No handler call (spec §28.3). |
| 50% | Head becomes P50 | `m_areaMilestoneHapticHandler(FiftyPercent)` |
| 100% | Head becomes P100 | `m_areaMilestoneHapticHandler(HundredPercent)` |
| First-goal | already `SetFirstGoalCompleteHandler` | Unchanged. |

Forbidden: `TriggerCollectionVibration` from presenter / Acknowledge / ingest.

SP-066 will attach real `VibratePattern` behind recording ∧ foreground ∧ toggle. This item only emits the event.

---

## 7. Test plan

C++ is the gate. Android instrumented/device → SP-069 / Phase 10.

### 7.1 Targets (actual names)

- Binary / CMake project: `street_pixels_tests` (`libs/map/street_pixels_tests/CMakeLists.txt`, `omim_add_test`).
- Existing store suite: `street_pixels_areas_tests` (`area_milestone_store_tests.cpp`) — regression only; do not move presentation tests there (presenter lives in `map`).
- Filter prefix: `AreaMilestonePresentation`.

Commands (from repo root; debug dir per `docs/implementation/README.md` §8.1):

```bash
./tools/unix/build_omim.sh -d street_pixels_tests street_pixels_areas_tests
cd ../omim-build-debug
./street_pixels_tests --filter=AreaMilestonePresentation
./street_pixels_areas_tests --filter=AreaMilestone
./street_pixels_tests --filter=AreaMilestone
```

Optional: `./tools/unix/run_tests.sh -b ../omim-build-debug -f "AreaMilestonePresentation"`.

Record counts in the work-item evidence table. Do not weaken existing `AreaMilestone_*` tests.

### 7.2 New file: `area_milestone_presentation_tests.cpp`

Reuse the SPA fixture pattern from `area_milestone_manager_tests.cpp` (`MakeAmFixture`: District osm 10 100% of one pixel, City osm 8 0%). Isolated presenter tests do not need a manager.

| Test name | Asserts |
| --- | --- |
| `AreaMilestonePresentation_MapsThresholds` | Enqueue P25/P50/P100 → Peek fields: threshold + `m_displayName` from lookup (“District”), `m_competitionLine.empty()`. |
| `AreaMilestonePresentation_QueueOrder100Then50Then25` | Unsorted input `{P25,P100,P50}` same osm → Peek P100; Ack → P50; Ack → P25; Ack → empty. |
| `AreaMilestonePresentation_OneAtATime` | After enqueue of 3, Peek stays P100 until Acknowledge. |
| `AreaMilestonePresentation_SkipAlreadyShownThisCrossing` | Manager: rebuild → ingest; Ack all; second `RebuildAreaCompletionCache` → Peek empty (SP-063 no re-fire + consume). |
| `AreaMilestonePresentation_SkipDuplicateInQueue` | Enqueue same P50 twice → single item. |
| `AreaMilestonePresentation_DisplayNameNeverMwmId` | Lookup returns `"District"`; Peek name ≠ fixture leaf MWM id; ≠ `std::to_string(osmId)`. |
| `AreaMilestonePresentation_BlankDisplayNameDropped` | Lookup returns `""` → queue empty (no fallback). |
| `AreaMilestonePresentation_CitySummaryDoesNotEnqueue` | Same as store fixture: district 100% fires area osm 10; city cache fraction 0.5 for osm 8; **presenter must not** have a crossing for osm 8. Extra: `SetFocusedArea(..., citySummary=true)` after consume must not Enqueue. |
| `AreaMilestonePresentation_Haptic50And100Not25` | Count haptic handler: triple-cross one update → 100 then 50 then 25: two haptic calls (`HundredPercent`, `FiftyPercent`) as each becomes head; P25 → 0 extra. |
| `AreaMilestonePresentation_DoesNotCallCollectionVibration` | `SetVibrationHandler` counter stays 0 across rebuild ingest (no `OnLocationUpdate`). |
| `AreaMilestonePresentation_FollowingDoesNotStopRoute` | Ingest + Ack 100% with recording session attached. **No** `routing_manager.hpp` in presenter. Test file may `#include` routing only to document unused APIs. Assert: do not call `CloseRouting` / `DisableFollowMode` — implement as: presenter/manager presentation methods have no routing sink; vibration/share counters 0; `GetCurrentAreaMilestonePresentation` still Peek 100%. Coding must not add a routing call to make this pass. |
| `AreaMilestonePresentation_HundredPercentDoesNotShare` | After 100% Peek and Acknowledge, no share side effect. Implement a test-only `int shareCalls` **not** wired in production; presentation code must not invoke it. If no share callback exists (preferred), the test is: Acknowledge 100% leaves no Intent and manager API list has no `Share` method — assert Peek/Ack only mutate the queue. |
| `AreaMilestonePresentation_CompetitionLineStubEmpty` | Set provider returning `"lead"` **must not be used in V1 default**. Default provider: empty. A second test may set a stub that returns a string and assert Peek copies it (proves the hook) **or** skip the setter test and only assert default empty. **Decision:** default-empty only; do not call a Phase 8 provider in production ingest. |
| `AreaMilestonePresentation_PreviouslyCompletedOnFocus` | After drop below 100% (existing manager fixture), `GetFocusedAreaProgress().m_previouslyCompleted == true` when focused on that area and `m_citySummary == false`; city-summary focus → false. |

### 7.3 Regression

Keep `AreaMilestoneManager_*` and `AreaMilestone_*` green. `FocusedAreaProgress` equality tests in `focused_area_badge_tests.cpp` must compile with the new field (default false).

### 7.4 Android residual (do not block C++ gate)

Manual from the work item: complete a fixture area without nav → 100% card + map usable; repeat with following on → guidance not dismissed. Device → SP-069 / Phase 10.

Static check while coding: celebration Java path does not reference `nativeCloseRouting`, `nativeDisableFollowing`, `SharingUtils`, `ACTION_SEND`.

---

## 8. Docs updates (coding agent)

1. `SP-065-area-milestone-presentation.md`
   - **Notes:** replace “Coding waits on OQ-13 / OQ-15 (draft SPD-050 / SPD-052)” with: **OQ-13 / OQ-15 are SPD-050 / SPD-052 Accepted (2026-08-19 via SP-062). Presentation queue is this item; SP-063 records fires only.**
   - **Status:** `In review` when the branch is ready for maintainer (not Accepted).
   - **Branch / completion evidence:** fill paths and test counts; Manual validation = device residual → SP-069 / Phase 10.
   - Agent does **not** set Accepted by / Accepted date.
2. `phases/phase-07-milestones-and-share-cards.md`
   - Current-code row “Milestone / card / first-goal”: first-goal exists (SP-064); 25/50/100 presentation symbol = `AreaMilestonePresenter` / JNI `AreaMilestonePresentation`. Card image still SP-067.
   - Drop stale “Not found” for celebration UI once this lands.
   - Known-uncertainties table still lists OQ-13/OQ-15 as unmet in the phase file — **fix those two rows** to Closed by SPD-050 / SPD-052 (phase file is stale vs `DECISIONS.md`).
3. `README.md` Phase 7 table: SP-065 → In review (proposal; maintainer §7).
4. This notes file: leave **Status: Draft plan** until coding starts; coding may add a one-line “implemented on branch …” under completion evidence, not Accepted.

---

## 9. Implementation sequence (coding)

1. Presenter + tests that do not need the manager (mapping, order, blank name, dedupe).
2. Manager ingest after evaluate + haptic + previously-completed field + remaining manager tests.
3. JNI + Java types + `MapButtonsController` + strings + card layout + detail sheet.
4. Grep: no `TriggerCollectionVibration` on the new path; no share; no routing stop.
5. Run the commands in §7.1. Fill work-item evidence. Status In review.

---

## 10. Risks / non-goals / follow-ups

| Risk | Mitigation |
| --- | --- |
| Consuming SP-063 pending without a presenter loses fires if UI is late | Ingest immediately in C++ queue; Android only Acks. |
| `nativeIsRoutingActive` mistaken for following | Use `RoutingController.isNavigating()`; do not pause on mere route planning. |
| Toast during nav covers manoeuvres | Keep chrome in exploration banner; no dialog. |
| JNI ctor break for `FocusedAreaProgress` | Update Java + C++ ctor in the same commit. |
| Badge pulse vs completed chrome | Pulse the FAB only; do not restyle overlay. |
| Share chrome looks broken | Visible Share, no-op, no error toast. |

**Non-goals:** fired-state (SP-063), first-100 m (SP-064), waveforms (SP-066), deny-list compositor / PNG (SP-067), OS share sheet / counters (SP-068), achievement list, Phase 8 boss copy, city-summary 25/50/100 celebrations, iOS.

**SP-066:** consume `AreaMilestoneHapticEvent` + existing first-goal complete handler; apply SPD-054 predicate.

**SP-067:** replace copy-only body with deny-list model + outline raster; keep this card chrome.

**SP-068:** Share `OnClickListener` → `ACTION_SEND` image only; increment card-generated / share-initiated; date opt-in default off (SPD-051). Do not auto-open.

---

## 11. Acceptance criteria → test / residual

| # | Criterion | Planned coverage |
| --- | --- | --- |
| 1 | 25 / 50 / 100 each show the specified acknowledgment once per area | `MapsThresholds` + `SkipAlreadyShownThisCrossing`; Android strings in both `values` files (residual visual). |
| 2 | Non-blocking; do not interrupt active following navigation | Overlay-in-banner (no dialog); `FollowingDoesNotStopRoute`; `DoesNotCallCollectionVibration`; device residual SP-069. |
| 3 | Three fires in one update present without dropping 100% | `QueueOrder100Then50Then25` + `OneAtATime`. |
| 4 | No achievement-history screen | No new Activity/Fragment beyond card include + existing detail sheet; §27.4 copy only. Reviewer grep. |
| 5 | 100% card surface + Share chrome without auto-opening the system sheet | `HundredPercentDoesNotShare`; Share click no-op; no PNG. Device residual SP-069. |
| 6 | Competition-off copy is first-person; no “invalid completion” wording | Strings table; `CompetitionLineStubEmpty`; body/title use `street_pixels_area_milestone_100` / `completion_card_body`. |

Additional required tests:

| Required test | Planned name |
| --- | --- |
| Presentation dispatcher mapping / queue / skip | `MapsThresholds`, `QueueOrder…`, `SkipAlreadyShownThisCrossing` |
| Following: no route-stop / follow-disable | `FollowingDoesNotStopRoute` |
| 100% fire / animation end does not invoke share | `HundredPercentDoesNotShare` |
| DisplayName: no MWM id | `DisplayNameNeverMwmId`, `BlankDisplayNameDropped` |
| City-summary updates do not enqueue area celebrations | `CitySummaryDoesNotEnqueue` |
