# Active Plan Todo

Status: Active
Source Idea: ideas/open/01_ast_to_hir_cpp_split_refactor.md
Source Plan: plan.md

## Current Active Item

- Step 1: Establish Header Ownership
- Current slice: target the remaining low-coupling template-struct registry/lookup helpers near the bottom of the inline `Lowerer` class body after the `resolve_struct_member_typedef_hir` extraction

## Todo

- [ ] Step 1: inventory declarations in `src/frontend/hir/ast_to_hir.cpp` and expand `src/frontend/hir/ast_to_hir.hpp`
- [ ] Step 2: introduce a safe transitional source layout and build wiring
- [ ] Step 3: extract lowering clusters into focused `hir_*.cpp` files
- [ ] Step 4: finalize the source layout and remove transitional scaffolding
- [ ] Step 5: run final build and regression validation

## Completed

- [x] Activated the runbook from `ideas/open/01_ast_to_hir_cpp_split_refactor.md`
- [x] Added the first Step 1 header declarations for top-level HIR lowering helpers and the `Lowerer` type in `src/frontend/hir/ast_to_hir.hpp`
- [x] Rebuilt the tree after the Step 1 header-surface slice
- [x] Promoted split-relevant HIR helper declarations (`QualifiedMethodRef`, op mappers, generic-type helpers, and layout entrypoints) into `src/frontend/hir/ast_to_hir.hpp`
- [x] Rebuilt and reran `ctest --test-dir build -j8 --output-on-failure`; the suite remained at the same 3 known failures
- [x] Peeled the initial program-orchestration `Lowerer` cluster out of the monolithic inline class body into out-of-class `Lowerer::...` definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran `ctest --test-dir build -j8 --output-on-failure` after the orchestration extraction; the suite still ended with the same 3 known failures
- [x] Extracted the next template-type helper cluster (`seed_pending_template_type`, deferred-template-owner helpers, and related typedef-resolution helpers) from the inline `Lowerer` class body into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt the tree after the template-type helper extraction
- [x] Reran targeted regression checks for `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c`; the same 3 historically recorded failures remained
- [x] Extracted the inline deferred-instantiation `Lowerer` methods (`instantiate_deferred_template` and `instantiate_deferred_template_type`) into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran both the targeted 3-test regression check and `ctest --test-dir build -j8 --output-on-failure`; the suite still ended with the same 3 known failures
- [x] Extracted the private `Lowerer` pack-binding and reference-type utility helpers (`is_lvalue_ref_ts`, `pack_binding_name`, `parse_pack_binding_name`, `count_pack_bindings_for_name`, `is_any_ref_ts`, `reference_storage_ts`, and `reference_value_ts`) into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite remained at 2671 total tests, 2668 passing, and the same 3 known failures
- [x] Extracted the private low-coupling `Lowerer` block helpers (`ensure_block`, `create_block`, and `append_stmt`) plus the struct-static-member lookup helpers (`find_struct_static_member_decl` and `find_struct_static_member_const_value`) into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite again finished at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the small declref/call-result helper pair (`infer_call_result_type_from_callee` and `storage_type_for_declref`) out of the inline `Lowerer` class body into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite remained at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the larger type-inference helper `infer_generic_ctrl_type` out of the inline `Lowerer` class body into an out-of-class definition in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite again finished at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the inline struct-method lookup helpers (`find_struct_method_mangled` and `find_struct_method_return_type`) into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite still finished at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the low-coupling inline `Lowerer` compile-time/utility helpers (`ct_state`, `resolve_typedef_to_struct`, `next_*_id`, `contains_stmt_expr`, and `qtype_from`) into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite still finished at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the inline function-pointer signature helper (`fn_ptr_sig_from_decl_node`) and NTTP const-eval utility (`eval_const_int_with_nttp_bindings`) out of the `Lowerer` class body into out-of-class definitions in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite still finished at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the inline `append_expr` helper out of the `Lowerer` class body into an out-of-class definition in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the targeted known-failure triplet plus the full `ctest --test-dir build -j8 --output-on-failure`; the suite still finished at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the inline enum-constant collector `collect_enum_def` out of the `Lowerer` class body into an out-of-class definition in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the full `ctest --test-dir build -j8 --output-on-failure`, then passed `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`; the suite remained at 2671 total tests, 2668 passing, and the same 3 historical failures
- [x] Extracted the inline struct-member-typedef resolver `resolve_struct_member_typedef_hir` out of the `Lowerer` class body into an out-of-class definition in `src/frontend/hir/ast_to_hir.cpp`
- [x] Rebuilt and reran the full `ctest --test-dir build -j8 --output-on-failure`, then passed `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`; the suite remained at 2671 total tests, 2668 passing, and the same 3 historical failures

## Next Slice

- Continue Step 1 by peeling the next low-coupling inline `Lowerer` utility cluster out of the class body before moving the full class definition into `ast_to_hir.hpp`, likely targeting the template-struct registry/lookup helpers (`find_template_struct_primary`, `find_template_struct_specializations`, `build_template_struct_env`, and the registration helpers) while still leaving `lower_struct_def` for a later, more deliberate extraction.

## Blockers

- The historical targeted failures remain: `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c`.

## Resume Notes

- This plan was activated because there was no active `plan.md` or `plan_todo.md`.
- Numeric idea ordering was respected; `01_ast_to_hir_cpp_split_refactor.md` was the lowest-numbered eligible source.
- Inventory confirms the current monolith owns top-level helpers such as `make_span(...)`, `make_ns_qual(...)`, string-decoding helpers, and the `Lowerer` class definition.
- The first execution slice keeps behavior unchanged and starts header ownership with forward declarations before moving larger helper/class surfaces.
- Validation after the slice: `cmake -S . -B build` and `cmake --build build -j8` succeeded; full `ctest` completed with 3 existing failures outside this HIR declaration-only change.
- The current slice moved reusable non-anonymous helper declarations into `src/frontend/hir/ast_to_hir.hpp` so future `hir_*.cpp` files can include one declaration surface instead of rediscovering local monolith-only types.
- The latest slice converted the initial program setup/orchestration helpers from inline `Lowerer` member bodies into out-of-class definitions, reducing the size of the monolithic class body without changing symbol ownership or behavior.
- The newest slice continued that extraction pattern for the deferred-template/type-resolution helper group, leaving behavior unchanged while shrinking the inline `Lowerer` class body again.
- The current slice moved the two deferred-instantiation entrypoints out of the inline `Lowerer` class body into out-of-class definitions, keeping the callback behavior unchanged while continuing the monolith peel-back.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; targeted reruns of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` finished with the same 3 failing tests and 2668 passing tests.
- The latest slice peeled the low-coupling pack-binding and reference-type utilities out of the inline `Lowerer` class body into out-of-class definitions, shrinking the monolithic class definition again without changing symbol ownership or behavior.
- The newest slice peeled the low-coupling block-management and struct-static-member lookup helpers out of the inline `Lowerer` class body into out-of-class definitions, continuing the monolith shrink without changing ownership or behavior.
- The current slice peeled the small declref/call-result helper pair (`infer_call_result_type_from_callee` and `storage_type_for_declref`) out of the inline `Lowerer` class body into out-of-class definitions, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; the targeted rerun of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests.
- The latest slice peeled the larger type-inference helper `infer_generic_ctrl_type` out of the inline `Lowerer` class body into an out-of-class definition, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; the targeted rerun of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests.
- The current slice targets the two inline struct-method lookup helpers (`find_struct_method_mangled` and `find_struct_method_return_type`) because they are self-contained, recursive lookups with no expected ownership changes beyond moving their bodies out of the class definition.
- The latest slice moved the recursive struct-method lookup helpers (`find_struct_method_mangled` and `find_struct_method_return_type`) out of the inline `Lowerer` class body into out-of-class definitions, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; the targeted rerun of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests.
- The current slice targeted the small compile-time state, allocator, and generic utility helpers because they are low-coupling one-liners or local recursion helpers that can move out of the inline class body without changing symbol ownership.
- The latest slice moved `ct_state`, `resolve_typedef_to_struct`, `next_fn_id`, `next_global_id`, `next_local_id`, `next_block_id`, `next_expr_id`, `contains_stmt_expr`, and `qtype_from` out of the inline `Lowerer` class body into out-of-class definitions, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; the targeted rerun of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests.
- The current slice targeted the inline function-pointer signature helper and the nearby NTTP const-eval utility because both are self-contained helpers used broadly by the remaining monolith but do not require any new declaration surface.
- The latest slice moved `fn_ptr_sig_from_decl_node` and `eval_const_int_with_nttp_bindings` out of the inline `Lowerer` class body into out-of-class definitions, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; the targeted rerun of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests.
- The current slice targeted `append_expr` because it is a small, high-fanout helper that could move out of the inline `Lowerer` class body without requiring any new declaration surface or ownership changes.
- The latest slice moved `append_expr` out of the inline `Lowerer` class body into an out-of-class definition, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; the targeted rerun of `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c` matched the historical blocker list; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests.
- The current slice targeted `collect_enum_def` because it is a small setup helper that still lived inline in the `Lowerer` class body and could move out-of-class without changing ownership or widening the declaration surface.
- The latest slice moved `collect_enum_def` out of the inline `Lowerer` class body into an out-of-class definition, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake -S . -B build` and `cmake --build build -j8` succeeded; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests; `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests.
- The current slice targeted `resolve_struct_member_typedef_hir` because it is a self-contained private helper that still lived inline in the `Lowerer` class body and could move out-of-class without changing ownership or widening the declaration surface.
- The latest slice moved `resolve_struct_member_typedef_hir` out of the inline `Lowerer` class body into an out-of-class definition, preserving behavior while continuing the monolith shrink.
- Validation on 2026-04-01: `cmake --build build -j8` succeeded; a full `ctest --test-dir build -j8 --output-on-failure` again finished at 2671 total tests, 2668 passing, and the same 3 failing tests; `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests.
