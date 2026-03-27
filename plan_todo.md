# AArch64 Backend Port Todo

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Source Plan: plan.md

## Current Active Item

- Step 1: inspect the ref AArch64 backend surfaces
- Iteration slice: identify the smallest ref-to-C++ file boundary for AArch64 return, integer arithmetic, and branch bring-up

## Next Intended Slice

- map the ref AArch64 backend files and helpers onto concrete new files under `src/backend/`
- record the first target-specific skeleton needed before porting instructions
- keep the first port slice limited to the current runbook scope

## Completed Items

- activated `ideas/open/02_backend_aarch64_port_plan.md` into `plan.md`
- initialized execution-state tracking for the active AArch64 runbook

## Blockers

- none recorded

## Resume Notes

- do not reopen the closed LIR adapter idea; build on the existing backend boundary
- keep the port mechanically close to ref before introducing C++-specific cleanup
- preserve the external toolchain fallback contract already established for backend runtime tests
