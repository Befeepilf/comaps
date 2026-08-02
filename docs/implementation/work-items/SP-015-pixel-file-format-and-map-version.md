# SP-015 — Pixel-file format version and map-data version header

**Phase:** 3 — Exploration storage and map-update reconciliation
**Status:** In progress
**Branch:** `street-pixels`

---

## Objective

Give `{countryId}.pix` an explicit on-disk format version and a map-data
version stamp, with a safe migration from today's headerless files. No rematch
and no source-flag behaviour in this work item.

## Motivation

The current `.pix` reader treats the entire file as a raw `span<df::StreetPixel>`.
There is no magic, no version, and no record of which MWM the universe was
derived from. SPD-013 and phase exit criteria 2 require both a format version
and a map-data version before rematch can land safely.

Landing the header first keeps SP-017's diff focused on rematch semantics
rather than inventing file layout mid-flight.

## In-scope behavior

- A versioned `.pix` header that includes at least: magic/format version and
  map-data version identity for the country MWM used at derivation time.
- Load path that recognises headerless legacy files and migrates them
  (rewrite with header, preserving explored bits) without losing exploration.
- Save / derive paths that always write the new format.
- Accessors so later work items can read the stamped map-data version.
- Tests for legacy read, round-trip, and rejection/handling of unknown future
  format versions.

## Out-of-scope behavior

- No source-flag / ever-live behaviour in this work item (SP-016).
- Rematch or any change to `CleanupStreetPixels` callers (SP-017 / SP-018).
- Derivation sampling distance (SP-019).
- Eligibility changes (SP-020).
- UI messaging (SP-021).
- Renderer colour or density behaviour.
- Changing HEALPix `nside` (SPD-017 locked).
- Source-bit layout / ever-live bit (SP-016); may reserve header fields/version
  numbers so SP-016 can bump format version without another header redesign.

## Relevant product requirements

- §14.1–§14.3 Fixed grid; determinism; pixel identity includes map-data version.
- §27.1–§27.5 Map-data updates and versioning.
- §3.6 Permanent personal progress (header migration must not wipe explored).
- SPD-015–017 storage constraints.

## Relevant source files or symbols

- `libs/map/street_pixels_manager.{hpp,cpp}` — `LoadStreetPixelsFromFile`,
  `SaveStreetPixelsToFile`, derivation entry points
- `libs/drape_frontend/street_pixel.{hpp,cpp}` — entry layout (explored bit
  unchanged here)
- `libs/coding/mmap_reader.*` — mmap span assumptions
- `libs/map/street_pixels_tests/*`

## Implementation notes / constraints

- Regional `.pix` ≈ 50 MB (Uusimaa). Header must be tiny and fixed-size.
  Migration must be streaming / atomic replace — do not hold two full
  extra copies longer than needed for rename.
- Renderer continues to see a `span<StreetPixel>` of entries only; the header
  must be skipped before constructing the span.
- Explored MSB layout stays unchanged in this work item.
- Record how map-data version is obtained from the registered MWM. Prefer an
  existing field such as `platform::LocalCountryFile::GetVersion()` / storage
  data version (`GetCurrentDataVersion` family) — do not invent a parallel
  versioning scheme. Pin the exact field and equality semantics in the SP-015
  implementation plan before coding the header layout.
- Do not add per-pixel arrays or SQLite mirrors of the universe.

## Acceptance criteria

1. New `.pix` files carry a format version and a map-data version stamp.
2. An existing headerless `.pix` with explored pixels loads, migrates, and
   retains the same explored HEALPix identifiers.
3. Unknown newer format versions fail closed (no silent misinterpretation).
4. No rematch, source-flag, sampling, or eligibility behaviour changes.
5. Covered by `street_pixels_tests`.

## Required automated tests

- Round-trip save/load of a headered file preserves ids and explored bits.
- Legacy headerless fixture migrates without explored loss.
- Map-data version stamp is readable after derive/save.
- Unsupported format version is rejected observably.

## Required manual validation

- Install over a build that already has a legacy `.pix`; confirm explored green
  pixels survive first launch after upgrade.

## Failure and rollback considerations

- Failed mid-migration must not leave an empty explored set; write to a temp
  file and replace atomically, or keep the legacy file until the new one is
  durable.
- Rollback: revert; legacy reader path can remain until rematch depends on the
  header.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `street-pixels` |
| Commits | |
| Test output | `/Users/mo/dev/omim-build-debug`: `ninja street_pixels_tests`; `./street_pixels_tests --filter=StreetPixelsFile` and full `./street_pixels_tests` — all passed (2026-08-03), including after catch-narrowing harden (`MayRecoverByDerive` only for `Corrupt`). |
| Manual validation | |
| Implemented by | Agent (implementation done; not accepted) |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Stamp `0` means unknown / unpinned after legacy migrate without a version; SP-017 must never treat `0` as equal to a real `LocalCountryFile::GetVersion()`. | Document / handle in SP-017 |
| Injected write-failure migrate test aborts under unit-test `LERROR` (from `WriteToTempAndRenameToFile`); covered instead by non-legacy migrate rejection leaving bytes intact. | Optional SP-015 follow-up if stronger IO-fault coverage needed |
| Unsupported / migration failure leave `OnUpdateCurrentCountry` transitioning to `Ready` with an empty in-memory span while the on-disk file is preserved. | Optional UX/status polish in a later item; not a wipe risk |
