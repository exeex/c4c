# Plan Execution State

## Baseline
- 1868/1868 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 5 is now implemented: NTTP forwarding in deferred template instantiation chains.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7**: Specialization identity is stable, hashable, serializable via LLVM named metadata.
- **Phase 5 slice 3**: Non-type template parameters (NTTP) — `template<int N>` etc. supported end-to-end.
- **Phase 5 slice 4**: Mixed type + NTTP params (`template<typename T, int N>`) and consteval NTTP functions supported end-to-end.
- **Phase 5 slice 5 (NEW)**: NTTP forwarding in deferred chains — `template<int N> f() { return g<N>(); }` where g is another template or consteval template. Parser recognizes identifier NTTP args; HIR resolves them through enclosing NTTP bindings; compile-time pass propagates NTTP bindings during deferred instantiation.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; **NTTP forwarding in deferred chains works**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 9)

### Phase 5 slice 5: NTTP forwarding in deferred template instantiation chains

**Goal**: Support forwarding NTTP params as template arguments in nested calls, e.g. `compute<N>()` inside `template<int N> outer()`.

**Root cause of gap**: Parser only recognized type names and integer literals as template arguments. Plain identifiers (forwarded NTTP names) were rejected, causing parse errors.

**Implementation changes**:

1. **`ast.hpp`**: Added `template_arg_nttp_names` (parallel array of `const char*`) to Node. Non-null entries indicate forwarded NTTP names needing HIR-time resolution.

2. **`expressions.cpp`**: Added third case in template argument parsing for `Identifier && !is_type_start()` — stores forwarded name in `template_arg_nttp_names[i]`.

3. **`ast_to_hir.cpp`**:
   - `build_call_nttp_bindings()` now accepts optional `enclosing_nttp` param. Resolves forwarded NTTP names through enclosing bindings.
   - `has_forwarded_nttp()` helper detects calls with unresolved forwarded NTTPs.
   - `collect_template_instantiations()` passes enclosing NTTP bindings when resolving inner calls. Skips recording for unresolved forwarded NTTPs (deferred).
   - `collect_consteval_template_instantiations()` same treatment.
   - `resolve_template_call_name()` accepts and passes enclosing NTTP.
   - Consteval NTTP arg resolution in `lower_expr` checks `template_arg_nttp_names` and resolves through `ctx->nttp_bindings`.
   - `deferred_lower` lambda passes NTTP bindings to `lower_function`.

4. **`ir.hpp`**: Added `NttpBindings nttp_args` to `TemplateCallInfo`. Moved `NttpBindings` typedef before `TemplateCallInfo`.

5. **`compile_time_pass.hpp`**: Updated `DeferredInstantiateFn` signature to include `NttpBindings`.

6. **`compile_time_pass.cpp`**: Deferred instantiation now skips NTTP params in type bindings, uses `tci.nttp_args`, and passes NTTP to callback. Spec key includes NTTP when present.

### Test additions
- **`deferred_consteval_nttp.cpp`**: Tests pure NTTP forwarding through deferred chain (`outer<5>` → `compute_square<5>` → `inner_square<5>` consteval) and multiple NTTP forwarding (`wrapper<3,10>` → `deferred_sum<3,10>` → `sum_nttp<3,10>` consteval).

### Files changed
- `src/frontend/parser/ast.hpp`
- `src/frontend/parser/expressions.cpp`
- `src/frontend/hir/ir.hpp`
- `src/frontend/hir/ast_to_hir.cpp`
- `src/frontend/hir/compile_time_pass.hpp`
- `src/frontend/hir/compile_time_pass.cpp`
- `tests/internal/cpp/postive_case/deferred_consteval_nttp.cpp` (new)
- `tests/internal/InternalTests.cmake`

---

## Recommended next milestone

Potential next work:
- Mixed type+NTTP forwarding in deferred chains (e.g. `template<typename T, int N> f() { g<T, N>(); }`)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
