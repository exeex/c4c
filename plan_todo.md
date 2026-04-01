# Active Plan Todo

Status: Active
Source Idea: ideas/open/01_ast_to_hir_cpp_split_refactor.md
Source Plan: plan.md

## Current Active Item

- Step 1: Establish Header Ownership

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

## Next Slice

- Continue Step 1 by introducing an internal declaration surface for `Lowerer`-owned helper structs and method groups, then migrate the next cohesive lowering cluster out of the inline class body without changing behavior.

## Blockers

- Full `ctest --test-dir build -j8 --output-on-failure` is not green in the current tree; observed failures were `positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c`.

## Resume Notes

- This plan was activated because there was no active `plan.md` or `plan_todo.md`.
- Numeric idea ordering was respected; `01_ast_to_hir_cpp_split_refactor.md` was the lowest-numbered eligible source.
- Inventory confirms the current monolith owns top-level helpers such as `make_span(...)`, `make_ns_qual(...)`, string-decoding helpers, and the `Lowerer` class definition.
- The first execution slice keeps behavior unchanged and starts header ownership with forward declarations before moving larger helper/class surfaces.
- Validation after the slice: `cmake -S . -B build` and `cmake --build build -j8` succeeded; full `ctest` completed with 3 existing failures outside this HIR declaration-only change.
- The current slice moved reusable non-anonymous helper declarations into `src/frontend/hir/ast_to_hir.hpp` so future `hir_*.cpp` files can include one declaration surface instead of rediscovering local monolith-only types.
- The latest slice converted the initial program setup/orchestration helpers from inline `Lowerer` member bodies into out-of-class definitions, reducing the size of the monolithic class body without changing symbol ownership or behavior.
