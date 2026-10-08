# SP-105 — SPA stamp mismatch: rings plus error badge

**Phase:** 5 residual / map interaction
**Status:** In review
**Depends on:** SPD-098; SP-034; SP-035; SP-036; SP-046
**Unblocks:** none

---

## Objective

When a parseable `.spa` stamp does not match the installed MWM, still draw
area rings. If the pixel universe and live policy still line up, keep
percentages. If they do not, hide % / competition and show a short badge
error that opens a details sheet with what to do.

## In-scope behavior

1. Stamp-optional resolver load when universe size + policy match (**SPD-098**).
2. Overlay rings from a parseable sidecar even on stamp mismatch.
3. `.spx` / ACC / competition only when universe-compatible.
4. `FocusedAreaProgress.m_sidecarIncompatible` + Android error badge / sheet.
5. Missing or corrupt sidecar stays the existing empty no-area state.

## Out-of-scope behavior

- Rewriting CDN or device SPA headers.
- Channel A pipeline pin.
- iOS.
- Product spec / technical audit edits.
- Marking this work item Accepted.

## Acceptance criteria

1. Stamp mismatch + matching universe and policy → overlay + valid fraction;
   `sidecarIncompatible` false.
2. Stamp mismatch + different universe size → overlay, no `.spx` write,
   `sidecarIncompatible` true, fraction invalid.
3. Missing sidecar → `noExplorationArea`, not `sidecarIncompatible`.
4. `TryLoad` with a wrong stamp still fail-closes; matching-universe helper
   succeeds.
5. Work item Status **In review**. Not Accepted.

## Test plan

| Case | Expect |
| --- | --- |
| Resolver matching-universe helper, stamp differs | load ok |
| Existing `TryLoad` wrong stamp | still nullopt |
| Rebuild + focus, stamp mismatch, same universe | fraction valid |
| Rebuild + focus, stamp mismatch, different universe | incompatible, no spx |
| Missing spa focus | noExplorationArea |
