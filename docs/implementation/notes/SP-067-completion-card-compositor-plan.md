# SP-067 — Completion-card compositor (implementation plan)

**Status:** Draft plan for coding; not Accepted.
**Work item:** [`SP-067-completion-card-compositor.md`](../work-items/SP-067-completion-card-compositor.md)
**Date:** 2026-08-20
**Locks (do not re-open):** SPD-046 (rings-only outline; never Drape/MapView), SPD-051 (store date always; card date default omitted; opt-in UI is SP-068), SPD-052 (first-person; no nickname required; competition stub empty; never “invalid completion”). SPD-008 (cards in Android V1).

This note is the coding contract. Do not invent a MapView screenshot, a stylised-map spike, a share sheet, a date toggle, or a growth counter.

---

## 0. Status of building blocks (verified 2026-08-20)

| Piece | Actual state | SP-067 action |
| --- | --- | --- |
| `ExplorationArea::m_rings` | Mercator outer rings only; holes not stored (`areas_types.hpp`). `FilterExplorationCandidate` converts lon/lat → Mercator and drops a duplicate closing vertex. | Copy **outers** into the card model. Never HEALPix, never GPS, never overlay clip. |
| `DisplayName` | `area.m_name` only (`exploration_sidecar.cpp`). Blank → empty. Never MWM / country id. | Card title uses this string. Drop compose if blank. |
| `FindAreaByCompactIndex` | Sentinel / OOB → `nullptr`. Compact index is the SPA row index. | Resolve rings at **ingest** (SPA file is in scope). Do not rely on `m_cachedFocusSpaFile` later. |
| `AreaMilestoneRecord::m_completed100At` | `optional<int64_t>` unix-ish seconds from `EvaluateAndRecordFires(..., nowSec)` with `base::Timer::LocalTime()`. Always stored on first 100% (SP-063). | Keep on compositor **input**. Output model includes a calendar date **only** if `includeDate == true`. Default `false`. |
| `AreaMilestonePresentation` | Queue head: osmId, compactIndex, threshold, displayName, empty `m_competitionLine`. JNI mirror exists. | Do **not** put deny-list fields or rings on this JNI type. Add a sibling card model. |
| 100% surface | `area_completion_card.xml` included in `map_exploration_banner.xml`. `MapButtonsController.applyAreaMilestonePresentation` shows title + `street_pixels_completion_card_body` + Share. Share click is `v -> {}`. | Keep chrome. Add outline view + branding. Nickname/date views GONE unless present. Share stays no-op. |
| `SharingUtils` | KML/GPX/text + `shareLocation` (`geo:` / ge0 from live `Location`). | Do **not** call. Do not attach tracks. |
| `qt/screenshoter.*` | Desktop QA map capture. | Forbidden. |
| `area_overlay` / Drape | In-app rings with pixels, puck, routes. `BuildAreaOverlayGeometry` clips nested winners. | In-app only. Do not screenshot. Do not reuse clip order as share geometry. |
| PNG helpers | `chart_generator` rasters altitude charts with **AGG** (route geometry — do not reuse). `3party/stb_image` already compiles `stb_image_write.h`. `coding` PNG decoder test is commented out. No neighbourhood PNG writer. | Headless outline raster in C++ (tests). Android Canvas draws the on-screen card from the JNI model. Transient overwrite file in `TmpDir`. |
| Nickname | `IdentityStore::HasUsername` / `GetUsername` (`Explore.Username`). | Pass into compose only when non-empty. Omit when absent. No nickname UI. |
| Analytics | No card-generated counter. SP-068 owns it (SPD-055). | Optional `std::function<void()>` hook; **no** payload; do not increment settings. |
| City-summary | Not enqueued by SP-065. | Do not compose a city card. |

Do not change `area_milestone_store` fire policy, first-goal, haptics, overlay styles, or `SharingUtils`.

No inline imports. No new comments. No formatting-only changes. English strings: `values/strings.xml` only (`values-en` is a symlink — do not edit it as a second file).

---

## 1. Architecture

### 1.1 Split

**C++ (`libs/street_pixels_areas`) owns the shared card model and rings-only raster path.** Privacy is construction: the output type has only permitted fields. Compose is a pure function of area name, outer rings, optional nickname, optional date, branding, competition stub. Headless raster maps those rings into a padded pixel box and strokes the outline. Tests assert the model and the raster without Android.

**C++ (`libs/map`) owns binding from the 100% presentation queue.** At ingest, while `resolver.GetFile()` is in scope, copy rings + stored 100% timestamp into a per-queue-item **source** on P100 entries. `GetCompletionCard(includeDate=false)` composes from that source. Do not reload Drape. Do not read live location.

**Android owns on-screen card chrome.** Canvas (or a custom `View`) draws the outline from the JNI model. Text comes from permitted string fields and existing SP-065 copy. JNI exposes the **model**, never a Drape/`MapView` bitmap. Share tap stays no-op (SP-068).

If coding cannot rasterise the outline without capturing the map, **stop and escalate**. Prefer no share image.

### 1.2 Card model fields (permit list)

Output type `street_pixels::CompletionCardModel`:

| Field | Type | Rule |
| --- | --- | --- |
| `m_areaDisplayName` | `std::string` | `DisplayName` only. Never MWM / country / OSM id / compact index. |
| `m_headline` | `std::string` | Stable “100% explored” (English constant in C++). Not a coordinate. |
| `m_outlineRings` | `std::vector<std::vector<m2::PointD>>` | Copy of `ExplorationArea::m_rings` (Mercator outers). Drawing input, not a label. |
| `m_nickname` | `std::optional<std::string>` | Absent/empty → field omitted (`nullopt`). |
| `m_completedDate` | `std::optional<std::string>` | Calendar `YYYY-MM-DD` only if `includeDate`. Default construction omits. No clock time, no visit timestamps. |
| `m_branding` | `std::string` | `"Street Pixels"`. |
| `m_competitionLine` | `std::string` | SPD-052 stub; empty until Phase 8. |

**The type must not have members for:** route/track geometry, home, live location, position marker, visit timestamps, other users, pixel ids / HEALPix, MWM / country id, osmId, compactIndex, lat/lon label strings, `geo:` / ge0 URLs.

Mercator `PointD` on `m_outlineRings` is share **geometry**, allowed by SPD-046. It must never be formatted into text, `contentDescription`, logs, or analytics.

### 1.3 Deny-list strategy

Two layers, both required:

1. **Type-level.** Permit-list struct above. No deny-list members. `PresentFieldNames(model)` returns only keys that are populated (`nickname` / `completedDate` omitted when `nullopt`). Canonical key strings: `areaDisplayName`, `headline`, `outlineRings`, `nickname`, `completedDate`, `branding`, `competitionLine`.
2. **Serialisation-level.** Concatenate all **text** fields (not ring coordinates). Assert deny tokens are absent as keys **and** as label text: `route`, `track`, `home`, `liveLocation`, `latitude`, `longitude`, `lat`, `lon`, `geo:`, `ge0`, `healpix`, `pixelId`, `mwmId`, `countryId`, `osmId`, `compactIndex`, `userId`, `visitTimestamp`, `positionMarker`, `gps`. Fixture osmId / MWM leaf / compact index must not appear in text fields or `DebugPrint` of the **labels**.

Do not JSON-dump ring x/y as `"lat"` / `"lon"`. If a debug dump of geometry is needed, name it `outlineRings` only.

### 1.4 Who rasterises

| Surface | Renderer | Input | Output |
| --- | --- | --- | --- |
| C++ tests (gate) | Headless stroker in `completion_card.cpp` | `CompletionCardModel` | RGBA buffer + optional transient PNG |
| Android 100% card | `Canvas` / custom `View` | JNI model (projected outline + text) | On-screen card. **Not** a map capture. |
| OS share sheet | SP-068 | Transient file from this item | `ACTION_SEND` image |

Algorithm (both C++ and Canvas; do not invent a second geometry source):

1. Bounding box of all outline vertices (`m2::RectD`, `Add(point)`).
2. Pad ~8% (keep the stroke inside the bitmap).
3. Uniform scale to a canonical outline box (`kCompletionCardOutlineSize` = 512). Y-flip so +y is down on screen. **No map projection, no tile fetch, no Drape camera.**
4. Stroke closed paths only. No fill of explored cells. No markers.
5. Do not `drawText` of numbers from the coordinates.

C++ raster may fill the **card** background and leave a rectangle for text; it must not draw a street map. Tests assert outline pixels, not typography.

Do **not** link `chart_generator` (it is a route-altitude chart). AGG is optional; a Bresenham / grid stroker in this library is enough. PNG encode: `stbi_write_png` from existing `stb_image` (already builds write). Do not add libpng.

### 1.5 Transient path

- Path: `GetPlatform().TmpPathForFile("street_pixels_completion_card.png")` — **fixed name**, `TmpDir() + file`.
- Do **not** call `TmpPathForFile()` or `TmpPathForFile(prefix, suffix)` (those are random and would accumulate).
- Do **not** write under `WritableDir()` (maps) or the gallery.
- Each successful raster **overwrites** that one file.
- `DeleteCompletionCardTransient()` on P100 Acknowledge / card hide (Android dismiss and C++ Acknowledge path). SP-068 may also delete after share hand-off.
- Tests: two rasters → one path; after delete, file gone.

### 1.6 How the 100% surface binds the outline

`StreetPixelsManager` does **not** keep the rebuild `SpaFile` after ingest. Focus cache is the wrong SPA. Therefore:

1. Extend the presenter **queue item** (private) with `std::optional<CompletionCardSource>` filled **only** for `P100`.
2. `IngestPendingAreaMilestonePresentations(file)` already has `FindAreaByCompactIndex`. For each P100 crossing, copy `DisplayName`, `m_rings`, `Get(osmId)->m_completed100At`, and `m_competitionLine` into `CompletionCardSource`. Skip card source if name empty (already dropped) or rings empty (fail closed: copy-only chrome, no image).
3. `GetCompletionCardForCurrentPresentation(includeDate = false)`: Peek; if not P100 or no source → `nullopt`. Else `ComposeCompletionCard(source, {includeDate, nickname})`.
4. Nickname: if `IdentityStore::HasUsername()`, pass `GetUsername()`; else omit. Date still omitted unless `includeDate`.
5. JNI `nativeGetCurrentCompletionCard(boolean includeDate)` returns the Java model or `null`.
6. `MapButtonsController.applyAreaMilestonePresentation`: on `THRESHOLD_100`, fetch card with `includeDate=false`, bind outline view + branding; nickname/date GONE if null; keep existing title/body strings. 25/50: hide outline extras. Share listener unchanged (`{}`).
7. One-line hook: after a **successful** compose for display, call `m_completionCardGeneratedFn` if set. No arguments. Do not pass osmId. Do not increment a counter here. Android may call JNI compose once per 100% head (same as today’s apply).

City-summary never reaches this path (not in the presenter queue).

---

## 2. Exact files

### 2.1 Add

| Path | Why |
| --- | --- |
| `libs/street_pixels_areas/completion_card.hpp` | Permit-list model, source, compose, project, raster, transient path, field-name helpers. |
| `libs/street_pixels_areas/completion_card.cpp` | Compose, deny-list names, rings→pixels, RGBA stroke, PNG overwrite, delete. |
| `libs/map/street_pixels_tests/completion_card_tests.cpp` | **Gate tests** (deny-list, omit nick/date, rings-only, bind from 100% peek, transient file). |
| `android/sdk/src/main/java/app/organicmaps/sdk/maplayer/streetpixels/CompletionCardModel.java` | JNI mirror. Nullable nickname/date. No osmId. |
| `android/app/src/main/java/app/organicmaps/maplayer/CompletionCardOutlineView.java` | Canvas stroke of JNI outline. No coordinate text. |

### 2.2 Modify

| Path | Why |
| --- | --- |
| `libs/street_pixels_areas/CMakeLists.txt` | Add `completion_card.cpp/.hpp`. Link `stb_image` **PRIVATE** if PNG write is used. |
| `libs/map/street_pixels_tests/CMakeLists.txt` | Add `completion_card_tests.cpp`. |
| `libs/map/area_milestone_presentation.hpp` / `.cpp` | Queue item carries optional `CompletionCardSource` for P100. Lookup at `Enqueue`. Peek source API. |
| `libs/map/street_pixels_manager.hpp` / `.cpp` | Fill source at ingest; `GetCompletionCardForCurrentPresentation`; generated hook; delete transient on P100 Acknowledge. |
| `android/sdk/src/main/cpp/.../streetpixels/StreetPixelsManager.cpp` | `ToJavaCompletionCardModel`; `nativeGetCurrentCompletionCard`. |
| `android/sdk/src/main/java/.../StreetPixelsManager.java` | Native + getter. |
| `android/app/src/main/res/layout/area_completion_card.xml` | Outline view, branding, optional nickname/date/competition (default `gone`). |
| `android/app/src/main/java/.../MapButtonsController.java` | Bind 100% outline from model; extras GONE when absent; Share remains no-op. |
| `android/app/src/main/res/values/strings.xml` | Branding (+ optional date format if shown). **Not** `values-en`. |
| `docs/implementation/work-items/SP-067-completion-card-compositor.md` | Status **In review**; evidence placeholders. Agent does **not** mark Accepted. |
| `docs/implementation/phases/phase-07-milestones-and-share-cards.md` | Current-code: compositor exists; rings-only; Share still no-op. |
| `docs/implementation/README.md` | SP-067 **In review**. |
| `docs/implementation/DECISIONS.md` SPD-046 Consequences | One-liner: SP-067 composes `CompletionCardModel` from `m_rings` and rasterises off-map. |

### 2.3 Do not edit

`SharingUtils.java` (including `shareLocation`), `qt/screenshoter.*`, `area_overlay.cpp` / drape overlay shaders, `area_milestone_store` fire policy, `chart_generator.*`, iOS, SP-068 counter keys, `values-en/strings.xml`.

---

## 3. Proposed types / APIs

### 3.1 `street_pixels` in `completion_card.hpp`

```cpp
inline constexpr char const kCompletionCardHeadline[] = "100% explored";
inline constexpr char const kCompletionCardBranding[] = "Street Pixels";
inline constexpr char const kCompletionCardTransientFile[] = "street_pixels_completion_card.png";
inline constexpr uint32_t kCompletionCardOutlineSize = 512;

struct CompletionCardOptions
{
  bool includeDate = false;
  std::optional<std::string> nickname;
};

struct CompletionCardSource
{
  std::string m_displayName;
  std::vector<std::vector<m2::PointD>> m_rings;
  std::optional<int64_t> m_completed100At;
  std::string m_competitionLine;
};

struct CompletionCardModel
{
  std::string m_areaDisplayName;
  std::string m_headline;
  std::vector<std::vector<m2::PointD>> m_outlineRings;
  std::optional<std::string> m_nickname;
  std::optional<std::string> m_completedDate;
  std::string m_branding;
  std::string m_competitionLine;
};

std::vector<std::string> CompletionCardPermittedKeys();
std::vector<std::string> CompletionCardDeniedKeys();
std::vector<std::string> PresentFieldNames(CompletionCardModel const & model);
std::string CompletionCardLabelText(CompletionCardModel const & model);

std::optional<CompletionCardModel> ComposeCompletionCard(CompletionCardSource const & source,
                                                         CompletionCardOptions const & options = {});

std::vector<std::vector<m2::PointD>> ProjectOutlineToPixels(
    std::vector<std::vector<m2::PointD>> const & rings, uint32_t width, uint32_t height, double padFraction = 0.08);

bool RasteriseCompletionCard(CompletionCardModel const & model, uint32_t width, uint32_t height,
                             std::vector<uint8_t> & rgba8888);
std::string CompletionCardTransientPath();
bool WriteCompletionCardTransient(CompletionCardModel const & model);
void DeleteCompletionCardTransient();

std::string DebugPrint(CompletionCardModel const &);
```

`ComposeCompletionCard` returns `nullopt` if `m_displayName` is blank or `m_rings` is empty / all rings `< 3` vertices. Default `options` omits date and nickname. `includeDate` formats `m_completed100At` as `YYYY-MM-DD` (no time); if timestamp missing, still succeed with date omitted. Nickname: skip if `!options.nickname` or empty/whitespace.

`CompletionCardLabelText` concatenates name, headline, branding, competition, nickname, date — **not** ring coordinates — for deny-list string asserts.

`RasteriseCompletionCard` strokes projected rings into RGBA (opaque background, stroke contrast). Tests use this buffer. `WriteCompletionCardTransient` rasters then `stbi_write_png` to `CompletionCardTransientPath()`, overwriting.

### 3.2 Presenter (`area_milestone_presentation.hpp`)

Keep `AreaMilestonePresentation` fields unchanged (osmId stays on the **queue** type, not the card).

```cpp
using CardSourceLookup = std::function<std::optional<CompletionCardSource>(uint32_t compactIndex, uint64_t osmId)>;

void Enqueue(std::vector<AreaMilestoneCrossing> const & crossings, NameLookup const & names,
             CardSourceLookup const & cards = {});
std::optional<CompletionCardSource> PeekCardSource() const;
```

Existing `Enqueue` call sites: add a lookup that returns `nullopt` for non-P100. Ingest in the manager:

```cpp
m_areaMilestonePresenter.Enqueue(crossings, nameLookup,
  [&file](uint32_t compactIndex, uint64_t osmId) -> std::optional<CompletionCardSource> {
    auto const * area = FindAreaByCompactIndex(file, compactIndex);
    if (!area || area->m_rings.empty())
      return std::nullopt;
    CompletionCardSource src;
    src.m_displayName = DisplayName(*area);
    src.m_rings = area->m_rings;
    if (auto rec = AreaMilestoneStore::Instance().Get(osmId))
      src.m_completed100At = rec->m_completed100At;
    return src;
  });
```

Set `m_competitionLine` from the same provider already used for presentation (default empty).

### 3.3 `StreetPixelsManager`

```cpp
using CompletionCardGeneratedFn = std::function<void()>;
void SetCompletionCardGeneratedHandler(CompletionCardGeneratedFn const & fn);
std::optional<street_pixels::CompletionCardModel> GetCompletionCardForCurrentPresentation(
    bool includeDate = false) const;
```

`GetCompletionCardForCurrentPresentation`:

- Peek presentation; require `P100` and `PeekCardSource()`.
- `CompletionCardOptions{includeDate, nick}` with nick from `IdentityStore` only if `HasUsername()`.
- Compose. On success, invoke hook (from the JNI/Android display path, or from getter if that is the single call site — **once per apply**, not per Peek). Prefer: hook inside the JNI getter after compose, so tests that only call C++ compose do not increment anything; manager tests that want the hook can call the getter.

`AcknowledgeAreaMilestonePresentation`: if the acknowledged head was P100, `DeleteCompletionCardTransient()`.

### 3.4 JNI / Java

`CompletionCardModel` constructor: `(String name, String headline, float[] xs, float[] ys, int[] ringLengths, String nicknameOrNull, String dateOrNull, String branding, String competitionLine)`. Flatten rings in order. **No** osmId, compactIndex, lat, lon, geo.

`StreetPixelsManager.nativeGetCurrentCompletionCard(boolean includeDate)`.

Do not add a `Bitmap` JNI that reads Drape.

### 3.5 Android Canvas (no coordinates in labels)

`CompletionCardOutlineView`:

- `setOutline(float[] xs, float[] ys, int[] ringLengths)` from the JNI model (already in canonical pixel space **or** view maps canonical 512 box onto its bounds with a scale matrix).
- `onDraw`: `Path` + `Paint.Style.STROKE`. No `drawText`. `setContentDescription` uses area display name or `street_pixels_area_milestone_100` — never formatted x/y.
- Hide the view if arrays empty.

Layout (`area_completion_card.xml`): keep title, body, Share. Add:

- `area_completion_card_outline` (`CompletionCardOutlineView`)
- `area_completion_card_nickname` (`gone`)
- `area_completion_card_date` (`gone`)
- `area_completion_card_competition` (`gone`)
- `area_completion_card_branding`

`applyAreaMilestonePresentation` THRESHOLD_100: `UiUtils.show` outline + branding; `UiUtils.showIf` nickname/date/competition on non-empty strings. Default compose → nickname and date stay `gone`.

Strings (English, `values/strings.xml` only):

| id | text |
| --- | --- |
| `street_pixels_completion_card_branding` | `Street Pixels` |
| `street_pixels_completion_card_headline` | `100% explored` |

Do not add “invalid completion”. Do not put lat/lon in strings. Reuse `street_pixels_area_milestone_100` / `street_pixels_completion_card_body` for title/body. Date, when SP-068 passes `includeDate`, is the model’s `YYYY-MM-DD` as plain text (no new toggle in this item).

---

## 4. Test names, asserts, commands

Gate binary: `street_pixels_tests`. Reuse presentation SPA fixtures (`PresAmLonLatBox` / District rings) or a **direct** `CompletionCardSource` with a known Mercator rectangle (preferred for equality — filter drops the closing vertex).

### 4.1 Required

| Test | Asserts |
| --- | --- |
| `CompletionCard_DenyListFieldsAbsent` | `PresentFieldNames(model)` ⊆ `CompletionCardPermittedKeys()`. Every `CompletionCardDeniedKeys()` token is absent from present keys **and** from `CompletionCardLabelText(model)`. Source area `m_osmId = 999`, `m_compactIndex = 7`; those decimal strings must not appear in label text. Type has no route/home/lat members — enforced by compiling against `CompletionCardModel` and the key list matching the struct (if a new field is added, this test fails until it is permit-listed; deny-listed names must never be added). |
| `CompletionCard_ComposeWithoutNicknameOrDate` | `ComposeCompletionCard(source)` with `m_completed100At = 1700000000` and default options → model engaged; `!m_nickname`; `!m_completedDate`; name/headline/branding set; `m_competitionLine.empty()`. |
| `CompletionCard_RingsOnlyGeometryMatchesOutline` | Source rings = rectangle `{{0,0},{10,0},{10,5},{0,5}}` (4 vertices). `model.m_outlineRings == source.m_rings`. A separate GPS-like polyline `{{1,1},{2,2},{3,1}}` is **not** stored. Projected pixels: min/max x/y form an axis-aligned rectangle (width≠height), not a single diagonal. Raster RGBA: ink near the four side midpoints; centre pixel not a track (background). |

### 4.2 Also required for AC / regression

| Test | Asserts |
| --- | --- |
| `CompletionCard_IncludeDateOnlyWhenRequested` | Default omit; `includeDate=true` → `m_completedDate` is `YYYY-MM-DD` (10 chars, two `'-'`), no `'T'` / `':'`. Missing timestamp + `includeDate` still composes with date omitted. |
| `CompletionCard_NicknameOmittedWhenEmpty` | `nickname = ""` or whitespace → `nullopt`. Non-empty → equal to input. |
| `CompletionCard_DisplayNameNeverMwmId` | Name `"District"`; label text does not contain a fake leaf `"Finland_FakeLeaf"`. |
| `CompletionCard_CompetitionLineStubEmpty` | Empty stub. Body/headline do not contain `"invalid"`. |
| `CompletionCard_EmptyRingsNoCard` | Empty rings → `nullopt`. Prefer no image. |
| `CompletionCard_TransientFileOverwritesAndDeletes` | `WriteCompletionCardTransient` twice → same `CompletionCardTransientPath()`; file exists. `DeleteCompletionCardTransient` → gone. Path contains tmp dir, not `WritableDir()` as a gallery dump. |
| `CompletionCard_ManagerBindsFromHundredPercentPeek` | Existing 100% presentation fixture (District). After rebuild, Peek is P100. `GetCompletionCardForCurrentPresentation()` equals `FindAreaByCompactIndex` rings for that compact index, not `samples` GPS points. `includeDate=false` omits date even though `GetAreaMilestoneRecord` has `m_completed100At`. 25/50 peek → `nullopt` card. |

### 4.3 Commands

```
./tools/unix/build_omim.sh -d -p /workspace street_pixels_tests
/workspace/omim-build-debug/street_pixels_tests --filter=CompletionCard
```

Optional extra (only if a file is also added under `street_pixels_areas_tests` — not required if all tests live in `street_pixels_tests`):

```
./tools/unix/build_omim.sh -d -p /workspace street_pixels_areas_tests
/workspace/omim-build-debug/street_pixels_areas_tests --filter=CompletionCard
```

Device PNG eyeball → SP-069 / Phase 10. Not a gate here.

### 4.4 What not to test here

- Android `ACTION_SEND` / share sheet (SP-068).
- Date opt-in checkbox (SP-068).
- Counter increment (SP-068); hook may be asserted as “callable with zero args” only.
- Pixel-perfect Canvas vs AGG.
- iOS.

---

## 5. Docs updates (coding agent, after tests)

| Doc | Edit |
| --- | --- |
| `work-items/SP-067-completion-card-compositor.md` | Status **In review**. Fill branch + test output + “M1 path used” = rings-only / `CompletionCardModel`. Manual inspection = residual SP-069. **Do not** set Accepted / Accepted by / date. |
| `phases/phase-07-milestones-and-share-cards.md` | Current-code: compositor is `CompletionCardModel` + rings outline; Share chrome still no-op; `SharingUtils` still not used. Privacy section: V1 geometry is SPD-046 rings-only (no longer “OQ-9 not Accepted”). |
| `README.md` Phase 7 table | SP-067 → **In review** — deny-list model + rings outline. |
| `DECISIONS.md` SPD-046 Consequences | Add one line: SP-067 implements `CompletionCardModel` from `m_rings` (headless raster + Android Canvas from that model; never Drape). Do not change Status. |

Agent does not mark Accepted.

---

## 6. Risks / non-goals / SP-068 follow-ups

### Risks

- **Screenshot temptation.** Overlay already draws rings. Capturing it leaks puck/track/pixels. Forbidden. Escalate rather than ship.
- **SPA gone at Peek time.** If rings are not copied at ingest, compose will guess from focus sidecar (wrong) or fail. Copy `CompletionCardSource` on the P100 queue item.
- **Random tmp names accumulate.** Only the fixed `street_pixels_completion_card.png` in `TmpDir`.
- **Date leak.** `m_completed100At` is on the source; default compose must not copy it into the model. `includeDate` is a parameter, not a stored default-on.
- **osmId on JNI presentation.** Keep it off `CompletionCardModel`. Analytics hook must not grow an area-id argument.
- **Mercator in log lines.** `DebugPrint` of the model should print ring **counts**, not a lat/lon dump. Equality tests compare `m_outlineRings` in C++ directly.
- **Empty rings 100%.** Fail closed: copy-only SP-065 chrome, no fake map.
- **Two rasterisers** (C++ vs Canvas) can look different. Geometry source is one model. Visual QA is SP-069.
- **`chart_generator` / screenshoter reuse** would draw the wrong thing (route or live map). Do not.

### Non-goals

- OS share sheet, `ACTION_SEND`, FileProvider URI wiring (SP-068).
- Date opt-in control (SP-068).
- Card-generated / share-initiated counters (SP-068). Hook only.
- Sharing KML/GPX/location; `SharingUtils.shareLocation`.
- Phase 8 leading / not-leading copy.
- Gallery / MediaStore persistence.
- City-summary share card.
- Non-outline stylised map (would need a new spike; not this item).
- iOS.

### SP-068 follow-ups

- Share button → image of the **transient** card file (or Canvas-compressed PNG of the same model). Still no tracks.
- `includeDate` from a default-off control; re-call `GetCompletionCardForCurrentPresentation(true)`.
- Call `DeleteCompletionCardTransient` after hand-off when practical.
- Implement the generated-card counter by setting `SetCompletionCardGeneratedHandler` (no area id).
- Share-initiated counter on tap, not on 100% appear.

---

## 7. Acceptance-criteria mapping

| AC (work item) | How this plan satisfies it |
| --- | --- |
| 1. Model only permitted fields; deny-list absent (type and/or serialisation) | Permit-list struct + `CompletionCard_DenyListFieldsAbsent`. |
| 2. Compose with no nickname and no date | Default `CompletionCardOptions`; `CompletionCard_ComposeWithoutNicknameOrDate`. |
| 3. Image from locked M1 path (rings), not live map capture | `Compose` copies `m_rings`; `RasteriseCompletionCard` / Canvas stroke; no MapView/Drape/screenshoter. |
| 4. No route, home, live location, per-visit timestamp in model or documented fixture | Deny-list test + rings equality vs GPS polyline; date is calendar-only and default omitted. Device residual → SP-069. |
| 5. Transient output; no unbounded shared storage | Fixed tmp PNG overwrite + delete on dismiss. |
| 6. Card from 100% SP-065 surface, not a debug-only path | `applyAreaMilestonePresentation` THRESHOLD_100 binds outline. Share tap remains no-op. |

Required automated tests map to §4.1. Manual image inspection is explicitly **not** this item’s gate.
