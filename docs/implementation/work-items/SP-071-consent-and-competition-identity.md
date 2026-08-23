# SP-071 — Consent record and competition identity

**Phase:** 8 — Competition
**Status:** In progress
**Branch:** `cursor/sp-071-consent-identity-c990`
**Depends on:** SP-070 locks (SPD-059, SPD-061, SPD-062, SPD-064)
**Unblocks:** SP-074 (upload gated on consent), SP-075 (register), SP-078
  (identity UI)

---

## Objective

Replace the boolean explore-consent flag with a competition consent record
(privacy-policy version + timestamp), introduce a competition profile with
a unique public nickname, and hide friends from public Android V1.

## Motivation

Spec §20.1–§20.6 and §21.1–§21.4. Today `Explore.ConsentGiven` is a
boolean, `Explore.Username` is ASCII 3–20, and `MyAccountDialogFragment`
is friends-first. An existing `true` is not informed consent for the
competition explanation (SPD-064). Friends are a V1 non-goal (SPD-061).
Nicknames are unique in V1 (SPD-059).

## In-scope behavior

- Consent record: competition enabled, aggregate sharing enabled, current
  privacy-policy version, consent timestamp. Separate from location
  permission.
- Treat existing `Explore.ConsentGiven == true` as **not** consented.
  Re-prompt. Do not silently upgrade the boolean.
- Nickname format per spec §21.1 (3–24 visible characters, Unicode,
  spaces, hyphens, limited punctuation, normalisation). Replace
  `IdentityStore::IsValidUsername` ASCII `[a-z0-9_]{3,20}`.
- Suggest a generated nickname; user may edit. No email or password.
- Uniqueness is a **server** rule (SPD-059). Client treats 409 as
  collision, retries generated names, and does not invent suffixes.
  Offline: keep the local draft; do not upload until a unique name is
  accepted. Seven-day rename limit locally; server is source of truth in
  SP-075 / SP-077.
- Hide friends UI and deep links from public Android V1 (SPD-061). Do not
  call friends endpoints from public builds. Do not delete friends code
  unless it is the only way to hide the surface.
- Device id remains the existing non-hardware SecureStorage id.
- Opt-in explanation must match spec §20.2 item by item (aggregates,
  nickname when enough participants, no routes, no raw GPS, no live
  location, no nearby discovery, can leave later).

## Out-of-scope behavior

- Upload cadence and payload (SP-074).
- Backend register / unique constraint implementation (SP-075).
- Nickname report / admin reset / deletion (SP-077).
- Competition map chrome, §22.10 card copy, 30-pixel hint (SP-078).
- Recency store (SP-072).
- Cross-device profile recovery (§20.3).

## Relevant product requirements

- Spec §20, §21.1–§21.4, §6 friends non-goal.
- SPD-059, SPD-061, SPD-064; SPD-014 (no GPS in identity).

## Relevant source files or symbols

- `libs/map/identity_store.{hpp,cpp}`
- `libs/map/friends_manager.{hpp,cpp}`
- Android `MyAccountDialogFragment`, `ExploreConsentDialogFragment`,
  `Friends.java`, `item_friend_row.xml`, add-friend manifest deep links
- `Framework.nativeHasExploreConsent` / `nativeSetExploreConsent`

## Implementation notes / constraints

- Shared C++ consent + nickname store (SPD-002). Android UI only.
- Public builds: friends off. Do not add a secret friends toggle that
  ships on.
- Do not upload GPS, tracks, or device advertising ids.

## Acceptance criteria

1. Consent record includes policy version and timestamp; boolean is not
   treated as consent.
2. Nickname validation matches §21.1; uniqueness collisions are handled
   as 409 without numeric suffixes.
3. Public Android V1 has no friends UI and makes no friends API calls.
4. Opt-in copy covers spec §20.2 items.
5. Automated tests cover consent record, invalid nicknames, collision
   handling, and friends-hidden public path.

## Required automated tests

- Boolean `true` is not consented; new record required.
- Reject nicknames outside §21.1; accept Unicode letters and allowed
  punctuation in range.
- Simulated 409 → client does not keep a colliding name.
- Friends refresh is a no-op when public-V1 friends are hidden.
- Consent off → no competition identity upload attempt (hook used by
  SP-074).

## Required manual validation

- Device residual → SP-079 / Phase 10. Desktop tests are the gate.

## Failure and rollback considerations

- Prefer fail-closed (not in competition) over assuming old boolean
  consent.
- Do not leave friends UI visible next to the new profile.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Test output | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| | |
