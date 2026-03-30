# x86 Backend Port Plan

Status: Completed 2026-03-29

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/01_backend_lir_adapter_plan.md`
- `ideas/closed/02_backend_aarch64_port_plan.md`
- `ideas/closed/03_backend_regalloc_peephole_port_plan.md`
- `ideas/closed/04_backend_binary_utils_contract_plan.md`
- lessons learned from:
  `ideas/closed/06_backend_builtin_assembler_aarch64_plan.md`
  and
  `ideas/closed/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Port `ref/claudes-c-compiler/src/backend/x86/` into `src/backend/x86/` as a ref-shaped C++ backend tree, then reconnect that mirrored tree to the current backend driver and LIR adapter with the smallest explicit shim surface needed.

## Current Starting Point

This idea starts after the first mechanical translation pass that mirrors the ref x86 tree into `src/backend/x86/` as `.cpp` files.

That translation pass is intentionally allowed to be non-compiling and non-integrated.

The next work for this idea is to turn that mirrored tree from passive translated files into a build-participating backend path.

## Why This Is Separate

- x86-64 backend bring-up should not interrupt the current unrelated active plan
- the first mechanical file translation is valuable even before integration work starts
- x86-specific ABI, addressing, and instruction encoding behavior should remain isolated from AArch64 follow-on work
- assembler and linker support should not be mixed into the first codegen-driver reconnect slice unless the codegen path explicitly requires it

## Porting Style

- prefer mechanical translation from Rust to C++
- keep file layout and module boundaries close to `ref/claudes-c-compiler/src/backend/x86/`
- treat the translated `.cpp` files as a mirrored substrate first, not as already-finished production code
- only introduce shared abstractions after the x86 target-local behavior is understood from ref
- keep the LIR adapter seam narrow and explicit
- accept temporary placeholders, commented mirrors, and stubbed seams during compile-integration

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/x86/mod.rs`
- `ref/claudes-c-compiler/src/backend/x86/codegen/`
- `ref/claudes-c-compiler/src/backend/x86/assembler/`
- `ref/claudes-c-compiler/src/backend/x86/linker/`
- shared backend layers already ported under:
  `src/backend/`

## Immediate Main Axis

This idea should proceed in two distinct phases:

1. preserve and normalize the mirrored `src/backend/x86/` translation tree so file coverage stays close to ref
2. reconnect that tree to the actual build, backend entry surfaces, and current LIR boundary in thin vertical slices

The main progress measure is not broad runtime coverage at first.

The main progress measure is:

- more translated x86 files become include-clean and build-participating
- the x86 backend path becomes reachable from normal backend entry points
- the existing LIR adapter grows a target-local attachment seam for x86 without destabilizing the current shared backend

## Scope

### Phase 1: Mirrored Tree Stabilization

- create and keep `src/backend/x86/` structurally aligned with the ref x86 module tree
- keep one-to-one `.rs` to `.cpp` coverage for target-local codegen, assembler, and linker files where practical
- add only the minimum namespace, include, and type-shim scaffolding required to keep the translated tree understandable
- do not force immediate compile correctness across the whole tree

### Phase 2: Compile Integration And Driver Reconnect

- make the translated x86 codegen subtree compile in bounded slices
- reconnect `src/backend/x86/mod.cpp` and codegen entry points to the existing backend driver
- define the narrowest x86-specific adapter between current LIR structures and ref-shaped codegen expectations
- switch the x86 target path from placeholder/unreachable status to real backend-owned assembly emission for a minimal supported subset
- defer built-in assembler and built-in linker activation unless the chosen slice requires them

### First Supported Behavioral Slice

- function prologue and epilogue
- integer return values
- basic ALU operations
- compare and branch
- direct calls and returns
- stack-relative locals
- RIP-relative global addressing

### Later Follow-Ons

- SSE float coverage
- variadic and ABI edge cases
- atomics
- i128 and f128 helper paths
- peephole and regalloc tightening specific to x86
- built-in assembler bring-up for x86
- built-in linker bring-up for x86

## Explicit Non-Goals

- modifying the current active `plan.md` while `std::vector` bring-up is active
- declaring the translated x86 tree complete just because the files exist
- full x86 assembler parity in the first integration slice
- full x86 linker parity in the first integration slice
- broad optimization or abstraction cleanup before the x86 path is wired through the normal backend entry

## Suggested Execution Order

1. audit the mirrored `src/backend/x86/` tree against ref and fill any missing file-coverage gaps
2. make the top-level `mod.cpp` and `codegen/mod.cpp` surfaces include-clean
3. define the narrowest x86 backend entry surface that can be called from the existing backend driver
4. reconnect the x86 path to the current LIR adapter through explicit target-local shims
5. port one minimal `emit`-driven vertical slice that can lower simple integer functions into x86 assembly text
6. widen behavior in ref order:
   `prologue`,
   `memory`,
   `alu`,
   `comparison`,
   `calls`,
   `globals`,
   `returns`,
   then later float, variadic, atomics, and wider utility slices
7. only after codegen output is stable, decide whether x86 built-in assembler or linker work should become separate open ideas

## Validation

- the mirrored x86 tree stays close to ref file coverage
- compile-integration progress is monotonic even if the whole target is not yet complete
- the backend driver can instantiate the x86 path without falling back through unrelated code paths
- minimal supported x86 cases emit backend-owned assembly text through the mirrored codegen path
- follow-on assembler or linker work is tracked as separate ideas rather than silently absorbed

## Good First Patch

Take the already translated `src/backend/x86/` mirror, make one thin codegen slice compile, connect it to the existing backend driver through an explicit x86 adapter seam, and emit assembly for a trivial integer-return function.

## Completion Notes

- Completed the first backend-owned x86 assembly slice through `src/backend/x86/codegen/emit.cpp`.
- `Target::X86_64` and `Target::I686` now route the minimal supported `main` return-immediate path through Intel-syntax x86 assembly instead of LLVM-text passthrough.
- Focused validation passed for `backend_lir_adapter_tests`, `backend_runtime_return_add`, and `backend_runtime_return_zero`.

## Leftover Issues

- Broader x86 runtime shapes still fall back to LLVM text, including branches, direct helper calls, local-slot patterns, parameter materialization, globals, and other non-immediate return flows.
- The next thin slice should target parameter/local/direct-call support before widening into branches, globals, assembler integration, or linker work.
