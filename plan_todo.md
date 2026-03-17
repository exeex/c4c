# Plan Execution State

## Baseline
- 1869/1869 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 6 is now implemented: mixed type+NTTP forwarding in deferred template instantiation chains.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7**: Specialization identity is stable, hashable, serializable via LLVM named metadata.
- **Phase 5 slice 3**: Non-type template parameters (NTTP) — `template<int N>` etc. supported end-to-end.
- **Phase 5 slice 4**: Mixed type + NTTP params (`template<typename T, int N>`) and consteval NTTP functions supported end-to-end.
- **Phase 5 slice 5**: NTTP forwarding in deferred chains — `template<int N> f() { return g<N>(); }` where g is another template or consteval template.
- **Phase 5 slice 6 (NEW)**: Mixed type+NTTP forwarding in deferred chains — `template<typename T, int N> f() { g<T, N>(); }` works end-to-end. No code changes were needed; the existing infrastructure from slices 4 and 5 already handles mixed forwarding correctly. Test validates the full flow through 3-level deferred chains.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; **mixed type+NTTP forwarding in deferred chains works**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 10)

### Phase 5 slice 6: Mixed type+NTTP forwarding in deferred template instantiation chains

**Goal**: Support forwarding both typename and NTTP params as template arguments in nested calls, e.g. `compute<T, N>()` inside `template<typename T, int N> outer()`.

**Finding**: No code changes were needed. The existing infrastructure from Phase 5 slices 4 and 5 already handles this case correctly:
- Parser correctly distinguishes type args (via `is_type_start()`) from forwarded NTTP identifiers
- `build_call_nttp_bindings()` resolves forwarded NTTP names through enclosing NTTP bindings
- Type bindings resolve through enclosing `ctx->tpl_bindings`
- `PendingConstevalExpr` correctly captures both `tpl_bindings` and `nttp_bindings`
- The compile-time pass evaluation callback properly passes both to the consteval interpreter

### Test additions
- **`mixed_type_nttp_forwarding.cpp`**: Tests mixed type+NTTP forwarding through 3-level deferred chains:
  - `outer<int, 5>` → `compute<int, 5>` → `inner<int, 5>` (consteval, returns 25)
  - `outer<long, 7>` → `compute<long, 7>` → `inner<long, 7>` (consteval, returns 49)
  - `apply<long, 3, 10>` → `deferred_op<long, 3, 10>` → `sum_typed<long, 3, 10>` (consteval, returns 13)
  - `apply<int, 100, 200>` → `deferred_op<int, 100, 200>` → `sum_typed<int, 100, 200>` (consteval, returns 300)
  - `tagged<int, 10>` and `tagged<long, 10>`: same NTTP, different type → distinct instantiations

### Files changed
- `tests/internal/cpp/postive_case/mixed_type_nttp_forwarding.cpp` (new)

---

## Recommended next milestone

Potential next work:
- `static_cast` support in consteval interpreter (currently fails for deferred consteval with static_cast)
- Template return type substitution (T as return type in template functions)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
