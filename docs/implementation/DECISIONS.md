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

**Status.** Accepted. The specific fallback-offer pair in Consequences is
**superseded by SPD-042**. Avoid remaining in public Android V1, and never
silently degrading, remain Accepted.

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
  **Superseded by SPD-042:** V1 fallback is an explicit switch to Prefer with
  the strength seekbar; there is no min-connection search.
- Failure modes (disconnected unexplored components, extreme detours,
  mid-navigation instability as the user explores) are product-visible states
  requiring designed handling, not silent fallbacks.

**Related documents.** Product spec §17.3, §31, §34 ("Routing"); audit §12,
spike 7; SPD-042.

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

## SPD-019 — Path sampling unified at 15 metres for V1

**Decision.** Derivation, live segment interpolation, and track/import sampling
all use **15.0 m** as the single path-sampling step for Street Pixels V1. Live
and track paths that today use 10.0 m (`kInterpolationStepMeters` and the
hard-coded `10.0` in `UpdateStreetStatsForTrack`) align to 15 m. Derivation
keeps `kSegmentLengthMeters = 15.0`. Do **not** densify the valid-pixel
universe to 10 m.

**Status.** Accepted.

**Context.** Spec §14 calls for ~10 m path sampling. Phase 3 originally planned
SP-019 to move derivation from 15 m → 10 m to match live/track. After the
Uusimaa `.pix` ≈ 50 MB measurement, densifying derivation would grow regional
files further. Maintainer product decision (2026-08-03): keep the coarser
15 m step everywhere instead, accepting a conscious V1 divergence from the
spec’s ~10 m figure in favour of storage/memory headroom and one constant.

**Consequences.**

- SP-019 changes live/track sampling to 15 m and collapses dual constants (and
  the legacy `10.0` literal) to one definition.
- Existing on-disk `.pix` universes derived at 15 m stay valid; this is not a
  universe densification and does not by itself require rematch.
- Spec §14 remains the product source of truth for intent; this SPD records the
  V1 implementation divergence. A post-V1 move to 10 m needs a new SPD plus
  rematch/size evidence.
- SP-011 tests and evidence that asserted 10 m sampling must be updated under
  SP-019.

**Related documents.** Product spec §14.1–§14.3; phase-03; SP-019; SPD-017.

---

## SPD-020 — Exploration polygons ship as a per-country downloadable sidecar

**Decision.** Exploration-area geometry (true closed administrative and place
rings selected by country policy) lives in a **per-country downloadable
sidecar**, not as a mandatory in-MWM section. The World MWM three-box
`cities_boundaries` section remains **search-only legacy** and is not the
exploration-area store.

**Status.** Accepted.

**Context.** SP-023 measured Finland true-ring retention at ~2.1 MiB
country-concat zlib(coded) and ~0.5 MiB for the Helsinki MWM slice — small
versus a regional MWM, but competition-optional data should not inflate every
map download. Existing sidecars (`.pix` / `packed_polygons.bin`) already
establish the pattern. In-MWM optional sections remain technically possible
later; V1 does not require polygons inside the country MWM. Hybrid reuse of
World three-box rings for assignment is rejected by SPD-025.

**Consequences.**

- SP-026 emits exploration polygons into the sidecar format; SP-027 loads that
  sidecar offline for an installed country.
- Missing sidecar → no exploration areas (settlement / no-area paths still
  apply per SPD-007 once settlement rings are present); never invent grids.
- World `CityBoundary` three-box data stays for search/routing containment
  helpers that already use it; it is not assignment authority (SPD-025).
- Phase 4 exit size budget is measured against the sidecar (and any
  assignment blob), not against mandatory MWM growth.

**Related documents.** Product spec §3.5, §8.3; SPD-006, SPD-021, SPD-025;
SP-023; SP-024; SP-026; SP-027; SP-029;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-021 — Pixel→area assignment is generator-precomputed

**Decision.** Pixel→area assignment for the valid street-pixel universe is
**generator-precomputed** into an offline blob that ships with map/sidecar
artifacts. The client **consumes and rematches** that blob; it does **not**
perform a primary full-universe on-device point-in-polygon rematch after every
map or policy change. Offline-first holds: no network boundary lookup.

**Status.** Accepted.

**Context.** SP-023 desktop PIP for a Uusimaa-class proxy (~6.5×10⁶ cells) was
~2.7 min — tolerable as a generator/derive job, painful as interactive
on-device rematch (phone-class hardware unmeasured and expected slower).
Precomputation keeps assignment deterministic for a fixed (map-data version,
policy_version) pair while preserving the offline invariant.

**Consequences.**

- SP-026/028 emit or stage the precomputed assignment artifact; SP-027 exposes
  load/verify APIs; SP-028 verifies consumption against §8.8 rules rather than
  owning a primary full-universe client PIP engine.
- On-device PIP may remain for tests, spot checks, or narrow fallbacks; it is
  not the V1 rematch path for the full universe.
- Rematch cost is dominated by reading/applying the new blob and updating
  sparse local state (SPD-022 / SP-030), not by re-running PIP over every
  valid cell.
- Phase-04 wording that assumed “assignment is always on-device” is obsolete;
  soften docs accordingly under SP-024.

**Related documents.** Product spec §8.8; SPD-006, SPD-022; SP-023; SP-024;
SP-026; SP-027; SP-028; SP-030;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-022 — Assignment persistence is sparse explored plus sidecar rematerialize

**Decision.** Durable client assignment state is a **sparse** map from explored
HEALPix ids to a **compact area index**. Unexplored (and any needed full-universe
answer) is **rematerialized** from a dense **uint16 or uint32** area-index map
in the downloadable sidecar. Do **not** persist a full-universe table of
uint64 OSM ids. All assignment state is keyed by the pair
**(map-data version, policy_version)**.

**Status.** Accepted.

**Context.** Spec §8.8 requires a deterministic answer for every valid street
pixel, but storing that answer twice (dense OSM ids on device for the whole
universe) is wasteful. SP-023 estimated ~26 MiB for a full uint32 index and
~52 MiB for uint64 OSM ids at Uusimaa scale; sparse explored-only storage is
far smaller for typical users. A dense compact index in the sidecar lets the
client rematerialize without live PIP (SPD-021) and without keeping uint64 OSM
ids for every cell.

**Consequences.**

- SP-030 implements sparse explored persistence + rematerialize-from-sidecar;
  document measured or budgeted size against SP-023 estimates.
- Area ids in the dense map are dense indices into a sidecar id table that
  carries stable OSM identifiers and display metadata — not raw OSM ids in the
  per-pixel column.
- Map-data or policy_version bumps trigger rematch: rebuild sparse state from
  the new dense map without wiping exploration bits (Phase 3 rematch hooks).
- Percentages for unexplored cells rematerialize on demand from the dense map;
  do not require an explored-only store to invent answers for never-visited
  cells.

**Related documents.** Product spec §8.8, §27.4; SPD-016, SPD-020, SPD-021;
SP-023; SP-024; SP-026; SP-027; SP-030;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-023 — Country policy is versioned JSON under `data/street_pixels/`

**Decision.** Country exploration-area policy is **versioned JSON** checked in
under `data/street_pixels/` (exact filenames and schema land in SP-025). Each
revision carries a monotonic **`policy_version`**. Countries are keyed by
**ISO 3166-1 alpha-2**. Changes land through normal PR review. Finland seed
priority (from SP-023 measurements): subdivision **admin_10**, then
**admin_9**, then **admin_11**; settlement **admin_8**; closed polygonal
**place=*** only as a sparse supplement, not the primary grain.

**Status.** Accepted.

**Context.** SPD-006 already requires a versioned country configuration.
SP-023 showed Finland grain is effectively admin_10 (with 9/11 present) plus
admin_8 settlement fallback; closed place=* rings are rare. Exact schema
fields, loader API, and unknown-country behaviour remain SP-025
implementation detail; this decision locks location, versioning, keying,
review process, and the Finland seed priority.

**Consequences.**

- SP-025 lands schema + Finland fixture under `data/street_pixels/` and a
  loader readable by generator and client.
- Assignment determinism keys on (map-data version, policy_version) —
  configuration changes can reassign without a map download (SP-030).
- Unknown / unconfigured countries fall through settlement / no-area
  (SPD-007); never invent grids or city allowlists (SPD-004).
- Expanding worldwide coverage is incremental data work, not a V1 exit gate.

**Related documents.** Product spec §8.3, §34; SPD-004, SPD-006, SPD-007,
SPD-024; SP-023; SP-024; SP-025;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-024 — Suitability and privacy use config rings; no invented numeric floors

**Decision.** V1 suitability is enforced by admitting **only closed, normally
named rings** selected by the country-config levels (SPD-023), plus the
deterministic assignment stack in §8.8 (priority, smallest-polygon,
stable-id tie-break). Do **not** invent numeric minimum pixel counts or
minimum area-size floors in the client. “Meaningfully smaller than the
containing settlement” is expressed by choosing subdivision levels in config
and by §8.8 smallest-polygon selection — not by a hard-coded metre or pixel
threshold. Sparse-area anonymity remains **§23.4 server-side** (fewer than
three opted-in participants → anonymous others). Any future client-side size
or pixel-count gate needs a **follow-up measurement** and a new decision
before it ships.

**Status.** Accepted.

**Context.** Spec §8.4 lists qualitative suitability criteria; §23.4 already
defines sparse-area privacy for competition. SP-023 did not measure per-area
pixel counts, so inventing floors now would be guesswork and could silently
drop real named neighbourhoods. Config-level selection plus closed/named
rings already excludes most unsuitable candidates; anonymity for sparse
competition is a server concern, not a reason to invent client floors in SP-024.

**Consequences.**

- SP-025/026 filter by configured levels and closed named geometry; they do
  not encode invented numeric floors.
- Phase 4 exit #7 measures sidecar/blob size acceptance; it does **not**
  require a client pixel-count gate.
- A later privacy or suitability floor (if product wants one) is a new SPD
  after measurement — record under discovered follow-ups until then.
- Competition Phase 8 still implements §23.4 server-side anonymity.

**Related documents.** Product spec §8.4, §8.8, §23.4; SPD-006, SPD-007,
SPD-023; SP-023; SP-024; SP-025; SP-026; SP-031;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-025 — Settlement assignment uses true municipal rings from the sidecar

**Decision.** Settlement membership for exploration-area assignment and
SPD-007 fallback uses **true municipal rings** from the exploration-area
sidecar (for Finland, admin_8). The World three-box `CityBoundary`
approximation is **not** assignment authority.

**Status.** Accepted.

**Context.** SP-023 measured national admin_8 coded size at ~1.0 MiB — affordable
in the per-country sidecar (SPD-020). Three-box World boundaries are a search
approximation (bbox ∩ calipers ∩ diamond) and can disagree with true municipal
geometry; using them for settlement fallback would mis-assign coastal,
fragmented, and boundary-straddling cases. Search may keep three-box data
unchanged.

**Consequences.**

- SP-026 emits settlement (municipal) rings into the same sidecar as
  subdivisions; SP-029 uses those rings for settlement fallback / no-area.
- SP-027 must expose true-ring containment for settlements used in assignment,
  not `CitiesBoundariesTable` three-box `HasPoint`, as the assignment path.
- World `cities_boundaries` remains available for existing search behaviour;
  do not silently redefine it as exploration geometry.
- Outside true settlement rings → no area (SPD-007); exploration and routing
  continue.

**Related documents.** Product spec §8.2, §8.5, §8.6; SPD-007, SPD-020,
SPD-023; SP-023; SP-024; SP-026; SP-027; SP-029;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-026 — Phase 5 personal area completion uses explored / total in the area

**Decision.** For Phase 5 personal progress, area completion is the percentage of
**valid street pixels in the exploration area** that the user has explored:
`explored / total` for that area's compact index. Both validated live pixels and
imported GPX pixels count. Zero-total areas report fraction `0` (no
divide-by-zero). No-area pixels contribute to no area row. Completion rows are
keyed by **compact area index** (and stable OSM id), never by MWM / country id.
This does **not** decide ownership-score or contested-state formulas (still
OQ-1 / Phase 8).

**Status.** Accepted (Phase 5 personal-completion slice only).

**Context.** Spec §7 surrounding text states the intent; the LaTeX formula
markup is blank (OQ-1). SP-034 needs a lock for cache arithmetic without
inventing a contested competition formula. Surrounding text plus SPD-007
no-area behaviour are unambiguous for personal completion.

**Consequences.**

- `AreaCompletionCache` and badge/detail UI (SP-035+) use this formula.
- OQ-1 remains open for ownership-score (§22.4) and contested threshold
  (§22.9); only the personal completion slice is closed here.
- Competition eligibility and recency stay live-only (SPD-015 / Phase 8).

**Related documents.** Product spec §7, §15.4; OQ-1; SPD-007, SPD-015,
SPD-021, SPD-022; SP-034; `phases/phase-05-area-progress-and-map-interaction.md`.

---

## SPD-027 — Leaf download couples MWM and advertised `.spa`

**Decision.** When downloading or updating a map leaf, the client **always
fetches the leaf `.spa`** if `countries.txt` meta advertises one for that leaf.
If meta **omits** spa fields, there is **no advertisement and no spa fetch**;
the **MWM still installs and remains usable**, and exploration **areas stay
empty** (fail-closed — no invented grids or pretend areas). Personal
street-pixel collection and map viewing are not blocked by a missing sidecar.
Advertised-but-unavailable downloads (HTTP 404, network error, checksum
mismatch, incomplete file) are owned by **SPD-031**, not this decision.

**Status.** Accepted.

**Context.** SPD-020 places exploration geometry in a downloadable sidecar with
per-MWM-leaf grain (`{mwmLeafId}.spa`, SP-026). Production download was never
wired; Phase 4 exit used an offline emit harness (SP-032). Product owner lock
(D1 = A, 2026-08-07): couple fetch to advertisement, never make `.spa`
mandatory for map install.

**Consequences.**

- SP-046 implements storage download of `.spa` only when meta advertises it.
- Absent meta is a valid worldwide state (incremental sidecar coverage);
  settlement / no-area / empty-area UX already apply (SPD-007, SPD-020, §31).
- Does not require in-MWM exploration geometry.
- Cross-ref **SPD-031** for advertised download failure / incomplete signaling.

**Related documents.** Product spec §3.5, §8.6, §31; SPD-007, SPD-020,
SPD-031; SP-026 notes; SP-042; SP-046; SP-048;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-028 — Optional `spa` / `spa_sha1_base64` in `countries.txt`

**Decision.** Leaf nodes in `countries.txt` may carry optional fields
**`"spa"`** (payload size in bytes) and **`"spa_sha1_base64"`** (SHA-1 of the
`.spa` file, base64-encoded), parallel to existing `"s"` / `"sha1_base64"` for
the MWM. **Omit both** when the leaf has no sidecar. Do not invent placeholder
sizes or hashes.

**Status.** Accepted.

**Context.** Map download already trusts leaf `"s"` / `"sha1_base64"` for MWM
integrity. Sidecar advertisement needs the same pattern without forcing every
leaf to ship exploration geometry. Product owner lock (D2 = A, 2026-08-07).

**Consequences.**

- SP-045 extends countries meta publish / parser for the optional fields.
- SP-046 uses presence of the fields as the advertisement signal (SPD-027).
- Size UI / download estimates may include `"spa"` when present; omitting
  fields must not break older clients that ignore unknown keys (parser
  tolerance is an SP-045 detail).

**Related documents.** SPD-020, SPD-027; SP-042; SP-045; SP-046; `data/countries.txt`;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-029 — No spa-diffs in V1; full `.spa` on map / dataVersion update

**Decision.** Android V1 does **not** ship or apply spa-diffs. On map update or
a new storage `dataVersion` that replaces a leaf MWM, the client downloads a
**full** `.spa` when meta advertises one for the new leaf. Partial / delta
sidecar updates are post-V1 unless a new SPD says otherwise.

**Status.** Accepted.

**Context.** Assignment and geometry are keyed by `(map_data_version,
policy_version)` (SPD-021/022). Sidecar content is version-bound to map data;
diff pipelines would add packaging and client complexity before production
emit exists. Recommended lock D3 = A (no contradiction with Accepted SPDs).

**Consequences.**

- SP-047 implements full `.spa` refetch beside full/leaf MWM update paths.
- CDN packaging publishes whole `.spa` objects per leaf version (**SP-044**
  emit / publish tree; SP-047 refetch). `countries.txt` spa size/hash fields
  remain **SP-045** (SPD-028); validation / incomplete signaling remain
  **SP-048** (SPD-031).
- MWM diff mechanisms (if any) must not be silently reused for `.spa`.

**Related documents.** SPD-021, SPD-022, SPD-027; SP-042; SP-044; SP-047;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-030 — Delete `.spa` with the map; personal files unchanged

**Decision.** Deleting a country / leaf map **deletes the corresponding
`.spa`** (version-bound exploration geometry). Personal exploration and
assignment artifacts keep their **existing** rules: `.pix` / `.pixr` per
**SPD-016** / SP-018; `.spx` per **existing SP-030** behaviour. This decision
does **not** reopen or weaken SPD-016 permanence.

**Status.** Accepted.

**Context.** `.spa` is downloadable map-adjacent geometry + dense assign
(SPD-020–022), not personal explored state. Keeping orphan `.spa` after map
delete wastes storage and risks mismatched versions on redownload. Recommended
lock D4 = A.

**Consequences.**

- SP-047 wires `.spa` removal on map delete / deregister paths beside MWM
  cleanup.
- Rematch-on-redownload still depends on retained `.pixr` (and `.spx` rules as
  already implemented); new `.spa` arrives with the new MWM when advertised.
- Do not delete `.pix` / `.pixr` as part of “sidecar cleanup.”

**Related documents.** Product spec §3.6, §27; SPD-016, SPD-020; SP-018;
SP-030 (sparse store); SP-042; SP-047;
`phases/phase-03-exploration-storage-and-reconciliation.md`,
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-031 — Advertised `.spa` download failure keeps MWM; areas fail-closed

**Decision.** If meta advertises a `.spa` but the download **fails** (HTTP 404
or other missing object, network error, checksum mismatch, incomplete file,
etc.), the client **keeps the MWM usable**. Exploration **areas fail-closed**:
do not invent rings, assignment, or completion percentages from missing /
corrupt sidecar data. Prefer **retry and incomplete / unavailable signaling**
that does not block map viewing, routing that does not need areas, or personal
pixel collection. Omitted meta (no advertisement) remains **SPD-027** only.

**Status.** Accepted.

**Context.** Aligns with SPD-020 (“missing sidecar → no exploration areas”)
and spec §31 (no selected exploration area: personal pixels still work).
Product-recommended lock D5 = A: never make a sidecar failure a map-install
hard failure. Separates D1 soft path (no advertisement) from D5 (advertised
but unavailable).

**Consequences.**

- SP-046/047/048 implement non-blocking failure paths and signaling.
- Corrupt partial `.spa` must not be treated as authoritative assignment
  input (fail closed / discard / retry — implementation detail in those
  items).
- Competition / area % / focus chrome stay unavailable until a verified
  sidecar is present (existing empty / no-area behaviour).

**Related documents.** Product spec §8.6, §31; SPD-007, SPD-020, SPD-027;
SP-042; SP-046; SP-048;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-032 — Freeze production `.spa` blob contract before CDN publish

**Decision.** Before publishing production `.spa` blobs to the CDN, freeze the
**production blob contract**: at least HEALPix **`nside`**, **universe
ordering** for the dense assign column, and **`format_version`** (plus any
header fields required so clients and generator cannot silently disagree).
**SP-042 only records that this freeze is required**; field-level specification
and header changes land in **SP-043**. Do not publish production sidecars
against an unfrozen contract.

**Status.** Accepted.

**Context.** SP-026 / SP-028 notes already state the `.spa` header does not yet
encode `nside` / an explicit universe-order tag; client/generator agreement is
informal until freeze. Shipping CDN bytes before freeze risks irreversible
mismatched assignment. Recommended lock D6 = A.

**Consequences.**

- SP-043 produces the frozen contract doc / format bump as needed.
- SP-044+ publish and client consume only post-freeze artifacts for
  production.
- Fixture / offline harness blobs (SP-032) remain non-CDN.

**Related documents.** SPD-017, SPD-021, SPD-022; SP-026 notes; SP-028 notes;
SP-042; SP-043; SP-044; SP-048;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-033 — Sidecar shipping is Phase 4 residual / pre-production packaging

**Decision.** Closing the production `.spa` download / packaging gap is its
**own track**: **Phase 4 residual / pre-production packaging** (SP-042 and
follow-ons SP-043–048). It is **not** a Phase 5 exit gate and **not** a Phase
10 device-walk residual. Phase 10 continues to list narrowed R1 as
pre-production only (not a device matrix item).

**Status.** Accepted.

**Context.** Phase 4 exit criteria were met with offline FI `.spa` (SP-032);
production mapgen emit and CDN download remain pre-production (R1). Phase 5
covers area progress UI; Phase 10 covers device hardening. Product owner lock
(D7 = A, 2026-08-07).

**Consequences.**

- README / phase-04 / phase-10 index this track under Phase 4 residual /
  pre-production packaging.
- Do not block Phase 5 acceptance on CDN `.spa` shipping.
- Do not fold SP-043–048 into Phase 10 device residuals.

**Related documents.** SP-031 R1; SP-032; SP-042; SP-043–048;
`phases/phase-04-administrative-area-pipeline.md`,
`phases/phase-10-android-release-hardening.md`;
`docs/implementation/README.md`.

---

## SPD-034 — Production `.spa` blob contract (`format_version` 2)

**Decision.** Freeze the production `.spa` header / assign-universe contract as
follows (little-endian; fields after `index_width` are **format_version 2**
only):

| Item | Value |
| --- | --- |
| `format_version` | **2** (`kSpaFormatVersion`) |
| `nside` | **1048576** (`kSpaNside`; SPD-017) |
| `universe_order` | **1** = AscendingNest (`kSpaUniverseOrderAscendingNest`) — dense `assign[i]` slots are strictly ascending HEALPix NEST ids, same order as `ScanUniverseAscending` |
| `reserved[3]` | must be **0** on write; non-zero → fail-closed on read |

**Writer.** Always emits format_version 2 with the production `nside` /
`universe_order` / zero reserved bytes.

**Reader (fail-closed).**

- format_version **2**: require `nside == 1048576`, `universe_order ==
  AscendingNest`, and `reserved == 0`; else corrupt / reject.
- format_version **1** + `assign_count == 0`: accept (geometry-only dual-read
  for fixtures / offline harness).
- format_version **1** + `assign_count > 0`: reject on production load paths.
- Other format versions: unsupported.

This closes the field-level freeze gated by **SPD-032**. Do not invent grids or
change `nside`. Mapgen emit of production assignment blobs remains **SP-044**.

**Status.** Accepted.

**Context.** SP-026 / SP-028 previously documented an informal universe-order
contract without header fields. Shipping CDN assignment blobs before encoding
`nside` / order risked silent client/generator mismatch. Product lock D6 /
SPD-032 required this freeze before CDN publish; SP-043 implements it.

**Consequences.**

- Notes SP-026 / SP-028 describe AscendingNest as the production contract
  (not unsorted emit order).
- SP-044+ emit and clients consume only post-freeze artifacts for production
  assignment sidecars.
- Geometry-only v1 fixtures remain readable until retired.

**Related documents.** SPD-017, SPD-021, SPD-022, SPD-032; SP-026 notes;
SP-028 notes; SP-042; SP-043; SP-044;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-035 — Single CDN≡LAN `.spa` publish layout

**Decision.** Production CDN and local-network (LAN) mirrors use **one**
on-disk / HTTP publish tree. Paths match what the client already builds
(`GetFileDownloadUrl`, `SERVER_MAPS_FILE`):

```text
{root}/
  meta/
    maps.json
  maps/
    {MAP_SERIES}/{dataVersion}/
      countries.txt
      countries.txt.sig
      {mwmLeafId}.mwm
      {mwmLeafId}.spa    # iff leaf has exploration sidecar
```

Do **not** invent `/spa/…`, query params, alternate extensions, a separate
debug URL scheme, serving `.spa` from a different base than maps, or embedding
`.spa` inside the `.mwm`.

**Status.** Accepted.

**Context.** Client download (SP-046) and meta parse (SP-045) are Accepted;
ops publish and LAN serve remain residual. Device testing cannot `adb push`
`.spa` into app-private storage. A second layout would diverge from CDN and
risk a debug-only path. Product-owner lock D8 (2026-08-08) via SP-049.

**Consequences.**

- SP-050 assembles this tree for both CDN publish and LAN serve; SP-051 serves
  it over HTTP.
- Advertisement remains `countries.txt` fields only (**SPD-028** / D9) —
  presence of both `"spa"` and `"spa_sha1_base64"` is the only spa signal; no
  headers, directory listing, or side manifest.
- Custom Maps server URL is never a build default (SP-004 / D12): LAN URL is
  entered in Advanced settings only; no flavor, debug build type, or
  `BuildConfig` may default to a private-network address.

**Related documents.** SPD-028; SP-004; SP-045–046; SP-049–051;
`notes/spa-local-download-current-state.md`;
`phases/phase-04-administrative-area-pipeline.md`.

---

## SPD-036 — Countries signature kept; Channel A meta-only version bump

**Decision.** Applying a new `countries.txt` from a custom/LAN server still
requires a valid `countries.txt.sig` verified with
`COUNTRIES_TXT_SIGNATURE_HEX`. Do **not** weaken Ed25519 verification when
`CustomMapServerUrl` is set.

When only spa advertisement (and optional other meta) changes, **Channel A**
bumps countries `"v"` and `meta/maps.json` `"latest"` together, keeps MWM
`"s"` / `"sha1_base64"` unchanged, resigns, and serves MWMs under the **new**
version directory (same MWM bytes). Same-version `maps.json` `latest` does
**not** apply spa-only meta changes (`Storage::RunCountriesCheckAsyncSaveOnly`
skips when `latest <= m_currentVersion`).

**Status.** Accepted.

**Context.** Unsigned apply would be a security hole inherited by community
custom servers. A same-version client refresh path is optional later and must
not block SP-050–053. Product-owner lock D10 (2026-08-08) via SP-049.

**Consequences.**

- SP-050 supports `--publish-version` for the meta-only bump; SP-052 documents
  Channel A as the preferred LAN advertisement path.
- Reject unsigned countries apply and “set latest == current and hope spa ads
  appear.”

**Related documents.** SPD-028, SPD-035; SP-049; SP-050; SP-052;
`notes/spa-local-download-implementation-plan.md`.

---

## SPD-037 — Temporary bundled countries spa inject channel

**Decision.** Until CDN publishes spa-bearing `countries.txt`, device testing
may use a **bounded, non-default** temporary channel: rebuild an APK with spa
fields injected into **bundled** `data/countries.txt` for FI leaves only (same
`"v"` / MWM hashes), and serve `.spa` (and optionally `.mwm`) from a LAN custom
server. Prefer signed LAN countries with Channel A version bump (**SPD-036**)
when available. WritableDir countries override via signed update is the same
production path as Channel A.

**Do not merge** spa advertisements into `street-pixels` `data/countries.txt`
until CDN (or equivalent) will serve matching blobs — otherwise stock CDN users
advertise missing spa → IncompleteSpa.

**Reject as V1 approach:** unsigned countries apply; ADB push into map dirs;
debug JNI “install spa from path”; making Spa mandatory for Map OnDisk.

**Status.** Accepted.

**Context.** Scoped storage blocks copying `.spa` onto device for Helsinki
walks. A temporary inject channel unblocks testing without inventing a second
download protocol. Product-owner lock D11 (2026-08-08) via SP-049.

**Consequences.**

- SP-052 documents Channel B (temporary inject) as debug support only; landing
  rule forbids early merge to stock bundled countries.
- SP-053 may use Channel B when Channel A is not yet operable.

**Related documents.** SPD-028, SPD-031, SPD-036; SP-049; SP-052; SP-053.

---

## SPD-038 — SP-049–053 track is Phase 4 residual / device enabler

**Decision.** The LAN/CDN `.spa` publish-mirror track (**SP-049–053**) is
**Phase 4 residual / pre-production packaging** continued (same track as
SP-042–048 per **SPD-033**), and is the **device enabler** for Phase 5 /
Phase 10 Helsinki walks. It is **not** a Phase 5 exit criterion, does not
reopen Phase 5 coding (SP-033–040), and is **not** Option A mapgen collectors.

**Status.** Accepted.

**Context.** SPD-033 placed sidecar shipping outside Phase 5 / Phase 10 device
residuals. SP-049–053 closes the ops residual (serve advertised `.spa` on a
production-shaped HTTP tree) so devices can download sidecars without
`adb push`. Product-owner lock D13 (2026-08-08) via SP-049.

**Consequences.**

- README / phase-04 index SP-049–053 under Phase 4 residual; SPD-033 still
  holds (not Phase 5 exit).
- Do not fold assemble / LAN server / advertise / device playbook into Phase 5
  exit or Option A collectors.
- Affirms SP-004 / D12: no build-default custom map-server URL (see also
  SPD-035).

**Related documents.** SPD-033, SPD-035; SP-042–048; SP-049–053;
`phases/phase-04-administrative-area-pipeline.md`;
`phases/phase-10-android-release-hardening.md`;
`docs/implementation/README.md`.

---

## SPD-039 — `meta/maps.json` field contract (`map-series`, `latest`, `status`)

**Decision.** Assemble / LAN / CDN `meta/maps.json` must match what
`ParseServerMapsAndGetLatestVersion` reads:

- top-level **`"map-series"`** (hyphen), object keyed by series string;
- per series: **`"latest"`** (integer), **`"status"`** (string; `"EOL"` sets
  EOL flag).

Non-EOL series use **`"status": "active"`** (CDN convention). Do not invent
values like `"current"`. Unknown non-EOL status strings may still parse today
(only `"EOL"` is special-cased) but matching CDN avoids drift.

**Status.** Accepted.

**Context.** Client meta parse already expects CDN field names. Divergent LAN
JSON would break version checks or silently EOL maps. Product-owner lock D14
(2026-08-08) via SP-049.

**Consequences.**

- SP-050 writes `meta/maps.json` with `"map-series"`, `"latest"`, and
  `"status": "active"` (or `"EOL"` when intentional).
- Channel A bumps (`SPD-036`) update `"latest"` together with countries `"v"`.

**Related documents.** SPD-035, SPD-036; SP-049; SP-050;
`notes/spa-local-download-implementation-plan.md`.

---

## SPD-040 — Routing uses the personal explored set including imported pixels

**Decision.** Prefer-unexplored and Avoid routing weights use
`df::StreetPixel::IsExplored()` — the personal explored set, including
imported-only cells. `IsEverLive()` is unused for routing. Competition
isolation is unchanged: imported exploration never affects recency, ownership,
eligibility, or weekly ranking.

**Status.** Accepted. Closes OQ-2 for V1.

**Context.** Product-owner lock R1 (2026-08-15) via SP-055. Spec §17.2 talks
about unvisited pixels on the personal map. Imported GPX turns pixels green
and counts for personal completion (SPD-026, §29.2). Treating imported-green
streets as unexplored for routing would contradict the map. Audit §12 already
framed this as personal routing vs competition. Current
`GetSegmentExplorationWeightMultiplier` already uses `IsExplored()`.

**Consequences.**

- OQ-2 is struck for V1.
- SP-056/057 tests treat imported-only cells the same as live-explored cells
  for weights and Avoid exclusion.
- Phase 8 must not infer competition eligibility from routing weights.

**Related documents.** Product spec §17.2, §29.2; SPD-015, SPD-026; OQ-2;
SP-055; `phases/phase-06-exploration-aware-routing.md`.

---

## SPD-041 — Walk/bike Prefer and Avoid options; keep the strength seekbar

**Decision.** Exploration routing options on walking and cycling surfaces are
**Prefer unexplored** and **Avoid explored**, mutually exclusive. Neither
selected is ordinary (standard) routing. The existing 0–100 **strength
seekbar** stays in V1 and applies to Prefer (walk, bike, and the existing car
Prefer control). A later replacement by max ETA or kilometre deviation from
the optimal route is post-V1. Avoid is pedestrian and bicycle only; it does
not apply to car routing. Car Prefer may remain on the driving-options
surface.

**Status.** Accepted.

**Context.** Product-owner locks R2–R4 (2026-08-15) via SP-055. Spec §17.2–§17.3
name Prefer and Avoid; they do not name a third “Standard” mode. The seekbar
already exists on `DrivingOptionsFragment`. Product wants that abstract
strength control on V1 rather than hiding it.

**Consequences.**

- SP-056 exposes Prefer + seekbar and the Avoid option (Avoid weights land in
  SP-057) on `WalkingOptionsFragment` and `CyclingOptionsFragment`.
- Persist Prefer vs Avoid vs neither; migrate `m_enabled == true` → Prefer.
  Strength remains persisted.
- Do not Pro-gate Prefer or Avoid (§29.1).

**Related documents.** Product spec §17.2, §17.3, §29.1; SP-055; SP-056;
`phases/phase-06-exploration-aware-routing.md`.

---

## SPD-042 — Avoid excludes fully explored edges; fallback is Prefer with strength

**Decision.** V1 Avoid:

1. **Edge test (R5).** Exclude an edge only when `exploredRatio == 1` (every
   **matched** HEALPix sample on the segment is explored). Edges that still
   have at least one unexplored matched sample remain usable. Unmatched
   samples (no `.pix` hit) are not explored.
2. **Algorithm (R12).** The strict pass uses **true exclusion** of those fully
   explored edges, not a large finite penalty as the Avoid implementation.
3. **No-route (R6).** If that pass finds no path, show a **clear no-route**
   result (distinct from generic `RouteNotFound` / missing maps). Offer a
   simple control that switches to **Prefer with the strength seekbar**.
   Never auto-switch. Never silently keep Avoid while using Prefer weights.
4. **No min-connection search (R7).** V1 does **not** implement “allow the
   minimum necessary explored connection” as a second optimisation. Fallback
   is Prefer with strength.
5. **Warning (R8).** Show the §17.3 warning before Avoid is applied: “This
   can produce very long routes or no available route.”

**Status.** Accepted. Supersedes the fallback-offer pair in SPD-009
consequences. Does not reopen whether Avoid ships in V1.

**Context.** Product-owner locks R5–R8 and R12 (2026-08-15) via SP-055.
Excluding every edge with any explored pixel (`exploredRatio > 0`) would
often yield no route or extreme detours. Soft 10× Prefer is not Avoid.

**Spec divergence (recorded, not silently resolved).**

- Spec §17.3 describes avoiding edges **containing** explored pixels. V1
  implements that as **fully explored** edges only (`exploredRatio == 1`).
- Spec §17.3 / §31 offer “allow the minimum necessary explored connection”
  (or “a small amount of explored routing”) and “return to normal routing.”
  V1 offers an explicit switch to **Prefer with strength** instead.

The product spec is not edited here. Phase 6 exit criteria follow this SPD.

**Consequences.**

- SP-057 implements exclusion at `exploredRatio == 1` and a distinct
  no-route result.
- SP-058 implements the warning, the no-route UI, and the Prefer+seekbar
  switch. It does **not** implement a min-connection cost function.
- Mixed-explored edges are weighted as ordinary (Avoid) or by the Prefer
  multiplier (Prefer mode), never excluded.
- SPD-009’s “never silently degrade to prefer” still holds: the switch is
  an explicit user action.

**Related documents.** Product spec §17.3, §31, §34; SPD-009; SP-055;
SP-057; SP-058; `phases/phase-06-exploration-aware-routing.md`.

---

## SPD-043 — Mid-navigation Avoid does not abandon a followed path that turned green

**Decision.** While **following** an Avoid route, newly explored pixels on
the remaining followed geometry must **not** by themselves trigger a
re-search that abandons that path. Off-route detection and user-requested
recalculation may re-apply Avoid from the new position. If the strict pass
then fails, show the SPD-042 Prefer+strength control — do not silently
inject fully explored edges.

**Status.** Accepted.

**Context.** Product-owner lock R9 (2026-08-15) via SP-055. Audit §12 listed
mid-navigation instability as a hard-Avoid risk.

**Consequences.**

- SP-059 implements follow-stability and off-route rebuild using SPD-042
  fallback, not a min-connection retry.

**Related documents.** Product spec §17.3; SPD-042; SP-055; SP-059;
audit §12.

---

## SPD-044 — Routing-mode analytics are count-only

**Decision.** Prefer-unexplored and Avoid usage are recorded as **counts
only**: prefer-used, avoid-used, avoid-fallback-prefer. No origin,
destination, geometry, pixel ids, or area ids. Implement a shared counter
API. If no privacy-safe upload sink exists, keep counters local and residual
**upload** to Phase 10. Do not send these events through Sentry.

**Status.** Accepted.

**Context.** Product-owner lock R10 (2026-08-15) via SP-055. SP-003 deferred
product-analytics events. Phase 6 exit #6 still requires mode-usage
measurement (spec §32.2).

**Consequences.**

- SP-060 implements the counters. Avoid-fallback-prefer increments when the
  user takes the SPD-042 switch. There is no min-connection counter.

**Related documents.** Product spec §32.2, §25.1, §34; SP-003; SP-055;
SP-060.

---

## SPD-045 — Routing weights use the segment MWM’s `.pix` when installed

**Decision.** Prefer and Avoid consult the `.pix` for the **segment’s MWM**
when that file is installed, not only the overlay’s `m_countryId`. Missing
`.pix` → unmatched samples → not explored (Prefer multiplier 1.0; Avoid
does not exclude). Do not load extra leaves into the renderer overlay.

**Status.** Accepted.

**Context.** Product-owner lock R11 (2026-08-15) via SP-055. Today
`GetSegmentExplorationWeightMultiplier` returns 1.0 when
`mwmCountryName != m_countryId`, so cross-leaf routes silently ignore
exploration.

**Consequences.**

- SP-056/057 replace the mismatch early-return with a per-segment leaf
  lookup. Optional cache of recently used leaf maps is an implementation
  detail; dropping weights to 1.0 is not.

**Related documents.** SP-055; SP-056; SP-057;
`phases/phase-06-exploration-aware-routing.md`.

---

## SPD-046 — Completion-card geometry is a rings-only outline

**Decision.** V1 share image is composed off-map from `ExplorationArea::m_rings`
(outer rings only). Shared card model in `libs/`; Android or a headless
rasteriser given only that model. Never capture Drape / `MapView`; never draw
explored HEALPix, route, home, live location, track, or position marker. Do not
use MWM / country id as the title. No lat/lon, `geo:`, or ge0 URLs. No
individual timestamps or other users’ personal information. `area_overlay` stays
in-app. City-summary does not fire area 25/50/100 or a city share card.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M1 (2026-08-19) via SP-062. Closes
OQ-9. Spec §19.1 allows “stylized map or boundary outline”; V1 chooses the
outline branch for deny-list safety.

**Consequences.**

- SP-067 composes from rings, not a live map screenshot.
- Phase 7 stylised-map entry criterion is met for coding.

**Related documents.** Spec §19.1; SPD-008; SP-062; SP-067; phase-07.

---

## SPD-047 — First-100 m counts 10 newly explored live pixels

**Decision.** V1 first-100 m counts **newly explored cells only** (today’s
`numNewlyExploredPixels`), not `IsEverLive` flips. Threshold is **10 new live
pixels** from spec §10 step 10 (`30 new live pixels ≈ 300 m`). This is not
geodesic 100 m. Import-only writes never count.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M2 (2026-08-19) via SP-062. Closes
OQ-10.

**Consequences.**

- SP-064 implements 10-pixel threshold with newly-explored counting only.
- A single 25 m collection pulse may complete the goal.

**Related documents.** Spec §10 steps 6, 9, 10; SP-062; SP-064; phase-07.

---

## SPD-048 — Area milestone fired-state is keyed by OSM id

**Decision.** Milestone fired-state and the original 100% completion date live in
a new local SQLite store (`area_milestones.db`), keyed by **OSM id**. Fired
thresholds are recorded in a per-area mask (25 / 50 / 100). Not `settings.ini`
unbounded rows; not `StreetStatsDB` feature-bitmask tables. Compact index is a
cache hint only.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M3 (2026-08-19) via SP-062. Closes
OQ-11.

**Consequences.**

- SP-063 introduces `AreaMilestoneStore` in `libs/street_pixels_areas/`.
- Stable across `.spa` regen and compact-index changes when OSM id is unchanged.

**Related documents.** Spec §18.5; SP-062; SP-063; phase-07.

---

## SPD-049 — Area milestones do not re-fire after map update

**Decision.** Milestone fired-state and the original 100% date survive rematch,
policy change, and `.spa` refetch. Re-crossing a threshold after a drop does
**not** re-fire.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M4 (2026-08-19) via SP-062. Closes
OQ-12. Spec §27.4.

**Consequences.**

- `InvalidateAreaCompletionCache` and rematch must not reset milestone store.
- UI may say an area was previously completed while showing current %.

**Related documents.** Spec §27.4; SP-062; SP-063; phase-07.

---

## SPD-050 — Multi-area milestone celebrations queue

**Decision.** When several areas cross thresholds in one update, queue
non-blocking acknowledgments. Show one at a time. Priority: 100% > 50% > 25%.
Never interrupt `IsRoutingFollowing`. First-100 m is independent. City-summary
rollup does not enqueue area celebrations.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M5 (2026-08-19) via SP-062. Closes
OQ-13.

**Consequences.**

- SP-065 owns presentation queue; SP-063 records fires only.

**Related documents.** Spec §18.4; SP-062; SP-065; phase-07.

---

## SPD-051 — Store 100% date always; card shows only if opted in

**Decision.** Always persist the original 100% completion date locally. The card
includes it only if the user opts in at share time; **default off**. Owner:
SP-068. Alternative: omit the date from the V1 card entirely (drops the
control). Do not default-on with no control.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M6 (2026-08-19) via SP-062. Closes
OQ-14.

**Consequences.**

- SP-063 stores `completed_100_at` on first 100% fire.
- SP-068 owns share-time opt-in.

**Related documents.** Spec §18.5, §19.1; SP-062; SP-067; SP-068; phase-07.

---

## SPD-052 — Completion card works without competition profile

**Decision.** Card and first-person copy work with no profile and no nickname.
Provide a stub hook for spec §22.10; Phase 8 fills leading / not-leading copy.
Never imply personal completion was invalid.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M7 (2026-08-19) via SP-062. Closes
OQ-15.

**Consequences.**

- SP-065 / SP-067 ship anonymous copy without Phase 8.

**Related documents.** Spec §19.2, §22.10; SP-062; SP-065; SP-067; phase-08.

---

## SPD-053 — First-100 m is once per install

**Decision.** First-100 m appears on first recording start, completes at the
M2 threshold, and never returns. Incomplete progress persists across recording
sessions until complete. Not per-session, not per-area.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M8 (2026-08-19) via SP-062. Closes
OQ-16.

**Consequences.**

- SP-064 owns install-scoped first-goal state.

**Related documents.** Spec §10 steps 6, 9; SP-062; SP-064; phase-07.

---

## SPD-054 — Exploration haptics predicate and milestone patterns

**Decision.** Exploration haptic pulse iff recording (not paused) **and**
application foreground **and** exploration-haptics toggle on (default on). One
collection pulse per collecting update, not per pixel. Stronger patterns for
first-100 m complete, 50% area, and 100% area. No extra pattern for 25%. Boss
pattern is out of V1 scope. Area milestones may fire without haptic when not
recording (e.g. import-driven threshold cross).

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M9 (2026-08-19) via SP-062. Closes
OQ-17.

**Consequences.**

- SP-066 wires foreground into Street Pixels and fixes per-pixel vibration.
- SP-064/065 emit milestone haptic events consumed by SP-066.

**Related documents.** Spec §28.1–§28.4; SP-062; SP-064; SP-065; SP-066;
phase-07.

---

## SPD-055 — Milestone growth analytics are count-only

**Decision.** Count-only local analytics: completion card generated and share
action initiated. No area name, OSM id, coordinates, or image bytes. Persist as
uint64 in settings (SPD-044 pattern). Upload residual → Phase 10 if no sink. Not
Sentry. Does not add spec §32.1 first-pixel / first-100 m product events in
Phase 7.

**Status.** Accepted.

**Context.** Maintainer lock of Phase 7 M10 (2026-08-19) via SP-062. Closes
OQ-18.

**Consequences.**

- SP-068 owns counters.

**Related documents.** Spec §32.4; SPD-044; SP-062; SP-068; phase-07.

---

## 15. Recorded open questions (not decisions)

These are carried from existing project documents. They are listed so they are
not lost. None of them is decided here, and nothing in this section may be
treated as authorisation.

Phase 7 M1–M10 were locked 2026-08-19 via SP-062 as **SPD-046–055** (see
numbered sections above). Remaining open questions:

| Ref | Question | Source | Blocks |
| --- | --- | --- | --- |
| OQ-1 | The area-completion formula (§7), ownership-score formula (§22.4), and contested-state threshold (§22.9) are empty in the product spec. **Personal completion slice closed by SPD-026** (explored/total; live+imported). Ownership / contested remain open. | Product spec; audit §2, §22 | Phase 8 (ownership / contested). Phase 5 personal completion → SPD-026. |
| OQ-2 | ~~Does prefer-unexplored routing use the personal explored set including imported pixels, or live-only?~~ | Audit §12, §27 Q4 | **Closed by SPD-040** — personal `IsExplored()` including imported; `IsEverLive()` unused for routing. |
| OQ-3 | Weekly leaderboard reset when a city's local time zone is unknown. | Audit §24 | Phase 8. |
| OQ-4 | Nickname uniqueness: the spec says nicknames need not be unique, but the current backend enforces a unique `username`. | Product spec §20.4; backend `core/models.py` | Phase 8. |
| OQ-5 | ~~Bridge and tunnel eligibility, and the motorway-with-explicit-bicycle-access case, after a tag-survival audit.~~ | Product spec §13.1; audit §6, §27 Q9 | **Closed by SP-020** — bridges include; tunnels exclude; motorway/motorway_link (incl. bridge) require `hwtag-yesbicycle`. |
| OQ-6 | Whether the in-progress friends feature is retained in Street Pixels builds. Friends exist in Android and in `comaps_backend` but are a product non-goal for V1. | Product spec §6; audit §15, §27 Q7 | Phase 1 (what a public build exposes) and Phase 8. |
| OQ-7 | Production API base URL, hosting region, and data-retention policy. | Audit §27 Q6 | Phase 8, and partially Phase 1 (SP-004). |
| OQ-8 | ~~Whether HEALPix `nside` stays at 1048576 after rendering measurement.~~ | Audit §27 Q8 | **Closed for V1 by SPD-017** — `nside = 1048576` locked. |
| OQ-9 | ~~Phase 7 M1 compositor: how is the stylised map on the completion card rendered?~~ | SP-062 (2026-08-19); spec §19.1 | **Closed by SPD-046** — rings-only outline from `m_rings`; never a live Drape / `MapView` screenshot. |
| OQ-10 | ~~Phase 7 M2: what is “approximately 100 metres of new live street pixels”?~~ | SP-062 (2026-08-19); spec §10 steps 9–10 | **Closed by SPD-047** — 10 newly explored live pixels; not `IsEverLive` flips. |
| OQ-11 | ~~Phase 7 M3: where does milestone fired-state live?~~ | SP-062 (2026-08-19) | **Closed by SPD-048** — `area_milestones.db` keyed by OSM id. |
| OQ-12 | ~~Phase 7 M4: re-fire after map update?~~ | SP-062 (2026-08-19); spec §27.4 | **Closed by SPD-049** — does not re-fire; date survives rematch. |
| OQ-13 | ~~Phase 7 M5: several areas cross thresholds in one session?~~ | SP-062 (2026-08-19); spec §18.4 | **Closed by SPD-050** — queue; never interrupt following. |
| OQ-14 | ~~Phase 7 M6: completion date on the card?~~ | SP-062 (2026-08-19); spec §19.1 | **Closed by SPD-051** — store always; card opt-in default off (SP-068). |
| OQ-15 | ~~Phase 7 M7: competition line on the card?~~ | SP-062 (2026-08-19); spec §19.2, §22.10 | **Closed by SPD-052** — stub; Phase 8 fills copy. |
| OQ-16 | ~~Phase 7 M8: first-100 m lifetime?~~ | SP-062 (2026-08-19); spec §10 steps 6 and 9 | **Closed by SPD-053** — once per install. |
| OQ-17 | ~~Phase 7 M9: exploration haptics predicate?~~ | SP-062 (2026-08-19); spec §28.1–§28.4 | **Closed by SPD-054** — recording ∧ foreground ∧ toggle; one pulse per update. |
| OQ-18 | ~~Phase 7 M10: growth analytics for cards?~~ | SP-062 (2026-08-19); spec §32.4 | **Closed by SPD-055** — count-only; no area id. |

When one of these is answered, add a new `SPD-NNN` entry above and strike the
row here with a reference to it.
