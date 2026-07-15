# DirectScalar Binary-LHS Authority Repair Runbook

Status: Active
Source Idea: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md
Switched from: ideas/open/825_lir_next_body_parameter_authority_handoff.md, Step 2

## Purpose

Resolve the single newly exposed native DirectScalar binary-LHS authority gap
before the paused direct switch-selector handoff resumes.

## Core Rule

Use only native structured parameter-definition facts. Do not derive authority
from operand spelling, type strings, names, diagnostics, printer output, or
testcase shape.

## Read First

- `ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md`
- `ideas/open/825_lir_next_body_parameter_authority_handoff.md` (resumption record)
- `src/codegen/lir/verify.cpp` (`verify_scalar_binary_lhs_authority`)

## Non-Goals

- Direct switch-selector authority and all preserved unaccepted Idea 825 code.
- Raw-BIR/importer/builder changes; RHS, return, pointer, generic-ABI, or
  second-row parameter work.

## Ordered Steps

### Step 1 - Trace the binary-LHS producer and authority seam

Goal: identify the native producer that creates the parameter-backed
`LirBinOp.lhs` and its exact existing verifier relation.

Actions:

- trace production through `src/codegen/lir/hir_to_lir/core.cpp` and the
  `LirBinOp.scalar_lhs_parameter_authority` schema in `src/codegen/lir/ir.hpp`;
- confirm the direct current-function parameter tuple and fail-closed boundary;
- do not modify the preserved Idea 825 selector slice.

Completion check: one exact native producer/verifier seam is identified with
no selector or other-row expansion.

### Step 2 - Identify the actual failing operation and reproduce its type relation

Goal: turn the Step 1 predicate mismatch into an evidence-backed, binary-LHS-only implementation target before designing a producer change.

Actions:

- run the full parent route only as diagnosis and capture the first exact lowered `LirBinOp` that aborts in `verify_scalar_binary_lhs_authority`;
- record the operation type, LHS value, matching native definition type, ABI, owner, and parameter index, showing specifically how value/ABI match while `definition.type` and binary-operation type differ;
- produce a minimal standalone source/reproduction that reaches that same mismatched type relation without using the preserved Idea 825 selector work;
- do not publish authority, change producer predicates, or treat an existing `ull x + 1` shape as proof of this relation.

Completion check: the actual first failing lowered operation and a standalone reproduction both demonstrate the Step 1 value/ABI-match plus type-mismatch relation, with no code, test, selector, or authority-schema change.

### Step 3 - Design and implement the bounded producer repair

Goal: after Step 2 evidence exists, make the producer/verifier relation agree for exactly the reproduced native DirectScalar binary-LHS case.

Actions:

- design the smallest structured producer/verifier repair justified by the Step 2 trace and reproduction; retain exact owner, value, index, ABI, role, LHS identity, and operation-type coherence checks;
- add nearby focused positive and malformed-authority coverage that exercises the reproduced relation rather than a previously covered arithmetic shape;
- do not edit or credit the preserved Idea 825 selector slice.

Completion check: the focused coverage proves the reproduced relation is represented by the exact structured tuple and malformed variants fail closed.

### Step 4 - Prove and return to 825

Goal: establish acceptance proof for the evidence-backed narrow repair and retain a precise return route.

Completion check: fresh `cmake --build --preset default` and exact `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'` pass with coverage for the Step 2 reproduction. The full `^frontend_lir_call_type_ref$` CTest is explicitly diagnostic/parent-composite evidence only, to run after Idea 825 resumes at Step 2 with its selector slice still unaccepted and untouched by this blocker.
