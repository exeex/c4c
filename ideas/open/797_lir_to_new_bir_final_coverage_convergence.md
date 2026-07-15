# LIR-To-New-BIR Final Coverage Convergence

Status: Open
Type: bounded dispatcher/proof and documentation convergence
Predecessor: all accepted producer/type-model handoffs and their matching 734 receiver receipts from the 793 successor queue
Parent: `ideas/closed/793_lir_to_new_bir_remaining_coverage_umbrella.md`

## Goal

After every valid current-LIR semantic family has an accepted typed disposition,
prove no-omission coverage, dispatcher completeness, whole-module transactional
behavior, and documentation convergence.

## In Scope

- Build the explicit per-row authority/destination/importer/verifier/proof
  matrix from accepted prior work.
- Close dispatcher/proof gaps with neighboring positive/negative and
  whole-module transactional evidence, then align documentation to code.

## Out Of Scope

- Publishing missing producer/type authority, inventing receiver rows, broad
  feature implementation, expectation downgrades, or treating a green subset
  as evidence of omitted families.

## Acceptance Criteria

- Every valid current-LIR fact has an explicit typed disposition; the
  dispatcher and reachable verifier are complete for those dispositions.
- Whole-module transactional proof and source-wide focused/broader acceptance
  are recorded, and documentation agrees with the accepted code route.

## Reviewer Reject Signals

- Reject a matrix that silently omits a family, labels text/monostate as typed
  authority, or claims completion before prerequisite handoffs and receipts.
- Reject test filtering, unsupported downgrades, verifier weakening, or a
  documentation-only claim of implementation convergence.
