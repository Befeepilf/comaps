# SP-068 — Share flow and growth analytics (implementation plan)

**Status:** Draft plan for coding; not Accepted.
**Work item:** [`SP-068-share-flow-and-growth-analytics.md`](../work-items/SP-068-share-flow-and-growth-analytics.md)
**Date:** 2026-08-20
**Locks (do not re-open):** SPD-051 (share-time date opt-in, default off), SPD-055 (count-only card-generated and share-initiated; no area id / coords / image bytes; not Sentry; Phase 10 upload residual), SPD-046 (share the rings-only card image, never a map/track), SPD-044 pattern (uint64 settings counters). SPD-008 Android V1.

This note is the coding contract. Do not auto-open the share sheet, attach KML/GPX/tracks, call `SharingUtils.shareLocation`, or put area identifiers on counters.

---

## 0. Status of building blocks (verified 2026-08-20)

| Piece | Actual state | SP-068 action |
| --- | --- | --- |
| 100% Share chrome | `area_completion_card_share` listener is `v -> {}`. | Own the tap: `ACTION_SEND` `image/png` of the transient file. |
| `CompletionCardModel` | Permit-list; `GetCompletionCardForCurrentPresentation(includeDate)` fires generated hook on every success. | Display path records generated. Share/rebind pass `recordGenerated=false`. |
| Transient PNG | `WriteCompletionCardTransient` → `TmpPathForFile("street_pixels_completion_card.png")`. Delete on P100 Acknowledge. | Write on Share tap before chooser. Keep Acknowledge delete. Do not accumulate random tmp names. |
| FileProvider | `file_paths.xml` has `cache-path`. Android `TmpDir` is `context.getCacheDir()`. | `StorageUtils.getUriForFilePath`. Grant read. No gallery/MediaStore. |
| `SharingUtils` | `shareLocation` (`geo:` / ge0), `shareFile` (`ACTION_SEND_MULTIPLE` + KML/GPX + `CREATE_DOCUMENT`). | **Do not call.** Dedicated image-only helper. |
| Date | Source has `m_completed100At`. Default compose omits. | MaterialCheckBox on the 100% card, **unchecked**. On → `includeDate=true`. |
| Generated hook | `SetCompletionCardGeneratedHandler` exists; nothing sets it. | Framework wires `CompletionCardAnalytics::RecordGenerated`. |
| Counters | Routing `StreetExplorationRoutingAnalytics` (SPD-044). No card counters. | New `CompletionCardAnalytics` with two uint64 keys. |
| Sentry | Not a product sink. | Do not `captureMessage`. |

No inline imports. No new comments. No formatting-only changes. English strings: `values/strings.xml` only.

---

## 1. Architecture

**C++ owns counters, share payload, and write.** `CompletionCardAnalytics` increments two settings keys. `PrepareCompletionCardShare(includeDate)` composes (no generated count), writes the fixed PNG, returns path + `image/png` + first-person text from permit-list labels only. `RecordShareInitiated` is a separate call from the tap path only.

**Android owns the chooser.** Share tap builds `ACTION_SEND` with `EXTRA_STREAM` = FileProvider URI of that path. Optional `EXTRA_TEXT` is the payload text. Date checkbox default off; toggling rebinds the on-screen card with `recordGenerated=false`.

Appear / 100% fire / animation end must not prepare share or increment share-initiated.

---

## 2. Exact files

### 2.1 Add

| Path | Why |
| --- | --- |
| `libs/map/completion_card_analytics.hpp` / `.cpp` | Count-only generated + share-initiated. |
| `libs/map/street_pixels_tests/completion_card_share_tests.cpp` | Gate tests (unique `ShAm*` fixture names). |
| `android/sdk/.../CompletionCardSharePayload.java` | JNI mirror: path, mime, text. No osmId. |
| `android/app/.../CompletionCardShare.java` | Image-only `ACTION_SEND`. |

### 2.2 Modify

| Path | Why |
| --- | --- |
| `libs/map/CMakeLists.txt` | Analytics sources. |
| `libs/map/street_pixels_tests/CMakeLists.txt` | Share tests. |
| `libs/map/street_pixels_manager.hpp` / `.cpp` | `recordGenerated`; prepare share; record share. |
| `libs/map/framework.cpp` | Wire generated handler. |
| JNI + `StreetPixelsManager.java` | Two-arg getter; prepare; record share. |
| `area_completion_card.xml` | Date checkbox default unchecked. |
| `MapButtonsController.java` | Share tap + checkbox. |
| `values/strings.xml` | Include-date label. Not `values-en`. |
| Work item / phase-07 / README / SPD-055 Consequences | In review; Phase 10 residual. Not Accepted. |

### 2.3 Do not edit

`SharingUtils.java` (`shareLocation` / `shareFile`), `qt/screenshoter.*`, compositor deny-list type, `area_milestone_store` fire policy, Sentry, routing analytics keys, iOS.

---

## 3. APIs

```cpp
// completion_card_analytics.hpp — keys must not contain lat/lon/pixel/area/coord/mwm/country/geometry
inline constexpr std::string_view kCardGeneratedKey = "Explore.CardGenerated";
inline constexpr std::string_view kShareInitiatedKey = "Explore.ShareInitiated";
inline constexpr std::string_view kCardGeneratedName = "card-generated";
inline constexpr std::string_view kShareInitiatedName = "share-initiated";

struct CompletionCardAnalyticsSnapshot { uint64_t m_generated = 0; uint64_t m_shareInitiated = 0; };

class CompletionCardAnalytics {
  static void RecordGenerated();
  static void RecordShareInitiated();
  static CompletionCardAnalyticsSnapshot LoadSnapshot();
  static std::array<std::pair<std::string_view, uint64_t>, 2> SerializedSnapshot();
  static void ResetForTesting();
};
```

```cpp
struct CompletionCardSharePayload {
  std::string m_path;
  std::string m_mimeType;  // "image/png"
  std::string m_text;
};

GetCompletionCardForCurrentPresentation(bool includeDate = false, bool recordGenerated = true);

std::optional<CompletionCardSharePayload> PrepareCompletionCardShare(bool includeDate);
void RecordCompletionCardShareInitiated();
```

`PrepareCompletionCardShare`: require P100 + source; compose with `recordGenerated=false`; `WriteCompletionCardTransient`; text = `CompletionCardLabelText(model)` (no ring coordinates). Fail closed → `nullopt` (no share increment).

Share tap (Android) only: prepare → chooser → `RecordCompletionCardShareInitiated`. Do not record share if prepare fails.

Generated: Framework sets handler to `RecordGenerated`. JNI display getter uses `recordGenerated=true`. Checkbox / prepare use false.

---

## 4. Tests (`street_pixels_tests --filter=CompletionCardShare`)

| Test | Asserts |
| --- | --- |
| `CompletionCardShare_AnalyticsDefaultZero` | Snapshot 0/0. |
| `CompletionCardShare_AnalyticsKeysHaveNoLocationOrArea` | Key strings, names, `DebugPrint` contain none of: lat, lon, latitude, longitude, geometry, polyline, pixel, area, coord, mwm, country, osm, healpix, gps, geo:, ge0. Names lowercase. |
| `CompletionCardShare_GeneratedIncrementsOnDisplayGet` | Handler wired; `Get(..., recordGenerated=true)` +1; second display Get +2. `Get(..., false)` does not increment. Share count stays 0. |
| `CompletionCardShare_ShareIncrementsOnlyOnRecord` | Rebuild 100% does not increment share. `RecordCompletionCardShareInitiated` +1. |
| `CompletionCardShare_PrepareUsesTransientPngNotTrack` | Path equals `CompletionCardTransientPath()`; suffix `.png`; under `TmpDir`; not `.kml`/`.gpx`/`.kmz`; mime `image/png`. |
| `CompletionCardShare_DateOptInDefaultOff` | Prepare(false): text/model omit date even if `m_completed100At` stored. |
| `CompletionCardShare_IncludeDateWhenRequested` | Prepare(true): YYYY-MM-DD, no `T` / `:`. |
| `CompletionCardShare_TextHasNoCoordinates` | Payload text has no deny tokens (same list as label test). |
| `CompletionCardShare_PrepareFailsWithoutHundredPercent` | After ack to P50, prepare is nullopt; share count unchanged. |

Also keep existing `CompletionCard` 10 tests green.

---

## 5. Docs after tests

Work item **In review**; branch; test counts; upload residual = Phase 10 / SPD-055. README SP-068 In review. phase-07 current-code: Share tap opens image sheet; counters exist; date checkbox default off. SPD-055 Consequences: SP-068 implements the two keys. Agent does **not** mark Accepted.

---

## 6. Non-goals

Device chooser eyeball (SP-069). Gallery persist. New network endpoint. Sentry. Competition share copy. Changing the 4s auto-ack duration (share writes the PNG immediately; delete still on Acknowledge).
