# LIR Rvalue Value Identity Preservation For Computed-Goto Runbook

Status: Active
Source Idea: ideas/open/758_lir_rvalue_value_identity_preservation_for_computed_goto.md
Resumed from: lifecycle record in the source idea and historical runbook commit
`32b2f83fb`; no Step 1 implementation packet was accepted before interruption.
Supersedes: paused 757 Step 1 while its separately scoped prerequisite is
completed; 757's durable resumption record contains the return point.

## Purpose

Keep already-produced local/parameter rvalue `LirValueId` authority intact at
the existing rvalue/operand boundary, rather than allowing consumers to infer
it from raw operand text.

## Goal

Expose the exact typed identity already emitted for eligible local/parameter
rvalues to immediate route consumers, with display spelling remaining
non-authoritative.

## Core Rule

Typed identity is the only semantic authority. `LirOperand` spelling, labels,
printer output, rendered LLVM, and testcase names must not select, recreate, or
repair it.

## Read First

- `ideas/open/758_lir_rvalue_value_identity_preservation_for_computed_goto.md`
- `ideas/open/757_lir_computed_goto_address_value_identity_publication.md`
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
- the `emit_rval_payload`, `emit_rval_expr`, `emit_rval_operand`, and
  `StmtEmitter::emit_control_flow_stmt(IndirBrStmt)` ownership route

## Non-Goals

- no `LirIndirectBrOp` address field or verifier change; 757 resumes that work
- no Raw-BIR, computed-goto receiver, successor, target-lowering, MIR, or
  emission work
- no new value creation, load-semantics rewrite, text recovery, or unrelated
  rvalue-family expansion

## Execution Rules

1. Preserve existing identity through the smallest general rvalue/operand
   carrier seam; do not add a computed-goto-only shortcut.
2. Keep the route fail closed when identity is absent, invalid, foreign, or
   unsuitable; display spelling remains a compatibility mirror only.
3. Test local and parameter paths plus malformed and misleading-display
   neighbors at the same route boundary.
4. Run a fresh build and the exact focused guard. Leave broader acceptance and
   regression-log roll-forward to the supervisor.

## Ordered Steps

### Step 1 - Preserve typed local and parameter rvalue identity

Goal: carry the typed value identity already emitted by eligible local and
parameter rvalues through the existing expression/operand route.

Primary targets:

- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp`
- the direct rvalue expression/operand result carriers and their immediate
  consumer boundary
- nearby focused LIR route coverage

Actions:

- trace and document the exact local and parameter identity loss points
- introduce the smallest route-level typed carrier needed to retain existing
  current-function identity alongside display spelling
- preserve absence and ownership/type failure states without manufacturing an
  identity or parsing text
- prove local and parameter positive paths plus missing, invalid, foreign,
  unsuitable, and misleading-display failures at the route boundary
- record the structured handoff: preserved result, failure boundary, and 757
  Step 1 return action

Completion check:

- a fresh build and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  prove the exact typed rvalue identity survives independently of display
  spelling; return source completion state to plan-owner rather than inferring
  closure.
