# AMD64 `va_arg` Unselected Alloca Compatibility Regression

Status: Closed
Type: narrowly scoped regression blocker
Parent: `ideas/open/753_lir_memory_va_pointer_authority_convergence.md`
Regression source: closed 799 implementation `c4e820a48` (closed idea remains closed)

## Goal

Restore verifier-compatible typed result construction for the unselected AMD64
`va_arg` overflow temporary, without weakening or changing the selected
aggregate-overflow carrier authority accepted in closed 799.

## Why This Exists

753 Step 3's required full baseline regressed to 3036/3037 at
`frontend_lir_call_type_ref`. The test aborts with
`LirAllocaOp.result: expected operand kind mismatch for '%t18'; got raw-text`.
The only code commit after the last full-green 3037/3037 acceptance
(`52f143765`) is `c4e820a48`. In
`emit_amd64_va_arg_from_overflow`, its unselected compatibility branch creates
`LirOperand::raw(fresh_tmp(ctx))` and then builds `LirAllocaOp` with a typed
`LirTypeRef`; the verifier rejects that raw-text result operand.

## In Scope

- Repair the unselected AMD64 overflow `va_arg` temporary's operand/result
  representation so it satisfies the existing `LirAllocaOp` typed contract.
- Preserve the selected `fresh_value` construction, carrier publication,
  `requires_native_memory_va_authority` boundary, and verifier authority rules
  accepted in `c4e820a48`.
- Add nearby focused proof if needed, then require a fresh build plus
  `./build/tests/frontend/frontend_lir_call_type_ref_test` and a matching full
  baseline before returning control to 753.

## Out Of Scope

- Reopening or altering closed 799's selected carrier contract; verifier
  relaxation; selected-route authority changes; aggregate/vector
  generalization; other targets; Raw-BIR/MIR/emission work; and 753's receiver
  handoff.

## Acceptance Criteria

- The unselected overflow route no longer creates an alloca result that fails
  the operand-kind verifier contract.
- The selected aggregate overflow route retains its accepted native carrier
  selection, fields, rejects, and authority behavior.
- A fresh build plus `./build/tests/frontend/frontend_lir_call_type_ref_test`
  passes, followed by a matching 100%-passing full baseline accepted by the
  supervisor before 753 resumes Step 3.

## Reviewer Reject Signals

- Reject verifier weakening, a raw-text exception, or any contract downgrade
  that merely admits the previous malformed `LirAllocaOp` result.
- Reject changing selected carrier fields, selection authority, or closed 799
  scope while claiming an unselected compatibility repair.
- Reject reconstructed semantic authority from rendered text, builtin names,
  or testcase-shaped branching.
- Reject broad AMD64 ABI, aggregate/vector, or cross-target rewrites outside
  the one unselected temporary construction seam.
- Reject a named-case-only test adjustment that leaves the existing typed
  result contract violated on the compatibility route.

## Closure Record

Disposition: capability complete; resolved regression blocker closed after
Step 2 return gate.

- **Accepted implementation:** `18e67ea70` changes only the unselected AMD64
  overflow `va_arg` temporary from `LirOperand::raw(fresh_tmp(ctx))` to
  `fresh_value(ctx)`, restoring the typed `LirAllocaOp` result contract.
- **Preserved boundary:** selected `fresh_value` construction, destination and
  carrier publication, `requires_native_memory_va_authority`, and closed 799's
  selected aggregate-overflow authority are unchanged.
- **Accepted proof:** fresh `cmake --build --preset default` and
  `./build/tests/frontend/frontend_lir_call_type_ref_test` passed. The
  supervisor's full `ctest --test-dir build -j --output-on-failure` passed
  3037/3037; regression guard comparison with the accepted 3037/3037 baseline
  using `--allow-non-decreasing-passed` found 0 new failures.
- **Parent return:** 753 resumes at Step 3 only: document exactly one
  receiver-ready handoff with native fields, guarantees, rejected forms, and
  accepted proof. It does not authorize rerunning Steps 1/2 or any Raw-BIR
  receiver implementation.
