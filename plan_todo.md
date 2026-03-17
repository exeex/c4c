# Plan Execution State

## Baseline
- 1872/1872 tests passing (2026-03-17)

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
- **Phase 5 slice 9 (NEW)**: `reinterpret_cast<T>(expr)` and `const_cast<T>(expr)` — all three C++ named casts now share the same parser path, producing NK_CAST nodes.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; type parameter substitution in signatures and locals works; **all three C++ named casts supported**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 13)

### Phase 5 slice 9: `reinterpret_cast<T>(expr)` and `const_cast<T>(expr)` support

**Changes**:
1. **Lexer**: Added `KwReinterpretCast` and `KwConstCast` token kinds; `reinterpret_cast` and `const_cast` recognized as keywords in C++ profiles
2. **Parser**: `parse_primary()` extended to handle all three named casts (`static_cast`, `reinterpret_cast`, `const_cast`) via a unified check — all produce NK_CAST nodes

**Design**: All C++ named casts reuse the existing NK_CAST node kind, same as C-style `(T)expr`. The semantic differences between the cast kinds are not enforced at this stage — they all lower to the same coercion in codegen. This is sufficient for correctness since the compiler already handles pointer/integer/const coercions uniformly.

### Test additions
- **`reinterpret_const_cast.cpp`**: Tests ptr-to-int, int-to-ptr, ptr-to-ptr reinterpret_cast; const_cast removing and adding const.

### Files changed
- `src/frontend/lexer/token.hpp` (KwReinterpretCast, KwConstCast token kinds)
- `src/frontend/lexer/token.cpp` (debug names + keyword recognition)
- `src/frontend/parser/expressions.cpp` (unified named cast parsing)
- `tests/internal/cpp/postive_case/reinterpret_const_cast.cpp` (new)

---

## Recommended next milestone

Potential next work:
- Template return type substitution for more complex types (struct, fn_ptr)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
- `dynamic_cast` support (requires RTTI, lower priority)
