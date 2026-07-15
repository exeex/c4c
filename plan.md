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

### Step 2 - Publish and verify one binary-LHS authority tuple

Goal: make the native producer and `verify_scalar_binary_lhs_authority` agree
on the existing structured LHS tuple.

Actions:

- emit authority only for a direct current-function DirectScalar LHS match;
- verify owner, value, index, type, ABI, `Lhs` role, LHS identity, and
  operation-type coherence;
- add focused positive and malformed-authority coverage.

Completion check: missing and malformed authority fails closed; no unrelated
authority family or Idea 825 selector code is edited.

### Step 3 - Prove and return to 825

Goal: establish acceptance proof for this narrow blocker and retain a precise
return route.

Completion check: fresh `cmake --build --preset default` and exact
`ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
pass; record that Idea 825 resumes at Step 2 without accepting its preserved
selector slice.
