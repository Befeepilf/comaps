# SP-062 — Milestone and share-card architecture (investigation)

**Date:** 2026-08-19
**Work item:** [SP-062](../work-items/SP-062-milestone-share-architecture-decisions.md)
**Open questions:** OQ-9–OQ-18 in `DECISIONS.md` §15
**Draft SPD ids:** SPD-046–055 (proposed only; last Accepted SPD remains SPD-045)

---

## 1. Purpose / status

This note records source inspection for Phase 7 locks M1–M10. It proposes
architecture so SP-063+ do not encode guesses. **Nothing here is Accepted.**
No numbered `## SPD-046`–`## SPD-055` entries exist in `DECISIONS.md`.
Maintainer lock of each OQ promotes that row to the matching draft SPD.

No compositor spike was run. Rings-only outline is deny-list-safe from
source inspection. A spike is a follow-up only if the maintainer wants a
non-outline stylised map (not a guess inside SP-067).

Phase 7 stylised-map **entry criterion remains unmet** until M1 / OQ-9 is
Accepted.

---

## 2. Compositor candidates

Spec §19.1 allows “Stylized map **or** boundary outline.” The
recommendation is the **outline branch**, not a ban on every non-screenshot
drawing and not a claim that the stylised-map wording is already locked.

### Rejected: Drape / `MapView` screenshot

`area_overlay.cpp` (`libs/street_pixels_areas/area_overlay.cpp`) and
`df::ExplorationAreaOverlayItem` (`libs/drape_frontend/exploration_area_overlay.hpp`)
draw rings **in-app**. That path is map chrome: it sits in the live Drape
scene with street-pixel circles, the position marker, routes, and tracks.
Capturing it would leak the §19.1 deny list (and the extra privacy items in
§3). Overlay clip order (SP-037 nested-winner `difference`) is **not** share
geometry.

`qt/screenshoter.hpp` / `qt/screenshoter.cpp` is a **desktop QA** tool
(`ScreenshotParams` points / rects / KML files → pixmap). It is not an
Android neighbourhood-card path and still captures the live map.

No Android neighbourhood screenshot API exists for this product surface.
`SharingUtils` shares KML / GPX / text / `shareLocation` — not a card.

### Rejected: explored HEALPix on the card

Drawing the user’s explored cells would be a corridor / trace of where they
went. Spec §19.1 forbids a raw GPS route; the same privacy intent forbids
an explored-pixel corridor, even without GPS coordinates.

### Recommended: rings-only from `ExplorationArea::m_rings`

`ExplorationArea` (`libs/street_pixels_areas/areas_types.hpp`):

```
  // Mercator outer rings (multipolygon outers; holes are not stored).
  std::vector<std::vector<m2::PointD>> m_rings;
```

Proposal (not Accepted): V1 share image is composed **off-map** from those
outers. Shared **card model** in `libs/`; Android `Canvas` **or** a headless
rasteriser is given **only** that model (plus permitted text fields). Never
capture Drape / `MapView`. Never draw explored HEALPix, route, home, live
location, track, or position marker. `area_overlay` stays in-app.

City-summary (`FocusedAreaProgress.m_citySummary`, SP-039) does **not** fire
area 25/50/100 or a city share card. Spec §18.1 is per exploration area.

---

## 3. Privacy deny list

Spec §19.1 must not include:

- Raw GPS route
- Home location
- Live location
- Individual timestamps
- Other users’ personal information

Additional deny-list items for the share image and its model (proposal):

- Position marker / location puck
- Recorded track geometry
- Explored HEALPix corridor
- MWM / country id as a title
- Coordinates (lat/lon, geo: / ge0 URLs)

`SharingUtils.shareLocation` is **forbidden** on this path. It builds
`Framework.nativeGetGeoUri` / `getHttpGe0Url` from the live `Location` and
opens a text share of those URLs
(`android/app/src/main/java/app/organicmaps/util/SharingUtils.java`).

Anonymous sharing must work with no account and no nickname (spec §19.2).

---

## 4. 100 m conversion evidence

Spec §10 step 9:

> After the equivalent of approximately 100 metres of new live street
> pixels has been collected, the first-goal badge completes with a small
> animation.

Spec §10 step 10:

> After the user has collected at least 30 new live pixels, approximately
> 300 metres, the application may show one temporary, non-blocking
> competition hint.

Those two sentences imply **10 new live pixels ≈ 100 m** if 30 ≈ 300 m is
taken as the conversion. That is **not** geodesic 100 m and **not** a
HEALPix-area formula.

Live collection today increments `numNewlyExploredPixels` only when the
cell was **not** already explored:

`StreetPixelsManager::OnLocationUpdate` (`libs/map/street_pixels_manager.cpp`):

```
      if (pixel == nullptr || (pixel->IsExplored() && pixel->IsEverLive()))
        continue;
      ...
      if (!pixel->IsExplored())
      {
        pixel->SetExplored(true);
        pixel->SetEverLive(true);
        ...
        ++numNewlyExploredPixels;
```

An imported→live flip (`IsExplored()` already true, then `SetEverLive(true)`)
does **not** increment that counter. Product must still lock (a) newly
explored only (today’s `numNewlyExploredPixels`) vs (b) `IsEverLive` flip.
**Recommendation (not decided): (a).** Import-only writes never count.

`kExploreRadiusMeters = 25.0` in `StreetPixelsManager` (spec §15.1). One
accepted update can collect many cells; a 10-pixel goal can complete in a
**single pulse**. SPD-019 unified path sampling at **15 m**; spec §14 / §15
sampling text is still **~10 m**.

Tensions are tabulated in §7. This note does **not** pick among them.
**Proposal:** 10 pixels + (a). Do not encode either count in SP-064 until
OQ-10 is Accepted.

---

## 5. Store options

### `street_stats.db` (`StreetStatsDB::InitSchema`)

Tables: `mwms` (`mwm_id`, `mwm_name`); `street_exploration` (`mwm_id`,
`feature_index`, `pixel_bitmask`); `processed_tracks` (`geometry_hash`,
`country_id`). Per-feature bitmasks, not per-area fired-state. Do not mix
milestone rows into these tables.

### `AreaCompletionCache`

Derived from `.pix` + `.spa`. Rows carry compact index **and** `m_osmId`.
Invalidated on collect / import / rematch. **No** fired-state, **no**
original 100% date. Compact index is sidecar-local and can change on `.spa`
regen. OSM id is the stable area identity Phase 4 already persists. Cache
hint only.

### `settings.ini` (`settings::TryGet` / `Set`)

Fine for a **bounded** key set (SPD-044 / `StreetExplorationRoutingAnalytics`
uint64 counters). Unbounded per-area rows are the wrong grain.

### Recommendation (not Accepted)

New local sqlite keyed by **OSM id + threshold**. Original 100% date in the
same store. Not `settings.ini` unbounded; not `StreetStatsDB` bitmask
tables. Compact index may be stored as a cache hint only if the OSM id is
the key.

---

## 6. M4–M10 with code anchors

**M4 (OQ-12 / draft SPD-049).** Proposal: does not re-fire. Fired-state and
original date survive rematch / policy / `.spa` refetch (spec §27.4).
`InvalidateAreaCompletionCache` must not reset that store.

**M5 (OQ-13 / draft SPD-050).** Proposal: queue; one at a time; 100% > 50%
> 25%; never interrupt `RoutingManager::IsRoutingFollowing()`
(`libs/map/routing_manager.hpp`). First-goal is independent of the area
queue. `FocusedAreaProgress.m_citySummary` (`focused_area_progress.hpp`;
set in `StreetPixelsManager`) does **not** enqueue area celebrations.

**M6 (OQ-14 / draft SPD-051).** Store the original 100% date always (spec
§18.5 / §27.4). Card shows it only if share-time opt-in, **default off**.
Spec §19.1 “Optional completion date” is not a required toggle. Alternative:
omit the date from the V1 card (then drop the control). Do not default-on
with no control. Owner of the control: SP-068.

**M7 (OQ-15 / draft SPD-052).** Card works with no profile and no nickname
(spec §19.2). Stub string/hook for §22.10; Phase 8 fills leading /
not-leading copy. Never imply completion was invalid.

**M8 (OQ-16 / draft SPD-053).** Once per install. Appears on first recording
start. Incomplete persists across sessions until complete. Never returns.
Not per-session, not per-area.

**M9 (OQ-17 / draft SPD-054).** Proposal: pulse iff recording (not paused)
**and** app foreground **and** exploration-haptics toggle on (default on).
One collection pulse per update, not per pixel. Stronger patterns: first-100
m complete, 50%, 100%. No extra for 25%. Boss out of scope (Phase 8).

Today `TriggerCollectionVibration` (`street_pixels_manager.cpp`) pulses
**once per pixel** when `numNewlyExploredPixels > 1` (`VibratePattern` up
to 10). Collection is session-gated because `OnLocationUpdate` returns
unless `m_recordingSession->IsRecording()` (SP-007). It is **not**
foreground-gated.

`OrganicMaps.nativeOnTransit` → `Framework::EnterForeground` /
`EnterBackground` (`OrganicMaps.cpp`). `Framework::EnterForeground` updates
usage stats, Drape, and traffic — **it does not notify Street Pixels**.
Wire that transit into the haptic predicate; do not assume it already does.

Import path `MarkExploredPixelIds` sets `SetExplored(true)` without
`SetEverLive` and without `TriggerCollectionVibration`, then invalidates
the completion cache. Personal % includes imported (SPD-026), so import can
cross 25/50/100 and show a card **with no haptic** (not recording). Confirm;
do not require an active session to fire area milestones.

**M10 (OQ-18 / draft SPD-055).** Count-only: card generated, share
initiated. No area id, OSM id, coordinates, or image. Local `settings`
uint64, copying `routing::StreetExplorationRoutingAnalytics`
(`libs/routing/street_exploration_routing_analytics.cpp`). Upload residual
→ Phase 10 if no sink. Not Sentry. Do **not** add spec §32.1 / §32.2 extra
events in Phase 7; M10 is §32.4 only.

---

## 7. Spec / code / audit contradictions (unresolved)

Reported, not resolved.

| Tension | Spec | Code / decisions / audit | Notes |
| --- | --- | --- | --- |
| ~100 m vs 30 px ≈ 300 m vs 25 m radius | §10 step 9 ≈ 100 m; step 10: 30 new live pixels ≈ 300 m | `kExploreRadiusMeters = 25`; one update can collect many cells | 10-pixel reading follows step 10; geodesic 100 m and 25 m pulse disagree. Not picked. |
| 10 m vs 15 m sampling | §14 / eligible-geometry sampling ≈ 10 m | SPD-019: 15 m live/track/derivation | Conscious V1 divergence. Affects “metres of pixels” intuition, not the 10-pixel proposal. |
| new live vs `IsEverLive` flip | §10 step 9 “new live street pixels” | `numNewlyExploredPixels` only on `!IsExplored()`; imported→live flip does not increment | (a) vs (b) must lock. Recommend (a). |
| stylized map or outline vs rings-only | §19.1 “Stylized map or boundary outline” | Rings exist; no neighbourhood compositor; overlay is in-app | Outline branch recommended. Stylised non-outline map would need a spike. |
| optional date vs default-off toggle | §19.1 “Optional completion date”; §18.5 may store date | No card, no control | “Optional” ≠ a toggle. Recommend opt-in default off; alternative omit from V1. |
| import-driven 100% with no haptic | §28.1 recording ∧ foreground; §18.4 100% “a stronger haptic may play” | SPD-026 imported counts for personal %; `MarkExploredPixelIds` has no vibrate | Area milestones may fire while not recording. Haptic stays gated. |
| city-summary vs per-area milestones | §18.1 “Each area has celebrations at 25/50/100” | `FocusedAreaProgress.m_citySummary` is a rollup (SP-039) | City-summary must not fire area 25/50/100 or a city share card. |
| audit haptics-outside-session stale | §28.1 session + foreground | Phase 2 `IsRecording()` gate on `OnLocationUpdate`; audit §5 “Pixel overlay… Gate on recording session” / collection without recording | Audit date 2026-07-20. Out-of-session collection (and therefore vibration as a side effect) is **stale**. Foreground, one-pulse-per-update, and toggle remain absent. |
| audit cards **Not found** still true | §19 compositor required | Audit §5 / §16: completion-card generation **Not found** | Still true on 2026-08-19 re-verify. No neighbourhood compositor. |

---

## 8. Draft SPD id map

| Lock | OQ | Draft SPD | Status |
| --- | --- | --- | --- |
| M1 compositor | OQ-9 | draft SPD-046 | Proposed — awaiting maintainer lock |
| M2 100 m conversion | OQ-10 | draft SPD-047 | Proposed — awaiting maintainer lock |
| M3 store OSM id | OQ-11 | draft SPD-048 | Proposed — awaiting maintainer lock |
| M4 no re-fire | OQ-12 | draft SPD-049 | Proposed — awaiting maintainer lock |
| M5 queue / no interrupt | OQ-13 | draft SPD-050 | Proposed — awaiting maintainer lock |
| M6 date opt-in default off | OQ-14 | draft SPD-051 | Proposed — awaiting maintainer lock |
| M7 competition stub | OQ-15 | draft SPD-052 | Proposed — awaiting maintainer lock |
| M8 first-100 m once per install | OQ-16 | draft SPD-053 | Proposed — awaiting maintainer lock |
| M9 haptics predicate | OQ-17 | draft SPD-054 | Proposed — awaiting maintainer lock |
| M10 growth analytics | OQ-18 | draft SPD-055 | Proposed — awaiting maintainer lock |

Last Accepted SPD remains **SPD-045**. These drafts live in SP-062 and this
note, not as numbered sections in `DECISIONS.md`.

---

## 9. Spike disposition

**None.** Source inspection can choose a deny-list-safe path: off-map
rings-only outline. Do not start SP-067 until M1 / OQ-9 is Accepted.

If the maintainer wants a **non-outline stylised map** (the other §19.1
branch), record a dedicated spike as follow-up. Do **not** guess that path
inside SP-067.
