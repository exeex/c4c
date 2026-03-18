# Plan Execution State

## Baseline
- 1878/1878 tests passing (2026-03-18)

## Overall Assessment

Phase 5 slice 15 is now implemented: template function + template struct combined usage.

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
- **Phase 5 slice 15 (NEW)**: Template function + template struct combined usage — `Pair<T>` inside template function bodies deferred and resolved at HIR instantiation time.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — template function + template struct interop now works
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-18, session 19)

### Phase 5 slice 15: Template function + template struct combined usage

**Problem**: When parsing a template function body that references `Pair<T>` (where T is a template type parameter), the parser eagerly instantiated a broken `Pair_T_T` struct with unresolved field types. The HIR lowerer then failed when trying to use this struct for concrete instantiations like `sum<int>(Pair<int>)`.

**Changes**:
1. **TypeSpec** (`ast.hpp`): Added `tpl_struct_origin` and `tpl_struct_arg_refs` fields to mark pending template struct instantiations that depend on unresolved template type params.

2. **Parser** (`types.cpp`): When template struct type args include unresolved template type params (`base == TB_TYPEDEF`), defer instantiation — don't create the struct def, instead store origin/args metadata on the TypeSpec for HIR resolution.

3. **Parser** (`declarations.cpp`): `is_incomplete_object_type` now skips pending template structs (they're resolved at HIR level).

4. **Sema** (`validate.cpp`): `is_complete_object_type` also skips pending template structs.

5. **HIR** (`ast_to_hir.cpp`):
   - Collects template struct defs in Phase 1 (like template fn defs)
   - `resolve_pending_tpl_struct()` helper: parses arg refs, substitutes via tpl_bindings, computes concrete mangled name, instantiates struct with proper field types if needed
   - Called at all type substitution points: return types, param types, local var decls, cast expressions

**Design**: The key insight is that template function bodies share a single AST across all instantiations. Template struct references like `Pair<T>` can't be eagerly resolved because different instantiations need different concrete structs (`Pair_T_int` vs `Pair_T_long`). The deferred approach stores the template struct origin + arg references, and the HIR resolves them per-instantiation using the concrete type bindings.

### Test additions
- **`template_fn_struct.cpp`**: Tests template struct as param, return type, local var, and with NTTP.

### Files changed
- `src/frontend/parser/ast.hpp` (TypeSpec fields)
- `src/frontend/parser/types.cpp` (deferred instantiation)
- `src/frontend/parser/declarations.cpp` (incomplete type skip)
- `src/frontend/sema/validate.cpp` (complete type skip)
- `src/frontend/hir/ast_to_hir.cpp` (template struct collection, resolution helper, call sites)
- `tests/internal/cpp/postive_case/template_fn_struct.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template struct pointer fields (`Pair<int*>`)
- Template struct with `struct Pair<int>` keyword syntax (currently only typedef-style `Pair<int>` supported)
- Nested template struct usage (`Pair<Pair<int>>`)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)
