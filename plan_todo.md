# Backend Regalloc And Peephole Port Todo

Status: Active
Source Idea: ideas/open/03_backend_regalloc_peephole_port_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 4: Port the stack-layout helper boundary that consumes regalloc results.
  Iteration target: thread the new shared regalloc result shape through the first stack-layout helper seam without pulling AArch64-specific policy into shared helpers.

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

## Next Intended Slice

- Start the stack-layout helper slice by replacing any remaining placeholder assumptions with reads from `RegAllocResult::assignments`, `used_regs`, and cached liveness where the current shared helpers already expose a natural seam.
- Keep AArch64 wiring deferred until the shared stack-layout seam compiles cleanly and has narrow tests around the new handoff data.

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
- Known baseline full-suite failures remain unchanged:
  `positive_sema_ok_fn_returns_variadic_fn_ptr_c`,
  `cpp_positive_sema_decltype_bf16_builtin_cpp`,
  `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`,
  `cpp_llvm_initializer_list_runtime_materialization`.
