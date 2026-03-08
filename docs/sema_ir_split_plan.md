# Sema / IR Split Migration Plan

## Goal

Refactor current pipeline from:

- `Parser -> IRBuilder(semantic + llvm lowering mixed)`

to:

- `Parser -> Sema (typed program) -> IR (project IR/DAG) -> Backend`
- keep `LLVM IR` path as a backend/debug path, not semantic core

Constraints for this migration:

- Keep old code and old pipeline intact during migration (no deletion yet).
- Build a parallel new pipeline with a separate `main`.
- Functional behavior must match old pipeline before cutover.
- Test validation excludes `llvm_gcc_c_torture` in early phases.

## Current-State Problems

- `src/frontend/sema/ir_builder.cpp` contains both semantic decisions and LLVM emission.
- Hard to add a non-LLVM backend without duplicating semantic logic.
- Hard to test semantic correctness independently from LLVM text output.

## Target Layout (aligned with ccc split intent)

```txt
src/
  frontend/
    sema/
      sema_context.hpp/.cpp
      type_check.hpp/.cpp
      decl_resolve.hpp/.cpp
      const_eval.hpp/.cpp
      sema_driver.hpp/.cpp
  ir/
    hir.hpp/.cpp              # typed frontend IR (minimal first)
    dag.hpp/.cpp              # optional early stub, expand later
    lowering/
      ast_to_hir.cpp
      hir_to_dag.cpp
  codegen/
    llvm/
      hir_to_llvm.cpp         # wraps existing IRBuilder behavior gradually
    native/
      # future backend
  apps/
    tiny-c2ll-legacy.cpp      # existing driver flow (unchanged)
    tiny-c2ll-next.cpp        # new flow
```

## Phase Plan

### Phase 0: Safety Rails and Parallel Entry

Deliverables:

- New executable target (example: `frontend_cxx_next`) with separate `main`.
- Existing `frontend_cxx_stage1` unchanged and still default in existing tests.
- Shared CLI flags between old/new mains (`--lex-only`, `--parse-only`, `-o`).

Acceptance:

- Both binaries build.
- Legacy tests still run via old binary.

### Phase 1: Extract Semantic Facade (No behavior change)

Deliverables:

- Add `frontend/sema/sema_driver.{hpp,cpp}`.
- Move semantic-only helper logic out of `ir_builder.cpp` into sema module.
- `ir_builder` now depends on sema outputs/helpers instead of owning logic.

Rules:

- Start with extraction wrappers; no broad rewrites.
- Keep old `IRBuilder` API for compatibility.

Acceptance:

- LLVM output of `legacy` and `next` binaries matches on small corpus.
- Existing non-`llvm_gcc_c_torture` tests pass.

### Phase 2: Introduce Minimal HIR

Deliverables:

- Define minimal `HIR` for existing frontend needs:
  - functions, params, locals, blocks
  - integer/float literals, var refs
  - unary/binary/assign/call
  - if/while/for/return
  - globals and simple initializers
- Add `ast_to_hir` lowering.

Scope control:

- Model only constructs already covered by tests first.
- Unsupported forms may fallback to legacy path behind flag.

Acceptance:

- `Parser -> Sema -> HIR` pipeline runs for current small suites.
- Snapshot stats available (`--dump-hir-summary`).

### Phase 3: HIR -> LLVM Backend (Parallel with legacy IRBuilder)

Deliverables:

- New LLVM backend module `codegen/llvm/hir_to_llvm.cpp`.
- Initially, this module may internally call adapted pieces of legacy `IRBuilder` to reduce risk.
- New `tiny-c2ll-next` flow:
  - preprocess -> lex -> parse -> sema -> hir -> llvm backend

Acceptance:

- For allowed tests, `legacy` and `next` output binaries/behavior match.
- Keep `legacy` as release default.

### Phase 4: Introduce DAG Layer Stub

Deliverables:

- Add `ir/dag` types and `hir_to_dag` lowering skeleton.
- Add `--emit-dag` debug output for inspection.
- No native codegen required yet.

Acceptance:

- `next` can run to DAG on representative files.
- No regression in existing LLVM path.

### Phase 5: Cutover Preparation

Deliverables:

- Ensure `next` passes:
  - `tiny_c2ll_tests`
  - `negative_tests`
  - `ccc_review_*`
  - `c_testsuite_*`
- Keep excluding `llvm_gcc_c_torture_*` for this stage.
- Add parity checker script for legacy vs next output diff.

Acceptance:

- Stable parity for agreed suite subset.
- Performance and memory within acceptable range (documented baseline).

### Phase 6: Default Switch and Legacy Removal (later)

Deliverables:

- Make `next` path default target.
- Keep legacy path behind option for one transition window.
- Remove legacy `ir_builder` monolith only after sustained green runs.

Acceptance:

- Full agreed suite green.
- Team sign-off before deletion.

## Test Strategy (Current Request)

Run/require in migration loop:

- `ctest -R tiny_c2ll_tests`
- `ctest -R negative_tests`
- `ctest -R ccc_review`
- `ctest -R c_testsuite`
- `ctest -E llvm_gcc_c_torture`

Do not gate early migration on:

- `llvm_gcc_c_torture_*`

## Implementation Notes

- Keep old file names and code paths for comparison.
- Add clear naming:
  - old: `legacy`
  - new: `next`
- Use feature flags in new driver if needed:
  - `--pipeline=legacy|next`
  - `--dump-hir`
  - `--dump-dag`

## Immediate Next 3 PRs

1. PR-1: Add `apps/tiny-c2ll-next.cpp`, CMake new target, no behavior change.
2. PR-2: Add `sema_driver` skeleton and move first semantic helpers from `ir_builder`.
3. PR-3: Add minimal `HIR` structs + `ast_to_hir` for basic expressions/statements.

## Current Progress (as of 2026-03-08)

### Phase 0 status: completed

- Added parallel entry binary `tiny-c2ll-next` (`frontend_cxx_next`) with its own `main`.
- Kept `tiny-c2ll-stage1` (`frontend_cxx_stage1`) unchanged as default test/compiler path.
- Matched shared CLI behavior for `--lex-only`, `--parse-only`, and `-o` on `next`.
- Verified both binaries build and legacy CTest flow still runs via stage1.

### Phase 1 status: completed

Completed in `src/frontend/sema/sema_driver.{hpp,cpp}`:

- Type construction and classification helpers:
  - `make_ts`, `int_ts`, `void_ts`, `double_ts`, `float_ts`, `char_ts`, `ll_ts`
  - `ptr_to`, `is_unsigned_base`, `is_float_base`, `is_complex_base`, `complex_component_ts`
- Array-shape helpers:
  - `array_rank_of`, `array_dim_at`, `is_array_ty`, `clear_array`,
    `set_array_dims`, `set_first_array_dim`, `drop_array_dim`
- String/constant-eval semantic helpers:
  - `is_wide_str_lit`, `str_lit_byte_len`, `allows_string_literal_ptr_target`
  - `decode_narrow_string_lit`, `normalize_printf_longdouble_format`, `decode_wide_string`
  - `infer_array_size_from_init`, `static_eval_int`, `static_eval_float`
- Expression type and promotion helpers:
  - `classify_int_literal_type`, `classify_float_literal_type`
  - `classify_unary_result_type`, `classify_binop_result_type`
  - `decay_array_to_ptr`
  - `remap_builtin_et_name`, `classify_known_call_return_type`

`ir_builder.cpp` now depends on `sema_driver.hpp` for the helpers above instead of owning local static copies.

Validation completed after extraction:

- `legacy` vs `next` LLVM IR parity check on representative input (`example.c`): no diff.
- Non-`llvm_gcc_c_torture` suites green:
  - `tiny_c2ll_tests`
  - `negative_tests`
  - `ccc_review_*`
  - `c_testsuite_*`

### Phase 2 status: in progress (bootstrap landed)

Completed in this step:

- Added a dedicated sema IR spec header:
  - `src/frontend/sema/ir.hpp`
- Added minimal AST -> HIR lowering module:
  - `src/frontend/sema/ast_to_hir.hpp`
  - `src/frontend/sema/ast_to_hir.cpp`
- Wired new module into build:
  - `CMakeLists.txt` (`FRONTEND_CXX_COMMON_SRCS` now includes `ast_to_hir.cpp`)
- Added phase-2 visibility flag to `next` driver:
  - `src/apps/tiny-c2ll-next.cpp`
  - new CLI option: `--dump-hir-summary`

Current behavior:

- `tiny-c2ll-next --dump-hir-summary <file.c>` now runs:
  - preprocess -> lex -> parse -> lower_ast_to_hir -> summary print
- `tiny-c2ll-next` default compile path remains unchanged:
  - still emits LLVM IR through legacy `IRBuilder` path

Validation snapshot:

- `frontend_cxx_next` builds successfully after wiring.
- Example run:
  - command: `./build/tiny-c2ll-next --dump-hir-summary tests/tiny_c2ll/example.c`
  - output: `HIR summary: functions=1 globals=0 blocks=1 statements=5 expressions=14`
- Legacy output path smoke-check still works:
  - `./build/tiny-c2ll-next tests/tiny_c2ll/example.c`
