# Backend Regalloc And Peephole Port Todo

Status: Active
Source Idea: ideas/open/03_backend_regalloc_peephole_port_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 4: Port the stack-layout helper boundary that consumes regalloc results.
  Iteration target: thread `StackLayoutAnalysis` into the next frame-sizing or stack-slot placement decision beyond entry-alloca pruning so Step 4 starts affecting actual slot layout, not only dead-slot elision.

## Todo Queue

- [x] Step 1: Inventory the current shared backend boundary and capture the first implementation slice.
- [x] Step 2: Port liveness and interval computation with targeted tests.
- [x] Step 3: Port shared linear-scan regalloc with targeted tests.
- [ ] Step 4: Port the stack-layout helper boundary that consumes regalloc results.
- [ ] Step 5: Wire the shared result into the AArch64 prologue and emit path.
- [ ] Step 6: Add the smallest required cleanup or peephole slice.
- [ ] Step 7: Run regression validation, record follow-ons, and prepare the next slice.

## Completed

- [x] Activated `ideas/open/03_backend_regalloc_peephole_port_plan.md` into `plan.md`.
- [x] Inventoried the current shared backend boundary, added the first compile-clean shared liveness/regalloc headers, and wired `src/backend/stack_layout/{analysis,regalloc_helpers,slot_assignment}.cpp` into the build graph.
- [x] Added backend unit coverage for shared interval lookup, deterministic placeholder register assignment, and inline-asm clobber filtering/merge behavior.
- [x] Rebuilt the tree, ran `backend_lir_adapter_tests`, ran full `ctest`, and verified monotonic non-regression with `check_monotonic_regression.py` (`before=570/574`, `after=570/574`, no new failures).
- [x] Replaced the placeholder shared liveness scan with a typed-LIR def/use pass that tracks terminator uses, call points, live-through blocks, and phi incoming edges for the current backend subset.
- [x] Added backend coverage for call-crossing and multi-block/phi-join live interval ranges, rebuilt the tree, reran `backend_lir_adapter_tests`, reran full `ctest`, and rechecked monotonic non-regression (`before=570/574`, `after=570/574`, no new failures).
- [x] Replaced the placeholder regalloc pass with the first shared linear-scan slice: call-spanning intervals use the callee-saved pool first, non-call-spanning intervals use caller-saved registers before callee spillover, and overlapping intervals spill instead of reusing a busy register.
- [x] Added backend allocator coverage for caller-saved versus callee-saved pool selection, non-overlapping register reuse, and bounded spill behavior; rebuilt the tree, reran `backend_lir_adapter_tests`, reran full `ctest`, and rechecked monotonic non-regression (`before=570/574`, `after=570/574`, no new failures).
- [x] Added the first shared stack-layout regalloc handoff seam in `regalloc_helpers`, covering assigned-register lookup, used-callee-saved queries, and cached-liveness access through a single helper boundary.
- [x] Extended `backend_lir_adapter_tests`, rebuilt the tree, reran `backend_lir_adapter_tests`, reran full `ctest`, and rechecked monotonic non-regression with `check_monotonic_regression.py` (`before=570/574`, `after=570/574`, no new failures).
- [x] Added the first shared stack-layout analysis object in `src/backend/stack_layout/analysis.{hpp,cpp}` so Step 4 consumers can query phi-aware value-use blocks, body-used values, and dead param allocas from the existing regalloc helper seam instead of poking backend integration state directly.
- [x] Extended `backend_lir_adapter_tests` with stack-layout analysis coverage, rebuilt the tree, reran `backend_lir_adapter_tests`, reran full `ctest`, and revalidated the saved baseline (`before=570/574`, `after=570/574`, same known failures only).
- [x] Added the first shared `src/backend/stack_layout/slot_assignment.{hpp,cpp}` consumer of `StackLayoutAnalysis`: param-alloca slot planning now turns dead-vs-live param allocas into explicit shared decisions instead of leaving those facts trapped in analysis-only tests.
- [x] Extended `backend_lir_adapter_tests` with shared slot-assignment coverage for dead and live param allocas, rebuilt `backend_lir_adapter_tests`, reran the binary, reran full `ctest`, and rechecked monotonic non-regression (`before=570/574`, `after=570/574`, no new failures).
- [x] Extended shared `slot_assignment` with dead-param-alloca pruning, then wired the AArch64 backend fallback printer to consume shared regalloc plus stack-layout analysis so dead param allocas are removed from emitted fallback LIR instead of remaining test-only metadata.
- [x] Added backend coverage for shared dead-param-alloca pruning and AArch64 fallback dead-vs-live param-slot rendering, rebuilt `backend_lir_adapter_tests`, reran the binary, reran full `ctest`, and rechecked the saved baseline (`before=570/574`, `after=570/574`, same known failures only).
- [x] Expanded shared `slot_assignment` beyond param-only handling: dead non-param entry allocas now plan and prune through the same shared seam, including paired entry zero-init stores, and the AArch64 fallback now consumes that broader shared entry-slot decision instead of local dead-param-only pruning.
- [x] Extended `backend_lir_adapter_tests` with shared dead-vs-live local-entry-alloca coverage, rebuilt the binary, reran `./build/backend_lir_adapter_tests`, reran full `ctest`, and revalidated the saved baseline by log comparison (`after=570/574`, same four known failures only).

## Next Intended Slice

- Keep building Step 4 inside shared stack-layout code by threading `StackLayoutAnalysis` into the next real slot-assignment or frame-sizing decision after entry-alloca pruning, ideally one that affects non-entry local slot placement, stack-slot reuse, or frame-size computation instead of only dead-slot removal.
- Keep Step 5 AArch64 prologue and emit wiring deferred until the shared helper and analysis seams feed a broader stack-layout consumer than fallback-LIR pruning, with narrow tests around that handoff.

## Blockers

- None recorded during activation.

## Resume Notes

- Inventory from this execution turn:
  `src/backend/liveness.cpp`, `src/backend/regalloc.cpp`, `src/backend/generation.cpp`,
  `src/backend/state.cpp`, and the `src/backend/stack_layout/*` regalloc-related files are still stub mirrors.
- Current compile/test reachability comes primarily through `backend_lir_adapter_tests`; the stack-layout helper `.cpp` files are not yet part of the build graph.
- AArch64 already documents the intended seam in commented ref notes under
  `src/backend/aarch64/codegen/prologue.cpp`, but no live shared regalloc interface is wired yet.
- First bounded patch target: establish compile-clean shared headers/types plus helper functions, add the stack-layout helper sources to CMake, and validate the surface with backend unit tests before any target-local behavior work.
- Current shared implementation is intentionally skeletal:
  `compute_live_intervals` now performs the first real typed-LIR def/use walk with block live-through extension,
  while `allocate_registers` still only establishes the initial placeholder handoff/result shape.
- This iteration is intentionally narrower than full stack slot placement:
  start by making shared helper entry points read `RegAllocIntegrationResult`
  in one place, then expand into analysis/slot assignment in a later slice.
- The new helper seam lives in `src/backend/stack_layout/regalloc_helpers.{hpp,cpp}` and is now the intended entry point for Step 4 consumers that need assigned-register, used-callee-saved, or cached-liveness reads.
- The new analysis seam lives in `src/backend/stack_layout/analysis.{hpp,cpp}` and currently covers phi-aware use-block attribution, used-value collection, and dead param alloca detection for the typed-LIR subset exercised by `backend_lir_adapter_tests`.
- Known baseline full-suite failures remain unchanged:
  `positive_sema_ok_fn_returns_variadic_fn_ptr_c`,
  `cpp_positive_sema_decltype_bf16_builtin_cpp`,
  `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`,
  `cpp_llvm_initializer_list_runtime_materialization`.
