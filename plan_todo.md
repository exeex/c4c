# Plan Execution State

## Baseline
- 1853/1853 tests passing (2026-03-17)

## Overall Assessment

Phase 5 is now further along: deferred consteval reduction is tracked alongside deferred template instantiation.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass** (Phase 5 milestone met for templates)
- **Deferred consteval reduction is tracked**: consteval calls inside deferred template instantiations are counted as "deferred consteval reductions" in the pass stats
- Direct (depth-0) template calls are still collected and lowered eagerly in Phase 1.6/2
- Consteval calls whose arguments are fully resolved at lowering time are still evaluated eagerly

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **partially complete** — deferred template instantiation works; deferred consteval reduction is tracked via the pass; true deferred-evaluation-from-HIR (where consteval is not evaluated during lowering at all) not yet implemented
- Phase 6: scaffolded, not yet a meaningful materialization boundary
- Phase 7: largely complete for current scope

---

## What was done in this session (2026-03-17, session 2)

### Deferred consteval reduction tracking

**Problem**: Consteval calls inside functions instantiated by the HIR compile-time reduction pass were not tracked separately. There was no proof that the pass could unlock consteval reductions.

**Test case**: `deferred_consteval_chain.cpp` — 3-level chain:
- `main()` → `outer<int>()` (depth-0, collected in Phase 1.6)
- `outer<int>()` → `middle<int>()` (nested, deferred to HIR pass)
- `middle<int>()` → `get_size<int>()` (consteval, evaluated during deferred lowering)

**Implementation changes**:

1. **`compile_time_pass.hpp`**: Added `consteval_deferred` field to `CompileTimePassStats`
2. **`compile_time_pass.cpp`**: `TemplateInstantiationStep` tracks `module.consteval_calls.size()` before/after each deferred instantiation; difference is accumulated as `consteval_unlocked`
3. **`compile_time_pass.cpp`**: `format_compile_time_stats` reports "N deferred consteval reductions" alongside deferred instantiations
4. **`ir.hpp`**: `Module::CompileTimeInfo` has new `deferred_consteval` field
5. **`ast_to_hir.cpp`**: Propagate `consteval_deferred` stat from compile-time pass to module info
6. **`c4cll.cpp`**: Propagate deferred consteval count for HIR dump

**Bug fixes required to make the 3-level chain work**:

1. **Generic lowering of template functions with no instances**: Template functions with no collected instances were unconditionally lowered generically (without bindings). For functions like `middle<T>()` that contain consteval calls with unresolved T, this causes hard errors. Fixed by adding `is_referenced_without_template_args()` check: if a template function is NOT called from any non-template function without explicit template args, skip generic lowering (it will be deferred instead).

2. **Consteval template collection from unresolved template bodies**: `collect_consteval_template_instantiations` was recording consteval calls (e.g., `get_size<T>()`) from template function bodies (e.g., `middle<T>()`) even when the enclosing function had no concrete instances. This produced entries with unresolved type bindings. Fixed by checking: if the enclosing function is a template with no instances, skip recording (the consteval will be handled during deferred instantiation).

### Files changed
- `src/frontend/hir/compile_time_pass.hpp` — consteval_deferred stat
- `src/frontend/hir/compile_time_pass.cpp` — track consteval unlocked by deferred instantiation, format output
- `src/frontend/hir/ir.hpp` — Module::CompileTimeInfo deferred_consteval
- `src/frontend/hir/ast_to_hir.cpp` — skip generic lowering for deferred templates, skip consteval collection from unresolved bodies, propagate stats
- `src/apps/c4cll.cpp` — propagate deferred consteval count
- `tests/internal/InternalTests.cmake` — new tests
- `tests/internal/cpp/postive_case/deferred_consteval_chain.cpp` — new test case

### Test additions
- `cpp_hir_deferred_consteval_chain`: verifies "deferred consteval reduction" appears in HIR dump (proves HIR pass unlocked the consteval)
- `cpp_positive_deferred_consteval_chain_cpp`: runtime correctness test (added via CPP_POSITIVE_RUNTIME_STEMS)

---

## Added Testcases

The work now includes 14 HIR-oriented regression tests plus the new deferred consteval tests:

Previous 13 + 1 new:
- **`cpp_hir_deferred_consteval_chain`** (NEW — proves HIR pass unlocks consteval reductions)

What these tests now verify:
- template metadata appears in `--dump-hir`
- template-originated calls keep printable template call info
- consteval calls are recorded in HIR dump output
- compile-time pass stats are printed
- specialization keys are printed
- materialization stats are printed
- deferred template instantiation by the HIR pass
- **deferred consteval reduction unlocked by HIR pass** (NEW)

What these tests do not yet verify:
- truly deferred consteval (where consteval call is NOT evaluated during lowering but deferred to a later pass)
- irreducible compile-time nodes flowing through the new diagnostics path
- a real materialization policy that leaves some functions non-materialized

---

## Phase 5: Add HIR compile-time reduction loop
**Status: PARTIALLY COMPLETE**

### What is now implemented
- `run_compile_time_reduction()` performs real deferred template instantiation via `DeferredInstantiateFn` callback
- Phase 1.6 collects only depth-0 instantiations; nested calls are left for the HIR pass
- Template functions with no collected instances and no plain-call references are deferred (not generically lowered)
- The pass discovers unresolved template calls, reconstructs bindings, and triggers on-demand lowering
- Consteval calls inside deferred instantiations are tracked as "deferred consteval reductions"
- The pass iterates until convergence (fixpoint loop)
- Stats track `templates_deferred` and `consteval_deferred`
- Regression tests prove the pass does real work

### What is not yet implemented
- truly deferred consteval (consteval calls that are NOT evaluated during lowering but instead stored as pending HIR nodes and evaluated by the pass)
- multi-step convergence where template instantiation unlocks new consteval calls that unlock new templates (the current system evaluates consteval eagerly during deferred lowering, so there's no true HIR-level consteval deferral)

### Remaining work
- represent pending consteval calls as HIR expression nodes (not IntLiteral placeholders)
- have the pass evaluate those pending nodes when bindings become available
- add a test where true multi-step convergence occurs at the HIR level

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

Implement truly deferred consteval in HIR:
- During lowering, if a consteval call's template arguments can't be resolved (because the enclosing function's bindings aren't concrete), emit a "pending consteval" marker expression instead of failing or eagerly evaluating
- The HIR pass would later evaluate these pending calls after deferred template instantiation resolves the bindings
- This would enable true multi-step convergence: template instantiation → consteval reduction → template instantiation (if consteval result feeds back into template args)
