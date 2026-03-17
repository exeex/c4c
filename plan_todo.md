# Plan Execution State

## Baseline
- 1858/1858 tests passing (2026-03-17)

## Overall Assessment

Phase 6 is now implemented with a meaningful materialization boundary.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: largely complete for current scope

---

## What was done in this session (2026-03-17, session 4)

### Phase 6: Materialization boundary

**Goal**: Separate compile-time specialization from code emission. Make materialization a real policy step, not a trivial "materialize everything" pass.

**Implementation changes**:

1. **`ir.hpp`**: Added `consteval_only` (bool) and `template_origin` (string) fields to `Function`
   - `consteval_only`: true for functions that exist only for compile-time analysis
   - `template_origin`: records which template a function was instantiated from (e.g., "add" for add_i)

2. **`ast_to_hir.cpp`**: Non-template consteval functions are now lowered to HIR as declaration-only entities (no body) with `consteval_only = true`. Previously they were completely skipped during lowering. Template instantiations now set `template_origin` both during Phase 2 (eager lowering) and Phase 3 (deferred lowering callback).

3. **`compile_time_pass.cpp`**: `materialize_ready_functions()` now implements a real materialization policy:
   - `consteval_only` functions → `materialized = false` (not emitted)
   - All other functions → `materialized = true` (emitted as LLVM code)
   - Stats report both materialized and "compile-time only" counts

4. **`hir_printer.cpp`**: `print_function()` now shows `consteval_only`, `template<origin>`, and `[non-materialized]` annotations

5. **`hir_emitter.cpp`**: Already had the `if (!fn.materialized) continue;` check — no changes needed

### Test additions
- **`materialization_boundary.cpp`**: Test case with 2 consteval functions + 1 template instantiation + main. Verifies consteval functions are not emitted and template instantiations work correctly.
- **`cpp_hir_materialization_boundary`**: HIR dump test verifying "2 compile-time only" appears
- **`cpp_hir_template_origin`**: HIR dump test verifying `template<apply>` appears
- **`cpp_positive_sema_materialization_boundary_cpp`**: Runtime correctness test (auto-discovered)

### Exit criteria met
- Concrete codegen is a policy step (`materialize_ready_functions()` decides; codegen only emits materialized)
- Template semantics remain valid even if emission is delayed (non-materialized functions persist in HIR)
- consteval_only functions demonstrate the boundary: visible in HIR, absent from LLVM IR

### Files changed
- `src/frontend/hir/ir.hpp` — `consteval_only`, `template_origin` fields on Function
- `src/frontend/hir/ast_to_hir.cpp` — consteval function declarations in HIR, template_origin tracking
- `src/frontend/hir/compile_time_pass.cpp` — real materialization policy
- `src/frontend/hir/hir_printer.cpp` — materialization annotations
- `tests/internal/InternalTests.cmake` — 2 new HIR tests
- `tests/internal/cpp/postive_case/materialization_boundary.cpp` — new test case

---

## Added Testcases

The work now includes 19 HIR-oriented regression tests:

Previous 16 + 3 new:
- **`cpp_hir_materialization_boundary`** (NEW — proves consteval_only functions are non-materialized)
- **`cpp_hir_template_origin`** (NEW — proves template_origin tracking works)
- **`cpp_positive_sema_materialization_boundary_cpp`** (NEW — runtime correctness for materialization boundary)

---

## Phase 6: Define materialization boundary
**Status: COMPLETE**

### What is now implemented
- `consteval_only` flag on `Function` for compile-time-only entities
- `template_origin` tracking for template instantiation provenance
- `materialize_ready_functions()` implements a real policy (not trivial "materialize all")
- Consteval functions are visible in HIR (declaration-only) but not emitted
- HIR printer shows materialization annotations
- Codegen filters on `materialized` flag (was already in place)

### Exit criteria
- [x] Decide what counts as a materialized specialized function
- [x] Keep non-materialized template entities representable in HIR
- [x] Ensure codegen only consumes materialized entities
- [x] Keep compile-time reduction semantics separate from emission policy

---

## Recommended next milestone

Phase 5 remaining work: non-type template parameters (prerequisite for consteval→template feedback loop, the final convergence test).

Alternatively, begin exploring Phase 7 depth: cross-TU dedup testing, or link-time serialization of specialization keys.
