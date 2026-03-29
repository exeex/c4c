# Backend Regalloc And Peephole Port Todo

Status: Active
Source Idea: ideas/open/03_backend_regalloc_peephole_port_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 5: Wire the shared result into the AArch64 prologue and emit path.
  Iteration target: thread one shared used-callee-saved or assigned-register handoff into a non-fallback AArch64 emit/prologue seam so Step 5 starts from real backend state instead of fallback-only LIR preparation.

## Todo Queue

- [x] Step 1: Inventory the current shared backend boundary and capture the first implementation slice.
- [x] Step 2: Port liveness and interval computation with targeted tests.
- [x] Step 3: Port shared linear-scan regalloc with targeted tests.
- [x] Step 4: Port the stack-layout helper boundary that consumes regalloc results.
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
- [x] Extended shared `StackLayoutAnalysis` with first-access tracking for entry allocas, then used shared `slot_assignment` to drop redundant paired scalar `zeroinitializer` stores when a live entry slot is overwritten before any read while preserving aggregate slots and read-before-store cases.
- [x] Extended `backend_lir_adapter_tests` with overwrite-before-read versus read-before-store scalar entry-slot coverage, reran `./build/backend_lir_adapter_tests`, reran `compare_smoke_struct`, reran full `ctest`, and rechecked monotonic non-regression with `check_monotonic_regression.py --allow-non-decreasing-passed` (`before=570/574`, `after=570/574`, no new failures).
- [x] Added the first shared `src/backend/stack_layout/alloca_coalescing.{hpp,cpp}` seam so Step 4 can classify non-param allocas as dead, single-block coalescable, or escaped after GEP-tracked use scanning instead of leaving that stack-slot-reuse boundary entirely stubbed.
- [x] Extended `backend_lir_adapter_tests` with shared alloca-coalescing coverage for dead, GEP-tracked single-block, and call-escaped local allocas; rebuilt `backend_lir_adapter_tests`, reran the binary, reran full `ctest`, and revalidated the saved baseline (`after=570/574`, same four known failures only).
- [x] Threaded shared `alloca_coalescing` into `src/backend/stack_layout/slot_assignment.{hpp,cpp}` so live non-param entry-alloca plans now carry a `coalesced_block` hint when their uses stay within one block instead of leaving that reuse classification trapped in analysis-only helpers.
- [x] Extended `backend_lir_adapter_tests` with slot-assignment coverage for single-block reuse hints and escaped-local exclusions, rebuilt `backend_lir_adapter_tests`, reran the binary, reran full `ctest`, and rechecked monotonic non-regression with `check_monotonic_regression.py --allow-non-decreasing-passed` (`before=570/574`, `after=570/574`, no new failures).
- [x] Extended shared `slot_assignment` so live entry allocas now receive a concrete `assigned_slot` decision: single-block allocas with compatible type/alignment can reuse one shared slot across disjoint blocks, while same-block allocas stay separate instead of stopping at `coalesced_block` metadata.
- [x] Extended `backend_lir_adapter_tests` with disjoint-block shared-slot reuse and same-block non-reuse coverage, rebuilt `backend_lir_adapter_tests`, reran the binary, reran full `ctest`, and rechecked monotonic non-regression with `check_monotonic_regression.py --allow-non-decreasing-passed` (`before=570/574`, `after=570/574`, no new failures).
- [x] Added shared `apply_entry_alloca_slot_plan` wiring so Step 4 now applies `assigned_slot` to a real stack-layout consumer: disjoint entry allocas collapse onto one retained entry slot, users are rewritten to the canonical slot, and the AArch64 fallback emitter consumes that prepared function instead of leaving shared slot reuse as planner-only metadata.
- [x] Extended `backend_lir_adapter_tests` with direct slot-plan application coverage plus AArch64 fallback checks for shared-slot canonicalization versus same-block non-reuse, rebuilt `backend_lir_adapter_tests`, reran the binary, reran full `ctest`, and rechecked monotonic non-regression with `check_monotonic_regression.py --allow-non-decreasing-passed` (`before=570/574`, `after=570/574`, no new failures).

## Next Intended Slice

- Start Step 5 by threading the shared used-callee-saved set or assigned-register lookup into one narrow non-fallback AArch64 emit/prologue seam with a test that proves the emitted assembly path, not just fallback LIR, depends on shared backend state.
- Keep broader frame-layout and stack-object materialization deferred until that first non-fallback AArch64 handoff is stable.

## Blockers

- None recorded during activation.

## Resume Notes

- Shared backend execution now reaches real Step 4 consumers through `src/backend/stack_layout/{analysis,alloca_coalescing,slot_assignment}.cpp` plus the AArch64 fallback preparation path in `src/backend/aarch64/codegen/emit.cpp`.
- Current compile/test reachability still comes primarily through `backend_lir_adapter_tests`, and the first executed shared-slot consumer is fallback-LIR preparation rather than the non-fallback AArch64 prologue/assembly path.
- AArch64 still documents the intended next seam in commented ref notes under `src/backend/aarch64/codegen/prologue.cpp`; the remaining gap is a live non-fallback shared regalloc/used-register handoff.
- The new helper seam lives in `src/backend/stack_layout/regalloc_helpers.{hpp,cpp}` and is now the intended entry point for Step 4 consumers that need assigned-register, used-callee-saved, or cached-liveness reads.
- The shared analysis plus slot-assignment seams now cover phi-aware use-block attribution, used-value collection, dead param allocas, entry-alloca overwrite tracking, single-block coalescing, concrete `assigned_slot` decisions, and canonical shared-slot application for the typed-LIR subset exercised by `backend_lir_adapter_tests`.
- Known baseline full-suite failures remain unchanged:
  `positive_sema_ok_fn_returns_variadic_fn_ptr_c`,
  `cpp_positive_sema_decltype_bf16_builtin_cpp`,
  `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`,
  `cpp_llvm_initializer_list_runtime_materialization`.
