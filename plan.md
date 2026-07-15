# DirectScalar Unary Fneg Authority Runbook

Status: Active
Source Idea: ideas/open/828_lir_direct_scalar_unary_fneg_authority.md
Switched from: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md, Step 2

## Purpose

Resolve the newly observed native DirectScalar unary floating `fneg` authority
gap without broadening into the parked binary or selector routes.

## Core Rule

Use structured native current-function parameter facts only. Do not infer
authority from operand spelling, rendered types, names, diagnostics, or a
testcase-shaped predicate.

## Read First

- `ideas/open/828_lir_direct_scalar_unary_fneg_authority.md`
- `ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md` (resumption record)
- `src/codegen/lir/hir_to_lir/expr/misc.cpp`
- `src/codegen/lir/verify.cpp`

## Non-Goals

- Binary producer repair, including 827's disproven mismatch premise.
- Direct switch-selector/Idea 825 work, Raw-BIR/importer, generic rows, and
  any non-unary-floating-`fneg` authority route.

## Ordered Steps

### Step 1 - Trace unary fneg publication and verifier shape

Goal: establish the exact producer/verifier seam for a native DirectScalar
parameter used by unary floating `fneg`, then decide whether a bounded repair
is warranted and what structured shape it must use.

Actions:

- trace `expr/misc.cpp` from unary minus to the emitted `LirBinOp` and inspect
  the verifier relation that reports missing authority;
- use the known standalone source
  `double lir_floating_unary_minus_ternary_phi_authority(int condition, double left, double right) { return condition ? -left : -right; }`
  only as the diagnostic anchor;
- record native value, owner, index, type, ABI, role, operand identity, and
  operation type, plus the smallest producer/verifier design decision;
- do not modify code or tests in this diagnosis packet.

Completion check: the unary `fneg` authority seam and a bounded
producer/verifier decision are documented from structured facts, with no
binary, selector, or generic-row expansion.

### Step 2 - Implement the justified unary fneg authority repair

Goal: implement only the Step 1-supported structured publication/verifier
shape and focused nearby coverage.

Completion check: positive and malformed cases demonstrate the selected unary
`fneg` contract and unrelated forms stay fail closed.

### Step 3 - Prove the narrow route and return to 827

Goal: obtain independently attributable acceptance evidence and hand the
result back to 827 for its explicit repair/close decision.

Completion check: run a fresh build and the Step 1-selected narrow CTest; do
not credit the parent composite CTest or dirty selector work as this idea's
acceptance proof.
