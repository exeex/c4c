# DirectScalar Unary Fneg Authority Regression-Guard Repair Runbook

Status: Active
Source Idea: ideas/open/828_lir_direct_scalar_unary_fneg_authority.md
Switched from: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md, Step 2

## Purpose

Finish the native DirectScalar unary floating `fneg` authority route only when
the expanded regression baseline is accounted for, without broadening into the
parked binary or selector routes.

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

## Accepted Progress

- Step 1 traced the unary `fneg` producer/verifier seam.
- Step 2 was implemented in accepted commit `524b24f64`: structured native
  DirectScalar authority publication, verifier fail-closed RHS validation, and
  nearby positive/malformed coverage.
- The fresh build and focused
  `^frontend_lir_function_signature_type_ref$` CTest passed, with matching
  narrow guard logs (1/1, no new failures).

## Ordered Steps

### Step 2R - Classify and resolve the expanded-baseline regression

Goal: determine whether the new `frontend_hir_tests` SEGFAULT is attributable
to `524b24f64` or to preserved unrelated dirty work, and restore an acceptable
expanded baseline before crediting this idea as complete.

Actions:

- preserve the dirty 821/822/825-related worktree changes while comparing a
  matched expanded baseline that isolates `524b24f64` from those changes;
- record the exact reproducer, comparison, and attribution from structured
  repository evidence; do not infer causality merely from the current dirty
  aggregate worktree;
- if the commit is causal, repair only the unary `fneg` route with nearby
  coverage and repeat build, focused proof, and the matched expanded guard;
- if the commit is not causal, name the responsible open route and switch only
  if that separate route blocks this idea's accepted proof.

Completion check: the matched expanded guard has no new failure, or a
separately scoped and active blocker owns a proven unrelated failure. The
`frontend_hir_tests` SEGFAULT must not be silently waived.

### Step 3 - Close or return to 827

Goal: after Step 2R accepts the expanded baseline, record 828's accepted
evidence and return to 827 for its explicit repair/close decision.

Completion check: 828 is closed only with the accepted regression guard and
the parent return record; do not credit the parent composite CTest or dirty
selector/call-type work as 828 proof.
