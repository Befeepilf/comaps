# SP-083 — Apply Explorer Pro gate to GPX surfaces

**Phase:** 9 — GPX and feature gating
**Status:** Accepted
**Branch:** `cursor/sp-083-gpx-pro-gate-db9d`
**Depends on:** SP-080 G4, G6, G7, G8 (**SPD-070**, **SPD-072–074**);
  SP-005 composition point; SP-081 path exists so gating cannot leave a
  free bookmark-replay painter
**Unblocks:** SP-084 (settings shown when the gate opens), SP-087 public-
  build checks

---

## Objective

Apply `explorer_pro::IsCapabilityEnabled` to GPX import, GPX export, and
batch import so public-configured builds expose no exploration-affecting
GPX tooling and no purchase action. Provide a debug-only entitlement
path so internal Pro-capable builds can actually open the gate.

## Motivation

SP-005 defined the gate and applied it to nothing. Favorites, place-page
share, and VIEW/SEND intents still import and export GPX for everyone.
`IsCapabilityEnabled` is available ∧ entitled; the stub always denies, so
`-PenableExplorerProCapabilities=true` alone cannot show working tools
(G7).

## In-scope behavior

- JNI getters for `IsCapabilityEnabled` / per-capability queries so Java
  can hide or no-op surfaces. Do not check only availability.
- Gate:
  - GPX document picker / Favorites import of `.gpx`
  - `KmzKmlProcessor` VIEW/SEND/SEND_MULTIPLE for GPX MIME and `*.gpx`
  - Category and track **Export GPX** menu items
  - Batch multi-URI GPX (`AdvancedTrackManagement` and/or `GpxImport` per
    G4)
- When the gate is closed (G6): refuse GPX files — no pixel paint, no
  track materialisation, **no** purchase / upgrade / pricing CTA.
  KML/KMZ bookmark import remains.
- Own-recording list, rename, colour, delete remain visible (G4).
- Debug entitlement (G7): install a non-stub source only when Pro
  capabilities are on **and** a debug-only BuildConfig override is true.
  Public release/beta with capabilities default false must not compile in
  a grant path that a user can flip. `StubEntitlementSource` unchanged.
- Information page (G8) may land here or in SP-084; pick one. No price,
  buy, or restore.
- Tests: four-cell matrix at each call site family (import, export,
  batch, share-sheet handler). Stub never grants via settings tampering
  (existing `ExplorerPro_StubEntitlementNeverGrants`).

## Out-of-scope behavior

- Settings preference rows (SP-084) unless the info page is placed there.
- Billing, Play Billing, restore, pricing copy (SPD-010).
- iOS.
- Monetisation counters (SP-086).
- Changing isolation rules (SP-082).

## Relevant product requirements

- Spec §7, §29, §29.2, §30, §34 Explorer Pro.
- SPD-010, SPD-011, SP-005.
- Phase 9 exit 3–4.

## Relevant source files or symbols

- `libs/map/explorer_pro.{hpp,cpp}`
- `android/sdk/build.gradle` `EXPLORER_PRO_*`,
  `enableExplorerProCapabilities`
- `Framework.nativeSetExplorerProCapabilities` (add getters)
- `OrganicMaps` platform init
- `Factory.KmzKmlProcessor`, `BookmarkManager.importBookmarksFile(s)`
- `BookmarkCategoriesFragment` import button; export GPX in
  `BookmarksListFragment`, `BookmarkCategoriesFragment`, `PlacePageView`
- `AndroidManifest.xml` GPX intent-filters (may remain; handler no-ops)
- `street_pixels_tests/explorer_pro_tests.cpp`

## Implementation notes / constraints

- Single composition point: `IsCapabilityEnabled`. Do not add a second
  “available only” UI gate that would show tools to unentitled users.
- Fail closed if JNI is not yet initialised.
- Do not add `BillingClient`.
- Strings: no “Upgrade”, “Buy”, “Restore”, or price in public resources
  used by this item.
- F-Droid / Huawei / web flavors stay capabilities-off unless the gradle
  property is passed.

## Acceptance criteria

1. Flag off + any entitlement: import, export, batch, share-sheet GPX
   all deny; KML/KMZ bookmarks still import.
2. Flag on + stub entitlement (no debug override): deny. No purchase
   CTA.
3. Flag on + debug entitlement (internal): import/export/batch work.
4. Public release-shaped BuildConfig cannot grant entitlement.
5. No new purchase action anywhere, including deep links.
6. Existing `ExplorerPro_*` matrix remains green; call-site tests added.

## Required automated tests

- C++ matrix already in `explorer_pro_tests.cpp` (keep).
- Handler-level tests: GPX import function returns denied / no pixel
  change when gate closed; allowed when enabled.
- Debug entitlement source: not used when override is false; used when
  true; stub restored afterwards.

## Required manual validation

- Public-configured APK: no GPX import/export entry in Favorites or
  track share; share-sheet GPX does not paint pixels and shows no buy
  button.
- Internal Pro build with debug entitle: tools appear and import works.
- Device residual → SP-087 / Phase 10.

## Failure and rollback considerations

- Prefer hiding tools over showing a disabled row that invites a
  purchase tap.
- If G7 is not locked, do not ship a runtime setting that grants
  entitlement in release.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-083-gpx-pro-gate-db9d` |
| Test output | `--filter='ExplorerPro_|GpxGate|IsolationHistoricalImport_Gate'` **23/23** All tests passed |
| Accepted by | Product owner |
| Accepted date | 2026-08-28 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| iOS GPX import/export remains ungated (`MWMBookmarksManager`) | Out of Android V1; post-V1 iOS |
| C++ `PrepareFileForSharing` / `PrepareTrackFileForSharing` still write GPX; Android JNI returns `FileError` first | **Accepted residual** 2026-08-28: Desktop/Qt ungated in Android V1 |
| `DebugEntitlementSource` and `UnfreezeConfigurationForTesting` remain in the native binary | **Closed** 2026-08-28: grant symbols `#ifdef DEBUG`; `UnfreezeConfigurationForTesting` remains |
| JNI `nativeIsGpx*Available` getters exist and are unused | SP-084 must gate UI on Enabled, not Available |
| G8 Explorer Pro information page not shipped | SP-084 optional |
| Device: public APK share-sheet GPX, Favorites hide, debug-entitle internal build | SP-087 / Phase 10 |
