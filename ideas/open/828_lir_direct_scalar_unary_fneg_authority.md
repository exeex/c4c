# LIR DirectScalar Unary Fneg Authority

Status: Open
Type: narrow native producer/verifier blocker
Blocked Parent: `ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md`, Step 2

## Goal

Determine and, only if justified by the trace, repair native current-function
`DirectScalar` parameter authority publication and verification for unary
floating `fneg` `LirBinOp` operations.

## Why This Exists

The parent diagnostic reaches `verify_scalar_binary_lhs_authority` at
`lir_floating_unary_minus_ternary_phi_authority` on a `double` `fneg` whose
`lhs` is native parameter `%p.left`. Its value, owner, index, type, and ABI
already match the native parameter. The missing publication originates in
unary lowering (`src/codegen/lir/hir_to_lir/expr/misc.cpp`), not the parked
binary producer route, so it needs a separately scoped trace before any repair
or verifier shape is selected.

## In Scope

- Native current-function `DirectScalar` parameter authority for unary floating
  `fneg` `LirBinOp` only.
- Trace the producer in unary lowering and the existing verifier relation;
  decide the smallest defensible publication and verifier shape from structured
  native parameter facts.
- If the diagnosis supports it, implement and test only that bounded unary
  floating `fneg` authority path with focused positive and malformed coverage.

## Out Of Scope

- Binary producer repair or any reuse of 827's unproven type-mismatch premise.
- Direct switch-selector work and all dirty Idea 825/822 worktree changes.
- Raw-BIR/importer work, generic rows, ABI conversion, RHS, return, pointer,
  or any non-unary-float-fneg parameter-use row.

## Acceptance Criteria

1. The first packet establishes the unary `fneg` producer/verifier path and
   whether existing scalar-binary authority is structurally appropriate; no
   authority shape is assumed from its name.
2. Any implementation publishes and verifies structured current-function
   native parameter facts only for a unary floating `fneg` whose operand is
   that direct scalar parameter, and unrelated rows remain fail closed.
3. Focused nearby positive and malformed coverage proves the selected shape;
   no preserved switch-selector work is needed or credited.
4. A fresh build and independently attributable narrow CTest are selected only
   after diagnosis; the parent composite CTest remains diagnostic until its
   separate parent route resumes.

## Reviewer Reject Signals

- Reject a testcase-specific bypass, expectation downgrade, or verifier
  exception that masks the missing unary authority.
- Reject binary producer edits, selector changes, Raw-BIR/importer work,
  generic admission, or unrelated authority rows claimed as unary-fneg work.
- Reject publication inferred from operand text, display/type strings, or the
  old binary mismatch theory instead of structured native parameter facts.
- Reject a producer-only or verifier-only change without focused positive and
  malformed coverage showing the chosen unary `fneg` contract fails closed.
