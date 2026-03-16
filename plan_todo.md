# Consteval Plan — Execution State

## Completed

### Phase 1: Strengthen existing constant-expression helper
- [x] ConstValue / ConstEvalResult / ConstEvalEnv types defined in consteval.hpp
- [x] Unified evaluator `evaluate_constant_expr()` used by validate.cpp and ast_to_hir.cpp
- [x] Global named constant folding for const/constexpr scalars
- [x] Local constant binding support in validate.cpp (scoped)
- [x] Integer cast folding
- [x] constexpr_var.cpp and const_named_fold.cpp passing
- [x] Legacy `eval_int_const_expr` removed (dead code cleanup)

### Phase 2: Add immediate-function interpretation
- [x] Function-body interpreter (interp_stmt / interp_expr / interp_block)
- [x] Parameter binding, local variable support (mutable locals)
- [x] Arithmetic, if/else, while/for/do-while, break/continue, return
- [x] Chained and recursive consteval calls
- [x] Block scoping with variable shadowing
- [x] Assignment expressions and comma expressions
- [x] sizeof/alignof in consteval bodies
- [x] Step limit and recursion depth limit
- [x] Tests: consteval_interp, consteval_mutable, consteval_call_resolution, consteval_block_scope, consteval_short_circuit, consteval_recursive_fib

### Phase 3: Enforce C++ subset consteval rules
- [x] Diagnose non-constant arguments to consteval calls (hard error)
- [x] Diagnose taking address of consteval function (hard error)
- [x] Forbid runtime fallback (consteval functions not emitted to LLVM IR)
- [x] Tests: consteval_diag (positive coverage after diagnostics added)

### Phase 4: Integrate with if constexpr, builtins, and templates (partial)
- [x] if constexpr condition folding through unified evaluator
- [x] sizeof/alignof as constant expressions in constexpr/if constexpr
- [x] Direct consteval template calls with concrete template args (e.g., `get_size<int>()`)
- [x] Template type substitution in consteval body (resolve_type with TypeBindings)
- [x] Tests: constexpr_if, constexpr_builtin_queries, consteval_template, consteval_template_sizeof

### Phase 4 slice: propagate template bindings through nested template → consteval calls
- [x] **COMPLETED** (2026-03-16)
- Template multi-instantiation support with iterative convergence
- Test: consteval_nested_template.cpp

### Cleanup: remove dead legacy API + add negative test coverage
- [x] **COMPLETED** (2026-03-16)
- Removed `eval_int_const_expr` from consteval.cpp/hpp (zero callers)
- Added `bad_consteval_runtime_call.cpp` negative test (call to non-consteval function from consteval body)

## Active Item

(none — all plan items complete)

## Status

All phases of the consteval plan are complete:
- Phase 1: constant-expression helper with unified evaluator ✓
- Phase 2: immediate-function interpretation ✓
- Phase 3: consteval rule enforcement ✓
- Phase 4: if constexpr, builtins, and template integration ✓
- Cleanup: dead code removal and negative test coverage ✓

Negative test coverage (9 cases):
- bad_consteval_non_constant_arg, bad_consteval_addr_of, bad_consteval_runtime_arg
- bad_consteval_on_variable, bad_consteval_local_variable, bad_consteval_constexpr_combo
- bad_consteval_non_constant_expr_stmt, bad_consteval_divide_by_zero
- bad_consteval_runtime_call (new)

Full suite: 1838/1838 passing
