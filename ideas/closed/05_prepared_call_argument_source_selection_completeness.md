# Prepared Call Argument Source Selection Completeness

## Goal

Make `PreparedCallArgumentSourceSelection` carry enough structured facts for
call argument emission without target-local fallback rederivation.

## Why This Exists

The `04_aarch64_prior_preservation_baseline_drift.md` regression showed that a
selection can name a semantic source choice without carrying all facts needed
by AArch64 emission. In particular, stack-slot prior preservation has explicit
selection fields, while callee-saved prior preservation still requires a lookup
back into preserved-value records. That asymmetry makes fallback retirement
fragile.

## In Scope

- Audit all `PreparedCallArgumentSourceSelectionKind` variants.
- Add missing fields or structured subrecords needed for emission.
- Ensure prior-preservation selections can represent stack-slot and
  callee-saved-register sources completely.
- Update prepared printers and diagnostics so missing fields are visible.
- Add focused tests for complete and intentionally incomplete selections.

## Out Of Scope

- Broad AArch64 calls file consolidation.
- Dispatch cleanup.
- Rewriting call ABI classification.
- Restoring target-local source reconstruction as the primary path.

## Acceptance Criteria

- Each selection kind documents and enforces its required fact set.
- AArch64 call argument emission can consume complete selections without
  searching unrelated prior homes.
- Incomplete selections fail closed with diagnostics or no selection, not by
  silently rederiving broad fallback sources.
- Focused tests cover stack-slot and callee-saved prior preservation.

## Reviewer Reject Signals

- A patch adds another target-local fallback instead of completing the prepared
  selection facts.
- A patch special-cases one c_testsuite failure rather than fixing the
  selection contract.
- A patch weakens or removes incomplete-selection tests.
- A patch makes shared prepared records depend on AArch64-only register names
  without an ABI-neutral representation or target-policy boundary.

## Closure Notes

Closed on 2026-05-25. The active runbook completed the selection audit,
completed stack-slot and callee-saved prior-preservation source facts, updated
AArch64 emission to consume the completed payloads, exposed the new facts
through prepared printing, and added focused coverage for complete and
intentionally incomplete prior-preservation selections.

Close validation used the final full-suite `test_after.log`, which reported
3410/3410 tests passed. For the close gate, `test_before.log` was normalized
from the accepted full-suite `test_baseline.log`, and
`c4c-regression-guard` passed with no new failures and no pass-count decrease.
