# Plan Execution State

## Baseline
- 1883/1883 tests passing (2026-03-18)

## Overall Assessment

Phase 5 slice 20 is now implemented: deferred method calls on template structs inside template function bodies.

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
- **Phase 5 slice 11**: Default template arguments (`template<typename T = int>`, `template<int N = 5>`) and empty angle brackets `f<>()` for using all defaults.
- **Phase 5 slice 12**: Explicit template specialization (`template<> int add<int>(...)`) — specialization bodies override generic template for matching type/NTTP args.
- **Phase 5 slice 13**: Template struct support — `template<typename T> struct Pair { T first; T second; };` with parser-level instantiation.
- **Phase 5 slice 14**: Template struct NTTP support — `template<typename T, int N> struct Array { T data[N]; };` and NTTP-only structs. NTTP values substitute into array dimensions via `array_size_expr` propagation. Default template arguments also work for template struct NTTP params.
- **Phase 5 slice 15**: Template function + template struct combined usage — `Pair<T>` inside template function bodies deferred and resolved at HIR instantiation time.
- **Phase 5 slice 16**: Nested template struct + >> disambiguation — `Pair<Pair<int>>`, `Box<Pair<T>>` in template functions, parser >> token splitting.
- **Phase 5 slice 17**: `struct Pair<int>` keyword syntax — `struct` keyword before template struct types now triggers template instantiation.
- **Phase 5 slice 18**: `class` keyword as synonym for `struct` — `KwClass` token, `template<class T>`, `class Name<args>` instantiation, `template<...> class Name { ... }` definitions.
- **Phase 5 slice 19**: Struct member functions (methods) — non-template and template struct methods lowered as standalone functions with implicit `this` pointer, method calls rewritten to mangled function calls.
- **Phase 5 slice 20 (NEW)**: Deferred method calls on template structs inside template function bodies — `resolve_pending_tpl_struct()` now registers and lowers methods when instantiating template structs, with full type parameter substitution in method signatures and bodies.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — template struct methods work in deferred contexts
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-18, session 24)

### Phase 5 slice 20: Deferred method calls on template structs in template functions

**Changes**:
1. **ast_to_hir.cpp**: `resolve_pending_tpl_struct()` now registers and immediately lowers methods from template struct definitions when instantiating concrete structs. Methods are added to `struct_methods_` map and lowered via `lower_struct_method()` with type bindings.
2. **ast_to_hir.cpp**: `lower_struct_method()` extended with optional `tpl_bindings` and `nttp_bindings` parameters for template type substitution in return type, parameter types, and method body expressions.

### Test additions
- **`template_deferred_method.cpp`**: Tests deferred method calls on template structs inside template function bodies — dot operator, arrow operator, method with argument, multiple type instantiations (int, long).

---

## Recommended next milestone

Potential next work:
- Template argument deduction from function arguments (implicit template args)
- Method calls on template structs inside template function bodies without explicit template args
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)
- Operator overloading for template structs
