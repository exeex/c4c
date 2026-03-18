# Plan Execution State

## Baseline
- 1881/1881 tests passing (2026-03-18)

## Overall Assessment

Phase 5 slice 18 is now implemented: `class` keyword as synonym for `struct` in template contexts.

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
- **Phase 5 slice 18 (NEW)**: `class` keyword as synonym for `struct` — `KwClass` token, `template<class T>`, `class Name<args>` instantiation, `template<...> class Name { ... }` definitions.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — class keyword synonym for struct now works
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-18, session 22)

### Phase 5 slice 18: `class` keyword as synonym for `struct` in template contexts

**Changes**:
1. **token.hpp/token.cpp**: Added `KwClass` token kind, mapped `"class"` → `KwClass` in C++ profile
2. **common.cpp**: Added `KwClass` to `is_type_kw()` so it's recognized as a type-start token
3. **types.cpp**: `parse_base_type()` handles `KwClass` identically to `KwStruct` (sets `has_struct=true`); nested struct field parsing also accepts `KwClass`
4. **declarations.cpp**: Template parameter parsing updated — `template<class T>` now matches `KwClass` token instead of identifier-string comparison (since `class` is now a keyword)

### Test additions
- **`template_class_keyword.cpp`**: Tests `class Pair<int>`, `class Array<int, 3>`, function return/param types with `class` keyword, mixing `class`/`struct`/bare syntax, `public:` access specifier for clang compatibility.

---

## Recommended next milestone

Potential next work:
- Template struct member functions (methods)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)

Already verified working (no action needed):
- Template struct pointer fields (`Pair<int*>`) — works
- `struct Pair<int>` / `class Pair<int>` keyword syntax — implemented in slices 17-18
- Triple-nested template structs (`Box<Box<Box<int>>>`) — works via recursive >> splitting
- Multi-type param template structs (`KVPair<int, long>`) — works
