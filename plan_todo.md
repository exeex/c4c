# AArch64 Backend Port Todo

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Source Plan: plan.md

## Current Active Item

- Step 3: port the first integer/control-flow slice
- Iteration slice: open stack-slot/load-store lowering for local integer temporaries now that the direct-call slice is landed

## Next Intended Slice

- port the ref prologue/return helpers deeper into `src/backend/aarch64/`
- add stack-slot and load/store support for local integer temporaries in simple local-variable backend cases
- extend runtime coverage from helper-call backend cases into local integer temporary/backend stack usage

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

## Blockers

- none recorded

## Resume Notes

- do not reopen the closed LIR adapter idea; build on the existing backend boundary
- keep the port mechanically close to ref before introducing C++-specific cleanup
- preserve the external toolchain fallback contract already established for backend runtime tests
- the scaffold should stay target-local: x86_64/i686/riscv64 keep the existing generic backend entry until their own plans are active
- the current AArch64 branch slice still relies on the frontend's existing LLVM-text LIR conventions; direct calls and stack slots are the next boundary to port
- direct calls now render through the target-local AArch64 emitter for simple direct `@helper(...)` cases; indirect calls and typed callee suffixes remain out of slice
