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
| Branch | backend `cursor/sp-077-nickname-moderation-deletion-f95c` (`Befeepilf/explorer`) at `a2875770bc72b68917b58356d17adfb39af2ea10`; client `cursor/sp-077-nickname-moderation-deletion-f95c` (`Befeepilf/comaps`) at `53c6118c5768a0d0242042aa8fdb78d7dc01f7d3` |
| Test output | See executed output below. Not Accepted. |
| Accepted by | |
| Accepted date | |

## Executed test output

Explorer (`cd /home/ubuntu/explorer-src/explorer && uv run pytest -q`):

```
........................................................................ [ 66%]
....................................                                     [100%]
108 passed, 4 warnings in 1.06s
```

Client (`./tools/unix/build_omim.sh -d -p "$HOME" street_pixels_tests` then `/home/ubuntu/omim-build-debug/street_pixels_tests --data_path=/workspace/data --user_resource_path=/workspace/data --filter='IdentityStore_|BackendConfig_Competition|CompetitionDeletion_'`):

```
All tests passed.
```

49 tests ran (19 BackendConfig_Competition, 2 CompetitionDeletion, 28 IdentityStore). Full logs: `/opt/cursor/artifacts/sp077_review_explorer_pytest.log`, `/opt/cursor/artifacts/sp077_review_street_pixels_tests.log`.

Independent review fixed client `ReportNickname` (blocked impersonation names must still POST) and tightened backend tests for leave extras plus the post-reset rename clock. Not Accepted.

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Client `ReportNickname` reused `IsValidNickname`, so blocked impersonation names could not be reported | Fixed in review: report validates length/reason only |
| V1 blocked list is a small whole-token set, not a complete §21.2 taxonomy | Residual; expand later |
| Temporary/permanent competition suspension for repeated abuse (§21.3) | Not SP-077 |
| Client still locally 7-day-gates after admin reset; server allows immediate POST | SP-078: when a snapshot nickname differs from local, adopt it without writing `Explore.NicknameLastChangedUnix` |
| HTTP 409 `rename_limited` currently mapped to `Collision` if it ever reaches the client | Residual; local gate should hide it |
| Failed `POST /leave` after local revoke has no retry queue | Follow-up retry |
| `TryClaimNickname` does not parse conflict `type` | Residual |
| Report expiry purge has no cron | Ops; command `competition_purge_silent` exists |
| Exact EU region string for privacy policy | SPD-062 ops lock |
| Unicode script-table vs category gaps between client and server | Residual; server remains authority (SP-075) |
