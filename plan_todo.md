# AArch64 Backend Port Todo

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Source Plan: plan.md

## Current Active Item

- Step 3: port the first integer/control-flow slice
- Iteration slice: split the ref prologue/return helpers deeper into `src/backend/aarch64/`

## Next Intended Slice

- start splitting the current frame/return path toward ref-shaped target-local helpers without changing the backend boundary
- keep the next helper extraction focused on target-local prologue/return structure, not new backend capability
- keep target-local casts aligned with real AArch64 addressing IR as new memory slices land

## Completed Items

- activated `ideas/open/02_backend_aarch64_port_plan.md` into `plan.md`
- initialized execution-state tracking for the active AArch64 runbook
- completed Step 1 inspection of the ref AArch64 backend surfaces
- mapped the first ref port slice onto `src/backend/aarch64/` plus explicit factory wiring in `src/backend/backend.cpp`
- landed the Step 2 scaffold in `src/backend/aarch64/emitter.*`
- added target-local scaffold coverage in `tests/backend/backend_lir_adapter_tests.cpp`
- split the AArch64 scaffold into target-local `frame`, `alu`, and `branch` helpers under `src/backend/aarch64/`
- replaced the scaffold delegation with target-local LLVM-text emission for `ret`, integer `add`, integer `icmp`, `zext`, `br`, and `condbr`
- added compare/branch unit coverage in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/branch_if_lt.c` to prove multi-block compare/branch lowering through the external AArch64 toolchain path
- added target-local direct-call rendering in `src/backend/aarch64/calls.*`
- added unit coverage for AArch64 direct helper calls in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/call_helper.c` to prove helper-call lowering through the external AArch64 toolchain path
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=488/502 -> 489/503` and zero newly failing tests
- allowed the AArch64 emitter to preserve module `type_decls` needed by simple function lowering
- added target-local AArch64 `alloca`/`load`/`store` rendering in `src/backend/aarch64/memory.*`
- added unit coverage for local temporary stack-slot emission in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/local_temp.c` for local integer temporary runtime coverage on supported AArch64 hosts
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=489/503 -> 491/504` and zero newly failing test names; one prior cpp EASTL failure dropped from the failing set
- added unit coverage for AArch64 modified-parameter slot emission plus typed direct helper calls in `tests/backend/backend_lir_adapter_tests.cpp`
- allowed the AArch64 call renderer to preserve typed direct-call suffixes while still rejecting indirect typed calls
- added `tests/c/internal/backend_case/param_slot.c` for modified-parameter runtime coverage on supported AArch64 hosts
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=490/504 -> 491/505` and zero newly failing tests
- added target-local AArch64 `getelementptr` rendering in `src/backend/aarch64/memory.cpp`
- extended the target-local AArch64 cast slice to render `sext` needed by local indexed addressing
- added unit coverage for AArch64 local array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/local_array.c` for local array runtime coverage on supported AArch64 hosts
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=491/505 -> 493/506` and zero newly failing tests
- added unit coverage for AArch64 struct-by-value parameter member-array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/param_member_array.c` for struct-by-value member-array runtime coverage on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_param_member_array` pass for the new parameter/member stack-addressing slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite after landing the new coverage; the new backend test passed, while the suite still reports unrelated existing failures outside this AArch64 slice
- added unit coverage for nested by-value member-array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/nested_param_member_array.c` for nested by-value member-array runtime coverage on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_nested_param_member_array` pass for the new nested by-value stack-addressing slice
- rebuilt from a clean `build/` directory, recorded `test_fail_after.log`, and passed the monotonic regression guard against `test_fail_before.log` with `passed=495/508 -> 496/509` and zero newly failing tests

## Blockers

- full-suite `ctest` is not clean yet due unrelated existing failures outside the active AArch64 slice (`backend_lir_missing_toolchain_diagnostic`, `compare_smoke_*`, `positive_sema_ok_fn_returns_variadic_fn_ptr_c`, and several longstanding C++ positive-case failures)

## Resume Notes

- do not reopen the closed LIR adapter idea; build on the existing backend boundary
- keep the port mechanically close to ref before introducing C++-specific cleanup
- preserve the external toolchain fallback contract already established for backend runtime tests
- the scaffold should stay target-local: x86_64/i686/riscv64 keep the existing generic backend entry until their own plans are active
- the current AArch64 branch slice still relies on the frontend's existing LLVM-text LIR conventions; direct calls and stack slots are the next boundary to port
- direct calls now render through the target-local AArch64 emitter for simple direct `@helper(...)` cases; indirect calls and typed callee suffixes remain out of slice
- simple local integer temporaries now emit through the target-local AArch64 memory helpers; broader slot flows and runtime execution still depend on supported AArch64 hosts/toolchains for end-to-end coverage
- modified-parameter slots now execute through the target-local AArch64 path, including typed direct helper calls with arguments
- local array stack-slot addressing now executes through target-local AArch64 `getelementptr` plus index-widening casts on supported hosts
- struct-by-value parameter member-array addressing is now covered in both backend emitter and runtime tests on the AArch64 path
- isolated AArch64 probes show nested `->` member-array addressing already lowers and executes through the current target-local memory helpers; this iteration is codifying that slice in repo tests
- added unit coverage for nested `->` member-array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/nested_member_pointer_array.c` for nested member-pointer runtime coverage on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_nested_member_pointer_array` pass for the new nested member-pointer stack-addressing slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite after landing the new coverage; the regression guard passed with `passed=494/507 -> 495/508`, zero newly failing tests, and the new backend runtime test accounted for the pass-count increase
- nested by-value aggregate member-array addressing also lowers through the current target-local AArch64 memory helpers; this iteration codifies that slice in repo tests
