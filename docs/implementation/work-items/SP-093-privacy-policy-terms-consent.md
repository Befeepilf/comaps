# SP-093 — Privacy policy, terms, and competition consent alignment

**Phase:** 10 — Android release hardening
**Status:** Planned
**Depends on:** SP-088 H4 Accepted (SPD-080). SP-092 listing inventory
  useful but not a hard gate. Phase 10 implementation entry.
**Unblocks:** SP-097 exit #5

---

## Objective

Make the privacy policy, terms, competition rules, and in-app
competition consent text match actual behaviour item by item: what
stays local, what is uploaded, delay/jitter, sparse-area anonymity,
deletion, nickname uniqueness (SPD-059), and that friends / live
location / nearby discovery are absent.

---

## Motivation

Help currently opens `https://comaps.app/privacy/` and `terms/`.
Those pages describe CoMaps, not Street Pixels competition aggregates
or session GPS. Spec §34 requires the policy to describe local vs
uploaded data, consent text to match behaviour, and terms to cover
public nicknames and rankings.

`explore_consent_message` is already closer to spec §20.2 than the
web policy. The web policy is the gap. Consent re-prompt is SPD-064.

---

## In-scope behavior

- Produce or link the H4-owned policy and terms. If the canonical
  text lives outside git, check in a dated snapshot under
  `docs/implementation/` that the in-app URLs must match.
- Point Help / settings rows at those URLs (coordinate with SP-090
  settings rows).
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

- Inventing a new upload field to make the policy “simpler”.
- Server-side anti-cheat theater (audit: accept residual cheating).
- Legal review as a substitute for the checklist; legal may add
  follow-up but this item’s gate is behavioural match.

## Relevant product requirements

- Spec §3.2, §20.2, §20.5, §21, §25, §26, §30, §34 Release
  governance.
- SPD-014, SPD-059, SPD-061, SPD-062, SPD-064, draft SPD-080.

## Relevant source files or symbols

- `HelpFragment` URL construction
- `explore_consent_message` and related strings
- `IdentityStore::kCompetitionPrivacyPolicyVersion`
- `CompetitionUploadService` payload
- Backend competition schema (explorer checkout if present)

## Implementation notes / constraints

- Do not edit `docs/STREET_PIXELS_PRODUCT_SPEC.md`.
- English first. Translations of legal text need a human owner
  (H4); do not machine-translate policies in this item.
- Consent must remain separate from location permission.

## Acceptance criteria

1. Checklist: every consent bullet is true of the binary + backend.
2. In-app URLs resolve to H4 text, not unmodified CoMaps pages
   (unless H4 explicitly keeps CoMaps pages after they are updated).
3. Policy version bump recorded if meaning changed.
4. Agent does not mark Accepted.

## Required automated tests

- Existing consent / upload allow-list tests remain green.
- If version string changes, tests that pin the old version are
  updated to expect re-prompt, not deleted.

## Required manual validation

- Open policy and terms from the app on a debug build; confirm
  destination. Device traffic capture that opt-out uploads nothing
  is SP-095.

## Failure and rollback considerations

- If legal text is not ready, this item stays Blocked; do not
  ship CoMaps privacy URL as a stand-in.

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Policy / terms URLs | |
| Consent checklist | |
| Policy version | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| (fill during implementation) | |
