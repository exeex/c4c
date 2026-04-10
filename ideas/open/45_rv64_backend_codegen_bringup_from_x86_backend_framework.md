# RV64 Translated Codegen Integration

Status: Open
Last Updated: 2026-04-10

## Why This Idea

The repository already contains a translated `src/backend/riscv/` tree,
including assembler, codegen, linker, and target entry files. The main problem
is no longer "translate the Rust files." The translated files are already in
the tree. The real problem is that they are not meaningfully integrated:

- the current build does not appear to compile any `src/backend/riscv/*.cpp`
  units
- `src/backend/riscv/codegen/mod.cpp` is still a structural mirror, not a real
  dispatcher
- `src/backend/riscv/codegen/emit.cpp` exposes only a narrow helper subset and
  explicitly notes that the shared `RiscvCodegen` / `CodegenState` C++ surface
  does not exist yet

So the right framing for RV64 is the same one now used for x86 translated-code
work: treat the translated inventory as real source assets that must be wired
into the build and active backend path, instead of re-describing the problem as
greenfield bring-up.

## Source Context

- `src/backend/riscv/mod.cpp`
- `src/backend/riscv/codegen/mod.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/*.cpp`
- `src/backend/x86/codegen/`
- `plan.md`

## Current State

The `src/backend/riscv/` tree already contains translated source for:

- assembler
- codegen
- linker
- target entry

Current observed integration state:

- translated codegen siblings exist for ALU, memory, calls, returns, prologue,
  globals, casts, variadic, atomics, floating-point, intrinsics, inline asm,
  `i128`, `f128`, and peephole support
- the build does not currently list `src/backend/riscv/*.cpp` sources
- the RV64 codegen surface is therefore present as inventory but not active

This means the highest-leverage work is not "translate more Rust," but:

- define the first real RV64 integration boundary
- bring translated units into the build in bounded slices
- make `riscv/codegen/emit.cpp` depend on real translated siblings instead of
  staying a narrow source mirror

## Initial Integration Inventory

The active inventory for this idea starts from the translated RV64 surface that
already exists in-tree.

### Translated top-level RV64 codegen units

- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/asm_emitter.cpp`
- `src/backend/riscv/codegen/atomics.cpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/float_ops.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/i128_ops.cpp`
- `src/backend/riscv/codegen/inline_asm.cpp`
- `src/backend/riscv/codegen/intrinsics.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/mod.cpp`
- `src/backend/riscv/codegen/peephole.cpp`
- `src/backend/riscv/codegen/prologue.cpp`
- `src/backend/riscv/codegen/returns.cpp`
- `src/backend/riscv/codegen/variadic.cpp`

### Additional translated RV64 subsystems already present

- assembler subtree under `src/backend/riscv/assembler/`
- linker subtree under `src/backend/riscv/linker/`
- target entry at `src/backend/riscv/mod.cpp`

This list is the starting execution queue for RV64 integration. Future RV64
work should consume items from this inventory rather than treating the tree as
missing translation work.

## Goal

Turn the translated RV64 backend surface into a real staged integration lane:

- compile translated RV64 units in controlled slices
- establish a real codegen ownership boundary
- make `riscv/codegen/emit.cpp` and sibling units part of an active backend path
- keep assembler and linker present as inventory, but integrate them only when
  a bounded codegen slice truly needs them

## Non-Goals

- do not treat this as a greenfield RV64 rewrite
- do not translate more Rust just to create more parked inventory
- do not widen this into a full all-target backend redesign
- do not silently absorb unrelated x86 or shared-BIR cleanup work
- do not promise immediate end-to-end runtime execution before the codegen
  integration boundary is real

## Working Model

- use the x86 translated-code integration lane as the primary process model
- prefer integrating existing translated units over inventing new local
  stand-ins
- move one bounded RV64 integration seam at a time
- treat assembler and linker as follow-on consumers unless codegen integration
  truly requires them
- keep the first ownership transfers centered on `riscv/codegen/`

## Proposed Plan

### Step 1: Audit translated RV64 coverage

Goal: make the translated RV64 inventory explicit by role and current
integration state.

Completion check:

- the translated-unit inventory is explicit
- the first integration slice is chosen from that inventory

### Step 2: Bring translated RV64 codegen units into the build

Goal: stop treating RV64 codegen as parked source inventory.

Concrete actions:

- update the build to compile the first bounded RV64 codegen slice
- resolve the minimum shared header/type surface needed for that slice
- avoid broad rewrites; connect only the units needed for the active seam

Completion check:

- at least one translated RV64 codegen unit or cluster is compiled
- the chosen slice is reachable from a real RV64 backend entry boundary

### Step 3: Establish a real RV64 codegen ownership seam

Goal: make `riscv/codegen/emit.cpp` depend on real translated siblings instead
of acting as a narrow helper mirror.

Completion check:

- at least one bounded RV64 codegen path uses real sibling units
- the active RV64 entry surface is no longer pure structural inventory

### Step 4: Expand RV64 integration carefully

Goal: continue consuming the translated inventory in leverage order.

Completion check:

- remaining disconnected translated RV64 units are explicit
- the next ownership-transfer slice is clear
