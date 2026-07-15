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

## Regression Guard Blocker

- Accepted implementation evidence is commit `524b24f64`, a fresh successful
  `cmake --build --preset default`, and the focused
  `^frontend_lir_function_signature_type_ref$` CTest with matching narrow logs
  (1/1, no new failures).
- Capability closure is rejected for now: the expanded baseline comparison
  changed from 0/3038 failures to `frontend_hir_tests` SEGFAULT (1 failure),
  recorded in `test_baseline.new.log`.
- The shared worktree also contains preserved unrelated 821/822/825 changes.
  Clean detached before/after comparison now proves the regression is not
  attributable to `524b24f64`; the dirty selector-authority route is owned by
  existing Idea 825.

## Closure Record

Disposition: capability complete.

- `524b24f64` implements the bounded unary `fneg` authority route; its fresh
  build and focused `^frontend_lir_function_signature_type_ref$` CTest passed.
- Clean detached `524b24f64^` and clean detached `524b24f64`, both
  backend-enabled, built and directly executed `frontend_hir_tests` with
  `PASS: frontend_hir_tests`; the aggregate dirty CTest SEGFAULT is not 828
  regression evidence.
- Return: parent Idea 827 resumes at Step 2 for its explicit repair/close
  decision; no binary-LHS implementation is authorized by this closure.
