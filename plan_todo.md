# Active Plan Todo

Status: Active
Source Idea: ideas/open/01_ast_to_hir_cpp_split_refactor.md
Source Plan: plan.md

## Current Active Item

- Step 1: Establish Header Ownership
- Current slice: select the next cohesive `Lowerer` helper cluster to peel out of the inline class body after `infer_generic_ctrl_type`

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

## Next Slice

- Continue Step 1 by selecting the next cohesive `Lowerer` helper cluster to peel out of the inline class body before moving the full class definition into `ast_to_hir.hpp`.

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
