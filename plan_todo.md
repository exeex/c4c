# Plan Execution State

## Baseline
- 1865/1865 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 3 is now implemented: non-type template parameters (NTTP).

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7**: Specialization identity is stable, hashable, serializable via LLVM named metadata.
- **Phase 5 slice 3 (NEW)**: Non-type template parameters (NTTP) — `template<int N>` etc. supported end-to-end: parser, sema validator, HIR lowering, mangling, and specialization keys.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 7)

### Phase 5 slice 3: Non-type template parameters (NTTP)

**Goal**: Support `template<int N>`, `template<int A, int B>` etc. where template parameters are integer constants instead of types.

**Implementation changes**:

1. **`ast.hpp`**: Added `template_param_is_nttp` (bool array), `template_arg_is_value` (bool array), `template_arg_values` (long long array) to Node for NTTP metadata.

2. **`declarations.cpp`**: Template parameter parsing now detects non-`typename`/`class` params. If a type keyword (int, unsigned, etc.) precedes the param name, it's marked as NTTP. NTTP params are NOT injected as typedef names.

3. **`expressions.cpp`**: Template argument parsing at call sites now supports integer constant expressions (including negative values via unary minus) alongside type arguments.

4. **`validate.cpp`**: NTTP param names are injected as int-typed locals in function scope so the sema validator doesn't reject references to them as undeclared.

5. **`ir.hpp`**: Added `NttpBindings` type alias. Added `nttp_bindings` to `HirTemplateInstantiation`. Added `make_specialization_key` overload that includes NTTP values.

6. **`ast_to_hir.cpp`**:
   - `FunctionCtx` gains `nttp_bindings` field
   - `build_call_nttp_bindings()` extracts NTTP value bindings from call-site args
   - `mangle_template_name()` includes NTTP values in mangled names (negative values use 'n' prefix)
   - `record_instance()` and `collect_template_instantiations()` propagate NTTP bindings
   - `lower_function()` accepts optional NTTP bindings and populates `ctx.nttp_bindings`
   - NK_VAR expression lowering: resolves NTTP param names to IntLiteral constants
   - NK_VAR type inference: resolves NTTP param names to TB_INT type

**Output example**:
```llvm
; template-origin: get_value
; spec-key: get_value<N=42>
define i32 @get_value_42() {
entry:
  ret i32 42
}

; template-origin: sum_params
; spec-key: sum_params<A=3,B=4>
define i32 @sum_params_3_4() {
entry:
  %t0 = add i32 3, 4
  ret i32 %t0
}
```

### Test additions
- **`template_nttp`**: Tests basic NTTP usage, NTTP in expressions, multiple NTTP, and negative values.

### Files changed
- `src/frontend/parser/ast.hpp` — added NTTP fields to Node
- `src/frontend/parser/declarations.cpp` — NTTP param detection in template<> header
- `src/frontend/parser/expressions.cpp` — integer constant template arguments at call sites
- `src/frontend/sema/validate.cpp` — inject NTTP params as int locals
- `src/frontend/hir/ir.hpp` — NttpBindings, updated HirTemplateInstantiation, spec key overload
- `src/frontend/hir/ast_to_hir.cpp` — NTTP bindings throughout template instantiation pipeline
- `tests/internal/cpp/postive_case/template_nttp.cpp` — new test
- `tests/internal/InternalTests.cmake` — registered new test

---

## Recommended next milestone

Potential next work:
- Mixed type + NTTP template parameters (e.g., `template<typename T, int N>`)
- NTTP in consteval template functions
- Phase 2-4 remaining: deeper template/consteval HIR preservation
