# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Activated from: user-prioritized switch on 2026-04-10

## Purpose

Finish the already-translated x86 codegen surface so it becomes part of the
real backend path instead of parked source inventory.

## Goal

Turn the existing translated x86 codegen tree into an active part of the build
and emission path:

- audit which translated units exist versus which are actually compiled
- bring reachable translated codegen units into the build in bounded slices
- reduce `emit.cpp`-local ownership where translated siblings already exist
- complete peephole orchestration and integration as one sub-slice
- prove each connected slice with targeted tests

## Core Rules

- Prefer enabling existing translated units over inventing new `emit.cpp`-local
  logic.
- Keep this plan scoped to x86 translated-code integration.
- Do not silently absorb shared-BIR cleanup from idea 44 into this runbook.
- Add targeted regression coverage before claiming integration.

## Current Scope

- translated-but-not-built top-level x86 codegen units:
  `alu.cpp`, `asm_emitter.cpp`, `atomics.cpp`, `calls.cpp`, `cast_ops.cpp`,
  `comparison.cpp`, `f128.cpp`, `float_ops.cpp`, `globals.cpp`,
  `i128_ops.cpp`, `inline_asm.cpp`, `intrinsics.cpp`, `memory.cpp`,
  `prologue.cpp`, `returns.cpp`, `variadic.cpp`
- translated peephole subtree not yet on the real main path:
  `peephole/mod.cpp`, `peephole/types.cpp`, and the translated pass files under
  `peephole/passes/*.cpp`
- `src/backend/x86/codegen/peephole/mod.cpp`
- `src/backend/x86/codegen/peephole/passes/*.cpp`
- `src/backend/x86/codegen/emit.cpp`
- `CMakeLists.txt`
- matching reference files under
  `ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/`

## Non-Goals

- no shared-BIR legacy matcher cleanup
- no generic x86 emitter redesign
- no unrelated runtime, ABI, or parser work
- no broad backend refactors outside the peephole integration boundary

## Execution Phases

### Step 1: Audit translated x86 codegen coverage

Goal: identify which translated x86 codegen and peephole units are present,
compiled, and reachable.

Completion check:

- the translated-unit inventory is explicit
- the first implementation slice is chosen from the recorded top-level or
  peephole inventory

### Step 2: Bring translated units into the build and reachable path

Goal: connect already-translated code instead of leaving ownership parked in
`emit.cpp`.

Completion check:

- at least one translated unit or cluster is compiled and reachable
- targeted tests prove the active path uses that code

### Step 3: Complete peephole orchestration and emission integration

Goal: ensure emitted x86 assembly actually flows through the peephole stage.

Completion check:

- `passes::peephole_optimize(...)` performs pass rounds instead of returning
  input unchanged
- x86 emission invokes `peephole_optimize(...)`
- targeted backend tests observe optimized output

### Step 4: Continue transferring ownership out of `emit.cpp`

Goal: continue shrinking the gap between translated x86 codegen inventory and
the real active path.

Completion check:

- remaining disconnected translated units are explicit
- the next ownership-transfer slice is clear
