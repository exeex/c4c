# Plan Execution State

## Baseline
- 1875/1875 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 12 is now implemented: explicit template specialization (`template<>`).

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
- **Phase 5 slice 12 (NEW)**: Explicit template specialization (`template<> int add<int>(...)`) — specialization bodies override generic template for matching type/NTTP args.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; type parameter substitution in signatures and locals works; all three C++ named casts supported; sizeof/alignof template type substitution works; default template arguments supported; **explicit template specialization supported**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 16)

### Phase 5 slice 12: Explicit template specialization

**Changes**:
1. **AST** (`ast.hpp`): Added `is_explicit_specialization` flag on `Node` for `template<>` functions.
2. **Parser** (`declarations.cpp`): Detect `template<>` (empty param list) at top-level, set flag, parse specialization type args `<Args>` after function name.
3. **Parser** (`parser.hpp`): Added `parsing_explicit_specialization_` context flag.
4. **Sema** (`validate.cpp`): Skip conflicting-types error for explicit specializations (they intentionally differ in signature from the generic template).
5. **HIR** (`ast_to_hir.cpp`):
   - `template_fn_specs_` map: mangled_name → specialization AST node
   - `mangle_specialization()`: builds mangled name from specialization's type args mapped to generic template's param names
   - Phase 1.6: collects explicit specializations into `template_fn_specs_`
   - Phase 2: checks `template_fn_specs_` before lowering each instantiation — uses specialization body when matched
   - Phase 3 (deferred): same specialization check in the deferred lowering callback
   - Explicit specializations are skipped during normal function lowering (they're only used via their mangled template name)

**Design**: Explicit specializations are stored by mangled name (same key as generic instantiations). When lowering a template instantiation, the mangled name is looked up in `template_fn_specs_` first. If found, the specialization body is lowered directly (no template bindings needed since it's already concrete). This works for both eager (Phase 2) and deferred (Phase 3) instantiation paths.

### Test additions
- **`template_explicit_spec.cpp`**: Tests `template<> int add<int>(...)` overriding generic `add<T>`, `template<> char identity<char>(...)` overriding generic, and that non-specialized instantiations still use the generic body.

### Files changed
- `src/frontend/parser/ast.hpp` (1 new Node flag)
- `src/frontend/parser/parser.hpp` (1 new context flag)
- `src/frontend/parser/declarations.cpp` (template<> detection + spec arg parsing)
- `src/frontend/sema/validate.cpp` (skip conflicting-types for specializations)
- `src/frontend/hir/ast_to_hir.cpp` (specialization collection, lookup, mangle helper)
- `tests/internal/cpp/postive_case/template_explicit_spec.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template return type substitution for more complex types (struct, fn_ptr)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
- `dynamic_cast` support (requires RTTI, lower priority)
