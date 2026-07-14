# Production Computed-Goto Address Authority Runbook

Status: Active
Source Idea: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Activated from: switched from 734 after accepted Step 7.24; 734 remains open
and resumable.

## Purpose

Repair only the production computed-goto producer route that fails to publish
the existing typed address authority required by the accepted Raw-BIR receiver.

## Goal

For `comp-goto-1.c`, publish a verified current-function pointer `LirValueId`
in `LirIndirectBrOp.addr_value` without any text-derived recovery.

## Core Rule

`addr_value` is the semantic address authority. `addr`, labels, printer output,
rendered LLVM, and testcase names are never inputs for deriving or repairing it.

## Read First

- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption record
- `ideas/closed/757_lir_computed_goto_address_value_identity_publication.md`
- `src/codegen/lir/hir_to_lir/stmt.cpp` and the production rvalue path reached
  by `IndirBrStmt`

## Non-Goals

- no Raw-BIR/importer receiver changes or rerun of 734 Step 7.24
- no display-text recovery, successor/CFG redesign, or ownership of unrelated
  full-suite failures
- no broad HIR/parser/sema/rvalue, PHI, local/object, memory/va,
  aggregate/vector, parameter, target-lowering, MIR, or emission expansion

## Execution Rules

1. Keep the repair at the first evidenced production owner that drops the
   current-function pointer value identity.
2. Preserve existing verifier fail-closed behavior; strengthen focused coverage
   rather than weakening checks.
3. Build and run focused producer proof before the handoff. The supervisor owns
   regression logs and broader/full acceptance.

## Ordered Steps

### Step 1 - Trace and publish production computed-goto address authority

Goal: identify the first production owner that leaves the `IndirBrStmt` target
without a `LirValueId`, then minimally publish its existing pointer identity to
`LirIndirectBrOp.addr_value`.

Primary targets:

- `src/codegen/lir/hir_to_lir/stmt.cpp`
- only the immediate production rvalue/operand seam evidenced by
  `comp-goto-1.c`
- focused frontend or production-route test coverage

Actions:

- reproduce `ctest --test-dir build -V -R '^llvm_gcc_c_torture_src_comp_goto_1_c$'`
  and trace the target operand to the first missing authority owner
- publish the valid current-function pointer ID through the existing carrier;
  do not parse any rendered spelling or invent a parallel identity model
- retain verifier rejection for missing, invalid, foreign, non-pointer, and
  display-mismatched authority
- add focused production-path positive coverage and neighbouring malformed
  authority proof, then record a handoff explicitly limited to this route

Completion check:

- fresh build plus focused proof establishes that the production computed-goto
  carrier publishes valid `addr_value`; return to 734 after completed Step 7.24
  for plan-owner disposition without modifying or repeating the receiver.
