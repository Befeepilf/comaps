# SP-088 — Launch-governance decisions

**Phase:** 10 — Android release hardening
**Status:** Accepted
**Branch:** `cursor/phase-10-work-items-6383`
**Depends on:** None for this docs item. Product-owner lock of H1–H10
  on 2026-08-29 (recommended positions, with brand and on-device testing
  residualised). Phase 10 implementation entry (other phases at
  exit) is **not** required for this item.
**Unblocks:** SP-089–097 (coding must not guess the locks listed here;
  residual WIs must not be treated as coding items)
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
ownership, flavor scope, Streifzug release-workflow reuse, ABL) would
otherwise be decided silently inside coding or device PRs.

Carried residuals from Phases 2–9 now outnumber the 2026-07-25 phase-10
list. Without a disposition table, later items will either fix
post-V1 work or skip launch-blocking defects.

---

## Product-owner lock 2026-08-29

Product owner accepted **all** recommended H1–H10 positions as
**SPD-077–086**, with this override:

- **Decisions** (matrix, Spike 1 bar, Google Play gate, local-only
  analytics, ABL absent, residual disposition classes, reuse release
  machinery, hide friends, local test gate) are **Accepted**.
- **Execution** of brand writing and on-device testing is **residual**
  — not implemented in later Phase 10 coding items; recorded explicitly.

**Brand-related residuals** (do not implement in SP-089–097):

- App name / product branding (Streifzug vs Street Pixels in
  user-visible strings, Help, listing title, location rationale that
  says “Streifzug”)
- Privacy policy text, hosting, and in-app URLs
  (`https://streifzug.app/privacy/` may stay for now)
- Terms of use text, hosting, and in-app URLs
- Play/F-Droid listing copy rewrite that is marketing/brand
  (H3/H8 listing identity)

**Device-test residuals** (do not execute hardware walks):

- Executing the H1 device matrix (D1 Pixel-class, D2 aggressive OEM)
- Executing H2 battery protocol and Spike 1 on a handset (SP-094
  device parts)
- All SP-095 device-matrix residual close-out walks
- SP-097 manual/device §34 observations that need a handset
- Traffic capture on device, screen-off OEM continuity, Helsinki
  walks, etc.

H4 (SPD-080): accept the *intended* position (product-owned Street
Pixels policy) but **landing the actual policy/terms/URLs is
residual**. SP-093 is residual, not a coding item.

---

## In-scope behavior

- Re-verify current code locations in
  `phases/phase-10-android-release-hardening.md` against this working
  tree (snapshot in the investigation note, 2026-08-29).
- Append **SPD-077–086** as Accepted. Strike OQ-30–OQ-39 in
  `DECISIONS.md` §15.
- Annotate SP-089–097 and phase-10 with the decision ids and residual
  notes.
- Classify every carried residual as Fix / Measure / Device-verify /
  Ops / Follow H5 / Accept / Not Phase 10 (H7 / SPD-083). Record
  Device-verify *execution* as residual; the Fix list remains SP-089
  (except brand).
- Do not edit the product spec or technical audit.

## Out-of-scope behavior

- Implementing residual defects, analytics, store copy, or device
  walks (SP-089–097). Brand writing and on-device testing stay residual.
- iOS release preparation (SPD-001, SPD-002).
- Explorer Pro purchasing (SPD-010).
- Option A mapgen `.spa` collectors (SPD-033).
- Friends feature revival (SPD-061).
- Marking Phase 10 exit Accepted unilaterally.
- Inventing a numeric battery ceiling without a protocol (H2).

---

## Locked decisions → SPD-077–086

Product-owner lock 2026-08-29 accepted every recommended H1–H10 position,
with brand writing and on-device testing residualised. Full text in
`DECISIONS.md`.

| Ref | Question | Accepted position | Why | OQ / SPD |
| --- | --- | --- | --- | --- |
| H1 | Which device matrix is sufficient? | **D1** Pixel-class already used in this project (Pixel 3a and/or Pixel 10a). **D2** one aggressive-OEM skin (Xiaomi / HyperOS, Samsung with aggressive sleep, or Huawei). Optional **D3** a second API level (Android 10–12 vs 14–15) if D1/D2 are the same generation. Screen-off recording, OEM kill, and Helsinki walks are defined on D1+D2. **Executing** the matrix on a handset is residual. | Audit and phase-10 already name Pixel-class + aggressive OEM. One device cannot close SP-014 exit #7. | OQ-30 / **SPD-077** |
| H2 | What is “acceptable” battery and rendering? | **Rendering:** keep Spike 1 — p95 ≥30 FPS at zoom 14–16 with a city loaded; overlay memory uplift &lt;150 MB (SP-033). **Battery:** lock the *protocol* now — multi-hour screen-off recording vs a same-device control (app installed, recording off, screen off, no navigation); record %/hour and mAh if available. Do **not** invent a numeric ceiling in this item. After SP-094 numbers, either accept, waive with store copy, or open a new SPD. Cold-start-to-first-interactive-frame is recorded, not gated, unless a later SPD adds a number. **Executing** the protocol and Spike 1 on a handset is residual. | Spec §34 Quality has no number. A guessed % would fail or pass by accident. Rendering already has a recorded bar. | OQ-31 / **SPD-078** |
| H3 | Which store flavors are the first public V1? | **Google Play `google` release** is the public V1 store gate (listing, data-safety, signing). **F-Droid** may ship the same artefact in the same slice but is not a separate product surface. **Huawei** and **web** are not V1 launch gates. Every flavor still must not expose a purchase action or Pro capabilities (SPD-010 / SPD-011). **Listing brand copy** (marketing/identity) is residual. | Spec is Android public V1, not “every store on day one”. Huawei review is a second pipeline. | OQ-32 / **SPD-079** |
| H4 | Where do privacy policy and terms live, and who owns them? | Product-owned **Street Pixels** privacy policy and terms (or a clearly versioned Streifzug addendum that describes session GPS, local `.pix`, competition aggregates, and deletion). Policy version string stays the consent key (`IdentityStore`). Exact EU region string remains ops (SPD-062). **Landing** the actual policy/terms text, hosting, and in-app URLs is residual; `https://streifzug.app/privacy/` / `terms/` may stay for now. **SP-093 is residual**, not a coding item. | Current Help opens unmodified Streifzug pages. Spec §34 requires the policy to describe local vs uploaded data; that landing work is residual. | OQ-33 / **SPD-080** |
| H5 | Do product-analytics counters upload in V1? | **No new public upload sink.** Keep count-only local uint64 (SPD-044 / SPD-055 / SPD-075). Do not send through Sentry. Do not attach analytics to competition POST. §32 “measure” for public V1 means the counters exist and are inspectable in debug; §33 hypotheses are closed-beta observation, not a telemetry pipeline. If a later SPD wants an aggregate sink, it needs a separate consent, a closed payload deny-list, and a new SPD that supersedes this one. **Closes the Phase 10 upload residual from SPD-044/055/075.** | A new endpoint without consent would violate private-by-default. Competition opt-in is not analytics consent. | OQ-34 / **SPD-081** |
| H6 | Add `ACCESS_BACKGROUND_LOCATION`? | **Keep absent** unless a later D2 measurement (SP-095) proves the location FGS does not survive screen-off on the aggressive OEM. If added: Play Console background-location declaration + justification video in SP-092; session-only copy; never claim tracking outside a session. D2 *execution* is residual, so Phase 10 coding keeps ABL absent. | SP-012 / SP-014 Pixel 3a worked without ABL. Adding ABL is a store-review and disclosure cost. | OQ-35 / **SPD-082** |
| H7 | How is each carried residual classified? | Use the disposition table in the investigation note. **Fix** → SP-089 (code defects; not brand, not device). **Measure** → SP-094 (protocol in docs; device execution residual). **Device-verify** → SP-095; *execution* residual. **Ops** → SP-096. **Follow H5** → SP-091. Option A mapgen, Qt/desktop GPX, iOS, friends revival, boss haptic stay out. If the Fix list is more than one non-trivial subsystem after lock, split extra `SP-NNN` files before coding rather than one mixed PR. | Phase 10 must not silently drop OEM/Helsinki evidence or silently implement post-V1 work. | OQ-36 / **SPD-083** |
| H8 | Reuse upstream Streifzug release workflows as-is? | **No.** Reuse the *machinery* (Gradle flavors, Forgejo `android-release.yaml` shape, `docs/CREDENTIALS.md` secret names) but the **listing, application identity, data-safety answers, and signing identity** must be Street Pixels / this fork. **Application name, listing copy, and privacy/terms URLs are residual.** | Shipping Streifzug metadata for a competition-capable explorer would fail spec §34 disclosures; brand writing is nonetheless residual for this slice. | OQ-37 / **SPD-084** |
| H9 | How far does SPD-061 hide friends in the public APK? | Hide friend settings, add-friend deep links, and friend-facing nickname copy in **public** builds (capability-off). Code may stay in-tree. Do not register `streifzug://add-friend` / HTTPS `/add-friend` in the public manifest if the OS still offers them. Do not reopen OQ-6. **This is implementable in SP-092** (not brand). | Manifest still registers add-friend. Store listing and policy would otherwise describe a feature V1 does not ship. | OQ-38 / **SPD-085** |
| H10 | Must Forgejo C++ test exclusions be narrowed before launch? | **No** as a launch blocker. V1 gate is recorded local `street_pixels_tests`, smoke, Android lint, `clang-format`, plus the SP-097 evidence log. Narrowing `CTEST_EXCLUDE_REGEX` / adding a GitHub C++ job remains the SP-002 follow-up, not Phase 10 exit. | README §8.1 already states this posture. Rewriting upstream CI is unrelated scope. | OQ-39 / **SPD-086** |

### Later work items: coding vs residual

| WI | Role after lock |
| --- | --- |
| SP-089 | **Coding.** H7 Fix list (code defects). Not brand, not device. |
| SP-090 | **Coding.** Spec §30/§31/§10 except privacy-policy/terms URL rows and app-name string rebrand (residual). Hide public friend settings (SPD-085). |
| SP-091 | **Coding.** Local §32 counters + payload-shape tests. No upload sink (SPD-081). |
| SP-092 | **Coding.** Hide friends (H9 / SPD-085); keep ABL absent (H6 / SPD-082). Do **not** rewrite Play listing brand copy (residual). |
| SP-093 | **Residual** (brand: privacy policy + terms). Not a coding item. |
| SP-094 | **Planned** for protocol docs only; **device execution residual.** |
| SP-095 | **Residual** (device walks). |
| SP-096 | **Coding (docs).** Risk-register table. Signed APK/ops may residual; brand listing residual. |
| SP-097 | **Coding (docs + automated).** Automated suites + evidence mapping. Device/manual hardware residual. |

### H2 — battery protocol (load-bearing)

**Decided in this item**, as a protocol. Execution on a handset is residual.

SP-094 cannot “pass” criterion 6 without a definition. Accepted
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

**Decided in this item.** No sink.

SP-060 / SP-068 / SP-086 all residualled upload to Phase 10. SPD-081
closes that residual: counters stay local; do not build a sink.

**Reject.** Uploading counters through Sentry. Bundling them into
`/competition/` POST. Uploading when competition is opted out without a
distinct analytics consent.

### H7 — residual disposition (load-bearing)

**Decided in this item.**

**Fix** list for SP-089:

1. Draw completed-area check glyph (`m_showCheck`).
2. Incomplete-`.spa` Android chrome (SP-048).
3. Remove share-time date checkbox (SPD-056).
4. Share PNG lifetime vs 4 s auto-ack.
5. `onResume` rebind incrementing card-generated counter.
6. Off-route Avoid Prefer+seekbar dialog (SP-061 R3).
7. Weekly city leaderboard JNI read (SP-079).
8. Clear `live_recency.db` on competition revoke (SP-072).

**Accept** without Phase 10 code: overlay bake retune;
in-app analytics debug readout; Qt ungated GPX; reload no-paint;
multi-category KMZ; FromLatLon; system expat; boss haptic; Option A
mapgen.

**SP-077 leftovers** (409 mapping, `/leave` retry, 7-day gates after
admin reset): **Accept** — not SP-089 Fix, not hidden inside SP-096.

Device-verify *execution* is residual. The classification remains
Device-verify → SP-095.

---

## Accepted SPD text

Full text lives in `DECISIONS.md` as **SPD-077–086** (Status Accepted).
Last Accepted SPD is **SPD-086**.

---

## Acceptance criteria

1. H1–H10 are recorded as Accepted SPD-077–086. OQ-30–OQ-39 are struck.
2. Product spec and technical audit are not edited.
3. SP-089–097 and phase-10 reference the decision ids and residual notes.
4. Status Accepted records the product-owner lock 2026-08-29 (agent
   records the lock; does not invent a person name beyond “product
   owner”). Phase 10 exit is not marked met.

## Required automated tests

- None (docs/decisions only).

## Required manual validation

- Product-owner lock of H1–H10 (received 2026-08-29).

## Failure and rollback considerations

- If H5 is rejected in favour of an upload sink, SP-091 must not proceed
  until payload deny-list, consent, and endpoint are locked. Do not
  reuse the competition POST. (H5 is Accepted local-only; a sink
  needs a superseding SPD.)
- If H6 is rejected in favour of adding ABL before D2 measurement,
  SP-092 must include Play declaration work in the same change set as
  the permission. (H6 is Accepted absent; D2 execution is residual.)
- If H7 moves a Fix item to Accept, SP-089 must not implement it.
- If H2 never gets a numeric ceiling, Phase 10 exit #6 is a maintainer
  accept/waive of SP-094 numbers, or residual until a handset run
  exists — not an agent pass.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | `cursor/phase-10-work-items-6383` |
| Product-owner lock | 2026-08-29 (recommended H1–H10 positions, with brand and on-device testing residualised) |
| Decision ids | SPD-077 (H1/OQ-30), SPD-078 (H2/OQ-31), SPD-079 (H3/OQ-32), SPD-080 (H4/OQ-33), SPD-081 (H5/OQ-34), SPD-082 (H6/OQ-35), SPD-083 (H7/OQ-36), SPD-084 (H8/OQ-37), SPD-085 (H9/OQ-38), SPD-086 (H10/OQ-39) |
| Accepted by | Product owner |
| Accepted date | 2026-08-29 |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Play listing still Streifzug + advertises GPX | Residual (brand listing copy). Not SP-092 coding. |
| Help privacy/terms URLs are `streifzug.app` | Residual (SP-093). `https://streifzug.app/privacy/` may stay for now. |
| `prefs_privacy.xml` has no Street Pixels rows | SP-090 except privacy-policy/terms URL rows (residual) and app-name rebrand (residual) |
| Spec §32.1 / §32.3 counters absent | SP-091 local uint64 after SPD-081; no upload sink |
| add-friend intent-filters still registered | SP-092 after SPD-085 (implementable) |
| Android JVM tests &gt;3 files; still no `androidTest` | Do not add instrumentation as a side effect |
| H1/H2/H7 Device-verify / SP-094 / SP-095 / SP-097 device walks | Residual (on-device testing). Do not execute hardware walks in coding items. |
