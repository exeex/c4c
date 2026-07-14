# Production LIR Computed-Goto Address Value Publication

Status: Open (active blocker for `ideas/open/734_lir_to_new_bir_container_completeness.md`)
Type: bounded production LIR computed-goto authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`, after accepted Step 7.24

## Goal

Make the active production computed-goto lowering publish a verifier-valid,
current-function pointer `LirValueId` in `LirIndirectBrOp.addr_value`, so the
already accepted Raw-BIR receiver can import its typed address without a text
fallback.

## Why This Exists

The bounded receiver in 734 Step 7.24 is accepted, but the fresh full suite
reproduces `llvm_gcc_c_torture_src_comp_goto_1_c` failing before Raw-BIR import:
`LirIndirectBrOp.addr_value: must carry current-function pointer LirValueId`.
The failure is production LIR from `tests/c/external/gcc_torture/src/comp-goto-1.c`,
not the receiver.

Closed idea 757 introduced the field, verifier, and a focused HIR lowering
handoff. Its `StmtEmitter::emit_control_flow_stmt(IndirBrStmt)` publication
copies `emit_rval_operand(...).value_id()`. The production route reaches that
owner with no ID, so the 757 fixture contract did not prove this distinct
producer route. This blocker starts at that first actual authority-loss owner;
it does not reopen 757 or the 734 receiver.

## In Scope

- reproduce and trace the production `comp-goto-1.c` lowering only far enough
  to identify why the `IndirBrStmt` target operand has no current-function
  `LirValueId`
- make that active producer path publish the existing pointer value identity
  through `LirIndirectBrOp.addr_value` without parsing operand, label, printer,
  LLVM, or testcase text
- retain and exercise existing verifier requirements for presence, validity,
  current-function ownership, pointer suitability, and display-mirror
  agreement
- add focused production-path coverage and a precise handoff proving valid
  `addr_value` reaches the computed-goto carrier for `comp-goto-1.c`

## Out Of Scope

- Raw-BIR containers, importer, receiver validation, or re-execution of 734
  Step 7.24
- recovering authority from display strings, labels, rendered output, or
  testcase identity
- successor/CFG publication already completed by 750, or a broad rewrite of
  HIR, parser, sema, rvalue, pointer, local/object, memory/va, PHI,
  aggregate/vector, parameter, call, target-lowering, MIR, or emission families
- ownership claims for the four other full-suite failures

## Acceptance Criteria

- `ctest --test-dir build -V -R '^llvm_gcc_c_torture_src_comp_goto_1_c$'`
  no longer fails because `LirIndirectBrOp.addr_value` is absent.
- The production carrier has a valid current-function pointer `LirValueId` and
  the existing verifier rejects missing, invalid, foreign, non-pointer, or
  display-mismatched authority without text fallback.
- Focused coverage proves the actual production computed-goto route, not only
  a hand-built LIR fixture, publishes the authority and preserves fail-closed
  malformed cases.
- The accepted handoff names the producer seam, typed field, proof, and exact
  return action: resume 734 after Step 7.24 for plan-owner disposition; do not
  re-execute the receiver packet.

## Reviewer Reject Signals

- Reject parsing `addr`, labels, rendered LLVM/printer output, or testcase text
  to derive or repair the value ID.
- Reject a named-case-only workaround, expectation downgrade, verifier
  relaxation, or a change that leaves the production carrier without the same
  typed current-function pointer authority.
- Reject Raw-BIR/importer changes, receiver rework, or changes that claim
  ownership of the other four full-suite failures.
- Reject broad rvalue, CFG, PHI, local/object, memory/va, aggregate/vector, or
  target-lowering rewrites instead of a minimal repair at the first evidenced
  production authority-loss owner.
