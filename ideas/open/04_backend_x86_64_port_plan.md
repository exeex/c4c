# x86-64 Backend Port Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/01_backend_lir_adapter_plan.md`
- `ideas/open/03_backend_regalloc_peephole_port_plan.md`
- lessons learned from `ideas/open/02_backend_aarch64_port_plan.md`

## Goal

Port the x86-64 backend from ref into C++ with a similar mechanical translation strategy after the shared backend entry and first target bring-up are stable.

## Why This Is Separate

x86-64 shares some backend scaffolding with AArch64, but its instruction forms, calling convention details, and encoding expectations are different enough that it should not ride inside the same active plan.

## Scope

- System V AMD64 call/return lowering
- integer ALU and load/store coverage
- compare and branch lowering
- RIP-relative global addressing
- basic SSE float coverage
- stack layout and target-specific integration with the shared regalloc/cleanup port

## Porting Rule

If a shared utility is needed, extract it only after the target-specific path is clearly understood from the ref implementation.

Do not invent target-generic abstractions prematurely.

## Validation

- simple executable tests pass on x86-64 Linux
- arithmetic, branch, and call tests pass
- c-testsuite subset runs cleanly for the supported feature set

## Good First Patch

Port the narrowest return/arithmetic/call slice from the ref x86-64 backend and keep the emitted assembly path easy to compare against ref output.
