# LIR-To-New-BIR Final Coverage Convergence

Status: Closed
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

## Closure

Closed after accepted runbook Steps 1-4.

Disposition: capability complete for this bounded convergence route. The
terminal disposition matrix records every valid current-LIR semantic family
known to 797 as an accepted terminal input or accepted no-change/evidence
disposition, including the accepted 734 Step 7.52 receipt commit `a680b50e8`
for closed 867's selected direct-local `LirVaStartOp` row. The Step 2 audit
found no matrix-proven dispatcher, verifier, or container gap authorized for
797 repair.

Proof record:
`docs/lir_to_new_bir_final_coverage_convergence/final_proof_record.md`.

Accepted final validation:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1
```

Result: passed, `100% tests passed, 0 tests failed out of 3038`.

Non-claims: closure does not absorb open 795, 796, 821, or 822; publish new
producer/type authority; invent receiver rows; or treat rendered text,
`monostate`, compatibility mirrors, printer output, diagnostics, or testcase
identity as semantic authority.
