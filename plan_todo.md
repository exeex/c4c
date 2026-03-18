# Plan Execution State

## Baseline
- 1884/1884 tests passing (2026-03-18)

## Overall Assessment

Phase 5 slice 21 is now implemented: template argument deduction from call arguments.

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
- **Phase 5 slice 20**: Deferred method calls on template structs inside template function bodies — `resolve_pending_tpl_struct()` now registers and lowers methods when instantiating template structs, with full type parameter substitution in method signatures and bodies.
- **Phase 5 slice 21 (NEW)**: Template argument deduction — `add(1, 2)` deduces `T = int` from call arguments without explicit `<int>`. Supports literals (int, float, char, string), variables (params and locals), address-of, dereference, and pointer-to-T patterns.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — template argument deduction works for common patterns
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-18, session 25)

### Phase 5 slice 21: Template argument deduction from call arguments

**Changes**:
1. **ast_to_hir.cpp**: Added `try_infer_arg_type_for_deduction()` — infers TypeSpec from AST expression nodes (literals, variables, NK_ADDR, NK_DEREF, NK_CAST).
2. **ast_to_hir.cpp**: Added `try_deduce_template_type_args()` — matches call argument types against template function parameter types to deduce type bindings. Handles direct type param match (T x) and pointer-to-param match (T* ptr).
3. **ast_to_hir.cpp**: Added `deduction_covers_all_type_params()` and `fill_deduced_defaults()` helpers.
4. **ast_to_hir.cpp**: Extended `collect_template_instantiations()` — new branch for calls without explicit template args tries deduction, stores results in `deduced_template_calls_` map.
5. **ast_to_hir.cpp**: Extended call lowering in `lower_expr()` — checks `deduced_template_calls_` map to resolve deduced template calls to mangled instantiation names.
6. **ast_to_hir.cpp**: Added `DeducedTemplateCall` struct and `deduced_template_calls_` member map.

### Test additions
- **`template_arg_deduction.cpp`**: Tests template argument deduction — deduction from int literals, long variables, single arg, mixed with explicit args, pointer parameter deduction via &.

---

## Recommended next milestone

Potential next work:
- Template argument deduction in deferred template contexts (nested deduction)
- Method calls on template structs inside template function bodies without explicit template args
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)
- Operator overloading for template structs
