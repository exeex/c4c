# Plan Execution State

## Baseline
- 1877/1877 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 14 is now implemented: template struct with NTTP parameters (`template<typename T, int N> struct Array { T data[N]; }`).

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
- **Phase 5 slice 14 (NEW)**: Template struct NTTP support — `template<typename T, int N> struct Array { T data[N]; };` and NTTP-only structs. NTTP values substitute into array dimensions via `array_size_expr` propagation. Default template arguments also work for template struct NTTP params.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; type parameter substitution in signatures and locals works; all three C++ named casts supported; sizeof/alignof template type substitution works; default template arguments supported; explicit template specialization supported; **template struct support with NTTP added**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 18)

### Phase 5 slice 14: Template struct NTTP support

**Changes**:
1. **Parser** (`types.cpp`): Template struct instantiation now handles both type and NTTP arguments:
   - Separate `type_bindings` and `nttp_bindings` vectors
   - NTTP params parsed as integer expressions (with sign support)
   - Default template arguments filled in for unspecified params
   - Mangled name includes NTTP values (e.g., `Array_T_int_N_4`)
   - Field cloning substitutes NTTP values into array dimensions via `array_size_expr` check

**Design**: When `array_size_expr` on a field TypeSpec is an NK_VAR node matching an NTTP param name, the first array dimension is replaced with the NTTP value. This uses the existing `array_size_expr` infrastructure from `parse_one_array_dim` which stores the unevaluated expression when `eval_const_int` fails during template struct body parsing.

### Test additions
- **`template_struct_nttp.cpp`**: Tests `Array<int, 4>`, `Array<long, 2>`, and NTTP-only `IntBuf<3>`.

### Files changed
- `src/frontend/parser/types.cpp` (NTTP argument parsing, mangling, field substitution)
- `tests/internal/cpp/postive_case/template_struct_nttp.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template struct pointer fields (`Pair<int*>`)
- Template struct used as function parameter/return type
- Template struct with `struct Pair<int>` keyword syntax (currently only typedef-style `Pair<int>` supported)
- Template function + template struct combined usage
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)
