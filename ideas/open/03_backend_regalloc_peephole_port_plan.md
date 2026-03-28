# Backend Regalloc And Peephole Port Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/02_backend_aarch64_port_plan.md`

Should precede:

- `ideas/open/04_backend_x86_64_port_plan.md`
- `ideas/open/05_backend_rv64_port_plan.md`

## Goal

Port the shared register-allocation, liveness, and peephole/cleanup mechanisms from `ref/claudes-c-compiler/src/backend/` into C++ after the first AArch64 backend slice is functionally useful but before the next target ports begin.

## Why This Sits Between AArch64 And x86-64

- the first AArch64 bring-up is the cheapest place to prove the existing backend boundary
- register allocation and late cleanup are shared backend mechanisms, not only AArch64 details
- porting those shared layers once before x86-64 and rv64 reduces duplicated target-specific workaround work
- later target ports should inherit the same ref-shaped liveness/regalloc/cleanup structure instead of each reinventing a stack-only path

## Porting Style

- prefer mechanical translation from Rust to C++
- keep file and phase boundaries close to ref
- treat target-specific register sets and ABI quirks as thin per-target inputs to shared machinery
- avoid mixing new optimization ideas with the ref-matching port

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`
- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/regalloc_helpers.rs`
- target-local prologue/emit integration points under:
  `ref/claudes-c-compiler/src/backend/arm/`,
  `ref/claudes-c-compiler/src/backend/x86/`,
  `ref/claudes-c-compiler/src/backend/riscv/`

## Scope

### Shared bring-up

- port liveness analysis needed by the ref linear-scan allocator
- port the shared linear-scan register allocator and its core data structures
- port the helper boundary that runs regalloc before final frame sizing
- port the smallest ref-shaped peephole or cleanup passes required for correctness or practical code quality

### Target integration

- thread the new shared machinery into the AArch64 backend first
- keep x86-64 and rv64 integration points ready to reuse the same shared layers
- preserve explicit target ownership of register pools, caller/callee-saved sets, and ABI-driven clobber rules

## Explicit Non-Goals

- full SSA redesign
- novel optimizer work beyond the ref backend's existing cleanup shape
- built-in assembler
- built-in linker
- broad target-generic backend refactors unrelated to regalloc or late cleanup

## Validation

- AArch64 runtime coverage still passes with the regalloc-enabled path
- pass-rate gains or code-shape improvements are attributable to the new shared machinery
- x86-64 and rv64 plans can depend on this idea instead of each carrying their own first regalloc port

## Good First Patch

Port the minimum shared liveness and regalloc helper boundary needed to let AArch64 compute frame usage with ref-shaped register-assignment data before stack-size finalization.
