# Phase 8 — Competition

**Status:** Not started
**Depends on:** Phase 4
**Blocks:** nothing; required for release

---

## Objective

Deliver the optional game layer: opt-in consent, pseudonymous identity and
nickname, area ownership with recency decay, boss eligibility, contested and
unclaimed states, a weekly city leaderboard, and a backend that accepts
aggregate area statistics and nothing else.

This phase spans both repositories: `comaps` (client) and `comaps_backend`
(API).

## Product-spec references

- §9.2 Competition layer contents.
- §10 steps 10–12 competition hint and opt-in journey.
- §20.1–§20.6 Opt-in default, explanation, installation identity, public
  nickname, consent record, leaving competition.
- §21.1–§21.5 Nickname rules, prohibited content, filtering and enforcement,
  renaming, limited profile surface.
- §22.1–§22.10 Ownership: competitive pixel set, recency weight with a ~30-day
  half-life, ownership score, boss eligibility, boss selection, unclaimed
  areas, server-side decay, contested state.
- §23.1–§23.5 Competition interface, ranking snapshot, sparse-area privacy,
  overtaking hints.
- §24.1–§24.4 Weekly city leaderboard.
- §25.1–§25.6 Local-only information, uploaded aggregates, delayed batching, no
  location discoverability, area-level implications, deletion.
- §26.2–§26.3 Competition while offline; conflict handling.
- §27.5 Competition versioning.
- §34 "Privacy and competition" launch requirements.

## Technical-audit references

- §15 Competition and backend feasibility, including the provisional API
  outline, the client-trust model, and decay from aggregates.
- §17 Privacy conflicts.
- §22 Risk register: client competition cheating, sparse-area privacy leaks,
  friends versus V1 non-goals.
- Spike 10.

## Current code locations

Verified 2026-07-25 against both working trees.

### Client — `comaps`

| Concern | Location | Observed state |
| --- | --- | --- |
| Identity | `libs/map/identity_store.cpp` | Device id in `SecureStorage` under `Explore.DeviceId`, 24 random bytes base64url; username in settings under `Explore.Username`; consent as a **boolean only** under `Explore.ConsentGiven`. No policy version, no timestamp. |
| Upload client | `libs/map/explore_stats_service.cpp` | Weekly per-region entries `{regionId, weekStartSec, exploredPixels, version}` plus device id and optional username. Periodic check every **1 minute**. Gated only by `m_syncEnabled`. Aggregation continues even when sharing is off. Persists to `explore_stats.json`. |
| Endpoint | `libs/map/backend_config.cpp` `GetStatsUploadUrl` | `{apiBase}/stats/upload` |
| Friends client | `libs/map/friends_manager.cpp` | Full friends API client with local cache, `X-Device-Id` and `X-Username` headers |
| Android UI | `MyAccountDialogFragment`, `ExploreConsentDialogFragment`, `Friends.java`, `item_friend_row.xml` | Friends-first account UI, consent dialog, add-friend deep links registered in the manifest |
| Ownership, recency, decay | — | Not found |

### Backend — `comaps_backend`

| Concern | Location | Observed state |
| --- | --- | --- |
| Stack | `pyproject.toml` | Python ≥3.12, Django 5.2.4, django-ninja 1.4.3, django-ninja-extra 0.30.1, psycopg, orjson |
| Settings | `comaps/settings/base.py`, `dev.py` | `django-environ`; SQLite by default; **no `prod.py`** |
| Models | `core/models.py` | `Explorer(AbstractUser + unique device_id)` and `Friendship`. **No competition, area, ownership, or stats models.** |
| Auth | `apis/auth.py` `DeviceIdAuth` | `X-Device-Id` plus `X-Username` headers resolve an `Explorer` |
| Endpoints | `apis/api.py` | `POST /signup`, `POST /update_username`, `GET /account/export`, `DELETE /account`, and five friends endpoints |
| `/stats/upload` | — | **Does not exist.** The client posts to an endpoint the server does not implement. |
| Throttles | `apis/throttling.py` | signup 5/h, username 10/h, delete 3/h, friends search 30/min, friends actions 60/h |
| Base path | `comaps/urls.py`, `apis/urls.py` | `/api/`, not `/api/v1/` |
| Tests, CI, deployment | — | No tests, no pytest config, no `.github/`, no Dockerfile, no Makefile |
| Linting | — | No ruff, black, or pre-commit configuration |

**Differences from the technical audit:** the backend has been restructured
since the audit. The `explorer/` package is gone, replaced by `comaps/`
(settings, urls, shared API services), `apis/` (controllers, auth, schemas,
throttling), and `core/` (models, admin, migrations). Authentication changed
from JWT to device-id header auth. The audit's substantive conclusions still
hold: only accounts and friends exist, and `/stats/upload` is still missing.

## Intended outcome

- A competition profile distinct from the friends-oriented account UI.
- A consent record carrying the privacy-policy version and a timestamp.
- Local computation of ownership scores, coverage, eligibility, and weekly
  city counts.
- Uploads restricted by schema to aggregate area statistics, batched at no more
  than once per 15 minutes plus up to 15 minutes of jitter.
- A backend implementing registration, nickname management, aggregate upload,
  area reads, weekly city reads, nickname reporting, and profile deletion, with
  server-side decay between uploads.
- Sparse-area anonymity enforced on the server, not only in client
  presentation.

## Dependencies

- Phase 4, for area identifiers. Without them there is nothing to own.
- Phase 3, for live-versus-imported flags, so imported exploration is excluded.
- OQ-1 must be resolved: the ownership-score, area-completion, and
  contested-state formulas are empty in the product spec.
- OQ-4 must be resolved: nickname uniqueness.
- OQ-7 must be resolved: production API base URL, hosting region, retention.

## Proposed work-item breakdown

Not yet decomposed. **This phase must not be broken into work items until OQ-1
is answered.** Coding a scoring system around an undefined formula produces
work that will be thrown away.

Likely shape once unblocked:

1. Consent record with policy version and timestamp, replacing the boolean.
2. Competition profile and nickname, separated from the friends account UI.
3. Local recency store and ownership computation.
4. Eligibility, contested, and unclaimed state evaluation.
5. Weekly city new-live-pixel counting.
6. Upload queue with the specified cadence, jitter, and offline behaviour,
   replacing the current one-minute poll.
7. Backend competition app: models, schema, endpoints, throttling.
8. Server-side decay.
9. Backend read APIs for area snapshot and weekly city board.
10. Sparse-area anonymity enforcement on the server.
11. Nickname validation, filtering, reporting, and administrative reset.
12. Profile and aggregate deletion.
13. Competition UI: mode control, area snapshot, ranking, overtaking hints.

## Data and migration concerns

- Per-pixel live recency is new local data. Storing a timestamp for every cell
  would change the storage profile materially; the audit recommends sparse
  storage or on-the-fly aggregation into area scores. Decide with measurement.
- The existing `explore_stats.json` weekly per-region entries are a different
  schema from competition aggregates. Decide whether to migrate or discard;
  there is no public user base yet, which argues for discarding.
- The consent boolean must migrate to a record with a policy version and
  timestamp. An existing `true` cannot be assumed to be informed consent for
  the new text; treat it as not consented and ask again.
- The backend has one migration and no production database. Adding competition
  models now is cheap.
- Uploads carry a map-data version and a score-calculation version so that the
  server can accept aggregates from supported older versions without strict
  normalisation.

## Privacy and security implications

This is the highest-risk phase in the plan for privacy.

- The upload payload is a closed allow-list (spec §25.2). The backend must
  **reject** anything else at the schema level. Documentation is not a control.
- No raw GPS, no tracks, no exact location, no per-pixel timestamps, no live
  movement state — ever, in any build, under any flag.
- Delayed batching exists specifically so competition data cannot function as a
  live-location signal. A "sync now" affordance would defeat it.
- Sparse-area anonymity must be enforced server-side. If the server returns
  nicknames for an area with fewer than three participants, client-side hiding
  is not protection.
- Competition necessarily reveals that a user explored a named area. The opt-in
  text must say so honestly (spec §25.5).
- Nicknames are user-generated content shown to others, requiring filtering,
  reporting, and administrative reset.
- The friends feature currently ships in both repositories and is a V1
  non-goal (OQ-6). Decide whether public builds expose it before this phase
  ships a competition profile UI next to it.
- The client-trust model is deliberately weak: device-id auth, client-computed
  aggregates. V1 accepts this and does not build anti-cheat theatre. Rate
  limiting, schema validation, and clamping impossible percentages are the
  controls.

## Automated testing strategy

Client:

- Recency decay: the weight is approximately 1.0 immediately, 0.5 at 30 days,
  0.25 at 60, 0.125 at 90; revisiting restores it.
- Ownership score against the resolved formula, with fixture areas.
- Eligibility: each of the three conditions independently, plus the
  fewer-than-50-pixel area exception.
- Imported pixels never contribute to recency, ownership, eligibility, or
  weekly counts. This deserves several tests; it is the load-bearing rule.
- Upload cadence: no upload more than once per 15 minutes; jitter within range;
  no upload when opted out; offline queueing and later flush.
- Payload shape asserted against a deny list, not only an allow list.

Backend (needs a test setup that does not exist yet):

- Schema rejection of any location-shaped field.
- Decay applied correctly between uploads and replaced by a newer upload.
- Sparse-area responses omit nicknames below three participants.
- Weekly leaderboard excludes revisits and imports.
- Deletion removes the profile and its aggregates.
- Throttling.

## Manual validation strategy

- Complete the opt-in flow and confirm the explanation matches actual
  behaviour, item by item against spec §20.2.
- Capture network traffic during a recording session with competition enabled
  and confirm no request contains coordinates, and that cadence and jitter
  hold.
- Confirm no upload occurs at all with competition disabled.
- Go offline, explore, return online, and confirm queued aggregates upload and
  stale rankings were labelled as stale.
- With fewer than three participants in an area, confirm no nicknames appear.
- Become boss, go inactive, and confirm decay eventually removes eligibility
  without opening the app.
- Delete the competition profile and confirm both server-side removal and
  intact local exploration.
- Confirm no surface anywhere indicates another participant's location or
  presence.

## Entry criteria

- Phase 4 exit criteria met.
- OQ-1 answered and recorded as a decision.
- OQ-4 and OQ-7 answered.
- A deployable backend environment exists, with a production settings module
  and a database that is not SQLite.

## Exit criteria

1. Competition is off by default and requires active confirmation separate from
   location permission.
2. The consent record includes the privacy-policy version and a timestamp.
3. Pseudonymous identity and nickname creation work with no email or password.
4. Uploads contain only the spec §25.2 fields; the backend rejects anything
   else at the schema level.
5. Upload cadence is at most once per 15 minutes plus jitter, with offline
   queueing.
6. Ownership, eligibility, boss selection, contested and unclaimed states work.
7. Server-side decay works between uploads.
8. The weekly city leaderboard excludes revisits and imports and resets weekly.
9. Sparse-area anonymity is enforced server-side.
10. Nickname validation, filtering, reporting, administrative reset, and the
    seven-day rename limit work.
11. Profile and aggregate deletion works and leaves local exploration intact.
12. No surface reveals another user's live location, exact location, or
    presence.

## Explicit non-goals

- Global and country-level public leaderboards. Spec §6.
- Friends, groups, family leaderboards, messaging, and social feeds. Spec §6.
- Nearby-user discovery, live locations, and exact-location sharing. Spec §6,
  §25.4.
- Sophisticated server-side anti-cheat and attested statistics. Spec §6, §35.
- Cross-device profile recovery. Spec §20.4.
- Strict server-side map-version normalisation. Spec §27.5.
- Biographies, profile images, external links, and free-form status. Spec §21.5.
- Uploading anything to make competition "more accurate".

## Known uncertainties

- The scoring formulas (OQ-1). This blocks the whole phase.
- Nickname uniqueness (OQ-4): the spec allows duplicates, the backend enforces
  a unique `username`.
- Weekly reset when a city's time zone is unknown (OQ-3).
- Whether the friends feature stays in public builds (OQ-6).
- Where competition lives in the backend: extending `apis/` and `core/`, or a
  separate Django app.
- Whether the API base path should move to `/api/v1/` before the first
  production client ships.
- Whether per-pixel recency is stored sparsely or aggregated on the fly.
- What "clamp impossible percentages" means concretely as a server-side rule.
- Hosting region and retention policy (OQ-7), which affect the privacy policy
  text and therefore the consent version.
