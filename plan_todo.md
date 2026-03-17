# Plan Execution State

## Baseline
- 1851/1851 tests passing (2026-03-17)

## Overall Assessment

Phase 5 has made meaningful progress: `run_compile_time_reduction()` now performs real deferred template instantiation work.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is now owned by the HIR compile-time reduction pass** (Phase 5 milestone met for templates)
- Consteval reduction is still performed eagerly during AST-to-HIR lowering
- Direct (depth-0) template calls are still collected and lowered eagerly in Phase 1.6/2

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **partially complete** — deferred template instantiation works; deferred consteval reduction not yet moved
- Phase 6: scaffolded, not yet a meaningful materialization boundary
- Phase 7: largely complete for current scope

---

## What was done in this session (2026-03-17)

### Deferred template instantiation via HIR compile-time pass

**Architecture change**: Nested template calls (e.g., `add<T>()` inside `twice<T>()`) are no longer discovered and lowered during the initial AST-to-HIR lowering fixpoint. Instead:

1. **Phase 1.6** only collects depth-0 instantiations (direct calls from non-template functions)
2. **Phase 2** lowers only those depth-0 instantiations; nested template calls produce DeclRefs to not-yet-existing functions
3. **Phase 3 (new)**: `run_compile_time_reduction()` discovers unresolved template calls, reconstructs TypeBindings from TemplateCallInfo + HirTemplateDef params, and invokes a deferred lowering callback to instantiate the missing functions
4. The pass iterates until convergence

**Key API change**: `run_compile_time_reduction()` now accepts an optional `DeferredInstantiateFn` callback. When provided, the pass performs real work (lowering new functions). When null, it operates in verification-only mode.

**Data flow**:
- `compile_time_pass.hpp`: added `DeferredInstantiateFn` callback type and `templates_deferred` stat
- `compile_time_pass.cpp`: `TemplateInstantiationStep` reconstructs bindings and calls the callback for pending template calls
- `ast_to_hir.cpp`: Phase 3 creates a lambda that invokes `lower_function()` for deferred templates
- `ir.hpp`: `Module::CompileTimeInfo` stores deferred instantiation stats
- `c4cll.cpp`: propagates deferred count from `ct_info` into verification stats for HIR dump

**Consteval still handled eagerly**: `collect_consteval_template_instantiations()` was added to Phase 1.6 to ensure consteval template calls within template bodies are still collected pre-lowering (since consteval evaluation requires all bindings at lowering time).

### Test additions
- `cpp_hir_deferred_template_instantiation`: verifies "1 deferred instantiation" appears in HIR dump for `template_chain.cpp`
- This test **only passes** if the HIR pass (not lowering) performs the nested instantiation

### Files changed
- `src/frontend/hir/compile_time_pass.hpp` — DeferredInstantiateFn, templates_deferred stat
- `src/frontend/hir/compile_time_pass.cpp` — deferred instantiation logic in TemplateInstantiationStep
- `src/frontend/hir/ast_to_hir.cpp` — Phase 1.6 depth-0 only, Phase 3 deferred callback, collect_consteval_template_instantiations
- `src/frontend/hir/ir.hpp` — Module::CompileTimeInfo
- `src/apps/c4cll.cpp` — propagate deferred count
- `tests/internal/InternalTests.cmake` — new test

---

## Added Testcases

The work added 13 HIR-oriented regression tests in `tests/internal/InternalTests.cmake`:

- `cpp_hir_template_def_dump`
- `cpp_hir_consteval_template_dump`
- `cpp_hir_template_call_info_dump`
- `cpp_hir_consteval_call_info_dump`
- `cpp_hir_consteval_template_call_info_dump`
- `cpp_hir_compile_time_reduction_stats`
- `cpp_hir_template_instantiation_resolved`
- `cpp_hir_consteval_reduction_verified`
- `cpp_hir_consteval_template_reduction_verified`
- `cpp_hir_fixpoint_convergence`
- `cpp_hir_specialization_key`
- `cpp_hir_materialization_stats`
- **`cpp_hir_deferred_template_instantiation`** (NEW — proves HIR pass does real work)

What these tests now verify:
- template metadata appears in `--dump-hir`
- template-originated calls keep printable template call info
- consteval calls are recorded in HIR dump output
- compile-time pass stats are printed
- specialization keys are printed
- materialization stats are printed
- **deferred template instantiation by the HIR pass** (NEW)

What these tests do not yet verify:
- deferred consteval reduction in HIR
- irreducible compile-time nodes flowing through the new diagnostics path
- a real materialization policy that leaves some functions non-materialized

---

## Phase 5: Add HIR compile-time reduction loop
**Status: PARTIALLY COMPLETE**

### What is now implemented
- `run_compile_time_reduction()` performs real deferred template instantiation via `DeferredInstantiateFn` callback
- Phase 1.6 collects only depth-0 instantiations; nested calls are left for the HIR pass
- The pass discovers unresolved template calls, reconstructs bindings, and triggers on-demand lowering
- The pass iterates until convergence (fixpoint loop)
- Stats track `templates_deferred` to distinguish pass-driven work from eager lowering
- Regression test proves the pass does real work (`cpp_hir_deferred_template_instantiation`)

### What is not yet implemented
- deferred consteval reduction (consteval is still evaluated eagerly during lowering)
- the pass does not yet mutate HIR expression nodes (consteval calls are not deferred)
- multi-step convergence where template instantiation unlocks new consteval calls that unlock new templates

### Remaining work
- move one class of consteval reduction into the HIR pass
- add a test where template instantiation unlocks a new consteval call that the pass evaluates
- prove multi-step convergence (template → consteval → template cycle)

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

Move one class of deferred consteval evaluation into `run_compile_time_reduction()`:
- During lowering of deferred template instantiations, consteval calls within those functions are already evaluated eagerly (since the callback invokes `lower_function` which runs the full lowering path including consteval)
- The next step would be to defer consteval calls that depend on template bindings not yet resolved, and have the pass evaluate them after the bindings become available
- Add a regression test that only passes if the HIR pass evaluates a consteval call that was unlocked by a deferred template instantiation
