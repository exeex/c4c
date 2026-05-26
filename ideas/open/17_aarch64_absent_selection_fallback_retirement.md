# AArch64 Absent-Selection Fallback Retirement

## Goal

Retire the remaining temporary AArch64 absent-selection fallback paths now that
prepared call/source facts should be complete enough for emission.

## Why This Exists

`07_aarch64_call_boundary_move_emission_only.md` closed with one explicit
follow-up debt: retire documented temporary absent-selection fallbacks once
prepared facts are complete for local aggregate address publication and
indirect byval lane payloads.

Those fallbacks are legacy safety paths. They are useful while prepared facts
are incomplete, but after the prepared records become authoritative they make
AArch64 lowering larger and blur the contract: emission can still appear to
work by rederiving sources locally instead of failing closed on missing shared
facts.

This should be a small cleanup idea, separate from the larger BIR edge-flow
authority migration.

## In Scope

- Audit documented absent-selection fallback paths related to:
  - local aggregate address publication
  - indirect byval lane payload publication
- Confirm whether each fallback is still reachable with current prepared facts.
- Replace reachable fallback use with consumption of complete prepared facts.
- If a prepared fact is still missing, add the minimal shared prepared field or
  query needed to make the path explicit.
- Remove or narrow tests that only protect the legacy fallback path, while
  preserving coverage for the semantic behavior.
- Ensure missing prepared facts fail closed with diagnostics instead of broad
  target-local source reconstruction.

## Out Of Scope

- Broad BIR edge value-flow migration.
- Dispatch edge-copy/publication authority cleanup beyond the named fallback
  paths.
- Calls file consolidation.
- New ABI classification work.
- Adding another temporary fallback under a different helper name.

## Acceptance Criteria

- The documented absent-selection fallbacks from idea 07 are either removed or
  proven unreachable and guarded by explicit diagnostics.
- AArch64 call-boundary and publication lowering uses prepared facts as the
  source of truth for the affected paths.
- Existing local aggregate address and indirect byval behavior remains covered
  by focused tests.
- Backend validation shows no regression.
- The final close note maps each retired legacy path to its replacement
  prepared fact or to a precise unreachable condition.

## Reviewer Reject Signals

- A patch keeps broad value-home or producer rederivation as the real source
  selection path.
- A patch only renames the fallback while preserving the same behavior.
- A patch weakens incomplete-selection coverage instead of making missing facts
  visible.
- A patch special-cases one failing testcase instead of retiring the fallback
  contract generally.
- A patch expands the scope into unrelated dispatch or ABI cleanup.
