# BIR Memory Entrypoints And Helper Boundaries Runbook

Status: Active
Source Idea: ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md

## Purpose

Clarify the BIR memory lowering boundary after the memory header/helper and
coordinator split work, without changing memory lowering semantics.

## Goal

Keep stateful memory lowering owned by `BirFunctionLowerer` member entrypoints
while making `memory_helpers.hpp` a normal pure-helper declaration surface.

## Core Rule

Use this boundary throughout the work:

```text
member entrypoint owns stateful lowering flow
free helper owns pure reusable reasoning
anonymous helper owns file-private glue
```

Do not replace stateful member lowering with broad free functions that take
`BirFunctionLowerer& self`.

## Read First

- `ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md`
- `src/c4c/lir_to_bir/lowering.hpp`
- `src/c4c/lir_to_bir/memory_helpers.hpp`
- memory-related `.cpp` files under `src/c4c/lir_to_bir/`

## Current Targets

- `lower_local_memory_alloca_inst`
- `lower_memory_gep_inst`
- `lower_memory_load_inst`
- `lower_memory_store_inst`
- `lower_memory_memcpy_inst`
- `lower_memory_memset_inst`
- any call-memory entrypoint introduced during this work, if needed
- pure memory helper structs/functions for layout facts, byte-offset
  projection, scalar leaf lookup, scalar subobject reasoning, byte-storage
  reinterpretation, repeated extents, and pointer-array extents

## Non-Goals

- Do not redesign BIR memory semantics.
- Do not introduce `MemoryLoweringState`.
- Do not split every memory `.cpp` into matching headers.
- Do not convert member lowering operations into functional-style free
  functions over the whole lowerer.
- Do not do broad coordinator or call-lowering redesign.
- Do not add new `.hpp` files.
- Do not rewrite test expectations.

## Working Model

- `lowering.hpp` remains the private index for lowerer state, member
  entrypoints, and member helper declarations.
- `memory_helpers.hpp` is a regular `#pragma once` helper header for pure
  reusable reasoning only.
- File-private glue stays in anonymous namespaces in the `.cpp` that uses it.
- `local_slots.cpp` should not grow as part of this work; any move out of it
  must be narrow, mechanical, and compile-proven.

## Execution Rules

- Keep changes behavior-preserving unless the source idea explicitly requires a
  semantic change.
- Prefer small, compile-proven refactoring packets.
- Keep every stateful memory flow visible as a `BirFunctionLowerer` member in
  `lowering.hpp`.
- Promote only reusable pure reasoning to `memory_helpers.hpp`.
- Leave one-off glue local to its implementation file.
- Run a fresh build proof for code-changing steps.
- Run relevant BIR/LIR-to-BIR narrow tests before accepting the refactor.

## Step 1: Inspect The Current Memory Boundary

Goal: identify the existing macro-included declaration pattern and the exact
memory member/helper declarations that must be preserved.

Primary targets:
- `src/c4c/lir_to_bir/lowering.hpp`
- `src/c4c/lir_to_bir/memory_helpers.hpp`
- memory-related `.cpp` files under `src/c4c/lir_to_bir/`

Actions:
- Inspect how `memory_helpers.hpp` is included and whether it has macro modes.
- List the current memory member entrypoints and member helpers.
- Identify helper declarations that are pure reusable reasoning versus
  stateful lowerer behavior.
- Record any `local_slots.cpp` memory families that are narrow enough to move
  later, but do not move them in this step.

Completion check:
- The executor can name the declarations that must move to explicit
  `lowering.hpp` declarations and the declarations that should remain pure
  helpers.

## Step 2: Remove Macro-Included Member Declarations

Goal: replace the `memory_helpers.hpp` member-fragment include pattern with
explicit declarations in `lowering.hpp`.

Primary targets:
- `src/c4c/lir_to_bir/lowering.hpp`
- `src/c4c/lir_to_bir/memory_helpers.hpp`

Actions:
- Copy the existing member entrypoint and member helper declarations into the
  correct private area of `BirFunctionLowerer` in `lowering.hpp`.
- Remove the macro include path or declaration-fragment mode from
  `memory_helpers.hpp`.
- Preserve declaration signatures and behavior.

Completion check:
- `lowering.hpp` explicitly lists memory member entrypoints and member helpers.
- `memory_helpers.hpp` is no longer used as a class-fragment include.
- `c4c_codegen` builds.

## Step 3: Normalize The Pure Helper Header

Goal: make `memory_helpers.hpp` a normal helper declaration surface.

Primary target:
- `src/c4c/lir_to_bir/memory_helpers.hpp`

Actions:
- Keep `memory_helpers.hpp` as a normal `#pragma once` header.
- Remove macro-mode guards and preprocessor shape used only for declaration
  injection.
- Keep helper result structs and pure helper function declarations that support
  layout facts, projections, scalar subobjects, byte reinterpretation, repeated
  extents, and pointer-array extents.
- Do not add new headers.

Completion check:
- `memory_helpers.hpp` exposes only pure helper structs/functions.
- No stateful `BirFunctionLowerer` member declarations are hidden in
  `memory_helpers.hpp`.
- `c4c_codegen` builds.

## Step 4: Review Entrypoint And Helper Placement

Goal: ensure the resulting boundary matches the source idea and does not move
stateful behavior into broad free helpers.

Primary targets:
- `src/c4c/lir_to_bir/lowering.hpp`
- memory-related `.cpp` files under `src/c4c/lir_to_bir/`

Actions:
- Confirm the target memory entrypoints remain `BirFunctionLowerer` members.
- Keep the number of exposed member entrypoints small and semantic.
- Move only reusable pure reasoning to `memory_helpers.hpp`.
- Keep file-private glue anonymous and local.
- If a non-local-slot memory family is already isolated enough to move out of
  `local_slots.cpp`, perform only a narrow mechanical move with compile proof.
  Otherwise leave it alone.

Completion check:
- No new broad `foo(BirFunctionLowerer& self, ...)` lowering rewrite exists.
- `local_slots.cpp` does not grow; any movement is narrow and mechanical.
- `c4c_codegen` builds.

## Step 5: Prove No Behavior Change

Goal: validate the refactor without expectation rewrites.

Actions:
- Build `c4c_codegen`.
- Run the relevant BIR/LIR-to-BIR narrow tests selected by the supervisor.
- Escalate to broader validation if the touched surface spans more than the
  memory boundary or if multiple narrow-only packets have landed.

Completion check:
- Build proof is fresh.
- Relevant narrow tests pass.
- No expectations were rewritten.
- The acceptance criteria in the source idea are satisfied.
