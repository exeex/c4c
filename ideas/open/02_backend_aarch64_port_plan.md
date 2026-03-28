# AArch64 Backend Port Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on: `ideas/open/01_backend_lir_adapter_plan.md`

## Goal

Port the AArch64 backend from `ref/claudes-c-compiler/src/backend/` into C++ with behavior and structure kept as close to ref as practical.

## Why AArch64 First

- it is the highest-priority target in the roadmap
- it is the most natural bring-up target for current development hardware
- it gives the fastest end-to-end proof that the native backend path is viable

## Porting Style

- prefer mechanical translation from Rust to C++
- keep file and mechanism boundaries close to ref
- only diverge where our LIR adapter boundary forces it
- prefer a stack-first, spill-heavy bring-up over early register-allocation work
- accept obviously non-optimal code shape during bring-up if it keeps correctness and backend boundaries easy to verify

## Scope

### Bring-up

- prologue and epilogue emission
- stack slot addressing for the initial simple frame model
- integer loads, stores, arithmetic, shifts, and compares
- conditional and unconditional branches
- direct calls and returns
- global addressing
- conservative call-boundary handling that can spill and reload aggressively to preserve correctness

### Completion

- float basics
- calling convention coverage needed by current tests
- stack layout improvements beyond the naive initial model
- a functionally complete stack-first path that can run the intended supported subset even when values are handled conservatively through stack slots across calls
- any AArch64-specific integration work needed to adopt the shared regalloc/cleanup follow-on tracked in `ideas/open/03_backend_regalloc_peephole_port_plan.md`

## Explicit Non-Goals

- x86-64 port
- rv64 port
- built-in assembler
- built-in linker

## Validation

- trivial return tests pass
- arithmetic and branch tests pass
- recursive and multi-call tests pass
- c-testsuite simple coverage works for the supported subset
- execution works through the agreed Linux target path
- supported cases remain correct under the deliberately conservative spill-heavy bring-up strategy

## Risk Notes

- host is not the same as the Linux execution target, so the toolchain wrapper must stay boring and deterministic
- avoid burying ABI fixes inside unrelated cleanup patches

## Good First Patch

Port the minimal AArch64 return-only and integer arithmetic path closely from ref and validate with external assembler/linker fallback.
