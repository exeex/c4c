# x86_64 Peephole Pipeline Completion

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Activated from: user-prioritized switch on 2026-04-10

## Purpose

Finish the translated x86 peephole pipeline so it becomes a real backend stage
instead of a stubbed reference mirror.

## Goal

Turn the existing `src/backend/x86/codegen/peephole/` tree into an active part
of x86 code generation:

- audit the translated pass surface
- implement real orchestration in `passes/mod.cpp`
- wire `peephole_optimize(...)` into the x86 emission path
- prove the pipeline changes emitted assembly in targeted tests

## Core Rules

- Prefer enabling existing translated passes over inventing new optimization
  logic.
- Keep this plan scoped to x86 peephole orchestration and integration.
- Do not silently absorb shared-BIR cleanup from idea 44 into this runbook.
- Add targeted regression coverage before claiming pass enablement.

## Current Scope

- `src/backend/x86/codegen/peephole/mod.cpp`
- `src/backend/x86/codegen/peephole/passes/*.cpp`
- `src/backend/x86/codegen/emit.cpp`
- matching reference files under
  `ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/`

## Non-Goals

- no shared-BIR legacy matcher cleanup
- no generic x86 emitter redesign
- no unrelated runtime, ABI, or parser work
- no broad backend refactors outside the peephole integration boundary

## Execution Phases

### Step 1: Audit the translated pass surface

Goal: identify the minimum viable pass subset and any missing infrastructure.

Completion check:

- the pass inventory is explicit
- the first implementation slice is chosen

### Step 2: Implement orchestration in `passes/mod.cpp`

Goal: replace the stub orchestration path with a real pass pipeline.

Completion check:

- `passes::peephole_optimize(...)` performs pass rounds instead of returning
  input unchanged
- targeted tests cover at least one local pass round and one global pass round

### Step 3: Integrate peephole optimization into x86 emission

Goal: ensure emitted x86 assembly actually flows through the peephole stage.

Completion check:

- x86 emission invokes `peephole_optimize(...)`
- targeted backend tests observe optimized output

### Step 4: Expand enabled pass coverage carefully

Goal: enable additional translated passes as their assumptions are verified on
current emitter output.

Completion check:

- enabled versus deferred passes are explicit
- any deferred pass has a concrete reason
