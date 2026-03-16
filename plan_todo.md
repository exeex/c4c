# Plan Execution State

## Baseline
- 1821/1821 tests pass (2026-03-16)

## Current Phase: Phase 3 — Enforce consteval rules

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

### Not Started
- Phase 3, Task 4: Tighten declaration rules (reject invalid combos as needed)
- Phase 4: Integrate with if constexpr, builtins, templates

## Next Intended Slice
- Phase 3, Task 4: Tighten declaration rules (e.g., consteval + non-function, consteval + constexpr on same decl)

## Blockers
- None
