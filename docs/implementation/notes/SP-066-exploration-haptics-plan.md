# SP-066 — Exploration haptics policy (implementation plan)

**Status:** Draft plan for coding; not Accepted. Implemented on `cursor/sp-066-exploration-haptics-c417`.
**Work item:** [`SP-066-exploration-haptics-policy.md`](../work-items/SP-066-exploration-haptics-policy.md)
**Date:** 2026-08-20
**Locks (do not re-open):** SPD-054 (recording ∧ foreground ∧ toggle; one collection pulse per update; stronger first-100 m / 50% / 100%; no extra 25%; boss out of V1; area milestones may fire without haptic when not recording). SPD-002 (shared C++ owns the persisted flag).

This note is the coding contract. Do not invent a second vibrate path from Java, a per-pixel pattern, a strength slider, or a boss haptic.

---

## 0. Status of building blocks (verified 2026-08-20)

| Piece | Actual state | SP-066 action |
| --- | --- | --- |
| `StreetPixelsManager::TriggerCollectionVibration` | After live collection in `OnLocationUpdate`. `numNewlyExploredPixels == 0` returns. If `m_vibrationHandler` set, tests intercept with `size_t` and return; else `Vibrate(50)` for 1 pixel, else `VibratePattern` **once per pixel up to 10**. | Keep the call site. Change policy: one 50 ms pulse if ≥1 **new** pixel **and** shared predicate. Hook signature becomes kind enum (see §2). |
| Session gate | `OnLocationUpdate` returns if `m_recordingSession == nullptr` or `!IsRecording()` (`Recording` only; Pause/Idle/Finished/Discarded do not collect). | Collection path stays session-gated. Predicate still checks recording so milestone plays fail closed. |
| `SetVibrationHandler` | Used by `CollectionGate_Recording_TriggersVibration` (expects 1 call after Start+OnLocationUpdate, **no foreground flag today**), `CollectionGate_Rejected_NoVibration`, `SampleAcceptanceManager_Rejected_NoVibration`, `EverLive_UpgradeDoesNotDoubleCount` (sums `size_t`), `AreaMilestonePresentation_DoesNotCallCollectionVibration` / `_FollowingDoesNotStopRoute`. | Update signature. Positive collection test **must** set foreground. Default foreground is **false** (fail closed). |
| `SetFirstGoalCompleteHandler` | Fires when `AddNewlyExploredLivePixels` returns true. JNI not wired. Tests: `FirstGoal_CompletesAtTenNewlyExploredLivePixels`. | Keep event ungated. Consume it to play `FirstGoalComplete` waveform **iff** predicate. |
| `SetAreaMilestoneHapticHandler` | Fires when presentation **head** becomes P50 / P100 (including after Acknowledge). P25 does not. JNI not wired. Test: `AreaMilestonePresentation_Haptic50And100Not25`. | Keep event ungated (import-driven UI still celebrates). Play waveform **iff** predicate. |
| `Framework::EnterForeground` / `EnterBackground` | `OrganicMaps.nativeOnTransit` → `g_framework->NativeFramework()->EnterForeground/Background`. Updates usage stats, drape, traffic. **Does not notify Street Pixels.** | Call `StreetPixelsManager::SetApplicationForeground`. Screen-off / `onStop` is background even if recording FGS runs. |
| Settings | Android `Config.getBool/setBool` → JNI `nativeGetBoolean/nativeSetBoolean` → C++ `settings`. Zoom pattern: `prefs_interface.xml` + `InterfaceSettingsFragment.initZoomPrefsCallbacks` + `Config.showZoomButtons()` default **true**. First-goal keys: `StreetPixels.FirstGoalCollected` / `StreetPixels.FirstGoalComplete`. | New key `StreetPixels.ExplorationHaptics`. Missing → **on**. |
| `platform::Vibrate` / `VibratePattern` | Android JNI to `Utils.vibrate` / `vibratePattern`. Desktop no-op. | Reuse. Do not add a second JNI vibrate. Do not log coordinates. |
| Exploration haptics pref | **Absent** (2026-08-20). | Add Interface switch. |
| `values-en/strings.xml` | Symlink to `values/strings.xml`. | Edit **only** `values/strings.xml`. |

Do not change `area_milestone_store` fire policy, first-goal threshold, or presentation queue order.

---

## 1. Architecture

### 1.1 Split

**Pure C++ predicate** (`street_pixels::ShouldPlayExplorationHaptic`): pulse iff

1. recording session is `Recording` (not Idle / Paused / Finished / Discarded),
2. application foreground flag is true,
3. exploration-haptics toggle is on (default on when the settings key is absent).

Collection additionally requires `newlyExploredPixels >= 1`. Milestone kinds ignore pixel count.

**StreetPixelsManager owns:** foreground flag, reading the toggle, deciding whether to play, intercepting tests, calling `platform::Vibrate` / `VibratePattern`.

**Framework owns:** forwarding existing `EnterForeground` / `EnterBackground` into the manager. No new JNI transit.

**Android UI owns:** the Interface switch that writes the same C++ settings key via `Config`. It does not vibrate.

**SP-064 / SP-065 events stay ungated.** `SetFirstGoalCompleteHandler` and `SetAreaMilestoneHapticHandler` still fire when the milestone happens (including import-driven 50/100 with no recording). SPD-054: presentation may fire without haptic. Tests that assert **events** (`AreaMilestonePresentation_Haptic50And100Not25`, `FirstGoal_CompletesAtTen…`) must keep passing without foreground.

### 1.2 One play hook, two event hooks

| Hook | When | Gated by SPD-054? | Purpose |
| --- | --- | --- | --- |
| `SetVibrationHandler(ExplorationHapticKind)` | After predicate passes, **instead of** JNI vibrate | Yes (only invoked if play is allowed) | Tests assert call **count** and **kind**. Desktop/Android JNI never runs in tests. |
| `SetFirstGoalCompleteHandler` | First-goal just completed | No | Existing SP-064 progress tests / future JNI. |
| `SetAreaMilestoneHapticHandler` | Head becomes P50 or P100 | No | Existing SP-065 event tests. |

`VibrationHandler` no longer receives pixel count. Policy is one collection play per update; tests distinguish kinds via the enum, not via `size_t`.

If the handler is set, `PlayExplorationHaptic` returns after the callback and **does not** call `platform::Vibrate*`. If unset, play the waveform for that kind.

### 1.3 Foreground wiring

- Member: `bool m_applicationForeground = false` on `StreetPixelsManager`. **Default false** — prefer no haptic over vibrating in background.
- API: `void SetApplicationForeground(bool foreground);`
- `Framework::EnterForeground`: set `true` **before** other work that might collect.
- `Framework::EnterBackground`: set `false` **first**.
- Android already maps process `onStart` / `onStop` to `nativeOnTransit`. Screen off that stops the activity is background. Do **not** treat recording FGS as foreground.
- Tests that expect a play call `Manager().SetApplicationForeground(true)`. Do **not** default the collection-gate fixture to foreground; fail-closed must remain the un-armed state.
- No Framework unit test required; manager tests cover the flag. Qt already calls `EnterForeground` / `EnterBackground`; wiring there is free and correct.

### 1.4 Toggle

- C++ key (single source of truth): `street_pixels::kExplorationHapticsSettingsKey` = `"StreetPixels.ExplorationHaptics"`.
- Read: `settings::TryGet(key, enabled)` with `enabled` initialized to `true`. Absent → on. Persist `"true"` / `"false"` via existing `settings::Set(bool)` (`ToString<bool>`).
- Read on every play (no cache). Settings UI writes immediately; next GPS tick / milestone head sees the new value.
- Android: `Config.explorationHapticsEnabled()` / `setExplorationHapticsEnabled(boolean)` wrapping `getBool(KEY, true)` / `setBool`. Mirror zoom buttons.

### 1.5 Collection vs milestone play

Keep `TriggerCollectionVibration(numNewlyExploredPixels)` on the collection path after live writes (do not add a Java GPS vibrate).

```
TriggerCollectionVibration:
  if newly == 0: return
  PlayIfAllowed(Collection)   // one pulse, ignore count beyond >= 1

OnLocationUpdate (after collection):
  bool justCompleted = false
  if newly > 0:
    justCompleted = m_firstGoalTracker.AddNewlyExploredLivePixels(...)
    NotifyFirstGoalProgressIfChanged()
    if justCompleted && m_firstGoalCompleteHandler: handler()
  if justCompleted:
    PlayIfAllowed(FirstGoalComplete)
  else:
    TriggerCollectionVibration(newly)

EmitAreaMilestoneHapticIfNeeded (unchanged event rules):
  if head P50: event FiftyPercent; PlayIfAllowed(FiftyPercent)
  if head P100: event HundredPercent; PlayIfAllowed(HundredPercent)
  if head P25: neither event nor play
```

**Same-update overlap:** first-goal complete happens on the collecting update that crosses 10 new live pixels. Play **only** `FirstGoalComplete` on that update (skip the collection pulse). 50/100 usually play later on cache rebuild (`Invalidate` in `OnLocationUpdate`, rebuild elsewhere), so they do not need a collection skip. If a rebuild ever ran on the same stack as collection, both a collection pulse and a milestone pattern would play; that is acceptable and not worth a cross-path latch.

`PlayIfAllowed(kind)`:

1. Build `ExplorationHapticGate{ IsRecording(), m_applicationForeground, ReadToggle() }`.
2. If `!ShouldPlayExplorationHaptic(gate)` return.
3. If `m_vibrationHandler`: `m_vibrationHandler(kind); return;`
4. `PlayExplorationHapticWaveform(kind)`.

`IsRecording()` is `m_recordingSession && m_recordingSession->IsRecording()`. Null session → not recording.

### 1.6 Waveforms (restrained)

Collection stays the current subtle one-shot. Milestones are clearly stronger, still well under one second, not a long buzz.

| Kind | API | Constants | Total on-device time (approx.) |
| --- | --- | --- | --- |
| `Collection` | `platform::Vibrate(kCollectionPulseMs)` | `kCollectionPulseMs = 50` | 50 ms |
| `FirstGoalComplete` | `VibratePattern` count 2 | durations `{80, 80}`, delays `{90}` | ~250 ms |
| `FiftyPercent` | `VibratePattern` count 2 | durations `{90, 90}`, delays `{110}` | ~290 ms |
| `HundredPercent` | `VibratePattern` count 3 | durations `{80, 80, 120}`, delays `{70, 70}` | ~420 ms |

`Utils.vibratePattern` builds `[0, d0, delay0, d1, …]`. Last delay slot is unused; still pass arrays of equal length `count`. Put named `constexpr` arrays in `exploration_haptics.hpp` next to the enum. Do not restore the old per-pixel `{30 ms × n, 20 ms gap}` pattern.

Desktop `vibration.cpp` remains a no-op; tests never rely on it.

### 1.7 Logging

Do not log latitude, longitude, pixel ids, or GPS timestamps on the haptic path. Existing `OnLocationUpdate` must not gain vibrate-adjacent coordinate logs.

---

## 2. Proposed types / APIs

### 2.1 `libs/map/exploration_haptics.hpp` (+ `.cpp` for `DebugPrint` / toggle read / waveform play)

```cpp
namespace street_pixels
{
inline constexpr char kExplorationHapticsSettingsKey[] = "StreetPixels.ExplorationHaptics";

enum class ExplorationHapticKind : uint8_t
{
  Collection = 0,
  FirstGoalComplete = 1,
  FiftyPercent = 2,
  HundredPercent = 3,
};

struct ExplorationHapticGate
{
  bool recording = false;
  bool foreground = false;
  bool toggleOn = true;
};

bool ShouldPlayExplorationHaptic(ExplorationHapticGate const & gate);
bool ShouldPlayCollectionPulse(ExplorationHapticGate const & gate, size_t newlyExploredPixels);
bool ExplorationHapticsToggleEnabled();  // TryGet; default true

uint32_t constexpr kCollectionPulseMs = 50;
uint32_t constexpr kFirstGoalDurations[] = {80, 80};
uint32_t constexpr kFirstGoalDelays[] = {90, 90};
uint32_t constexpr kFiftyDurations[] = {90, 90};
uint32_t constexpr kFiftyDelays[] = {110, 110};
uint32_t constexpr kHundredDurations[] = {80, 80, 120};
uint32_t constexpr kHundredDelays[] = {70, 70, 70};

void PlayExplorationHapticWaveform(ExplorationHapticKind kind);
std::string DebugPrint(ExplorationHapticKind kind);
}
```

`ShouldPlayExplorationHaptic` = `gate.recording && gate.foreground && gate.toggleOn`.
`ShouldPlayCollectionPulse` = that ∧ `newlyExploredPixels >= 1`.

`ExplorationHapticsToggleEnabled`: `bool enabled = true; settings::TryGet(kExplorationHapticsSettingsKey, enabled); return enabled;`

`PlayExplorationHapticWaveform`: switch on kind; Collection → `Vibrate(50)`; others → `VibratePattern` with the arrays and `std::size(...)`. No settings/foreground checks here.

### 2.2 `StreetPixelsManager`

```cpp
using VibrationHandler = std::function<void(street_pixels::ExplorationHapticKind kind)>;
void SetVibrationHandler(VibrationHandler const & handler);
void SetApplicationForeground(bool foreground);
```

Private: `bool m_applicationForeground = false;`
Private: `void PlayExplorationHaptic(street_pixels::ExplorationHapticKind kind);`  // predicate + hook + waveform
Keep `TriggerCollectionVibration(size_t)` as the collection-path wrapper.

Do **not** add JNI for `SetApplicationForeground` or haptic events.

### 2.3 Android Config / prefs

`Config.java`:

```java
private static final String KEY_PREF_EXPLORATION_HAPTICS = "StreetPixels.ExplorationHaptics";

public static boolean explorationHapticsEnabled()
{
  return getBool(KEY_PREF_EXPLORATION_HAPTICS, true);
}
public static void setExplorationHapticsEnabled(boolean enabled)
{
  setBool(KEY_PREF_EXPLORATION_HAPTICS, enabled);
}
```

`InterfaceSettingsFragment.initExplorationHapticsPrefsCallbacks()`: copy `initZoomPrefsCallbacks` (getPreference → `TwoStatePreference.setChecked` → `Config.set…` on change). Call from `onViewCreated` next to zoom.

---

## 3. Exact files

### 3.1 Add

| Path | Why |
| --- | --- |
| `libs/map/exploration_haptics.hpp` | Gate, kinds, waveform constants, toggle key. |
| `libs/map/exploration_haptics.cpp` | `ShouldPlay*`, `ExplorationHapticsToggleEnabled`, `PlayExplorationHapticWaveform`, `DebugPrint`. |
| `libs/map/street_pixels_tests/exploration_haptics_tests.cpp` | Pure predicate matrix + manager collection/milestone plays. |

### 3.2 Modify

| Path | Why |
| --- | --- |
| `libs/map/CMakeLists.txt` | Add `exploration_haptics.cpp/.hpp` next to `first_goal.cpp`. |
| `libs/map/street_pixels_tests/CMakeLists.txt` | Add `exploration_haptics_tests.cpp`. |
| `libs/map/street_pixels_manager.hpp` / `.cpp` | Foreground flag; new `VibrationHandler`; `PlayExplorationHaptic`; collection + first-goal + area-head consume predicate; `SetApplicationForeground`. |
| `libs/map/framework.cpp` | `EnterForeground` / `EnterBackground` → `m_streetPixelsManager->SetApplicationForeground`. |
| `libs/map/street_pixels_tests/collection_gate_tests.cpp` | Hook signature; **arm foreground** on the positive vibration test. |
| `libs/map/street_pixels_tests/sample_acceptance_manager_tests.cpp` | Hook signature only (`Rejected_NoVibration` still expects 0). |
| `libs/map/street_pixels_tests/ever_live_tests.cpp` | Hook: count calls, do not sum `size_t`. |
| `libs/map/street_pixels_tests/area_milestone_presentation_tests.cpp` | Hook signature. Event tests unchanged. `DoesNotCallCollectionVibration` still 0 (rebuild, not recording, default background). |
| `android/sdk/src/main/java/app/organicmaps/sdk/util/Config.java` | Getter/setter + key. |
| `android/app/src/main/java/app/organicmaps/settings/InterfaceSettingsFragment.java` | Bind switch. |
| `android/app/src/main/res/xml/prefs_interface.xml` | Switch after zoom buttons. |
| `android/app/src/main/res/values/donottranslate.xml` | Pref key string = C++ key. |
| `android/app/src/main/res/values/strings.xml` | Title + summary. **Not** `values-en` (symlink). |
| `docs/implementation/work-items/SP-066-exploration-haptics-policy.md` | Status **In review**, notes, evidence placeholders. Agent does **not** mark Accepted. |
| `docs/implementation/phases/phase-07-milestones-and-share-cards.md` | Current-code haptic / foreground / setting rows. |
| `docs/implementation/README.md` | Phase 7 table: propose SP-066 **In review**. |
| `docs/implementation/DECISIONS.md` | One-line consequence under SPD-054 / OQ-17: coding is SP-066; do not re-open the lock. |

Do **not** edit: `libs/platform/vibration.cpp`, `Utils.vibrate*`, `OrganicMaps.nativeOnTransit`, `area_milestone_presentation` queue logic (except manager consume already there), iOS, first-goal threshold, SP-065 event semantics.

No inline imports. No new comments. No formatting-only changes.

---

## 4. Settings strings and prefs XML

### 4.1 `donottranslate.xml`

```xml
<string name="pref_exploration_haptics" translatable="false">StreetPixels.ExplorationHaptics</string>
```

Place with the other `pref_*` keys. Value **must** equal `kExplorationHapticsSettingsKey`.

### 4.2 `values/strings.xml` (English; `values-en` is a symlink — do not duplicate)

```xml
<string name="pref_exploration_haptics_title">Exploration haptics</string>
<string name="pref_exploration_haptics_summary">Pulse while recording in the foreground</string>
```

Place next to `pref_zoom_title` / `pref_zoom_summary`.

### 4.3 `prefs_interface.xml`

Insert immediately after the zoom `SwitchPreferenceCompat`, before left-button:

```xml
<SwitchPreferenceCompat
  android:key="@string/pref_exploration_haptics"
  android:summary="@string/pref_exploration_haptics_summary"
  android:title="@string/pref_exploration_haptics_title"
  android:widgetLayout="@layout/preference_switch"
  app:singleLineTitle="false" />
```

Do **not** set `android:defaultValue`. Zoom does not; `init*` overwrites from `Config` (C++ default on). Android Preference persistence may also write the key into app SharedPreferences; source of truth for play remains C++ `settings`, same as zoom.

---

## 5. How existing CollectionGate (and sibling) vibration tests change

**Defaults in production and tests**

| Input | Default | Who sets it in tests |
| --- | --- | --- |
| Foreground | `false` | `SetApplicationForeground(true)` on every test that expects a play |
| Toggle | on if key absent | `settings::Delete(kExplorationHapticsSettingsKey)` in cleanup; off-tests `settings::Set(..., false)` |
| Recording | session state | existing `Start` / `Pause` / `Finish` |

Cleanup RAII in new tests (and any test that writes the toggle):

```cpp
settings::Delete(street_pixels::kExplorationHapticsSettingsKey);
settings::Delete("RecordingSessionActive");
```

Do **not** put `SetApplicationForeground(true)` in `CollectionGateFixture`’s constructor. Un-armed tests document fail-closed.

### 5.1 Must change

| Test | Change |
| --- | --- |
| `CollectionGate_Recording_TriggersVibration` | `SetApplicationForeground(true)` after construct (or after Start). Lambda `(ExplorationHapticKind kind) { ++calls; last = kind; }`. Assert `calls == 1` and `kind == Collection`. Toggle left default-on. |
| `CollectionGate_Rejected_NoVibration` | Lambda signature only. Still 0 (idle then paused; never recording+foreground collect). |
| `SampleAcceptanceManager_Rejected_NoVibration` | Lambda signature only. Still 0 (rejected sample never collects). |
| `EverLive_UpgradeDoesNotDoubleCount` | Replace `newlyExplored` sum with a call counter. Still 0 (imported pixel, `numNewlyExploredPixels == 0`). Foreground optional. |
| `AreaMilestonePresentation_DoesNotCallCollectionVibration` | Lambda signature. Still 0: rebuild is not recording (and default background). **Events** still fire via `SetAreaMilestoneHapticHandler`. |
| `AreaMilestonePresentation_FollowingDoesNotStopRoute` | Lambda signature. Still 0 vibration hook: recording but default **background**, so SPD-054 blocks play. Session stays `Recording`. |

`AreaMilestonePresentation_Haptic50And100Not25` stays on the **event** handler. Do not require foreground there.

`FirstGoal_*` tests do not use `SetVibrationHandler`. They keep passing. New first-goal **play** coverage lives in `exploration_haptics_tests.cpp`.

If the hook is compiled against the old `size_t` signature, **the suite will not build**. Update every `SetVibrationHandler` call site in one change.

---

## 6. Test plan

C++ is the gate. Device residual (feel pulse, screen-off, toggle off) → SP-069 / Phase 10. Do not add Android instrumented tests here.

### 6.1 Build / run

From repo root (debug dir per `docs/implementation/README.md` §8.1). This environment’s sources live at `/workspace`; if `build_omim.sh` is not invoked from the tree root, pass `-p /workspace`:

```bash
./tools/unix/build_omim.sh -d street_pixels_tests -p /workspace
/workspace/omim-build-debug/street_pixels_tests --filter=ExplorationHaptic
/workspace/omim-build-debug/street_pixels_tests --filter=CollectionGate
/workspace/omim-build-debug/street_pixels_tests --filter=AreaMilestonePresentation
/workspace/omim-build-debug/street_pixels_tests --filter=FirstGoal
/workspace/omim-build-debug/street_pixels_tests --filter=EverLive_UpgradeDoesNotDoubleCount
/workspace/omim-build-debug/street_pixels_tests --filter=SampleAcceptanceManager_Rejected_NoVibration
```

If the current working directory is already `/workspace` and default `../omim-build-debug` is used:

```bash
./tools/unix/build_omim.sh -d street_pixels_tests
../omim-build-debug/street_pixels_tests --filter=ExplorationHaptic
```

Regression: run the full `street_pixels_tests` binary once after the focused filters. Do not weaken existing tests. Record counts in the work-item evidence table.

Optional: `./tools/unix/run_tests.sh -b /workspace/omim-build-debug -f "ExplorationHaptic"`.

### 6.2 Pure predicate (`exploration_haptics_tests.cpp`, no manager)

`ShouldPlayExplorationHaptic` / `ShouldPlayCollectionPulse` — exhaustive boolean matrix plus pixel counts `{0, 1, 2, 10}`:

| Test name | Asserts |
| --- | --- |
| `ExplorationHaptic_Predicate_AllTrue_Allows` | recording ∧ foreground ∧ toggle → true. |
| `ExplorationHaptic_Predicate_NotRecording_Denies` | Idle/Paused/Finished/Discarded represented as `recording=false` → false even if fg+toggle. |
| `ExplorationHaptic_Predicate_Background_Denies` | `foreground=false` → false. |
| `ExplorationHaptic_Predicate_ToggleOff_Denies` | `toggleOn=false` → false. |
| `ExplorationHaptic_CollectionPulse_ZeroPixels_Denies` | gate allowed, `newly=0` → `ShouldPlayCollectionPulse` false. |
| `ExplorationHaptic_CollectionPulse_OneOrMany_Same` | allowed gate, `newly=1` and `newly=3` both true (count is not a multiplier). |
| `ExplorationHaptic_Toggle_DefaultOnWhenKeyMissing` | `settings::Delete(key)` then `ExplorationHapticsToggleEnabled() == true`. |
| `ExplorationHaptic_Toggle_OffWhenSetFalse` | `settings::Set(key, false)` → `false`; cleanup Delete. |

Do not call `platform::Vibrate` in these tests.

### 6.3 Manager collection

Reuse collection-gate GPS helpers (`CollectionGateFixture` pattern: `SetStreetPixelsForTesting`, `Start`, `OnLocationUpdate`). **Always** `SetVibrationHandler` so JNI is never hit. Arm: `SetApplicationForeground(true)` + delete toggle key unless the test is toggle-off.

| Test name | Asserts |
| --- | --- |
| `ExplorationHaptic_Manager_ZeroNewPixels_NoPulse` | Recording + fg + on; already-explored (or imported live upgrade) update → 0 handler calls. Can share setup with ever-live: mark explored then visit. |
| `ExplorationHaptic_Manager_OneNewPixel_OneCollectionPulse` | Recording + fg + on; one unexplored pixel in radius → exactly 1 call, kind `Collection`. |
| `ExplorationHaptic_Manager_ManyNewPixels_OneCollectionPulse` | Two unexplored pixels ~10 m apart (both inside 25 m radius: `PixelIdForLatLon` of A and of `OffsetLatLonByMeters(lat, lon, 10, 0)`). One GPS at A. Both become explored. Handler calls **1**, kind `Collection`. Not 2. |
| `ExplorationHaptic_Manager_Background_NoCollectionPulse` | Same as one-pixel but **omit** `SetApplicationForeground` (or set false) → 0. Pixel **is** still explored (collection is not foreground-gated). |
| `ExplorationHaptic_Manager_ToggleOff_NoCollectionPulse` | Fg + recording, `settings::Set(key, false)` → 0 plays; pixel still explored. |
| `ExplorationHaptic_Manager_Paused_NoCollectionPulse` | Start, Pause, fg + on → 0 plays and pixel not explored (`IsRecording()` false). |

### 6.4 Manager milestones

Reuse `MakePresAmFixture` from `area_milestone_presentation_tests.cpp` (District osm 10 at 100% of its one universe pixel → queue P100 then P50 then P25). Copy the fixture into the new test file **or** extract a shared helper only if the copy is error-prone; duplicating the small SPA fixture (as SP-065 did vs manager tests) is acceptable.

Arm recording with `RecordingSession::Start` + `SetRecordingSession` + `SetApplicationForeground(true)` + toggle default on when a play is expected.

| Test name | Asserts |
| --- | --- |
| `ExplorationHaptic_Manager_P100ThenP50_PlayOnceEachWhenAllowed` | Handler kinds: `HundredPercent` then after Acknowledge `FiftyPercent`. Event handler may also be set; both fire. After Ack to P25: **no** additional play. Total plays 2. |
| `ExplorationHaptic_Manager_P25_NoPattern` | Drain to P25 head (Ack twice) / or a fixture that only crosses 25% if easier — assert no `VibrationHandler` call whose kind is not 50/100. Combined with the row above is enough if P25 Ack does not increment. |
| `ExplorationHaptic_Manager_Milestone_ToggleOff_NoPlay` | `settings::Set(key, false)`, recording + fg, rebuild → event handler still gets `HundredPercent`; `VibrationHandler` calls 0. |
| `ExplorationHaptic_Manager_Milestone_Background_NoPlay` | Recording, foreground **false**, rebuild → events fire, plays 0. |
| `ExplorationHaptic_Manager_Milestone_NotRecording_NoPlay` | No `Start` (Idle). Fg + toggle on. Rebuild (import-style). Events fire (`HundredPercent` then `FiftyPercent` on Ack). Plays 0. This is the SPD-054 import case. |
| `ExplorationHaptic_Manager_FirstGoalComplete_StrongerOnce` | `FirstGoalFixture` pattern: 12 unexplored, Start, fg on, collect 9 → 9 `Collection` plays, 0 `FirstGoalComplete`. Collect 10th → **one** `FirstGoalComplete`, **no** extra `Collection` on that update. Complete handler still 1 (ungated). |
| `ExplorationHaptic_Manager_FirstGoalComplete_Background_NoPlay` | Same collect-10 with foreground false → complete handler 1, vibration hook 0 (including no collection pulses). |

### 6.5 What not to test here

- Android `Utils.vibrate` / Vibrator permission.
- Screen-off while FGS recording (manual / SP-069).
- iOS `UIFeedbackGenerator`.
- Framework `EnterForeground` integration (trust existing `nativeOnTransit`; manager flag is the unit).

---

## 7. Docs updates (coding agent, after tests)

| Doc | Edit |
| --- | --- |
| `work-items/SP-066-exploration-haptics-policy.md` | Status **In review**. Notes: SPD-054 already Accepted; coding landed predicate + one pulse + toggle + waveforms. Fill branch + test output placeholders. **Do not** set Accepted / Accepted by / date. |
| `phases/phase-07-milestones-and-share-cards.md` | Current-code: collection haptic is one pulse, foreground-gated, toggle exists; `SetApplicationForeground` from `EnterForeground`; setting row no longer “absent”. Phase-entry “haptics still not foreground-gated / per-pixel” sentence → done for SP-066 (device feel still SP-069). |
| `README.md` Phase 7 table | SP-066 → **In review** — recording ∧ foreground ∧ toggle; one collection pulse; 50/100/first-goal patterns. |
| `DECISIONS.md` SPD-054 Consequences | Add: SP-066 implements the predicate in `exploration_haptics` + manager; default foreground false; toggle key `StreetPixels.ExplorationHaptics`. OQ-17 stays struck / closed. Do not change Status. |

---

## 8. Risks / non-goals

### Risks

- **Default foreground false breaks** `CollectionGate_Recording_TriggersVibration` until it arms foreground. That update is in-scope, not a test weaken.
- **Hook signature change** is a compile break until every `SetVibrationHandler` lambda is updated.
- **Missed `EnterForeground`** (if `nativeOnTransit(true)` never runs) → no exploration haptics. Prefer that over background buzz. Existing crash-if-`g_framework` null is pre-existing.
- **Same-tick first-goal + collection:** skipped collection pulse (see §1.5). If coding keeps both, the first-goal manager test must assert 2 calls on the 10th pixel; do not mix the two behaviors.
- **Toggle key mismatch** between XML, `Config`, and C++ → switch that does not affect play. Use one literal `StreetPixels.ExplorationHaptics`.
- **Per-pixel regression:** do not leave the `numNewlyExploredPixels > 1` `VibratePattern` branch.

### Non-goals

- Strength sliders (§28.4).
- Boss haptic (Phase 8).
- Routing / UI haptics.
- iOS `UIFeedbackGenerator`.
- JNI for first-goal complete or area haptic events (still unused on Java).
- Gating collection itself on foreground (pixels still collect in background while recording).
- Marking SP-066 Accepted.

---

## 9. Acceptance criteria → test / residual

| # | Criterion | Evidence |
| --- | --- | --- |
| 1 | Recording + foreground + toggle on + ≥1 new pixel → exactly one collection pulse per update | `ExplorationHaptic_Manager_OneNewPixel_OneCollectionPulse`; `CollectionGate_Recording_TriggersVibration` (updated). |
| 2 | Recording + background or screen-off → no exploration haptic | Manager: `…_Background_NoCollectionPulse`, `…_Milestone_Background_NoPlay`. Screen-off ≡ `EnterBackground` via existing `onStop`. Device feel → SP-069. |
| 3 | Not recording (including paused) → no exploration haptic | `ExplorationHaptic_Manager_Paused_NoCollectionPulse`; `…_Milestone_NotRecording_NoPlay`; existing CollectionGate idle/paused/finished/discarded (no collect ⇒ no collection pulse). |
| 4 | Toggle off suppresses collection and milestone patterns | `…_ToggleOff_NoCollectionPulse`; `…_Milestone_ToggleOff_NoPlay`. Events may still fire. |
| 5 | Multi-pixel updates do not produce per-pixel pulses | `ExplorationHaptic_Manager_ManyNewPixels_OneCollectionPulse`; predicate `…_OneOrMany_Same`. |
| 6 | Foreground + toggle on + first-goal / 50% / 100% → stronger pattern once; 25% none | `…_FirstGoalComplete_StrongerOnce`; `…_P100ThenP50_PlayOnceEachWhenAllowed`; `…_P25_NoPattern`. Existing `AreaMilestonePresentation_Haptic50And100Not25` for **events**. |
| 7 | Automated predicate / manager tests exist | `exploration_haptics_tests.cpp` + commands in §6.1. |

Manual work-item steps (record in foreground; screen off; toggle off) remain device residual → SP-069 / Phase 10.
