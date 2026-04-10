# x86_64 Translated Codegen Integration

Status: Open
Last Updated: 2026-04-10

## Why This Idea

The x86 backend already contains a large translated implementation surface under
`src/backend/x86/codegen/`, including both top-level codegen units and the
peephole optimizer. A substantial portion of that surface appears to be a
mechanical translation from the Rust reference, but the current build and entry
path still rely almost entirely on:

- `src/backend/x86/codegen/mod.cpp`
- `src/backend/x86/codegen/emit.cpp`

At the moment, many translated `.cpp` files exist but are not actually wired
into the build or the active x86 emission path. That means the repo is paying
the maintenance cost of translated code without receiving the ownership and
coverage benefits.

This is not the same problem as shared-BIR legacy matcher cleanup. That work
belongs to idea 44. This idea is specifically about integrating already
translated x86 codegen and peephole pieces so they become real backend stages
instead of parked source inventory.

## Source Context

- `ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/mod.rs`
- `src/backend/x86/codegen/peephole/mod.cpp`
- `src/backend/x86/codegen/peephole/passes/mod.cpp`
- `src/backend/x86/codegen/emit.cpp`

## Current State

The current tree contains many translated x86 codegen units, including:

- ALU / memory / calls / returns / prologue / globals / variadic and related
  top-level codegen units
- a full `peephole/` subtree with translated pass files
- helper/type scaffolding for the peephole pipeline

On the current C++ tree:

- CMake still compiles only `codegen/mod.cpp` and `codegen/emit.cpp`
- `mod.cpp` is effectively an anchor, not a dispatcher
- `emit.cpp` still owns the practical emission route
- the peephole orchestration entry point is still stubbed
- the x86 emission path does not currently flow through the translated peephole
  optimizer
- many translated top-level codegen units are therefore not yet real
  participants in the backend

So the immediate gap is integration, build wiring, and ownership transfer, not
raw file presence.

## Initial Integration Inventory

The active inventory for this idea starts from the currently translated but not
meaningfully integrated x86 codegen surface.

### Already Present But Not In The Build: top-level codegen units

- `src/backend/x86/codegen/alu.cpp`
- `src/backend/x86/codegen/asm_emitter.cpp`
- `src/backend/x86/codegen/atomics.cpp`
- `src/backend/x86/codegen/calls.cpp`
- `src/backend/x86/codegen/cast_ops.cpp`
- `src/backend/x86/codegen/comparison.cpp`
- `src/backend/x86/codegen/f128.cpp`
- `src/backend/x86/codegen/float_ops.cpp`
- `src/backend/x86/codegen/globals.cpp`
- `src/backend/x86/codegen/i128_ops.cpp`
- `src/backend/x86/codegen/inline_asm.cpp`
- `src/backend/x86/codegen/intrinsics.cpp`
- `src/backend/x86/codegen/memory.cpp`
- `src/backend/x86/codegen/prologue.cpp`
- `src/backend/x86/codegen/returns.cpp`
- `src/backend/x86/codegen/variadic.cpp`

### Present But Not Yet On The Real Main Path: peephole subtree

- `src/backend/x86/codegen/peephole/mod.cpp`
- `src/backend/x86/codegen/peephole/types.cpp`
- `src/backend/x86/codegen/peephole/passes/callee_saves.cpp`
- `src/backend/x86/codegen/peephole/passes/compare_branch.cpp`
- `src/backend/x86/codegen/peephole/passes/copy_propagation.cpp`
- `src/backend/x86/codegen/peephole/passes/dead_code.cpp`
- `src/backend/x86/codegen/peephole/passes/frame_compact.cpp`
- `src/backend/x86/codegen/peephole/passes/helpers.cpp`
- `src/backend/x86/codegen/peephole/passes/local_patterns.cpp`
- `src/backend/x86/codegen/peephole/passes/loop_trampoline.cpp`
- `src/backend/x86/codegen/peephole/passes/memory_fold.cpp`
- `src/backend/x86/codegen/peephole/passes/mod.cpp`
- `src/backend/x86/codegen/peephole/passes/push_pop.cpp`
- `src/backend/x86/codegen/peephole/passes/store_forwarding.cpp`
- `src/backend/x86/codegen/peephole/passes/tail_call.cpp`

This list is the starting execution queue for idea 43. Future slices should
explicitly consume items from this inventory instead of inventing unrelated x86
work.

## Goal

Make the translated x86 codegen surface real and reachable:

- audit which translated units are present versus actually connected
- bring already-translated x86 codegen units into the build in a controlled way
- wire reachable translated functionality into the active x86 emission path
- complete peephole orchestration and integration as one sub-part of that work
- add focused regressions that prove the connected translated code is actually
  running

## Non-Goals

- do not treat peephole completion as a substitute for shared-BIR cleanup
- do not widen this into a generic x86 emitter rewrite
- do not silently absorb unrelated ABI/runtime or shared-BIR matcher debt
- do not promise full parity with the Rust reference in one slice if the
  infrastructure still needs bounded enablement work

## Working Model

- prefer integrating existing translated units over inventing new local
  `emit.cpp` ownership
- use the Rust/reference layout as the intended ownership map
- move one bounded integration seam at a time
- prove each newly connected slice with targeted asm-shape or backend tests
- keep this plan focused on translated code already in the tree

## Proposed Plan

### Step 1: Audit translated x86 codegen coverage

Goal: determine which translated codegen and peephole units already exist, which
are compiled, and which are still disconnected.

Completion check:

- the translated-unit inventory is explicit
- the first integration slice is identified

### Step 2: Bring translated units into the build and reachable path

Goal: start connecting already-translated code instead of leaving ownership in
`emit.cpp`-local seams.

Completion check:

- at least one translated unit or cluster is compiled and reachable
- targeted tests prove the active path uses that code

### Step 3: Complete peephole orchestration and emission integration

Goal: make the translated peephole pipeline a real backend stage once the
surrounding integration boundary is clear.

Completion check:

- `passes::peephole_optimize(...)` no longer returns input unchanged
- emitted x86 assembly flows through `peephole_optimize(...)`
- targeted backend tests demonstrate observable optimized output

### Step 4: Continue transferring ownership out of `emit.cpp`

Goal: keep reducing the gap between translated x86 codegen inventory and the
actual build/runtime path.

Completion check:

- the remaining disconnected translated units are explicit
- the next ownership-transfer slice is clear
