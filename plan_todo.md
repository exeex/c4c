# AArch64 Backend Bring-up Todo

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 1: Evaluate the next compile-safe shared backend unit after the stub-only `state`/`traits`/`liveness`/`regalloc`/`generation` slice is now build-reachable

## Planned Queue

- [ ] Step 1: Inventory and compile the shared backend slice
- [ ] Step 2: Inventory and compile the AArch64-local slice
- [ ] Step 3: Reconnect backend entry points
- [ ] Step 4: Attach the current LIR boundary with the smallest shim
- [ ] Step 5: Emit minimal AArch64 assembly
- [ ] Step 6: Prove external `llvm-mc` assembly to ELF object
- [ ] Step 7: Tighten behavior in ref order after the bring-up gates pass

## Completed Items

- [x] Activated `ideas/open/02_backend_aarch64_port_plan.md` into the active runbook
- [x] Restored real-build reachability for the current backend entry slice by aligning CMake with the mirrored AArch64 subtree and adding a thin `src/backend/aarch64/codegen/emit.*` shim
- [x] Repaired the shared backend entry contract so known non-AArch64 targets still route through the backend path while AArch64 keeps explicit target-local validation
- [x] Added the first five stub-only shared backend units (`state`, `traits`, `liveness`, `regalloc`, `generation`) to both `c4cll` and `backend_lir_adapter_tests`, keeping the full suite monotonic at the existing 4 known failures

## Next Intended Slice

- evaluate whether `stack_layout/mod.cpp` is the next safe shared backend file to make build-reachable, or whether another top-level shared file has a smaller include surface
- keep the AArch64 shim thin until the shared backend slice grows enough to replace LLVM-text passthroughs deliberately
- defer ELF and shared linker modules until the plain shared backend slice is build-reachable

## Blockers

- mirrored shared backend files under `src/backend/` are still mostly disconnected from the build graph beyond the minimal entry slice
- full suite still has unrelated pre-existing failures: `positive_sema_ok_fn_returns_variadic_fn_ptr_c`, `cpp_positive_sema_decltype_bf16_builtin_cpp`, `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`, `cpp_llvm_initializer_list_runtime_materialization`

## Resume Notes

- the active plan was activated from the highest-priority open backend idea
- keep this run focused on compile integration and backend/LIR wiring before broader behavior work
- do not absorb regalloc, built-in assembler, or built-in linker initiatives unless they are strictly required for the current slice
- current slice outcome: `cmake -S . -B build` and `cmake --build build -j8` now succeed with the mirrored AArch64 entry path wired through `src/backend/aarch64/codegen/emit.*`
- validation after this slice: targeted backend tests pass and full `ctest --test-dir build -j8 --output-on-failure` now reports 4 failing tests instead of the earlier 5, removing the backend entry regression
- current iteration target: make the first five shared stub modules build-reachable in both `c4cll` and `backend_lir_adapter_tests` before attempting deeper shared backend files
- latest validation: `test_before.log` and `test_after.log` both report the same 4 failing tests (`positive_sema_ok_fn_returns_variadic_fn_ptr_c`, `cpp_positive_sema_decltype_bf16_builtin_cpp`, `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`, `cpp_llvm_initializer_list_runtime_materialization`)
