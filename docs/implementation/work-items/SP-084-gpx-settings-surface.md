# SP-084 — GPX settings surface

**Phase:** 9 — GPX and feature gating
**Status:** Accepted
**Branch:** `cursor/sp-084-gpx-settings-surface-db9d`
**Depends on:** SP-080 G4, G6, G8; SP-083 gate and JNI getters
**Unblocks:** SP-087 settings and public-build checks

---

## Objective

Add GPX import, export, and related track-management entries to settings
only when Explorer Pro is enabled in the build **and** the user is
entitled. Public-configured builds have no such rows and no purchase
settings.

## Motivation

Spec §30: when the flag is on and the user is entitled, settings may
include GPX tools. Today settings have no GPX; tools live always-on in
Favorites. After SP-083 hides those, entitled internal users need a
discoverable surface. Public V1 must not grow purchase, restore, or
pricing settings (SPD-010).

## In-scope behavior

- Settings rows (or a nested screen) for:
  - GPX import (document picker)
  - GPX export of a chosen local track / category (existing exporter)
  - Batch import if G4 places it here rather than only in Favorites
- Rows exist in the view hierarchy only when
  `IsCapabilityEnabled` for that capability. Hidden, not disabled-with-
  CTA.
- Optional Explorer Pro information page (if not shipped in SP-083):
  explanation only; no price, buy, restore (G8).
- Public builds (capabilities off): prefs XML/code paths add nothing
  observable; dump of settings screens contains no GPX / Pro / buy
  strings from this item.
- Local recording management already in the product remains free and
  visible.

## Out-of-scope behavior

- Billing, SKUs, restore (SPD-010).
- Making Favorites the only Pro surface and skipping settings — spec §30
  allows settings when the gate opens; this item is that surface.
  Favorites may still offer the same actions when enabled (SP-083);
  do not duplicate business logic, share the C++/JNI gate.
- Additional export formats.
- Radius or HEALPix settings (spec §30 forbids).

## Relevant product requirements

- Spec §30, §29.2, §34 Explorer Pro.
- SPD-010, SPD-011.

## Relevant source files or symbols

- `android/app/src/main/res/xml/prefs_*.xml`
- Settings fragments under `android/app/src/main/java/app/organicmaps/`
- SP-083 JNI getters
- `docs/JAVA_STYLE.md`

## Implementation notes / constraints

- Hide, do not show a greyed “Pro” row in public builds.
- No new network endpoint.
- English strings in `values/strings.xml`; do not mass-translate in this
  item (follow repo translation process as follow-up if required).

## Acceptance criteria

1. Gate open: settings show GPX import/export (and batch if G4).
2. Gate closed: those rows are absent; no purchase settings added.
3. Public-configured build: no GPX/Pro/buy preference reachable.
4. Free local recording management still reachable.

## Required automated tests

- JVM or C++ tests for the visibility predicate if it lives in shared
  code. If purely Android view code, document that instrumented tests
  do not exist (repo has none) and cover the predicate in
  `street_pixels_tests` / a small Java unit test if one can be added
  beside the three existing SDK tests without standing up instrumentation.

## Required manual validation

- Walk settings in public vs internal Pro builds (SP-087).

## Failure and rollback considerations

- Missing settings in a Pro build is better than a public buy button.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-084-gpx-settings-surface-db9d` |
| Test output | `ExplorerProGateTest` **10/10**; `GpxSettingsVisibilityTest` **12/12**. `BookmarkManagerGpxGateTest` still `UnsatisfiedLinkError` on this host (`nativeGetBookmarksFilesExts`, unchanged class). |
| Accepted by | Product owner |
| Accepted date | 2026-08-28 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Instrumented settings dumps and public vs internal Pro device walks | SP-087 (repo has no Espresso harness) |
| `prefs_gpx.xml` and English GPX/Pro strings exist in the public APK | SP-087 must dump the inflated Preference tree, not `aapt` of all prefs XML or a `strings.xml` grep |
| `BookmarkManagerGpxGateTest` cannot load JNI in this JVM | **Closed** 2026-08-28: lazy `getBookmarksExtensions()` so clinit does not call Framework |
