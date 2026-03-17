# Plan Execution State

## Baseline
- 1855/1855 tests passing (2026-03-17)

## Overall Assessment

Phase 5 has reached a major milestone: **truly deferred consteval evaluation** is now implemented.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- The compile-time pass fixpoint loop has 3 steps per iteration:
  1. Template instantiation (with optional deferred lowering)
  2. Pending consteval evaluation (`PendingConstevalExpr` → `IntLiteral`)
  3. Consteval reduction verification

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr
- Phase 6: scaffolded, not yet a meaningful materialization boundary
- Phase 7: largely complete for current scope

---

## What was done in this session (2026-03-17, session 3)

### Truly deferred consteval evaluation

**Problem**: Consteval calls inside deferred template instantiations were evaluated eagerly during the re-lowering, making the "deferred consteval" tracking only observational (counting side effects) rather than architectural (true separation of instantiation and evaluation).

**Solution**: Added `PendingConstevalExpr` as a first-class HIR expression type. During deferred template instantiation, consteval calls create `PendingConstevalExpr` nodes instead of evaluating immediately. The compile-time reduction pass evaluates these pending nodes in a separate step.

**Implementation changes**:

1. **`ir.hpp`**: Added `PendingConstevalExpr` struct (fn_name, const_args, tpl_bindings, call_span) and added it to the `ExprPayload` variant
2. **`ast_to_hir.cpp`**: Added `defer_consteval_` flag; when set during deferred lowering callback, consteval calls create `PendingConstevalExpr` instead of evaluating. Added `deferred_consteval` evaluation callback using AST-level consteval interpreter.
3. **`compile_time_pass.hpp`**: Added `DeferredConstevalEvalFn` callback type; updated `run_compile_time_reduction` signature to accept it
4. **`compile_time_pass.cpp`**: Added `PendingConstevalEvalStep` struct that walks `PendingConstevalExpr` nodes, calls the evaluation callback, replaces with `IntLiteral` on success, records `ConstevalCallInfo`. Integrated into fixpoint loop as Step 2 (between template instantiation and verification).
5. **`hir_printer.cpp`**: Added printer for `PendingConstevalExpr` showing fn_name, args, and template bindings

### Test additions
- **`deferred_consteval_multi.cpp`**: 4-level chain with 2 consteval calls (get_size + get_align) inside a deferred function
- **`cpp_hir_deferred_consteval_multi`**: HIR dump test verifying "2 deferred consteval reductions" appears
- **`cpp_positive_sema_deferred_consteval_multi_cpp`**: Runtime correctness test

### Files changed
- `src/frontend/hir/ir.hpp` — PendingConstevalExpr type + ExprPayload variant
- `src/frontend/hir/ast_to_hir.cpp` — defer_consteval_ flag, PendingConstevalExpr creation, DeferredConstevalEvalFn callback
- `src/frontend/hir/compile_time_pass.hpp` — DeferredConstevalEvalFn, updated run_compile_time_reduction signature
- `src/frontend/hir/compile_time_pass.cpp` — PendingConstevalEvalStep, fixpoint loop integration
- `src/frontend/hir/hir_printer.cpp` — PendingConstevalExpr printer
- `tests/internal/InternalTests.cmake` — new tests
- `tests/internal/cpp/postive_case/deferred_consteval_multi.cpp` — new test case

---

## Added Testcases

The work now includes 16 HIR-oriented regression tests:

Previous 14 + 2 new:
- **`cpp_hir_deferred_consteval_multi`** (NEW — proves multiple truly deferred consteval reductions)
- **`cpp_positive_sema_deferred_consteval_multi_cpp`** (NEW — runtime correctness for multi-consteval)

What these tests now verify:
- template metadata appears in `--dump-hir`
- template-originated calls keep printable template call info
- consteval calls are recorded in HIR dump output
- compile-time pass stats are printed
- specialization keys are printed
- materialization stats are printed
- deferred template instantiation by the HIR pass
- deferred consteval reduction unlocked by HIR pass
- **truly deferred consteval: PendingConstevalExpr created during deferred lowering and evaluated by the pass** (NEW)

What these tests do not yet verify:
- multi-step convergence where consteval result feeds back into template instantiation (requires non-type template parameters)
- irreducible compile-time nodes flowing through diagnostics path
- a real materialization policy that leaves some functions non-materialized

---

## Phase 5: Add HIR compile-time reduction loop
**Status: SUBSTANTIALLY COMPLETE**

### What is now implemented
- `run_compile_time_reduction()` performs real deferred template instantiation via `DeferredInstantiateFn` callback
- Phase 1.6 collects only depth-0 instantiations; nested calls are left for the HIR pass
- Template functions with no collected instances and no plain-call references are deferred (not generically lowered)
- The pass discovers unresolved template calls, reconstructs bindings, and triggers on-demand lowering
- **Truly deferred consteval**: during deferred lowering, consteval calls create `PendingConstevalExpr` nodes (NOT eagerly evaluated)
- **The pass evaluates `PendingConstevalExpr`** nodes using `DeferredConstevalEvalFn` callback, replacing them with `IntLiteral`
- `ConstevalCallInfo` records are created by the pass (not during lowering) for deferred consteval
- The pass iterates until convergence (fixpoint loop with 3 steps)
- Stats track `templates_deferred` and `consteval_deferred`
- Regression tests prove the pass does real work with true deferral

### What is not yet implemented
- multi-step convergence where consteval result feeds back into template args (requires non-type template parameters)
- generic lowering of template functions with symbolic bindings (currently re-lowers from AST)

### Remaining work
- non-type template parameters (prerequisite for consteval→template feedback loop)
- test case where true multi-step convergence occurs (template instantiation → consteval → template instantiation)

---

## Phase 6: Define materialization boundary
**Status: NOT COMPLETE**

(unchanged from previous assessment)

---

## Phase 7: Specialization identity and caching
**Status: MOSTLY COMPLETE**

(unchanged from previous assessment)

---

## Recommended next milestone

Phase 6: Define a meaningful materialization boundary. Currently all functions are materialized. The next step would be to identify template-only or consteval-only functions that should NOT be materialized (emitted as code), implementing a real materialization policy.
