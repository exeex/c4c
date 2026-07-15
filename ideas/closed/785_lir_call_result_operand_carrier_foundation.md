# LIR Call-Result Operand Carrier Foundation

Status: Closed
Type: bounded frontend-LIR generic call-result prerequisite
Blocked Parent: `ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md`

## Goal

Establish the smallest current-function native result carrier contract for
`make_lir_call_op` and its directly required call-result support, so structured
callers retain native call results rather than only result strings.

## Why This Exists

784's FP128 alignment route calls `make_lir_call_op` for ptrmask. The factory's
string result erases the native `LirOperand`/`LirValueId` before
`aligned_stack_ptr` reaches the required PHI input. The generic factory is the
first unresolvable carrier-loss boundary; assigning an ID later would fabricate
authority, and this is outside 784's vaarg-only foundation scope.

## In Scope

- Inventory `make_lir_call_op` and directly required call-result support to
  determine the smallest current-function native result carrier boundary.
- Establish focused frontend-LIR structural-probe feasibility for a structured
  caller that must retain a native call result across that factory boundary.
- Implement and prove only the minimal result carrier contract needed for
  `make_lir_call_op` callers to preserve native call-result authority beside
  compatibility spelling.
- Add focused structural and applicable fail-closed coverage for the direct
  carrier contract.
- Publish a handoff allowing 784 to retry Step 2, then separately retain its
  value-only PHI transport decision.

## Out of Scope

- Broad generic-expression redesign or any unrelated generic call/argument
  family; if direct support cannot carry `LirOperand` without reaching those
  families, name that exact narrower successor instead of expanding this idea.
- PHI incoming transport or verification, predecessor/edge identity,
  Raw-BIR/importer, backend, target lowering, MIR, emission, and vaarg-specific
  helper work.
- Textual recovery from names, labels, rendered output, instruction order,
  side tables, or result-name maps.

## Acceptance Criteria

- Source-level evidence identifies the direct `make_lir_call_op` result and
  supporting type boundary required to retain a current-function native result.
- Focused frontend-LIR structural coverage proves a structured caller retains
  native result authority through the factory, with compatibility spelling not
  used as identity.
- The selected support type is demonstrably limited to direct call-result
  transport; any need to redesign generic call/argument families is reported as
  an explicit successor.
- 784 can retry Step 2 with the generic call-result carrier available, while
  its separate value-only PHI transport decision remains unchanged.
- The accepted focused proof passes 1/1:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.

## Reviewer Reject Signals

- Reject broad generic-expression or generic call/argument redesign presented
  as direct call-result carrier progress.
- Reject PHI incoming transport/verification, predecessor/edge identity,
  Raw-BIR/importer, backend, target lowering, MIR, emission, or vaarg helper
  changes folded into this foundation.
- Reject result strings, names, labels, printed text, instruction order, side
  tables, result-name maps, or testcase-shaped branches as native authority.
- Reject helper renames, weaker expectations, or an abstraction that still
  discards the native call result at `make_lir_call_op`.

## Closure Decision

Close accepted. Commit `3b53451c0` uses the existing direct native-result
factory support at the AArch64 FP/alignment ptrmask call in
`hir_to_lir/call/vaarg.cpp:145`: `fresh_value(ctx)` and
`make_lir_call_op_with_return_type_ref` retain the call result through its
immediate typed GEP consumer. The AArch64 long-double vaarg structural probe
verifies the native result ID and pointer return type without rendered text.

This satisfies the direct carrier boundary and focused evidence criteria
without generic API expansion, generic-expression redesign, unrelated generic
call/argument changes, PHI work, or conversion of the sibling HFA ptrmask
route. The exact focused command passed 1/1:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_lir_call_type_ref$'`. Regression guard
`check_monotonic_regression.py --before test_before.log --after test_after.log
--allow-non-decreasing-passed` passed with before/after 1 passed, 0 failed, and
no new failures or timeouts.

## Handoff

Resume `ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md` at Step
2, `Bind the minimal generic carrier contract`. The direct FP128 ptrmask call
result is now available; retain 784's separately scoped decision on value-only
PHI incoming transport. Do not treat this closure as coverage for HFA ptrmask
or unrelated generic call families.
