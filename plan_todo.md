# Plan Execution State

## Baseline
- 1844/1844 tests passing (2026-03-17)

## Phase 1: Constrain sema to conservative compile-time work
**Status: COMPLETE** (already satisfied by current architecture)

### Audit Results

Consteval entry points:

1. **`src/frontend/sema/validate.cpp`** — uses `evaluate_constant_expr()` only for:
   - Tracking const/constexpr local values for switch case label evaluation (line 671)
   - Validating case labels reduce to integer constants (line 792)
   - These are standard C/C++ diagnostics — safe to keep in sema.

2. **`src/frontend/sema/sema.cpp`** — no consteval usage at all.

3. **`src/frontend/hir/ast_to_hir.cpp`** — owns all heavy consteval work:
   - Phase 1.5: collects consteval function definitions (line 448-452)
   - Phase 1.6: collects template instantiations with fixpoint iteration (line 454-475)
   - Call-site consteval interpretation with `evaluate_consteval_call()` (line 3015-3078)
   - Template call name resolution (line 3082-3091)
   - Address-of-consteval rejection (line 2962-2965)

### Conclusion
- Sema is already conservative: only simple constant expression folding for diagnostics
- No template instantiation or consteval function body interpretation in sema
- All template materialization and consteval reduction happens in HIR lowering (ast_to_hir.cpp)
- No changes needed — Phase 1 exit criteria are met

### Folds safe to keep in sema:
- `evaluate_constant_expr()` for const/constexpr init tracking
- `evaluate_constant_expr()` for case label validation
- enum constant tracking

### Folds that must stay in HIR (ast_to_hir.cpp), not move to sema:
- `evaluate_consteval_call()` — function body interpretation
- Template instantiation discovery and lowering
- Template call name resolution and mangling
- Consteval + template combined evaluation (type_bindings propagation)

---

## Phase 2: Make HIR preserve template function definitions
**Status: COMPLETE**

### Goal
Lower template functions into HIR without immediately erasing them into concrete functions only.
Keep template definitions as first-class HIR entities alongside instantiated copies.

### Completed Slices
- [x] Slice 1: Add `HirTemplateDef`, `HirTemplateInstantiation`, `TypeBindings` to `ir.hpp`
- [x] Slice 2: Populate `HirTemplateDef` during Phase 1.7 of lowering in `ast_to_hir.cpp`
- [x] Slice 3: Add HIR printer support for template definitions (`hir_printer.cpp`)
- [x] Slice 4: Add `--dump-hir` tests: `cpp_hir_template_def_dump` and `cpp_hir_consteval_template_dump`

### What was added
- `ir.hpp`: `TypeBindings`, `HirTemplateInstantiation`, `HirTemplateDef` structs
- `ir.hpp`: `Module::template_defs` map
- `ast_to_hir.cpp`: Phase 1.7 populates `template_defs` from `template_fn_defs_` and `template_fn_instances_`
- `hir_printer.cpp`: `print_template_def()` method, `--- templates ---` section in dump output
- `InternalTests.cmake`: 2 new `--dump-hir` regex tests

---

## Phase 3: Preserve template applications/calls in HIR
**Status: COMPLETE**

### Goal
Distinguish template application (e.g., `add<int>`) from ordinary function call in HIR.

### Completed Slices
- [x] Slice 1: Add `TemplateCallInfo` struct to `ir.hpp` with `source_template` and `template_args`; add `optional<TemplateCallInfo>` field to `CallExpr`
- [x] Slice 2: Populate `TemplateCallInfo` during lowering in `ast_to_hir.cpp` when resolving template call names; added `get_template_param_order()` helper
- [x] Slice 3: Add printer support — `CallExpr` printer shows `add<int>(...)` syntax when `template_info` is present
- [x] Slice 4: Add `cpp_hir_template_call_info_dump` test matching `add<int>(20, 22)` pattern

### What was added
- `ir.hpp`: `TemplateCallInfo` struct; `CallExpr::template_info` optional field
- `ast_to_hir.cpp`: `get_template_param_order()` static helper; template call info population at call sites
- `hir_printer.cpp`: template-aware `CallExpr` printer using `source_template<args>(...)` format
- `InternalTests.cmake`: `cpp_hir_template_call_info_dump` test

---

## Phase 4: Preserve consteval as HIR-reducible nodes
**Status: COMPLETE**

### Goal
Stop treating consteval reduction as a lowering-only side effect. Preserve consteval call intent in HIR.

### Completed Slices
- [x] Slice 1: Add `ConstevalCallInfo` to HIR (function name, args, result) in `ir.hpp`
- [x] Slice 2: Populate consteval call metadata during lowering in `ast_to_hir.cpp`
- [x] Slice 3: Add printer support — `--- consteval calls ---` section with `consteval fn<bindings>(args) = result` format
- [x] Slice 4: Add `cpp_hir_consteval_call_info_dump` and `cpp_hir_consteval_template_call_info_dump` tests

### What was added
- `ir.hpp`: `ConstevalCallInfo` struct with fn_name, const_args, template_bindings, result_value, result_expr, span
- `ir.hpp`: `Module::consteval_calls` vector
- `ast_to_hir.cpp`: records `ConstevalCallInfo` after successful `evaluate_consteval_call()` evaluation
- `hir_printer.cpp`: `print_consteval_call()` method, `--- consteval calls ---` section in dump output
- `InternalTests.cmake`: 2 new `--dump-hir` regex tests

### Exit criteria met
- Consteval call intent is representable in HIR (ConstevalCallInfo preserves fn name, args, bindings, result)
- Environment info attached: constant args and template bindings preserved
- `evaluate_consteval_call()` remains reusable — metadata recording is non-invasive
- Consteval reduction is no longer an invisible side effect: the `--- consteval calls ---` section makes it inspectable
- HIR can defer consteval when inputs are not ready yet (future Phase 5 work — the data structure supports it)

---

## Phase 5: HIR compile-time reduction loop
**Status: IN PROGRESS**

### Goal
Make compile-time behavior converge through repeated passes instead of fixed frontend ordering.

### Completed Slices
- [x] Slice 1: Add HIR pass entry point for compile-time reduction
- [x] Slice 2: Implement one iteration step for template instantiation

### What was added (Slice 1)
- `compile_time_pass.hpp`: `CompileTimePassStats`, `run_compile_time_reduction()`, `format_compile_time_stats()` API
- `compile_time_pass.cpp`: Scanner walks `expr_pool` for template calls + counts consteval records; reports convergence stats
- `c4cll.cpp`: Pass runs after lowering, before `--dump-hir` output; stats shown in `--- compile-time reduction ---` section
- `CMakeLists.txt`: Added `compile_time_pass.cpp` to build
- `InternalTests.cmake`: `cpp_hir_compile_time_reduction_stats` test

### What was added (Slice 2)
- `compile_time_pass.hpp`: Added `templates_pending` field to `CompileTimePassStats`
- `compile_time_pass.cpp`: `TemplateInstantiationStep` — walks CallExpr nodes with `template_info`, resolves callee DeclRef against module function list, counts resolved vs pending; fixpoint loop with kMaxIterations=8
- `compile_time_pass.cpp`: Updated `format_compile_time_stats()` to show "N template calls resolved" and optional pending count
- `InternalTests.cmake`: `cpp_hir_template_instantiation_resolved` test verifying "1 template call resolved" + "(converged)"

### Remaining Slices
- [ ] Slice 3: Implement one iteration step for consteval reduction
- [ ] Slice 4: Re-run until convergence or explicit iteration limit
- [ ] Slice 5: Surface structured diagnostics on irreducible required compile-time nodes

---

## Remaining Phases
- Phase 6: Materialization boundary
- Phase 7: Specialization identity and caching
