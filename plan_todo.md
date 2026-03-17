# Plan Execution State

## Baseline
- 1874/1874 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 11 is now implemented: default template arguments and empty angle brackets `<>`.

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
- **Phase 5 slice 10**: `sizeof(T)`, `__alignof__(T)`, and `sizeof(local_var)` in template bodies — template type parameter substitution applied to sizeof/alignof target types.
- **Phase 5 slice 11 (NEW)**: Default template arguments (`template<typename T = int>`, `template<int N = 5>`) and empty angle brackets `f<>()` for using all defaults.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; type parameter substitution in signatures and locals works; all three C++ named casts supported; sizeof/alignof template type substitution works; **default template arguments supported**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 15)

### Phase 5 slice 11: Default template arguments

**Changes**:
1. **AST** (`ast.hpp`): Added `template_param_has_default`, `template_param_default_types`, `template_param_default_values` arrays on `Node` for storing default template parameter values.
2. **AST** (`ast.hpp`): Added `has_template_args` boolean to distinguish `f()` from `f<>()`.
3. **Parser** (`declarations.cpp`): Template parameter default values are now captured instead of skipped. Type defaults use `parse_type_name()`, NTTP defaults parse integer literals.
4. **Parser** (`expressions.cpp`): Empty `<>` at call sites now accepted (previously rejected by `!template_args.empty()` check). Sets `has_template_args = true`.
5. **HIR** (`ast_to_hir.cpp`): `build_call_bindings()` and `build_call_nttp_bindings()` fill missing args from `fn_def`'s defaults. Template call detection updated to recognize `has_template_args` for empty `<>` calls.
6. **HIR** (`ast_to_hir.cpp`): Consteval inline binding path also fills defaults for missing params.

**Design**: Default template args are stored alongside the existing parallel arrays for template params. At call sites, explicit args are consumed first, then remaining params are filled from defaults. The `has_template_args` flag distinguishes `f()` (not a template call) from `f<>()` (template call with all defaults).

### Test additions
- **`template_default_args.cpp`**: Tests default type args, default NTTP args, partial defaults, empty `<>`, mixed type+NTTP defaults.

### Files changed
- `src/frontend/parser/ast.hpp` (3 new Node fields + has_template_args flag)
- `src/frontend/parser/declarations.cpp` (capture defaults instead of skip)
- `src/frontend/parser/expressions.cpp` (allow empty `<>`)
- `src/frontend/hir/ast_to_hir.cpp` (fill defaults in binding builders + detect empty `<>`)
- `tests/internal/cpp/postive_case/template_default_args.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Explicit template specialization (`template<> T foo<T>()`)
- Template return type substitution for more complex types (struct, fn_ptr)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
- `dynamic_cast` support (requires RTTI, lower priority)
