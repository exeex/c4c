# RV64 Backend Codegen Bring-Up From AArch64

Status: Open
Last Updated: 2026-04-09

## Why This Idea

The repository now has a copied and translated `src/backend/riscv` tree, but
that translation is still mostly inventory and isolated helper logic. It does
not yet define the actual native RV64 backend bring-up strategy for the current
c4c backend pipeline.

The right first step is not to finish every RISC-V subsystem at once. The
highest-leverage seam is the native codegen path, especially the emitter and
lowering surface that AArch64 already proves out.

This idea therefore narrows RV64 bring-up to native codegen only. The initial
goal is to establish an RV64 emitter/lowering skeleton by following the
AArch64 backend structure, rather than widening scope into assembler or linker
completion.

## Source Context

- `src/backend/aarch64/mod.cpp`
- `src/backend/aarch64/codegen/mod.cpp`
- `src/backend/aarch64/codegen/emit.cpp`
- `src/backend/aarch64/codegen/emit.hpp`
- `src/backend/riscv/mod.cpp`
- `src/backend/riscv/codegen/mod.cpp`
- `src/backend/riscv/codegen/emit.cpp`

## Current Snapshot

As of 2026-04-09:

- `src/backend/riscv` exists in-tree
- all translated Rust source inventory has been converted into `.cpp` siblings
- all legacy `.rs` files under `src/backend/riscv` have been removed
- the RISC-V tree contains many useful helper translations, but no integrated
  RV64 native backend contract yet

Current shape:

- `assembler` and `linker` translations exist, but they are not the first
  bring-up target
- `codegen` contains many translated helpers and structural mirrors
- the main missing seam is a coherent RV64 emitter/lowering contract comparable
  to AArch64's native backend path

## Goal

Bring up the first coherent RV64 native codegen path by porting the AArch64
backend method incrementally, starting from the emitter/lowering seam centered
on `src/backend/aarch64/codegen/emit.cpp`.

## Non-Goals

- do not try to finish the RISC-V assembler in the first wave
- do not try to finish the RISC-V linker in the first wave
- do not widen this into a full all-target backend rewrite
- do not force immediate parity with every AArch64 optimization or helper
- do not treat the current translated `riscv/*.cpp` files as already integrated
  production code

## Working Model

- treat AArch64 as the primary reference backend for structure and sequencing
- build RV64 around the same backend contracts where possible instead of
  inventing a separate architecture-specific pipeline shape
- prioritize emitter/lowering seams that unlock end-to-end native codegen
  earliest
- keep assembler and linker out of scope unless a tiny stub is strictly needed
  to keep the codegen work coherent
- prefer a thin working RV64 path over a broad but disconnected translation set

## Proposed Plan

### Step 1: Define the RV64 emitter ownership boundary

Goal: make `src/backend/riscv/codegen/emit.cpp` the explicit bring-up center.

Concrete actions:

- compare `src/backend/aarch64/codegen/emit.cpp` and
  `src/backend/riscv/codegen/emit.cpp`
- identify the minimum shared backend contracts RV64 must satisfy:
  - module validation
  - LIR/BIR entry routing
  - target register model
  - call ABI scaffolding
  - prologue/epilogue ownership
- write down which current `riscv/codegen/*.cpp` files should become true
  dependencies of `emit.cpp` first, and which should stay as follow-on slices

Completion check:

- RV64 bring-up is framed around a concrete emitter-centered dependency graph
- `emit.cpp` is confirmed as the first integration seam, not just another
  translated file

### Step 2: Port the minimal AArch64 emitter skeleton to RV64

Goal: establish the same top-level native codegen flow shape that AArch64 uses.

Concrete actions:

- mirror the AArch64 emitter structure into RV64 where architecture-neutral
  backend flow should match
- add or tighten RV64 equivalents for:
  - unsupported-slice diagnostics
  - module/function validation
  - minimal lowering entry points
  - native code emission staging
- keep the first version intentionally narrow, even if many operations still
  fail with explicit unsupported diagnostics

Completion check:

- `src/backend/riscv/codegen/emit.cpp` owns a real RV64 backend control flow
- unsupported paths fail at explicit RV64-native boundaries instead of through
  disconnected helper inventory

### Step 3: Integrate the first RV64 codegen dependency slice

Goal: connect the smallest set of helper files needed for an end-to-end native
RV64 path.

Concrete actions:

- choose the minimum dependency cluster beneath `emit.cpp`, likely from:
  - `calls.cpp`
  - `returns.cpp`
  - `prologue.cpp`
  - `memory.cpp`
  - `comparison.cpp`
- unify any duplicated local stand-in types into the first real RV64 codegen
  surface where necessary
- avoid broad cleanup; only consolidate what the emitter integration truly
  needs

Completion check:

- at least one bounded RV64 codegen path is integrated through real shared
  C++ interfaces rather than file-local stand-ins

### Step 4: Prove one minimal RV64 native path end to end

Goal: make one narrow RV64 backend slice actually work through the emitter.

Concrete actions:

- pick the smallest viable supported slice, such as:
  - affine integer return
  - simple arithmetic on integer arguments
  - minimal call/return path
- add focused regression coverage for that slice
- verify the path runs through RV64 native codegen ownership rather than
  placeholder structure only

Completion check:

- one narrow RV64-native path is demonstrably owned by the integrated backend
- the test or internal backend check pins that slice

### Step 5: Expand RV64 codegen breadth without widening scope

Goal: grow native RV64 support while keeping the initiative centered on codegen.

Concrete actions:

- add seams in leverage order from `emit.cpp` outward
- prefer slices that collapse several future gaps at once:
  - ABI/register assignment
  - stack frame setup
  - loads/stores
  - comparisons and branches
  - returns
- spin assembler/linker work into separate follow-on ideas if they become
  independently large

Completion check:

- RV64 codegen support expands through intentional emitter-led slices
- assembler/linker remain clearly out of first-wave ownership

## Acceptance Criteria

- the repo has an explicit RV64 backend bring-up plan centered on
  `src/backend/riscv/codegen/emit.cpp`
- AArch64 is used as the reference structure for RV64 native emitter bring-up
- at least one minimal RV64 codegen path is planned as an end-to-end owned seam
- assembler and linker are explicitly kept out of first-wave scope
- any broader RISC-V subsystem work discovered during execution is split into a
  separate idea instead of silently expanding this one

## Notes

- The translated `src/backend/riscv` tree is now useful as implementation
  inventory, but not yet evidence of an integrated backend.
- The first real integration work should happen where backend ownership is
  already clearest: the emitter/lowering seam represented by
  `src/backend/aarch64/codegen/emit.cpp`.
