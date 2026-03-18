# Plan Execution State

## Baseline
- 1879/1879 tests passing (2026-03-18)

## Overall Assessment

Phase 5 slice 16 is now implemented: nested template struct usage and >> disambiguation.

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
- **Phase 5 slice 16 (NEW)**: Nested template struct + >> disambiguation — `Pair<Pair<int>>`, `Box<Pair<T>>` in template functions, parser >> token splitting.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — nested template structs and >> disambiguation now work
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-18, session 20)

### Phase 5 slice 16: Nested template struct + >> disambiguation

Three fixes in one slice:

**Fix 1: `>>` token splitting in template argument contexts**
- Added `check_template_close()`, `match_template_close()`, `expect_template_close()` helpers to Parser
- When `>>` (GreaterGreater) is encountered where `>` is expected (closing a template arg list), the token is split: mutate current token to `>` and don't advance, leaving the remaining `>` for the outer context
- Applied to: template struct type arg parsing (types.cpp), function template arg parsing (expressions.cpp)

**Fix 2: Nested pending template struct propagation in parser**
- When parsing `Box<Pair<T>>` in a template function body, `Pair<T>` is deferred (pending). Previously the outer `Box` check only looked for `TB_TYPEDEF` as unresolved — now also checks `tpl_struct_origin` to detect nested pending template structs
- Arg_refs encoding: nested pending template structs use `@origin:inner_args` format (e.g., `@Pair:T` for `Pair<T>`)
- HIR resolver: when an arg_ref starts with `@`, recursively resolves the inner pending template struct before using it as a type argument

**Fix 3: Template param attachment corruption**
- **Root cause**: After parsing `template<typename T> T unbox_sum(Box<Pair<T>> bp)`, the code checked `struct_defs_.back()` to attach template params. But `struct_defs_.back()` was `Pair_T_Pair_T_int` (from a previous non-template function), not the template struct being defined.
- **Symptom**: `Pair_T_Pair_T_int` got `n_template_params=1` incorrectly, causing HIR to skip it as a template def instead of lowering it as a concrete struct.
- **Fix**: Save `struct_defs_.size()` before parsing the template body; only check the last struct def if new structs were added during this parse.

### Test additions
- **`template_struct_nested.cpp`**: Tests `Box<Pair<int>>`, `Pair<Pair<int>>`, template functions with nested template struct params and return types.

### Files changed
- `src/frontend/parser/parser.hpp` (new template close helpers)
- `src/frontend/parser/parse.cpp` (implement template close helpers)
- `src/frontend/parser/types.cpp` (>> handling, nested pending struct detection, arg_refs encoding)
- `src/frontend/parser/expressions.cpp` (>> handling in function template args)
- `src/frontend/parser/declarations.cpp` (template param attachment fix)
- `src/frontend/hir/ast_to_hir.cpp` (recursive nested pending template struct resolution)
- `tests/internal/cpp/postive_case/template_struct_nested.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template struct pointer fields (`Pair<int*>`)
- Template struct with `struct Pair<int>` keyword syntax (currently only typedef-style `Pair<int>` supported)
- Triple-nested template structs (`Pair<Pair<Pair<int>>>` — would need `>>>` splitting or recursive `>>` handling)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- `dynamic_cast` support (requires RTTI, lower priority)
