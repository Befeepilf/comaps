# SP-089 — Locked residual defect close-out

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H7 Accepted (**SPD-083**). Phase 10 implementation
  entry (other phases at exit).
**Unblocks:** SP-094 / SP-095 / SP-097 (those items observe the fixed
  behaviour; device execution of SP-094/SP-095/SP-097 is residual)
**Notes:** Implements only residuals H7 classifies as **Fix**
  (**SPD-083**). One logical commit per defect. If the locked Fix list
  spans more than one non-trivial subsystem, split extra `SP-NNN`
  files before coding. **Not brand, not device.** Do not rewrite app
  name, listing copy, privacy/terms URLs, or execute hardware walks.

---

## Objective

Close the launch-blocking product defects that earlier phases
residualled to Phase 10, using the H7 disposition table, so device
verification and the §34 pass are not blocked by known holes.

---

## Motivation

Phase 10 is verification-first, but several residuals are defects, not
missing evidence: the completed check glyph is reserved and not drawn;
SPD-056 forbids a share-time date checkbox that is still in the UI;
off-route Avoid never shows the Prefer fallback; weekly city rankings
have no JNI read path; competition revoke leaves recency rows.

Leaving them until SP-097 guarantees a failed exit or a last-minute
mixed PR.

---

## In-scope behavior

Implement **only** the H7 **Fix** rows (**SPD-083**, locked
2026-08-29). Not brand, not device:

1. Draw the completed-area check glyph when `m_showCheck` is set
   (`libs/street_pixels_areas/area_overlay.*`, drape/style path that
   already strokes completed chrome).
2. Incomplete-`.spa` Android signalling chrome (toast or settings
   row) using the existing `IsSpaIncomplete` API from SP-048.
3. Remove the share-time completion-date checkbox; always include the
   stored date (SPD-056).
4. Keep the completion-card PNG until the share sheet returns, or
   otherwise stop 4 s auto-ack from deleting the file under the
   target (SP-069 R4).
5. Stop `onResume` rebind from incrementing `Explore.CardGenerated`
   and from resetting date UI (SP-069 R5).
6. Off-route Avoid rebuild that yields `AvoidExploredNoRoute` must
   surface the SP-058 Prefer+seekbar control (SP-061 R3;
   `CheckLocationForRouting` / `OnRemoveRoute` nullptr).
7. Wire weekly city leaderboard read through JNI so the Android UI
   can show the server ranking SP-076 already serves (SP-079).
8. On competition revoke / profile delete, drop `live_recency.db`
   rows (not local `.pix` exploration) (SP-072).

Each defect: focused automated tests, no drive-by refactors, no
comment additions.

Re-verify symbols against the tree at implementation time.

## Out-of-scope behavior

- Items H7 marks Accept, Measure, Device-verify, Ops, Follow H5, or
  Not Phase 10.
- Option A mapgen, Qt GPX, iOS, friends revival, boss haptic, overlay
  bake retune, analytics debug readout.
- New features not in the Fix list.
- Schema changes unless a Fix item requires one (crash-safe; own
  commit; report in discovered-follow-up).
- Device walks (SP-095 is residual).
- Brand writing (app name, listing copy, privacy/terms URLs).

## Relevant product requirements

- Spec §18.6 completed visual state; §19.1 / SPD-056 card date;
  §17.3 / §31 Avoid impossible route; §24 weekly rankings; §20.6 /
  §25.5 deletion; §27.3 incomplete sidecar; SPD-042 fallback.
- Phase 10 “defect found during verification” rule: these were found
  in earlier exits and deferred, not invented here.

## Relevant source files or symbols

- `libs/street_pixels_areas/area_overlay.cpp` (`m_showCheck`)
- Completion card Android bind / `CompletionCardShare` / auto-ack
- `RoutingManager` / `AsyncRouter` / `CheckLocationForRouting`
- Weekly store + JNI + competition UI
- `LiveRecencyStore` + revoke/delete path
- SPA incomplete flag + Data Management settings

## Implementation notes / constraints

- SPD-083 is Accepted. If a later SPD drops a row, do not implement it.
- Prefer one commit per defect (`[map]`, `[android]`, `[routing]` as
  appropriate).
- Do not weaken tests. If a test blocks a Fix, stop and report.
- Weekly JNI is not a new backend; it binds an existing read.
- Recency delete must not un-explore pixels (spec §3.6, §15.2).

## Acceptance criteria

1. Every H7 Fix row has a commit, tests, and completion evidence, or
   is explicitly reclassified by a new SPD.
2. Unrelated files are not touched.
3. Focused `street_pixels_tests` / `street_pixels_areas_tests` /
   routing tests named per defect are green (counts recorded).
4. Agent does not mark Accepted.

## Required automated tests

- Overlay: completed style still sets `m_showCheck`; a render or
  geometry test proves the glyph path is invoked (or a documented
  drape hook test if pixel-proof is impossible on desktop).
- Share: no checkbox in the card bind; date always present; generated
  counter increments once per 100% presentation; PNG still readable
  after share start.
- Routing: off-route Avoid no-route delivers the same fallback
  control as the initial no-route path (not a nullptr remove).
- Weekly: JNI/native test that a fixture ranking round-trips.
- Revoke: recency rows gone; `.pix` explored bits unchanged.
- SPA incomplete: existing unit tests plus Android visibility test
  if JVM-reachable.

## Required manual validation

- Desktop/harness where the defect is UI. Device eyeball → SP-095
  (residual; do not execute hardware walks in this item).
  Do not fabricate a handset.

## Failure and rollback considerations

- If drawing the glyph regresses Spike 1 qualitatively, stop and
  report; do not add LOD in this item.
- If weekly JNI needs a new auth header, that is SP-096/ops, not a
  silent schema change.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| H7 Fix rows closed | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
