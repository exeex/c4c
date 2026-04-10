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

On the current C++ tree as of 2026-04-10:

- CMake compiles `codegen/mod.cpp`, `codegen/emit.cpp`, the local
  `direct_*.cpp` seam files, and the full translated `peephole/` subtree
- CMake already compiles `globals.cpp`, which carries bounded translated-owner
  behavior for the minimal scalar global-load slices
- CMake now also compiles `returns.cpp`, but only as explicit symbol/link
  coverage because the current public `x86_codegen.hpp` surface does not yet
  expose the exporter-backed `X86Codegen` return state its real translated
  bodies require
- CMake still does not compile the translated top-level owner units such as
  `alu.cpp`, `asm_emitter.cpp`, `atomics.cpp`, `calls.cpp`, `cast_ops.cpp`,
  `comparison.cpp`, `f128.cpp`, `float_ops.cpp`, `i128_ops.cpp`,
  `inline_asm.cpp`, `intrinsics.cpp`, `memory.cpp`, `prologue.cpp`, and
  `variadic.cpp`
- a 2026-04-10 compile probe against `calls.cpp` and the remaining untranslated
  top-level owners showed that the blocker is not just missing CMake wiring:
  the current public `x86_codegen.hpp` surface still forward-declares key
  translated ABI/IR types and omits exporter-backed `X86Codegen` helper/state
  declarations that those units depend on, so the next practical integration
  slice is header surfacing rather than another blind owner-file wire-in
- `mod.cpp` is still only an anchor, not the active dispatcher
- `emit.cpp` still owns the practical direct-BIR and prepared-LIR emission
  route through a large matcher/dispatcher surface
- the translated peephole entry is no longer stubbed:
  `emit.cpp` routes successful asm output through
  `codegen::peephole::peephole_optimize(...)`
- many top-level translated codegen units therefore remain parked source
  inventory even though the peephole subtree is already real backend code

So the immediate gap is no longer peephole activation. It is top-level build
wiring, emitter-entry ownership transfer, and the support-surface cleanup
required to let the translated owner path replace `emit.cpp`.

## Initial Integration Inventory

The active inventory for this idea starts from the currently translated but not
yet owner-integrated x86 codegen surface.

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

### Already Built And Reachable: peephole subtree

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

## Dependency Audit Refresh

The translated reference owner is
`ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs`.

Its dependency surface splits into four buckets on the current C++ tree:

1. already-built local ownership:
   `emit.cpp`, the `direct_*.cpp` sibling seams, and the translated peephole
   subtree
2. translated top-level implementation units that exist in-tree but are still
   excluded from CMake:
   `alu.cpp`, `asm_emitter.cpp`, `atomics.cpp`, `calls.cpp`, `cast_ops.cpp`,
   `comparison.cpp`, `f128.cpp`, `float_ops.cpp`, `globals.cpp`,
   `i128_ops.cpp`, `inline_asm.cpp`, `intrinsics.cpp`, `memory.cpp`,
   `prologue.cpp`, `returns.cpp`, and `variadic.cpp`
3. translated-support gaps that will block a straight owner cutover even after
   build wiring:
   `inline_asm.cpp` and `asm_emitter.cpp` are still placeholder-heavy, and
   several top-level units emit template markers rather than settled assembly
4. legacy owner surface that must eventually disappear:
   `try_emit_module(...)`, `try_emit_prepared_lir_module(...)`, both
   `emit_module(...)` overloads, and the associated direct matcher helpers in
   `emit.cpp`

## First Migration Cluster

The first compile-oriented migration cluster should stay narrow:

- add the translated leaf-like top-level units that depend on the existing
  accumulator/helper surface but do not require immediate prologue/call-frame
  ownership
- `globals.cpp` was the first successful candidate because it is relatively
  self-contained against `prologue.cpp`, `calls.cpp`, `variadic.cpp`, and
  inline-asm work
- `returns.cpp` can now join the build only as symbol/link coverage; its real
  bodies remain blocked on the hidden exporter-backed `X86Codegen` return-state
  surface
- the next migration slice should expose the minimum shared
  `x86_codegen.hpp` type/helper surface needed by the parked translated owners;
  `calls.cpp` is the current reference blocker because it immediately needs
  real `CallAbiConfig` / `CallArgClass` / `IrType` definitions plus
  exporter-backed `X86Codegen` members such as `state`, `operand_to_rax`,
  `store_rax_to`, and `store_rax_rdx_to`

This keeps the next execution slice focused on compiling translated owner units
into the build before attempting the larger `emit.cpp` dispatcher replacement.

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
