# SP-093 — Privacy policy, terms, and competition consent alignment

**Phase:** 10 — Android release hardening
**Status:** Residual
**Depends on:** SP-088 H4 Accepted (**SPD-080**). Landing text/URLs is
  residual; this item is **not** a Phase 10 coding item.
**Unblocks:** none in this slice (SP-097 exit #5 privacy-policy match
  remains residual until a later WI lands the text)
**Notes:** Brand residual. Product-owner lock 2026-08-29 accepted the
  *intended* H4 position (product-owned Street Pixels policy) but
  **landing the actual policy/terms/URLs is residual**. Phase 10
  slice close-out 2026-08-29 records current Streifzug URLs and consent
  copy; it does **not** land H4 text. Do not implement landing in
  SP-089–097.

---

## Objective

Make the privacy policy, terms, competition rules, and in-app
competition consent text match actual behaviour item by item: what
stays local, what is uploaded, delay/jitter, sparse-area anonymity,
deletion, nickname uniqueness (SPD-059), and that friends / live
location / nearby discovery are absent.

**This work is residual.** Do not produce, host, or retarget policy
or terms in this Phase 10 coding slice. `https://streifzug.app/privacy/`
and `terms/` may stay for now (**SPD-080**).

---

## Motivation

Help currently opens `https://streifzug.app/privacy/` and `terms/`.
Those pages describe Streifzug, not Street Pixels competition aggregates
or session GPS. Spec §34 requires the policy to describe local vs
uploaded data, consent text to match behaviour, and terms to cover
public nicknames and rankings.

`explore_consent_message` is already closer to spec §20.2 than the
web policy. The web policy is the gap. Consent re-prompt is SPD-064.

Product-owner lock 2026-08-29 residualised landing this text.

---

## In-scope behavior

**None in Phase 10 coding.** Recorded residual work, for a later WI:

- Produce or link the H4-owned policy and terms. If the canonical
  text lives outside git, check in a dated snapshot under
  `docs/implementation/` that the in-app URLs must match.
- Point Help / settings rows at those URLs (coordinate with SP-090
  settings rows; SP-090 itself does not land the URLs).
- Line-by-line checklist of `explore_consent_message` vs spec §20.2
  vs `CompetitionUploadService` allow-list vs backend schema.
  Correct copy that over- or under-claims.
- Terms: public nicknames (unique per SPD-059), rankings, reporting,
  7-day rename, deletion leaves local exploration (spec §21 / §26).
- Record policy version string used in `IdentityStore` consent;
  bump if text meaning changes (SPD-064 re-prompt).
- EU hosting / retention: SPD-062; exact region string is ops — the
  policy must not name a wrong region.
- Confirm clearing app data vs delete-competition-profile vs delete
  map: policy sentences match code (SP-018 archive, SP-077 delete).

## Out-of-scope behavior

- All of the above, in this Phase 10 slice.
- Inventing a new upload field to make the policy “simpler”.
- Server-side anti-cheat theater (audit: accept residual cheating).
- Legal review as a substitute for the checklist; legal may add
  follow-up but this item’s gate is behavioural match.

## Relevant product requirements

- Spec §3.2, §20.2, §20.5, §21, §25, §26, §30, §34 Release
  governance.
- SPD-014, SPD-059, SPD-061, SPD-062, SPD-064, **SPD-080**.

## Relevant source files or symbols

- `HelpFragment` URL construction
- `explore_consent_message` and related strings
- `IdentityStore::kCompetitionPrivacyPolicyVersion`
- `CompetitionUploadService` payload
- Backend competition schema (explorer checkout if present)

## Implementation notes / constraints

- Do not edit `docs/STREET_PIXELS_PRODUCT_SPEC.md`.
- Do not implement this item in SP-089–097.
- English first. Translations of legal text need a human owner
  (H4); do not machine-translate policies in this item.
- Consent must remain separate from location permission.

## Acceptance criteria

Not applicable in this Phase 10 coding slice. Residual until a later
work item lands SPD-080 text.

When that later item runs:

1. Checklist: every consent bullet is true of the binary + backend.
2. In-app URLs resolve to H4 text, not unmodified Streifzug pages
   (unless a later SPD explicitly keeps Streifzug pages after they are
   updated).
3. Policy version bump recorded if meaning changed.
4. Agent does not mark Accepted.

## Required automated tests

- None while residual.

## Required manual validation

- None while residual. Device traffic capture that opt-out uploads
  nothing is SP-095 (**residual**).

## Failure and rollback considerations

- Do not ship a rewritten Streifzug privacy URL as a stand-in while
  this item is residual; leaving `streifzug.app` is the recorded
  residual posture (**SPD-080**).

## Completion evidence

Phase 10 **residual slice close-out** only. No binary change. No hosted
policy invented. Help URLs and Streifzug product name unchanged (**SPD-080**).
Landing H4 text/URLs remains open. **This item is not Accepted.**

Independent review 2026-08-29 checked `HelpFragment`,
`explore_consent_message` / `explore_consent_title`, and
`IdentityStore::kCompetitionPrivacyPolicyVersion` against this tree.
Corrections below are documentation only (localised URL exceptions,
Help row ids, nearby-discovery evidence, empty Accepted fields).

| Field | Value |
| --- | --- |
| Branch | `cursor/sp-093-privacy-residual-6383` |
| Policy / terms URLs | Residual — English Help still opens `https://streifzug.app/privacy/` and `terms/`. Snapshot below. |
| Consent checklist | Residual for landing. Current `explore_consent_title` / `explore_consent_message` snapshot below; copy not rewritten (does not over-claim). |
| Policy version | `"1"` (`IdentityStore::kCompetitionPrivacyPolicyVersion`). Not bumped: consent meaning unchanged. |
| Binary / strings | Unchanged in this slice (`git diff street-pixels...HEAD` is docs only) |
| Independent review | 2026-08-29 — residual snapshot vs tree; over-claims corrected. Not Accepted. |
| Accepted by | |
| Accepted date | |

### Surfaces that still open Streifzug pages

Android Help is the public-V1 URL surface. Construction is
`R.string.app_site_url` + `"privacy/"` or `"terms/"` in
`HelpFragment` (`privacy_policy`, `term_of_use_link`). English
`app_site_url` (`values/` and `values-en/`) is `https://streifzug.app/`,
so those locales resolve to `https://streifzug.app/privacy/` and
`https://streifzug.app/terms/`. Most other `values-*/` `app_site_url`
strings stay on the `streifzug.app` host with an optional language
prefix (for example `https://streifzug.app/de/` + `privacy/` →
`https://streifzug.app/de/privacy/`). Recorded exceptions, not
retargeted here:

- `values-fr-rCA`: `https://www.streifzug.app/fr/`
- `values-ar`: `https://streifzug.app/ar` (no trailing slash)
- `values-eo`: `https://streifzug.app/eo` (no trailing slash)
- `values-fa`: `https://streifzug.app` (no trailing slash)

Those pages describe Streifzug, not Street Pixels session GPS, local
`.pix`, or competition aggregates.

Related Help rows that also stay on Streifzug (not retargeted): site
home (`R.id.web` → `app_site_url`), news (`R.id.news` →
`https://www.streifzug.app/news/`), Support us (`R.id.support_us` →
`app_site_url` + `community/`).

Settings Privacy information and Terms / competition rules (SP-090)
are **in-app dialogs**. They reuse `location_privacy_info` +
`explore_consent_message` (and leave/delete copy for rules). They do
**not** open `privacy/` or `terms/`.

iOS is not Android V1. `iphone/Maps/UI/Help/About/AboutView.swift`
still concatenates `translated_om_site_url` + `privacy/` / `terms/`
(often `https://www.streifzug.app/…`). Out of this slice.

Play Data safety still needs a policy URL before Publish
(`docs/implementation/play-data-safety.md`). That URL is not invented
here. Listing brand copy remains Streifzug (**SPD-079** / **SPD-084**).

### `explore_consent_message` current state (not rewritten)

English `values/strings.xml` and `values-en/strings.xml` (identical;
no other locale overrides these keys). Dialog title
(`explore_consent_title`):

> Join local exploration rankings

Message body (`explore_consent_message`):

> Join local exploration rankings.
>
> When you opt in:
> • Aggregated area statistics are uploaded.
> • Your public nickname is shown where rankings have enough participants.
> • Exact routes are not uploaded.
> • Raw GPS is not uploaded.
> • Live location is not shared.
> • Nearby-user discovery does not exist in this version.
> • You can disable participation later.

Surfaces: `ExploreConsentDialogFragment` (opt-in); Privacy settings
information and competition-rules dialogs (SP-090). Consent stays
separate from location permission.

| Spec §20.2 bullet | Consent copy | Vs `CompetitionUploadPayload` allow-list |
| --- | --- | --- |
| Aggregated area statistics uploaded | States it | True: `areas[]` (`area_osm_id`, `ownership_score`, `live_coverage_pct`, `eligible`) plus weekly city `new_live_count`. Copy does not name weekly-city counts (under-claim, not over-claim). |
| Public nickname where rankings have enough participants | States it | True: `nickname` is in the payload. Sparse-area nickname hiding is server-side (spec §23.4); copy does not claim otherwise. |
| Exact routes not uploaded | States it | True: no route/polyline/track keys in the allow-list. |
| Raw GPS not uploaded | States it | True: no lat/lon/gps keys. |
| Live location not shared | States it | True: no live lat/lon; upload is delayed batch (spec §25.3), not described in this string. |
| Nearby-user discovery absent in V1 | “does not exist in this version” | True of the upload allow-list (no presence / other-user lat-lon keys) and of public V1: no nearby-discovery client surface. Friends leftover (SPD-085 hide) is a different feature, not this bullet. |
| Participation can be disabled later | States it | True: settings competition toggle / leave / delete. |

Does **not** over-claim raw GPS, live location, routes, or nearby
discovery. `explore_consent_title` and the message lead-in omit spec
§20.2’s “and compete for neighborhood control” (under-claim). Copy
does not mention nickname uniqueness,
delay/jitter, EU region, or that profile deletion leaves local
exploration — those belong in landed policy/terms, not this string.

Allow-list keys (closed): `profile_id`, `nickname`,
`map_data_version`, `score_calc_version`, `last_update_unix`,
`areas`, `area_osm_id`, `ownership_score`, `live_coverage_pct`,
`eligible`, `weekly_cities`, `city_osm_id`, `new_live_count`
(`competition_upload_payload.hpp`; asserted in
`ProductAnalytics` payload-shape tests).

Policy version `"1"` is the consent key (`Explore.ConsentPolicyVersion`).
`HasCompetitionConsent` requires competition + aggregate sharing +
version match + non-zero timestamp (spec §20.5 / SPD-064). Bump only
when landed text meaning changes.

### Contradictions recorded (not resolved here)

- Spec §20.4 says nicknames need not be globally unique; **SPD-059**
  requires unique V1 nicknames (spec not edited). Consent does not
  mention uniqueness. Landed terms must follow SPD-059.
- Technical audit (2026-07-20) described consent as a boolean with no
  policy version. Current `IdentityStore` stores version `"1"` and a
  timestamp. Code wins; audit is dated.
- Explorer checkout (`/agent/repos/explorer`) has friends/`Explorer`
  schemas only — no competition upload schema to match. Backend
  line-by-line check waits on the competition app (SPD-065) in a
  later WI.

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Landing Street Pixels policy/terms/URLs (H4 / SPD-080) | Residual (this item); not SP-089–097 coding. Later WI produces/hosts text, retargets Help, snapshots canonical copy under `docs/implementation/` if hosted outside git. |
| Play Data safety form still needs a privacy-policy URL | Residual with landing. Do not invent a URL. |
| Play listing still Streifzug (“does not track / does not collect personal information”) | Residual **SPD-079** / **SPD-084**. Not this slice. |
| iOS About privacy/terms still `translated_om_site_url` + `privacy/` / `terms/` | Out of Android V1; retarget with iOS work. |
| Consent under-claims weekly-city aggregates, delay/jitter, uniqueness (SPD-059), deletion vs local `.pix` | Optional copy pass in the landing WI; must stay inside the upload allow-list. Not rewritten here. |
| Explorer checkout has no competition schema | Later WI checks the competition backend, not this friends-only tree. |
| A few `app_site_url` locales omit a trailing slash (`values-ar`, `values-eo`, `values-fa`); `values-fr-rCA` uses `www.streifzug.app` | Pre-existing Streifzug concatenation. Do not rewrite Help URLs in this residual slice. Landing WI should emit well-formed Street Pixels URLs. |
