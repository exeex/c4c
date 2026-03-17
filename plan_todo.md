# Plan Execution State

## Baseline
- 1867/1867 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 4 is now implemented: mixed type + NTTP template parameters and consteval NTTP support.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7**: Specialization identity is stable, hashable, serializable via LLVM named metadata.
- **Phase 5 slice 3**: Non-type template parameters (NTTP) — `template<int N>` etc. supported end-to-end.
- **Phase 5 slice 4 (NEW)**: Mixed type + NTTP params (`template<typename T, int N>`) and consteval NTTP functions supported end-to-end. NTTP bindings flow through consteval evaluation (both eager and deferred paths). HIR printer distinguishes `int N` from `typename T` params.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 8)

### Phase 5 slice 4: Mixed type + NTTP template parameters and consteval NTTP

**Goal**: Support `template<typename T, int N>` mixed params and `consteval` functions with NTTP.

**Implementation changes**:

1. **`ir.hpp`**: Added `param_is_nttp` vector to `HirTemplateDef`. Added `nttp_bindings` to `PendingConstevalExpr`.

2. **`ast_to_hir.cpp`**: Populate `param_is_nttp` when building template defs. In consteval call handling, separate NTTP args from type args and build `ce_nttp_bindings` map. Set `arg_env.nttp_bindings` for consteval evaluation. Store NTTP bindings in PendingConstevalExpr. Updated deferred_consteval lambda to accept and propagate NTTP bindings.

3. **`consteval.hpp`**: Added `nttp_bindings` pointer to `ConstEvalEnv`. Added NTTP lookup (step 6) to `lookup()` method.

4. **`compile_time_pass.hpp`**: Updated `DeferredConstevalEvalFn` signature to include `NttpBindings`.

5. **`compile_time_pass.cpp`**: Pass `pce->nttp_bindings` to eval callback.

6. **`hir_printer.cpp`**: Show `int N` for NTTP params instead of `typename N`.

### Test additions
- **`template_mixed_params`**: Tests mixed type+NTTP params in various orderings.
- **`consteval_nttp`**: Tests consteval functions with pure NTTP, mixed type+NTTP, and multiple NTTP.

### Files changed
- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/hir/compile_time_pass.hpp`
- `src/frontend/hir/compile_time_pass.cpp`
- `src/frontend/hir/hir_printer.cpp`
- `src/frontend/sema/consteval.hpp`
- `tests/internal/cpp/postive_case/template_mixed_params.cpp` (new)
- `tests/internal/cpp/postive_case/consteval_nttp.cpp` (new)
- `tests/internal/InternalTests.cmake`

---

## Recommended next milestone

Potential next work:
- NTTP in deferred consteval chains (consteval NTTP inside deferred template instantiation)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
