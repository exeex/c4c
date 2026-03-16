# Consteval Plan — Execution State

## Completed

### Phase 1: Strengthen existing constant-expression helper
- [x] ConstValue / ConstEvalResult / ConstEvalEnv types defined in consteval.hpp
- [x] Unified evaluator `evaluate_constant_expr()` used by validate.cpp and ast_to_hir.cpp
- [x] Global named constant folding for const/constexpr scalars
- [x] Local constant binding support in validate.cpp (scoped)
- [x] Integer cast folding
- [x] constexpr_var.cpp and const_named_fold.cpp passing
- [x] Legacy `eval_int_const_expr` has no callers (dead code, can be removed)

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
- **Problem**: when a non-consteval template function like `process<int>()` calls a consteval function like `get_size<T>()`, the template arg `T` at the call site is still an unresolved `TB_TYPEDEF`. The consteval interpreter can't resolve `sizeof(T)`.
- **Root cause**: the HIR lowering builds `tpl_bindings` only from the immediate call site's template_arg_types, which inside a template body are still typedef references.
- **Fix**: added template multi-instantiation support:
  - Pre-scan all function bodies to collect unique template arg sets per function
  - Iterative convergence loop to propagate bindings transitively (inner template calls resolved through outer)
  - Each template function lowered once per unique concrete instantiation with mangled name
  - Call sites resolve to the correct mangled name based on template arg types
  - Consteval calls inside template functions get correct type bindings from the enclosing instantiation
- **Key files changed**: `src/frontend/hir/ast_to_hir.cpp`
- **Test**: `consteval_nested_template.cpp` — covers direct, wrapped, if-constexpr, and chained consteval calls from template functions with multiple type instantiations

## Active Item

(none — pick next item)

## Next Items

- Phase 1 cleanup: remove dead `eval_int_const_expr` legacy API
- Phase 4 remaining: additional template + consteval integration tests
- Negative test cases for consteval diagnostics
