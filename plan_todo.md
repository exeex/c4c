# AArch64 Backend Port Todo

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Source Plan: plan.md

## Current Active Item

- Step 3: port the first integer/control-flow slice
- Iteration slice: split the AArch64 scaffold into frame, ALU, and branch helpers and replace the scaffold delegation with mechanical target-local emission

## Next Intended Slice

- port the ref prologue/return helpers into `src/backend/aarch64/` before broadening arithmetic coverage
- bring up add/compare/branch emission against the current LIR block model
- add backend runtime cases once the AArch64 path can lower beyond the current adapter slice

## Completed Items

- activated `ideas/open/02_backend_aarch64_port_plan.md` into `plan.md`
- initialized execution-state tracking for the active AArch64 runbook
- completed Step 1 inspection of the ref AArch64 backend surfaces
- mapped the first ref port slice onto `src/backend/aarch64/` plus explicit factory wiring in `src/backend/backend.cpp`
- landed the Step 2 scaffold in `src/backend/aarch64/emitter.*`
- added target-local scaffold coverage in `tests/backend/backend_lir_adapter_tests.cpp`

## Blockers

- none recorded

## Resume Notes

- do not reopen the closed LIR adapter idea; build on the existing backend boundary
- keep the port mechanically close to ref before introducing C++-specific cleanup
- preserve the external toolchain fallback contract already established for backend runtime tests
- the scaffold should stay target-local: x86_64/i686/riscv64 keep the existing generic backend entry until their own plans are active
