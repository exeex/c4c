# LIR Scalar LHS Parameter Authority Baseline Repair

Status: Closed
Type: baseline blocker repair for current-LIR scalar LHS parameter authority
Parent Source: ideas/open/860_lir_next_body_parameter_authority_handoff.md

## Goal

Repair the current-LIR scalar LHS parameter authority verifier/producer failure
family so the full-suite baseline no longer fails on valid nonselected RHS
forms.

## Why This Exists

Idea 860 is ready to select one next body-parameter-use row, but current `HEAD`
already fails the full-suite baseline before any 860 implementation can be
accepted. The failure reports:

`error: LirBinOp.scalar_lhs_parameter_authority: selected floating binary LHS
authority requires a nonselected scalar RHS`

This reopens an accepted DirectScalar row contract and is outside 860's
producer-side next-row selection scope.

## Baseline Evidence

- Fresh build at `HEAD` had no work to do.
- Fresh full suite failed with `99% tests passed, 12 tests failed out of 3038`.
- Failed tests:
  `cpp_positive_sema_specialization_identity_cpp`,
  `cpp_llvm_spec_key_metadata`, `cpp_llvm_spec_key_named_metadata`,
  `cpp_llvm_spec_key_named_metadata_entry`,
  `llvm_gcc_c_torture_src_20020314_1_c`,
  `llvm_gcc_c_torture_src_921208_1_c`,
  `llvm_gcc_c_torture_src_990127_2_c`,
  `llvm_gcc_c_torture_src_990829_1_c`,
  `llvm_gcc_c_torture_src_cbrt_c`,
  `llvm_gcc_c_torture_src_conversion_c`,
  `llvm_gcc_c_torture_src_gofast_c`, and
  `llvm_gcc_c_torture_src_ieee_unsafe_fp_assoc_1_c`.
- `test_baseline.log` is green at `380ee782...`.
- `test_baseline.new.log` failed at historical commit `a23031c8...` with the
  same family plus two extra failures.
- Supervisor rejected the baseline candidate through
  `scripts/plan_review_state.py reject-baseline`.

## In Scope

- Diagnose the current-LIR producer and verifier paths for
  `LirBinOp.scalar_lhs_parameter_authority`.
- Preserve the accepted DirectScalar authority contract for selected floating
  binary LHS parameter authority.
- Accept valid nonselected scalar RHS forms when the selected LHS authority is
  coherent.
- Add nearby focused positive and malformed coverage for the repaired
  verifier/producer behavior.
- Run supervisor-selected focused proof and a fresh full-suite baseline proof
  sufficient for 860 to resume.

## Out Of Scope

- Weakening accepted DirectScalar authority contracts.
- Expectation downgrades, supported-to-unsupported reclassification, or
  testcase-shaped exceptions.
- Raw-BIR receiver, importer, container, view, or backend receiver work unless
  investigation explicitly proves it is necessary; if so, route that work as a
  separately scoped idea before editing those surfaces.
- Selecting or publishing any new 860 body-parameter-use row.
- Broad LIR schema rewrites, ABI redesign, target-lowering changes, or
  unrelated body-parameter authority families.

## Acceptance Criteria

- The exact old failure family no longer appears for valid nonselected scalar
  RHS forms with selected floating binary LHS authority.
- The DirectScalar selected LHS authority contract remains fail-closed for
  absent, foreign, owner/index/type/ABI/role-incoherent, and consumer-incoherent
  authority.
- Focused tests cover both the valid nonselected RHS form and nearby malformed
  scalar LHS authority cases.
- Fresh build passes.
- Supervisor-selected focused proof passes.
- Fresh full-suite baseline passes or is accepted by the supervisor as no
  worse than the current canonical baseline.
- `git diff --check` passes.

## Closure

Disposition: capability complete.

Implementation commit: `df0e29445` (`Repair scalar parameter authority
publication`).

Accepted evidence:

- Focused proof passed:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'; } > test_after.log 2>&1`.
- Full-suite guard passed: pre-patch `test_before.log` had `3026/3038`, and
  post-patch `test_after.log` had `3038/3038` with no new failures.
- Hook baseline candidate at `df0e29445` was green:
  `100% tests passed, 0 tests failed out of 3038`.
- Supervisor accepted the baseline with
  `scripts/plan_review_state.py accept-baseline`.

Completion notes:

- The old scalar LHS parameter authority failure family no longer appears for
  valid nonselected scalar RHS forms with selected floating binary LHS
  authority.
- DirectScalar selected LHS authority remains fail-closed for malformed and
  incoherent authority.
- Focused tests cover valid nonselected RHS, unsupported suppression, duplicate
  selected rows, and malformed explicit selected RHS authority.
- Parent return point is preserved in
  `ideas/open/860_lir_next_body_parameter_authority_handoff.md`: resume at
  Step 1, `Select and publish one next body-parameter authority row`.

## Reviewer Reject Signals

- Reject any fix that downgrades expectations, changes supported tests to
  unsupported, deletes the failing coverage, or weakens the accepted
  DirectScalar verifier contract.
- Reject named-test, named-commit, rendered-text, diagnostic-text, or
  testcase-shaped shortcuts that special-case the listed failures without
  repairing the scalar LHS authority model.
- Reject classification-only edits, helper renames, or message rewrites claimed
  as baseline repair while the old failure family remains possible.
- Reject Raw-BIR receiver/importer/container/backend changes unless the
  blocker explicitly proves they are required and opens a separate scoped idea
  for that work.
- Reject selecting or implementing a new 860 body-parameter row before the
  baseline blocker is closed.
- Reject broad LIR schema, ABI, target-lowering, or unrelated authority-family
  rewrites not needed to repair the scalar LHS verifier/producer failure.
