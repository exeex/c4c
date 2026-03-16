# Plan Execution State

## Baseline
- 1814/1814 tests pass (2026-03-16)

## Current Phase: Phase 1 — Strengthen existing constant-expression helper

### Completed
- [x] Global named constant folding for const/constexpr scalar globals
- [x] Global binding propagation (const_int_bindings_ in ast_to_hir.cpp)
- [x] Integer cast folding (unsigned int/short/char casts)
- [x] constexpr_var.cpp folds correctly and runs
- [x] const_named_fold.cpp added as regression coverage
- [x] Phase 1, Task 1: Refactor consteval.cpp with ConstValue/ConstEvalResult/ConstEvalEnv
- [x] Phase 1, Task 2: Local constant binding support in evaluator
  - ConstEvalEnv gains `local_consts` field
  - validate.cpp tracks local const/constexpr values in scoped maps; passes them to case label eval
  - ast_to_hir.cpp tracks local_const_bindings in FunctionCtx with proper block scoping
  - All ConstEvalEnv usage sites (case, case_range, ternary) receive local bindings
  - constexpr_local_switch.cpp test added
- [x] Phase 1, Task 3: Replace duplicated integer-folding in validate.cpp
  - Extended ConstEvalEnv with scoped lookup support (enum_scopes, local_const_scopes)
  - Removed validate.cpp's local eval_int_const_expr, EnumConstLookup, lookup_enum_const
  - Both call sites (NK_DECL const tracking, NK_CASE label eval) now use evaluate_constant_expr
  - Added consteval.hpp include and using declarations to validate.cpp
  - Phase 1 complete: all constant evaluation goes through unified evaluator

### Not Started
- Phase 2: Immediate-function interpretation
- Phase 3: Enforce consteval rules
- Phase 4: Integrate with if constexpr, builtins, templates

## Next Intended Slice
- Phase 2, Task 1: Build minimal function-body interpreter for consteval functions

## Blockers
- None
