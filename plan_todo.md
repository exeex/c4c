# Plan Execution State

## Baseline
- 1824/1824 tests pass (2026-03-16)

## Current Phase: Phase 3 — Enforce consteval rules (COMPLETE)

### Completed
- [x] Phase 1 (all tasks complete — see previous commits)
- [x] Phase 2 (all tasks complete — see previous commits)
- [x] Phase 3, Task 1: Diagnose non-immediate calls
  - Non-constant args to consteval already produced hard error (from Phase 2 Task 4)
  - Added C++ negative test infrastructure (cpp/negative_case/ directory, CMake registration)
  - Added 3 negative test cases: bad_consteval_non_constant_arg, bad_consteval_runtime_arg, bad_consteval_addr_of
- [x] Phase 3, Task 2: Diagnose taking function address
  - Added check in NK_ADDR handler in ast_to_hir.cpp: if operand is reference to consteval function, throw error
  - Verified: `&consteval_fn` now produces "cannot take address of consteval function" error
- [x] Phase 3, Task 3: Forbid runtime fallback (completed in Phase 2 Task 4)
  - Failed consteval calls throw hard error, no runtime fallback
  - Consteval function bodies are skipped during HIR lowering
- [x] Phase 3, Task 4: Tighten declaration rules
  - Reject `consteval` on local variable declarations (parse_local_decl)
  - Reject `consteval` on global variable declarations (make_gvar in parse_global_decl_or_function)
  - Reject `consteval` + `constexpr` combined on same declaration
  - Added 3 negative tests: bad_consteval_on_variable, bad_consteval_local_variable, bad_consteval_constexpr_combo

### Not Started
- Phase 4: Integrate with if constexpr, builtins, templates

## Next Intended Slice
- Phase 4, Task 1: Route `if constexpr` condition folding through the unified evaluator

## Blockers
- None
