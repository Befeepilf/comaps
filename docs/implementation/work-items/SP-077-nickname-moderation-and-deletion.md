# SP-077 — Nickname moderation and profile deletion

**Phase:** 8 — Competition
**Status:** In progress
**Branch:** backend `cursor/sp-077-nickname-moderation-deletion-f95c` (`Befeepilf/explorer`); client `cursor/sp-077-nickname-moderation-deletion-f95c` (`Befeepilf/comaps`)
**Depends on:** SP-071 identity; SP-075 profiles
**Unblocks:** SP-079 deletion / rename checks
**Repositories:** `comaps` (client) and `comaps_backend`

---

## Objective

Enforce nickname filtering, reporting, administrative reset, the
seven-day rename limit, and deletion of the public profile plus
aggregates without touching local exploration.

## Motivation

Spec §21.2–§21.4, §20.6, §25.6. Nicknames are user-generated content
shown to others. Leaving competition must offer keep-stats vs delete.

## In-scope behavior

- Client + server validation of §21.1 format (server is authoritative).
- Basic blocked-term filter; reject prohibited content (§21.2).
- Nickname report endpoint; store reports 12 months (SPD-062).
- Admin reset/removal that does **not** consume the seven-day rename
  budget.
- Rename at most once per seven days; 409 on unique collision (SPD-059).
- Leave competition: (a) stop uploads, keep public stats; (b) delete
  profile + uploaded aggregates. Local `.pix` / recency / personal %
  unchanged.
- Account export if spec §20 / backend already has `/account/export` —
  competition aggregates included or documented as in-scope here.
- Throttles on rename, report, delete.

## Out-of-scope behavior

- Biographies, avatars, links (§21.5).
- Cross-device recovery (§20.4).
- Friends deletion (friends hidden, SPD-061).

## Relevant product requirements

- Spec §20.6, §21, §25.6.
- SPD-059, SPD-061, SPD-062.

## Relevant source files or symbols

- Client: `IdentityStore`, settings competition UI (SP-071 / SP-078)
- Backend: `competition` app; existing `DELETE /account`

## Implementation notes / constraints

- Deletion is hard-delete of public competition data, not a soft hide
  that still returns nicknames.
- Local exploration must survive even if deletion HTTP fails; retry
  deletion rather than wiping `.pix`.

## Acceptance criteria

1. Rename interval and uniqueness hold; admin reset does not start the
   interval.
2. Reports persist; blocked terms rejected.
3. Delete removes server profile + aggregates; local exploration
   remains.
4. Leave-with-retain stops uploads but keeps last public aggregates
   until retention/decay rules.

## Required automated tests

- Rename too soon rejected; after seven days allowed.
- Collision 409.
- Admin reset then immediate user rename allowed.
- Delete then GET snapshot omits the profile.
- Client: delete success does not clear `.pix` / recency.

## Required manual validation

- SP-079: delete on device; map still shows personal green.

## Failure and rollback considerations

- Prefer keeping local data on failed delete request.
- Do not implement “delete my streets from the map” as competition
  leave.

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
