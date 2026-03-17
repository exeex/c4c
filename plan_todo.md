# Plan Execution State

## Baseline
- 1841/1841 tests passing (2026-03-17)

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

## Next: Phase 4: Preserve consteval as HIR-reducible nodes

### Goal
Stop treating consteval reduction as a lowering-only side effect. Preserve consteval call intent in HIR.

### Planned Slices
- [ ] Slice 1: Add `ConstevalCallInfo` to HIR (function name, args, result) in `ir.hpp`
- [ ] Slice 2: Populate consteval call metadata during lowering
- [ ] Slice 3: Add printer support for consteval call nodes
- [ ] Slice 4: Add test for `--dump-hir` consteval call inspection

---

## Remaining Phases
- Phase 5: HIR compile-time reduction loop
- Phase 6: Materialization boundary
- Phase 7: Specialization identity and caching
