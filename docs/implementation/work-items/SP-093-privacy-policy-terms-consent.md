# SP-093 — Privacy policy, terms, and competition consent alignment

**Phase:** 10 — Android release hardening
**Status:** Residual
**Depends on:** SP-088 H4 Accepted (**SPD-080**). Landing text/URLs is
  residual; this item is **not** a Phase 10 coding item.
**Unblocks:** none in this slice (SP-097 exit #5 privacy-policy match
  remains residual until a later WI lands the text)
**Notes:** Brand residual. Product-owner lock 2026-08-29 accepted the
  *intended* H4 position (product-owned Street Pixels policy) but
  **landing the actual policy/terms/URLs is residual**. Do not
  implement in SP-089–097.

---

## Objective

Make the privacy policy, terms, competition rules, and in-app
competition consent text match actual behaviour item by item: what
stays local, what is uploaded, delay/jitter, sparse-area anonymity,
deletion, nickname uniqueness (SPD-059), and that friends / live
location / nearby discovery are absent.

**This work is residual.** Do not produce, host, or retarget policy
or terms in this Phase 10 coding slice. `https://comaps.app/privacy/`
and `terms/` may stay for now (**SPD-080**).

---

## Motivation

Help currently opens `https://comaps.app/privacy/` and `terms/`.
Those pages describe CoMaps, not Street Pixels competition aggregates
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
2. In-app URLs resolve to H4 text, not unmodified CoMaps pages
   (unless a later SPD explicitly keeps CoMaps pages after they are
   updated).
3. Policy version bump recorded if meaning changed.
4. Agent does not mark Accepted.

## Required automated tests

- None while residual.

## Required manual validation

- None while residual. Device traffic capture that opt-out uploads
  nothing is SP-095 (**residual**).

## Failure and rollback considerations

- Do not ship a rewritten CoMaps privacy URL as a stand-in while
  this item is residual; leaving `comaps.app` is the recorded
  residual posture (**SPD-080**).

## Completion evidence

| Field | Value |
| --- | --- |
| Branch | |
| Policy / terms URLs | Residual — `https://comaps.app/privacy/` may stay for now |
| Consent checklist | Residual |
| Policy version | |
| Accepted by | |
| Accepted date | |

## Discovered follow-up

| Finding | Proposed disposition |
| --- | --- |
| Landing Street Pixels policy/terms/URLs | Residual (this item); not SP-089–097 coding |
