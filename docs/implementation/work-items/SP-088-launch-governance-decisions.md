# SP-088 — Launch-governance decisions

**Phase:** 10 — Android release hardening
**Status:** Planned
**Branch:** `cursor/phase-10-work-items-6383`
**Depends on:** None for this docs item. Product-owner lock of H1–H10
  before SP-089+ coding. Phase 10 implementation entry (other phases at
  exit) is **not** required for this item.
**Unblocks:** SP-089–097 (coding and device work must not guess the locks
  listed here)
**Investigation note:**
  [`notes/SP-088-launch-governance-architecture.md`](../notes/SP-088-launch-governance-architecture.md)

---

## Objective

Record accepted decisions for the public-V1 device matrix, battery and
rendering pass bars, store-flavor scope, privacy-policy and terms
ownership, product-analytics upload, `ACCESS_BACKGROUND_LOCATION`,
carried-residual disposition, release-pipeline reuse, friends-surface
stripping, and the CI versus checklist gate — so SP-089+ do not encode
guesses.

Docs / `DECISIONS.md` only in the implementation of this work item. No
production binary changes here.

---

## Motivation

Phase 10 adds no features. Everything it touches is verification,
disclosure, or a defect found during verification. The phase file’s
known uncertainties (battery “acceptable”, device matrix, policy
ownership, flavor scope, CoMaps release-workflow reuse, ABL) would
otherwise be decided silently inside coding or device PRs.

Carried residuals from Phases 2–9 now outnumber the 2026-07-25 phase-10
list. Without a disposition table, later items will either fix
post-V1 work or skip launch-blocking defects.

---

## In-scope behavior

- Re-verify current code locations in
  `phases/phase-10-android-release-hardening.md` against this working
  tree (snapshot in the investigation note, 2026-08-29).
- Append **SPD-077–086** when the maintainer locks H1–H10. Until then,
  record recommended positions and leave Status explicit (proposed /
  awaiting lock). Do not invent Accepted thresholds.
- Strike or add OQ rows in `DECISIONS.md` §15 for OQ-30–OQ-39.
- Annotate SP-089–097 and phase-10 with the decision ids.
- Classify every carried residual as Fix / Measure / Device-verify /
  Ops / Follow H5 / Accept / Not Phase 10 (H7).
- Do not edit the product spec or technical audit.

## Out-of-scope behavior

- Implementing residual defects, analytics, store copy, or device
  walks (SP-089–097).
- iOS release preparation (SPD-001, SPD-002).
- Explorer Pro purchasing (SPD-010).
- Option A mapgen `.spa` collectors (SPD-033).
- Friends feature revival (SPD-061).
- Marking this work item or Phase 10 exit Accepted unilaterally.
- Inventing a numeric battery ceiling without a protocol (H2).

---

## Locks this item must record (recommended positions, not Accepted)

These are starting recommendations from the phase file, spec §30–§34,
audit §22 / §26, and 2026-08-29 code inspection. SP-088 implementation
confirms them as **proposals**. They are recorded as OQ-30–OQ-39 in
`DECISIONS.md` §15 (draft SPD-077–086). They are **not** Accepted.
Maintainer lock promotes each OQ to the matching SPD.

| Ref | Question | Recommended position | Why | OQ / draft SPD |
| --- | --- | --- | --- | --- |
| H1 | Which device matrix is sufficient? | **D1** Pixel-class already used in this project (Pixel 3a and/or Pixel 10a). **D2** one aggressive-OEM skin (Xiaomi / HyperOS, Samsung with aggressive sleep, or Huawei). Optional **D3** a second API level (Android 10–12 vs 14–15) if D1/D2 are the same generation. Screen-off recording, OEM kill, and Helsinki walks run on D1+D2. | Audit and phase-10 already name Pixel-class + aggressive OEM. One device cannot close SP-014 exit #7. | OQ-30 / draft SPD-077 |
| H2 | What is “acceptable” battery and rendering? | **Rendering:** keep Spike 1 — p95 ≥30 FPS at zoom 14–16 with a city loaded; overlay memory uplift &lt;150 MB (SP-033). **Battery:** lock the *protocol* now — multi-hour screen-off recording vs a same-device control (app installed, recording off, screen off, no navigation); record %/hour and mAh if available. Do **not** invent a numeric ceiling in this item. After SP-094 numbers, either accept, waive with store copy, or open a new SPD. Cold-start-to-first-interactive-frame is recorded, not gated, unless the maintainer adds a number. | Spec §34 Quality has no number. A guessed % would fail or pass by accident. Rendering already has a recorded bar. | OQ-31 / draft SPD-078 |
| H3 | Which store flavors are the first public V1? | **Google Play `google` release** is the public V1 store gate (listing, data-safety, signing). **F-Droid** may ship the same artefact in the same slice but is not a separate product surface. **Huawei** and **web** are not V1 launch gates. Every flavor still must not expose a purchase action or Pro capabilities (SPD-010 / SPD-011). | Spec is Android public V1, not “every store on day one”. Huawei review is a second pipeline. | OQ-32 / draft SPD-079 |
| H4 | Where do privacy policy and terms live, and who owns them? | Product-owned **Street Pixels** privacy policy and terms (or a clearly versioned CoMaps addendum that describes session GPS, local `.pix`, competition aggregates, and deletion). In-app Help links must not remain `https://comaps.app/privacy/` / `terms/` unless that page is updated to match this product. Policy version string stays the consent key (`IdentityStore`). Exact EU region string remains ops (SPD-062). | Current Help opens unmodified CoMaps pages. Spec §34 requires the policy to describe local vs uploaded data. | OQ-33 / draft SPD-080 |
| H5 | Do product-analytics counters upload in V1? | **No new public upload sink.** Keep count-only local uint64 (SPD-044 / SPD-055 / SPD-075). Do not send through Sentry. Do not attach analytics to competition POST. §32 “measure” for public V1 means the counters exist and are inspectable in debug; §33 hypotheses are closed-beta observation, not a telemetry pipeline. If the maintainer instead wants an aggregate sink, it needs a separate consent, a closed payload deny-list, and a new SPD that supersedes this one. | A new endpoint without consent would violate private-by-default. Competition opt-in is not analytics consent. | OQ-34 / draft SPD-081 |
| H6 | Add `ACCESS_BACKGROUND_LOCATION`? | **Keep absent** unless D2 measurement (SP-095) proves the location FGS does not survive screen-off on the aggressive OEM. If added: Play Console background-location declaration + justification video in SP-092; session-only copy; never claim tracking outside a session. | SP-012 / SP-014 Pixel 3a worked without ABL. Adding ABL is a store-review and disclosure cost. | OQ-35 / draft SPD-082 |
| H7 | How is each carried residual classified? | Use the disposition table in the investigation note. **Fix** → SP-089. **Measure** → SP-094. **Device-verify** → SP-095. **Ops** → SP-096. **Follow H5** → SP-091. Option A mapgen, Qt/desktop GPX, iOS, friends revival, boss haptic stay out. If the Fix list is more than one non-trivial subsystem after lock, split extra `SP-NNN` files before coding rather than one mixed PR. | Phase 10 must not silently drop OEM/Helsinki evidence or silently implement post-V1 work. | OQ-36 / draft SPD-083 |
| H8 | Reuse upstream CoMaps release workflows as-is? | **No.** Reuse the *machinery* (Gradle flavors, Forgejo `android-release.yaml` shape, `docs/CREDENTIALS.md` secret names) but the **listing, application identity, data-safety answers, and signing identity** must be Street Pixels / this fork. Unmodified CoMaps Play copy currently advertises GPX import/export and “does not track”. | Shipping CoMaps metadata for a competition-capable explorer would fail spec §34 disclosures. | OQ-37 / draft SPD-084 |
| H9 | How far does SPD-061 hide friends in the public APK? | Hide friend settings, add-friend deep links, and friend-facing nickname copy in **public** builds (capability-off). Code may stay in-tree. Do not register `comaps://add-friend` / HTTPS `/add-friend` in the public manifest if the OS still offers them. Do not reopen OQ-6. | Manifest still registers add-friend. Store listing and policy would otherwise describe a feature V1 does not ship. | OQ-38 / draft SPD-085 |
| H10 | Must Forgejo C++ test exclusions be narrowed before launch? | **No** as a launch blocker. V1 gate is recorded local `street_pixels_tests`, smoke, Android lint, `clang-format`, plus the SP-097 evidence log. Narrowing `CTEST_EXCLUDE_REGEX` / adding a GitHub C++ job remains the SP-002 follow-up, not Phase 10 exit. | README §8.1 already states this posture. Rewriting upstream CI is unrelated scope. | OQ-39 / draft SPD-086 |

### H2 — battery protocol (load-bearing)

**Must be decided in this item**, at least as a protocol.

SP-094 cannot “pass” criterion 6 without a definition. Recommended
protocol (not a number):

- Same device, same OS battery saver settings (explicitly recorded).
- Session A: screen-off, recording active, no navigation, ≥2 hours.
- Session B: screen-off, recording off, app not force-stopped, ≥2 hours.
- Record start/end battery %, wall-clock, whether the FGS stayed up,
  and whether pixels continued to collect (A only).
- Maintainer accepts or waives after seeing A vs B, not before.

**Reject.** Declaring “acceptable” with no measurement. Inventing a
percentage in this docs item.

### H5 — analytics upload (load-bearing)

**Must be decided in this item.**

SP-060 / SP-068 / SP-086 all residualled upload to Phase 10. Coding a
sink in SP-091 without a lock either ships location-adjacent telemetry
or wastes a PR.

**Reject.** Uploading counters through Sentry. Bundling them into
`/competition/` POST. Uploading when competition is opted out without a
distinct analytics consent.

### H7 — residual disposition (load-bearing)

**Must be decided in this item.**

Recommended **Fix** list for SP-089 (subject to lock):

1. Draw completed-area check glyph (`m_showCheck`).
2. Incomplete-`.spa` Android chrome (SP-048).
3. Remove share-time date checkbox (SPD-056).
4. Share PNG lifetime vs 4 s auto-ack.
5. `onResume` rebind incrementing card-generated counter.
6. Off-route Avoid Prefer+seekbar dialog (SP-061 R3).
7. Weekly city leaderboard JNI read (SP-079).
8. Clear `live_recency.db` on competition revoke (SP-072).

Recommended **Accept** without Phase 10 code: overlay bake retune;
in-app analytics debug readout; Qt ungated GPX; reload no-paint;
multi-category KMZ; FromLatLon; system expat; boss haptic; Option A
mapgen.

SP-077 leftovers (409 mapping, `/leave` retry, 7-day gates) need an
explicit Fix vs Accept mark per row; do not hide them inside SP-096.

---

## Acceptance criteria

1. H1–H10 are recorded as Accepted SPD-077–086, **or** remain
   proposed with OQ-30–OQ-39 until the maintainer locks them.
2. Product spec and technical audit are not edited.
3. SP-089–097 and phase-10 reference the decision ids.
4. This work item is not marked Accepted by an agent.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Maintainer / product-owner lock of H1–H10.

## Failure and rollback considerations

- If H5 is rejected in favour of an upload sink, SP-091 must not proceed
  until payload deny-list, consent, and endpoint are locked. Do not
  reuse the competition POST.
- If H6 is rejected in favour of adding ABL before D2 measurement,
  SP-092 must include Play declaration work in the same change set as
  the permission.
- If H7 moves a Fix item to Accept, SP-089 must not implement it.
- If H2 never gets a numeric ceiling, Phase 10 exit #6 is a maintainer
  accept/waive of SP-094 numbers, not an agent pass.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-10-work-items-6383` |
| Product-owner lock | |
| Decision ids | draft SPD-077–086 (OQ-30–OQ-39) |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Play listing still CoMaps + advertises GPX | SP-092 after H3/H8 |
| Help privacy/terms URLs are `comaps.app` | SP-093 after H4 |
| `prefs_privacy.xml` has no Street Pixels rows | SP-090 |
| Spec §32.1 / §32.3 counters absent | SP-091 after H5 |
| add-friend intent-filters still registered | SP-092 after H9 |
| Android JVM tests &gt;3 files; still no `androidTest` | Do not add instrumentation as a side effect |
