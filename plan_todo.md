# Plan Execution State

## Baseline
- 1876/1876 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 13 is now implemented: basic template struct support (`template<typename T> struct Pair { T first; T second; };`).

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
- **Phase 5 slice 13 (NEW)**: Template struct support — `template<typename T> struct Pair { T first; T second; };` with parser-level instantiation. `Pair<int>` generates a concrete struct `Pair_T_int` with substituted field types.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; type parameter substitution in signatures and locals works; all three C++ named casts supported; sizeof/alignof template type substitution works; default template arguments supported; explicit template specialization supported; **template struct support added**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 17)

### Phase 5 slice 13: Template struct support

**Changes**:
1. **Parser** (`parser.hpp`): Added `template_struct_defs_` map (struct name → NK_STRUCT_DEF*) and `instantiated_template_structs_` set for tracking already-instantiated mangled names.
2. **Parser** (`declarations.cpp`): After `attach_template_params` in template parsing, detect NK_STRUCT_DEF in `struct_defs_` and:
   - Store in `template_struct_defs_`
   - Register struct name as typedef (C++ mode: struct names usable as type names)
3. **Parser** (`types.cpp`): In `parse_base_type()` typedef resolution path, detect template struct usage followed by `<Args>`:
   - Parse template type arguments
   - Build mangled struct tag name (e.g., `Pair_T_int`)
   - Create concrete NK_STRUCT_DEF with cloned fields and substituted types
   - Register in `struct_defs_`, `struct_tag_def_map_`, `defined_struct_tags_`
4. **HIR** (`ast_to_hir.cpp`): Skip template struct defs (n_template_params > 0) in `lower_struct_def()` — only instantiated concrete structs are lowered.

**Design**: Parser-level instantiation approach — when `Pair<int>` is encountered as a type, the parser creates a concrete NK_STRUCT_DEF with mangled name `Pair_T_int` and cloned/substituted fields. HIR sees this as a regular struct def and lowers it normally with no special handling needed.

### Test additions
- **`template_struct.cpp`**: Tests `template<typename T> struct Pair { T first; T second; };` with int and long instantiations, plus a second template struct `Box<T>` with single field.

### Files changed
- `src/frontend/parser/parser.hpp` (2 new data structures)
- `src/frontend/parser/declarations.cpp` (template struct registration)
- `src/frontend/parser/types.cpp` (template struct instantiation in parse_base_type)
- `src/frontend/hir/ast_to_hir.cpp` (skip template struct defs)
- `tests/internal/cpp/postive_case/template_struct.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template struct with NTTP parameters (`template<typename T, int N> struct Array { T data[N]; }`)
- Template struct pointer fields (`Pair<int*>`)
- Template struct used as function parameter/return type
- Template struct with `struct Pair<int>` keyword syntax (currently only typedef-style `Pair<int>` supported)
- Template function + template struct combined usage
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)
