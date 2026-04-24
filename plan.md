# BIR Memory Helper Consolidation Runbook

Status: Active
Source Idea: ideas/open/02_bir-memory-helper-consolidation.md

## Purpose

Consolidate duplicated BIR memory helper logic after the memory vocabulary
headers exist.

## Goal

Reduce duplicated layout walking, byte-offset projection, scalar leaf lookup,
scalar subobject checks, repeated aggregate extents, pointer-array offset
queries, and byte-storage reinterpretation checks across the memory lowering
implementation without changing BIR semantics.

## Core Rule

Keep `BirFunctionLowerer` as the state owner and keep `lowering.hpp` as the
complete private lowerer index. Shared helpers must stay pure and
argument-driven behind the existing memory helper surface.

## Read First

- `ideas/open/02_bir-memory-helper-consolidation.md`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`

## Current Targets

- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp` only as needed to preserve the
  private index and declaration surface

## Non-Goals

- Do not create additional `.hpp` files.
- Do not split `coordinator.cpp`.
- Do not introduce `MemoryLoweringState`.
- Do not move state ownership out of `BirFunctionLowerer`.
- Do not redesign address provenance semantics.
- Do not rewrite test expectations to claim progress.

## Working Model

- `memory_types.hpp` owns memory vocabulary types and aliases.
- `memory_helpers.hpp` owns shared, pure helper declarations or definitions for
  matching memory semantics.
- Implementation files keep caller-specific policy, mutable state updates, and
  map mutation near their existing owners.
- Equivalent traversal or projection logic may be shared; similar-looking logic
  with different policy remains separate.

## Execution Rules

- Use AST-backed checks where useful before moving declarations or helper
  call sites.
- Keep each code-changing step behavior-preserving.
- Prefer small result structs over duplicated partial computations when the
  callers need related facts from the same traversal.
- Do not hide caller policy inside overly generic helpers.
- For code-changing steps, run at least:
  `cmake --build --preset default --target c4c_backend`.
- When a step changes multiple memory implementation files, also run the
  relevant backend subset:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Run `git diff --check` before handing a code slice back.

## Steps

### Step 1: Inventory Equivalent Helpers

Goal: identify the concrete duplicated helper families before changing code.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`

Actions:
- Compare layout walking, offset projection, scalar leaf lookup, scalar
  subobject checks, byte-storage reinterpretation, repeated aggregate extent
  lookup, and pointer-array length-at-offset lookup.
- Mark helpers that differ only by return shape, naming, or local control flow.
- Mark helpers whose semantics are genuinely caller-specific and should remain
  local.
- Record the candidate merge order in `todo.md` so implementation packets stay
  narrow.

Completion check:
- `todo.md` names the concrete helper families to merge, the files they touch,
  and the families intentionally left separate.

### Step 2: Merge Scalar Leaf And Scalar Subobject Reasoning

Goal: consolidate duplicate scalar layout fact checks where semantics match.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`

Actions:
- Extract or reuse shared pure helpers for scalar leaf lookup by byte offset
  and scalar subobject addressability when they inspect the same layout facts.
- Use a small result struct if multiple callers need related facts from one
  traversal.
- Keep caller-specific addressability, alias, or provenance policy in the
  caller.
- Preserve existing behavior for unsupported or ambiguous layout cases.

Completion check:
- Duplicate scalar layout reasoning is reduced without moving mutable
  `BirFunctionLowerer` state access into shared helpers.
- `cmake --build --preset default --target c4c_backend` passes.
- `git diff --check` passes.

### Step 3: Merge Repeated Aggregate And Pointer-Array Extent Helpers

Goal: reuse common traversal code for repeated aggregate extent and
pointer-array length-at-offset queries where traversal semantics match.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`

Actions:
- Compare existing repeated aggregate extent and pointer-array offset helpers.
- Extract shared traversal or projection pieces only where array and struct
  offset semantics are identical.
- Keep caller-specific policy for which extents count, how ambiguity is
  reported, and when a query should fall back.

Completion check:
- Equivalent traversal logic is shared behind `memory_helpers.hpp`, while
  caller policy remains visible in implementation files.
- `cmake --build --preset default --target c4c_backend` passes.
- `git diff --check` passes.

### Step 4: Normalize Byte-Storage Reinterpretation Checks

Goal: centralize common checks for whether a byte-storage view can be
interpreted as a target type at a target offset.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`

Actions:
- Identify duplicated "can this byte storage view be interpreted as this
  target type" logic.
- Extract a pure helper that takes explicit type declarations and target
  offsets.
- Keep provenance updates, alias map mutation, and caller-specific fallback
  behavior in the existing implementation files.

Completion check:
- Shared byte-storage reinterpretation logic lives behind
  `memory_helpers.hpp`.
- No helper reaches into mutable `BirFunctionLowerer` state.
- `cmake --build --preset default --target c4c_backend` passes.
- `git diff --check` passes.

### Step 5: Validate Helper Boundaries

Goal: prove the consolidation stayed within the source idea boundaries.

Primary targets:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/*.cpp`

Actions:
- Confirm no new memory headers were introduced.
- Confirm `coordinator.cpp` was not split as part of this idea.
- Confirm `BirFunctionLowerer` remains the memory state owner.
- Confirm shared helpers in `memory_helpers.hpp` are pure and argument-driven,
  and mutable map updates remain in implementation files.
- Confirm no expectation rewrites were used as proof.
- Run:
  `cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Run `git diff --check`.

Completion check:
- Backend build and backend subset pass.
- Structural checks satisfy the source idea acceptance criteria.
- `todo.md` records close-readiness or any remaining source-idea gap.
