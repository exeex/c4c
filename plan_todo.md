# Active Plan Todo

Status: Active
Source Idea: ideas/open/02_stmt_emitter_cpp_split_refactor.md
Source Plan: plan.md

## Current Active Item

- Step 4: wire the split files into the build
- Current slice: add the staged `stmt_emitter_*.cpp` files to `CMakeLists.txt`, then use compiler/linker diagnostics to trim duplicate ownership out of `stmt_emitter.cpp`

## Todo

- [x] Step 1: survey `stmt_emitter.cpp` and `stmt_emitter.hpp`
- [x] Step 2: create draft staging files for the first semantic clusters
- [x] Step 3: expand the draft set and normalize boundaries
- [ ] Step 4: wire the split files into the build
- [ ] Step 5: reduce the monolith cleanly
- [ ] Step 6: run final build and regression validation

## Completed

- Activated from [`ideas/open/02_stmt_emitter_cpp_split_refactor.md`](/workspaces/c4c/ideas/open/02_stmt_emitter_cpp_split_refactor.md) on 2026-04-02 after closing the completed `ast_to_hir.cpp` split refactor.
- Completed Step 1 on 2026-04-02 by surveying [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp), confirming the header already exposes the main ownership seams needed for the split.
- Completed Step 2 on 2026-04-02 by staging draft ownership files [`src/codegen/lir/stmt_emitter_stmt.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_stmt.cpp), [`src/codegen/lir/stmt_emitter_call.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_call.cpp), and [`src/codegen/lir/stmt_emitter_expr.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_expr.cpp) without changing build wiring or removing the live definitions from [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp).

## Notes

- The idea explicitly prefers a draft-first extraction pass before build wiring; the initial execution slice should respect that guardrail.
- Current regression baseline on 2026-04-02: `cmake -S . -B build`, `cmake --build build -j8`, and `ctest --test-dir build -j 8 --output-on-failure > test_fail_before.log` completed with 2668/2671 passing tests. The existing failures are `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c`.
- First-pass ownership map:
  - `stmt_emitter_core.cpp`: module setup, label/terminator helpers, string/global helpers, coercions, and shared ABI/type utilities near the top of the file.
  - `stmt_emitter_types.cpp`: field-chain lookup, field type resolution, bitfield promotion, and other struct/type-centric helpers.
  - `stmt_emitter_lvalue.cpp`: lvalue dispatch, member/indexed address formation, assignable-load/store helpers, and compound-assignment plumbing.
  - `stmt_emitter_expr.cpp`: expression type resolution plus non-call rvalue payload lowering for literals, decl refs, unary, binary, ternary, member, index, cast, and sizeof forms.
  - `stmt_emitter_call.cpp`: call-target resolution, argument preparation, builtin-call lowering, and vararg lowering for amd64/aarch64 paths.
  - `stmt_emitter_stmt.cpp`: `emit_stmt`, non-control-flow statements, control-flow statements, switch-label handling, and inline-asm statement helpers.
- The initial Step 2 extraction target should stay draft-only because [`CMakeLists.txt`](/workspaces/c4c/CMakeLists.txt) still compiles only [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp).
- The staged Step 2 files currently mix copied live bodies with explicit draft placeholders so ownership is visible before duplicate-definition cleanup starts; Step 3 should replace those placeholders by continuing the extraction out of [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp).
- Step 3 progress on 2026-04-02: drafted [`src/codegen/lir/stmt_emitter_core.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_core.cpp), [`src/codegen/lir/stmt_emitter_types.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_types.cpp), and [`src/codegen/lir/stmt_emitter_lvalue.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_lvalue.cpp) by copying the core setup helpers, type/field lookup helpers, and lvalue / assignable-store cluster out of the monolith without changing build wiring.
- Completed Step 3 on 2026-04-02 by backfilling the remaining expression / call ownership gaps in [`src/codegen/lir/stmt_emitter_expr.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_expr.cpp) and [`src/codegen/lir/stmt_emitter_call.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter_call.cpp), including complex arithmetic helpers, call-target resolution, variadic argument preparation, builtin lowering, and AArch64 `va_arg` handling that previously still lived only in [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp).
- Validation note on 2026-04-02: this slice did not run a compile or regression pass because [`CMakeLists.txt`](/workspaces/c4c/CMakeLists.txt) still only builds [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp); the next slice should wire the staged files into the build so compiler/linker diagnostics can validate ownership cleanup.
