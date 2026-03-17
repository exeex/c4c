# Plan Execution State

## Baseline
- 1873/1873 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 9 is now implemented: `reinterpret_cast<T>(expr)` and `const_cast<T>(expr)` C++ named cast syntax.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7**: Specialization identity is stable, hashable, serializable via LLVM named metadata.
- **Phase 5 slice 3**: Non-type template parameters (NTTP) — `template<int N>` etc. supported end-to-end.
- **Phase 5 slice 4**: Mixed type + NTTP params (`template<typename T, int N>`) and consteval NTTP functions supported end-to-end.
- **Phase 5 slice 5**: NTTP forwarding in deferred chains — `template<int N> f() { return g<N>(); }` where g is another template or consteval template.
- **Phase 5 slice 6**: Mixed type+NTTP forwarding in deferred chains works end-to-end.
- **Phase 5 slice 7**: Template type parameter substitution in function signatures and locals.
- **Phase 5 slice 8**: `static_cast<T>(expr)` support — C++ named cast syntax parsed into NK_CAST nodes, works in regular code, consteval, and template contexts.
- **Phase 5 slice 9**: `reinterpret_cast<T>(expr)` and `const_cast<T>(expr)` — all three C++ named casts now share the same parser path, producing NK_CAST nodes.
- **Phase 5 slice 10 (NEW)**: `sizeof(T)`, `__alignof__(T)`, and `sizeof(local_var)` in template bodies — template type parameter substitution applied to sizeof/alignof target types.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; type parameter substitution in signatures and locals works; **all three C++ named casts supported**; sizeof/alignof template type substitution works
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 14)

### Phase 5 slice 10: `sizeof(T)` and `__alignof__(T)` template substitution

**Changes**:
1. **HIR lowering** (`ast_to_hir.cpp`): `NK_SIZEOF_TYPE` and `NK_ALIGNOF_TYPE` now substitute template type parameters from `ctx->tpl_bindings` before computing size/alignment. This handles `sizeof(T)`, `__alignof__(T)`, and `sizeof(local_var)` where the local has template-substituted type.

**Design**: When the sizeof/alignof target type is a typedef matching a template type parameter binding, the base type and tag are replaced with the concrete type. Pointer levels from the concrete type are added to any existing pointer level.

### Test additions
- **`template_sizeof.cpp`**: Tests `sizeof(T)` for char/short/int/long/long long/pointer types; `__alignof__(T)` for char/int/long; `sizeof(local_var)` with template-typed locals.

### Files changed
- `src/frontend/hir/ast_to_hir.cpp` (sizeof/alignof template type substitution)
- `tests/internal/cpp/postive_case/template_sizeof.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template return type substitution for more complex types (struct, fn_ptr)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
- `dynamic_cast` support (requires RTTI, lower priority)
