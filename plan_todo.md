# Active Plan Todo

Status: Active
Source Idea: ideas/open/02_stmt_emitter_cpp_split_refactor.md
Source Plan: plan.md

## Current Active Item

- Step 2: create draft staging files for the first semantic clusters
- Current slice: stage `stmt_emitter_stmt.cpp`, `stmt_emitter_call.cpp`, and `stmt_emitter_expr.cpp` as draft ownership targets while leaving `src/codegen/lir/stmt_emitter.cpp` as the live implementation until build wiring begins

## Todo

- [x] Step 1: survey `stmt_emitter.cpp` and `stmt_emitter.hpp`
- [ ] Step 2: create draft staging files for the first semantic clusters
- [ ] Step 3: expand the draft set and normalize boundaries
- [ ] Step 4: wire the split files into the build
- [ ] Step 5: reduce the monolith cleanly
- [ ] Step 6: run final build and regression validation

## Completed

- Activated from [`ideas/open/02_stmt_emitter_cpp_split_refactor.md`](/workspaces/c4c/ideas/open/02_stmt_emitter_cpp_split_refactor.md) on 2026-04-02 after closing the completed `ast_to_hir.cpp` split refactor.
- Completed Step 1 on 2026-04-02 by surveying [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp), confirming the header already exposes the main ownership seams needed for the split.

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
- Next intended slice: create the three draft files named in Step 2 and copy their strongest semantic clusters first, without removing live definitions from [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp).
