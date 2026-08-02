# Street Pixels — Decision Log

Lightweight ADR-style record of decisions that are already confirmed.

**Format:** Decision, Status, Context, Consequences, Related documents.

**Status values:** `Accepted`, `Superseded`, `Reversed`.

**Rules**

- Decisions are appended, not rewritten. Changing a decision adds a new entry
  and marks the old one `Superseded`, naming the successor.
- Only confirmed decisions appear in the numbered sections. Unresolved matters
  are recorded in §15 and are explicitly not decisions.
- A decision here does not authorise scope change. It records a choice already
  made by the product owner.

---

## SPD-001 — Android is the public V1 platform

**Decision.** Public V1 of Street Pixels ships on Android only.

**Status.** Accepted.

**Context.** The Android app already carries the Street Pixels surface: the
`STREET_PIXELS` map-layer mode, the explore consent dialog, the explore menu
entries in `MwmActivity`, and a foreground-service track recorder. The
equivalent iOS surface does not exist. Building both at once would double the
launch-blocking work without changing what the product proves.

**Consequences.**

- No iOS work is a V1 release gate.
- Platform-specific work concentrates in `android/`.
- Shared behaviour is implemented in `libs/` rather than in Android code, so
  that iOS can adopt it later without a rewrite.
- Any V1 exit criterion phrased in terms of iOS is invalid.

**Related documents.** Product spec §1, §5, §6; `phases/phase-01-*`,
`phases/phase-10-*`.

---

## SPD-002 — iOS follows after Android V1

**Decision.** iOS is a post-V1 platform. Street Pixels iOS UI, StoreKit, iOS
permission flows, and iOS release gates are deferred.

**Status.** Accepted.

**Context.** Follows from SPD-001. The shared C++ core is already
cross-platform; the gap is the iOS product surface.

**Consequences.**

- Shared-core work must not take Android-only shortcuts that would need
  unwinding for iOS: session state, GPS acceptance, storage, area assignment,
  and scoring belong in `libs/`.
- Android-specific code may assume Android without abstraction cost.
- iOS parity is not tracked as a V1 risk.

**Related documents.** Product spec §6, §35; audit §19.

---

## SPD-003 — The product is available worldwide

**Decision.** Street Pixels is available wherever compatible CoMaps map data
exists.

**Status.** Accepted.

**Context.** Personal exploration depends only on MWM highway geometry, which
exists globally. Restricting availability would remove the product's core value
for most users while solving no technical problem.

**Consequences.**

- Personal exploration, recording, and routing must work outside recognised
  settlements.
- Area completion and competition may be unavailable in a given place; that
  must be presented as "no exploration area here", never as "unsupported
  region".
- Area-pipeline coverage gaps degrade gracefully rather than blocking the app.

**Related documents.** Product spec §8.1, §8.6, §8.7.

---

## SPD-004 — No pilot-city runtime restrictions

**Decision.** V1 contains no city allowlist, no pilot-only runtime behaviour,
and no city-specific feature restriction.

**Status.** Accepted.

**Context.** The technical audit proposed narrowing to curated pilot cities as
a way to make the area model tractable. Product rejected that as a *runtime*
mechanism. Tractability is solved by the country-configured pipeline
(SPD-006) and settlement fallback (SPD-007), not by gating users.

**Consequences.**

- No allowlist data structure, config, or feature flag keyed on city identity.
- Reviewers reject any change that makes behaviour depend on which city the
  user is in.
- Sparse competition outside promoted cities is an accepted outcome, handled by
  empty-state copy rather than by hiding the feature.

**Related documents.** Product spec §6, §8.7, §34; audit §24 ("Replace"),
which this decision explicitly overrides.

---

## SPD-005 — City-focused marketing campaigns are allowed

**Decision.** Marketing and community-building may feature specific cities.
This is a campaign concept with no runtime effect.

**Status.** Accepted.

**Context.** A featured launch city is useful for building enough local density
that competition feels alive. It must not leak into the product as a
capability boundary.

**Consequences.**

- No code, configuration, or data change is required or permitted to support a
  campaign.
- In-app copy must never imply that non-featured places are unsupported.

**Related documents.** Product spec §8.7; SPD-004.

---

## SPD-006 — Exploration areas come from a country-configured administrative polygon pipeline

**Decision.** Exploration areas are derived from real closed administrative or
place polygons, selected by a versioned, country-specific configuration of
which `admin_level` values are valid and in what priority order. No arbitrary
grid areas. No polygons invented around `place=*` nodes.

**Status.** Accepted.

**Context.** This is the largest gap between the product and the current data
pipeline. In `data/mapcss-mapping.csv`, only `boundary|administrative|2`, `|3`,
and `|4` are retained; `|7`, `|9`, `|10`, and `|11` are deprecated, and levels
5, 6, and 8 have no `boundary|administrative` entries at all. At runtime, city
boundaries are stored as an approximation — the intersection of a bounding box,
a calipers box, and a diamond box (`libs/indexer/city_boundary.hpp`) — not as
true polygons. `place|suburb`, `place|quarter`, and `place|neighbourhood` exist
as searchable place types, not as exploration polygons. A single global
`admin_level` rule is not semantically valid across countries.

**Consequences.**

- Phase 4 requires generator and map-data work, not only client work. It is the
  highest-effort phase in the plan.
- The country configuration is versioned data with its own review process, and
  area assignment must be reproducible for a fixed map-data plus policy version
  pair.
- MWM country identifiers must never be presented as neighbourhoods.
- Competition (Phase 8) cannot start before Phase 4 produces area identifiers.

**Related documents.** Product spec §3.5, §8.3, §8.4, §8.8; audit §10;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-007 — Settlement fallback through city, town, or village

**Decision.** Where a settlement has no suitable administrative subdivision or
place-boundary polygon, the whole settlement becomes one exploration area.
Outside any recognised settlement there is no exploration area, and personal
exploration continues without area completion or competition.

**Status.** Accepted.

**Context.** Administrative subdivision coverage in OSM varies enormously by
country. Without a fallback, large parts of the world would have progress
tracking that silently does nothing.

**Consequences.**

- Every area-aware surface must handle three states: local subdivision area,
  settlement-level area, and no area.
- Settlement completion and local-area completion are the same number in the
  fallback case, and the interface must not present them as two achievements.
- The "no selected exploration area" empty state is a required, tested state,
  not an edge case.

**Related documents.** Product spec §7 (focused area), §8.5, §8.6, §31.

---

## SPD-008 — Share cards remain in Android V1

**Decision.** Shareable 100% completion cards ship in public Android V1.

**Status.** Accepted.

**Context.** The technical audit listed share cards as a candidate to defer if
resources are tight. Product declined; the card is the product's main organic
growth surface and the reward for the hardest thing a user can do.

**Consequences.**

- Phase 7 is a release-blocking phase, not optional polish.
- Card generation is new work: no share compositor exists today.
- Card content is privacy-constrained by construction: no route geometry, no
  home or live location, no per-visit timestamps, no other users' data.
- The card must work with no competition profile and no nickname.

**Related documents.** Product spec §19, §34 ("Sharing"); audit §24 ("Defer"),
which this decision overrides.

---

## SPD-009 — Hard avoid-explored routing remains in Android V1

**Decision.** The **Avoid explored streets** option ships in public Android V1,
with an explicit fallback offer when no fully unexplored route exists.

**Status.** Accepted.

**Context.** Today only a soft preference exists: `ApplyStreetExplorationMultiplier`
scales segment weight by `1 + strength * 9 * exploredRatio`, capped near 10×,
which can always be outweighed. No hard exclusion exists anywhere in `libs/`.
The audit proposed deferring the hard mode if the routing spike failed. Product
declined: routing for curiosity rather than efficiency is a defining feature.

**Consequences.**

- Phase 6 must deliver both prefer and avoid modes.
- Avoid mode must never silently degrade to prefer mode. When a strictly
  unexplored route is impossible, the user is explicitly offered "allow the
  minimum necessary explored connection" or "return to normal routing".
- Failure modes (disconnected unexplored components, extreme detours,
  mid-navigation instability as the user explores) are product-visible states
  requiring designed handling, not silent fallbacks.

**Related documents.** Product spec §17.3, §31, §34 ("Routing"); audit §12,
spike 7.

---

## SPD-010 — Explorer Pro purchasing is deferred

**Decision.** Public Android V1 ships without Google Play Billing, without a
purchase flow, and without purchase restoration.

**Status.** Accepted.

**Context.** Billing does not exist in the Android sources today. Building it
would add store review surface, a purchase-restoration failure mode, and legal
copy to a launch whose purpose is to prove the exploration mechanic.

**Consequences.**

- No billing dependency is added to the Android build for V1.
- Public builds must not present a purchase action that cannot complete.
- Monetisation analytics for purchase conversion and restoration are post-V1.
- Deferral applies to *purchasing*, not to the architecture; see SPD-011.

**Related documents.** Product spec §5, §6, §29, §29.3, §34, §35.

---

## SPD-011 — Pro functionality uses build availability plus entitlement

**Decision.** Explorer Pro capability is gated by two independent conditions:
"is this feature available in this build" (feature flag) and "does this user
hold the entitlement" (entitlement abstraction). Both remain in the codebase in
V1, with Pro disabled in public builds.

**Status.** Accepted.

**Context.** Keeping the two-condition shape now avoids retrofitting gates
throughout the GPX and track-management code when purchasing is activated
later. Neither mechanism exists today.

**Consequences.**

- Phase 1 delivers the abstraction; Phase 9 applies it to GPX tooling.
- Public V1 sets the Pro feature flag off; the entitlement source can be a stub
  that always returns "not entitled".
- A stub entitlement source must never be reachable as a way to *grant*
  entitlement in a public build.
- Imported GPX data is excluded from competition regardless of flag or
  entitlement state. That exclusion is a data rule, not a gating rule, and is
  never implemented inside the gate.

**Related documents.** Product spec §7 (Explorer Pro), §29, §29.2, §30;
`work-items/SP-005-feature-flag-foundation.md`.

---

## SPD-012 — Rendering, background recording, and GPS validation are treated as feasible

**Decision.** Street-pixel rendering at product scale, background recording
capability, and the specified GPS validation and interpolation behaviour are
accepted as feasible. They are engineering and tuning problems, not open
feasibility questions, and no phase is blocked pending a feasibility verdict.

**Status.** Accepted.

**Context.** Each already has a working foundation. Rendering uses GPU circle
points bucketed at zoom 15 and hidden below zoom 9, with a radius-per-zoom
table (`libs/drape_frontend/street_pixel_renderer.cpp`). Background recording
uses an Android foreground service typed `location`
(`TrackRecordingService`). GPS filtering logic exists for the track path in
`GpsTrackFilter`, though with far looser defaults than the spec and not applied
to pixel collection at all.

**Consequences.**

- The corresponding audit spikes become *validation and tuning* tasks with
  measured pass criteria, not go/no-go gates.
- Measurement is still mandatory. "Feasible" is not "verified": rendering
  performance, screen-off reliability across OEM skins, and filter thresholds
  all require recorded evidence before the relevant phase exits.
- If measurement contradicts this decision, the correct response is to report
  it and revisit this entry, not to quietly reduce scope.

**Related documents.** Product spec §16, §11.2, §33, §34 ("Quality"); audit §7,
§8, §9, spikes 1, 3, 5.

---

## SPD-013 — Map-update reconciliation is mandatory

**Decision.** Map-data updates must rematch explored HEALPix identifiers
against the new dataset. Deleting exploration state on update is not
acceptable.

**Status.** Accepted.

**Context.** Current behaviour is the opposite. `Framework::OnCountryFileDownloaded`
calls `StreetPixelsManager::CleanupStreetPixels`, which removes the `.pix`,
`.pixa`, and `.pixf` files for the country and deletes its rows from
`street_stats.db`. The next load regenerates an all-red pixel set. For a
product whose central promise is permanent personal progress, updating the map
currently erases the user's history.

**Consequences.**

- Phase 3 is release-blocking and is a prerequisite for Phases 4, 6, 8, and 9.
- The pixel file format needs a map-data version stamp; it currently stores
  only a HEALPix identifier with the explored state in the most significant
  bit.
- Migration must be crash-safe: the previous explored state is retained until
  the new state is durable.
- Percentage decreases caused by newly added streets are communicated as "there
  is more to explore", never as lost progress.

**Related documents.** Product spec §27, §34 ("Offline and map updates"); audit
§6, §14, spike 8; `phases/phase-03-*`.

---

## SPD-014 — Competition uploads aggregate area statistics only

**Decision.** Competition uploads contain aggregate, area-scoped statistics.
Raw GPS samples, recorded tracks, exact locations, and per-pixel visit
timestamps are never uploaded, under any configuration, in any build.

**Status.** Accepted.

**Context.** This is the load-bearing privacy promise of the product. It is
also what makes an opt-in competition defensible at all. The backend today has
no competition surface: `comaps_backend` contains only `Explorer` and
`Friendship` models with account and friends endpoints, and the `/stats/upload`
endpoint the client posts to does not exist.

**Consequences.**

- The upload payload is a closed list: pseudonymous profile identifier,
  nickname, area identifier, aggregate ownership score, live coverage
  percentage, eligibility state, weekly new-live-pixel count by city,
  map-data version, score-calculation version, last update time.
- The backend must be built so that it *cannot* accept raw location data;
  schema rejection, not documentation, is the control.
- Uploads are batched and delayed — no more than once per 15 minutes plus up to
  15 minutes of jitter — so that competition data cannot function as a
  live-location signal.
- Server-side decay operates on the stored aggregate, which is why per-pixel
  timestamps are not needed server-side.
- Any future feature that would upload finer-grained location requires a new
  decision entry and a separate privacy review.

**Related documents.** Product spec §3.2, §25, §25.2, §25.3, §34 ("Privacy and
competition"); audit §15, §17; `phases/phase-08-competition.md`.

---

## SPD-015 — Exploration live-eligibility bit packed in `.pix`

**Decision.** Each explored pixel stores a single **ever-live** bit in a spare
bit of the existing `.pix` `int64_t` entry, not in a side table and not as a
multi-value `live` / `imported` / `both` enum.

- Explored + ever-live `0` → imported-only (personal completion only).
- Explored + ever-live `1` → live-eligible (including cells that were first
  imported and later validated live).
- Live collection sets the bit; GPX/import must not clear it. “Both” is not
  stored separately — once live, the cell counts as live for competition and
  related consumers.

The explored MSB remains bit 63. `GetPixelId()` must return only the HEALPix
id (mask out flag bits). Phase 8 live-recency timestamps stay out of `.pix`
and use a sparse store later.

**Status.** Accepted.

**Context.** Spec §15.2–§15.3 require distinguishing live from imported for
competition. Competition and later live visits need **ever-live** behaviour,
not an archaeological first-source enum: an imported-then-walked cell must
become competition-eligible (§15.2 later live visits). A two-bit `both` state
is unnecessary for V1. Uusimaa `.pix` ≈ 50 MB (~6.5×10⁶ cells) ruled out a
HEALPix-keyed side table; one spare bit adds **zero** bytes. Literal §15.2
“first-explored source” wording is satisfied operationally by this bit meaning
ever-live / live-eligible; true first-source archaeology is not a V1 store
requirement.

**Consequences.**

- SP-016 implements one ever-live bit and updates `df::StreetPixel` accessors;
  format version must bump when the layout is written.
- Rematch (SP-017) preserves the bit by copying it from old entries.
- Do not add per-pixel timestamps or other wide fields into `.pix`.
- Phase 9 GPX marks imported-only when first exploring via import; if the cell
  is already ever-live, leave the bit set.

**Related documents.** Product spec §15.2–§15.3; phase-03; SP-016; SPD-017.

---

## SPD-016 — Explored state survives map delete and redownload

**Decision.** Deleting a country map must not destroy the user's explored
HEALPix set or ever-live bits. On later redownload, rematch retained exploration
onto the new derived universe. Retention must use a **compact explored-only
archive**, not an indefinite keep of the full valid-universe `.pix` (regional
files are tens of megabytes).

**Status.** Accepted.

**Context.** Spec §3.6 permanence is not limited to in-place updates. Today's
`OnCountryFileDelete` / `OnMapDeregistered` paths call `CleanupStreetPixels`
and wipe exploration. Keeping a full ~50 MB `.pix` per deleted region would
punish users who free map storage.

**Consequences.**

- SP-018 implements compact retention + rematch-on-redownload.
- Download/update rematch remains SP-017; delete path must not call today's
  wipe for explored state.

**Related documents.** Product spec §3.6, §27; phase-03; SP-018.

---

## SPD-017 — HEALPix `nside` locked at 1048576 for V1

**Decision.** Street Pixels V1 keeps HEALPix `nside = 1048576` (NEST). Changing
`nside` is out of scope for Phase 3 and for public Android V1; it would
redefine every pixel id and invalidate every stored exploration file.

**Status.** Accepted. Closes OQ-8 for V1.

**Context.** Maintainer lock during Phase 3 entry. Regional `.pix` files are
already large (~50 MB for Uusimaa); a finer grid would multiply storage and
memory. Rendering performance remains a separate measurement concern but does
not reopen `nside` for V1 without a new decision that accepts a full migration.

**Consequences.**

- OQ-8 is struck for V1; a post-V1 change needs a new SPD and a migration plan.
- Phase 3 non-goal reinforced; SP-015/019 must not alter `nside`.

**Related documents.** Product spec §14; audit §27 Q8; phase-03; OQ-8.

---

## SPD-018 — `.pixf` is unused and not part of the V1 store

**Decision.** The `.pixf` extension has no reader or writer in the tree. It is
not part of the V1 exploration store. Cleanup may continue deleting stray
`.pixf` files; no feature may start writing them without a new decision.

**Status.** Accepted.

**Context.** Phase 3 re-verify (2026-08-03) confirmed `.pixf` is only named in
`CleanupStreetPixels` and docs.

**Consequences.** Rematch and retention designs ignore `.pixf`.

**Related documents.** phase-03; SP-017.

---

## 15. Recorded open questions (not decisions)

These are carried from existing project documents. They are listed so they are
not lost. None of them is decided here, and nothing in this section may be
treated as authorisation.

| Ref | Question | Source | Blocks |
| --- | --- | --- | --- |
| OQ-1 | The area-completion formula (§7), ownership-score formula (§22.4), and contested-state threshold (§22.9) are empty in the product spec. | Product spec; audit §2, §22 | Phase 8. Phase 5 needs the completion formula, though its intent is unambiguous from surrounding text. |
| OQ-2 | Does prefer-unexplored routing use the personal explored set including imported pixels, or live-only? | Audit §12, §27 Q4 | Phase 6 acceptance criteria. |
| OQ-3 | Weekly leaderboard reset when a city's local time zone is unknown. | Audit §24 | Phase 8. |
| OQ-4 | Nickname uniqueness: the spec says nicknames need not be unique, but the current backend enforces a unique `username`. | Product spec §20.4; backend `core/models.py` | Phase 8. |
| OQ-5 | Bridge and tunnel eligibility, and the motorway-with-explicit-bicycle-access case, after a tag-survival audit. | Product spec §13.1; audit §6, §27 Q9 | Phase 3 (eligibility tightening). |
| OQ-6 | Whether the in-progress friends feature is retained in Street Pixels builds. Friends exist in Android and in `comaps_backend` but are a product non-goal for V1. | Product spec §6; audit §15, §27 Q7 | Phase 1 (what a public build exposes) and Phase 8. |
| OQ-7 | Production API base URL, hosting region, and data-retention policy. | Audit §27 Q6 | Phase 8, and partially Phase 1 (SP-004). |
| OQ-8 | ~~Whether HEALPix `nside` stays at 1048576 after rendering measurement.~~ | Audit §27 Q8 | **Closed for V1 by SPD-017** — `nside = 1048576` locked. |

When one of these is answered, add a new `SPD-NNN` entry above and strike the
row here with a reference to it.
