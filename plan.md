# BIR Memory Coordinator Dispatch Split Runbook

Status: Active
Source Idea: ideas/open/03_bir-memory-coordinator-dispatch-split.md

## Purpose

Split the oversized BIR memory coordinator into smaller instruction-family
handlers while preserving the existing private index and memory header budget.

## Goal

Make `lower_scalar_or_local_memory_inst` a thin dispatcher over scalar,
address-int/cast, alloca/local-slot, GEP, load/store, and runtime memory
intrinsic families without changing BIR semantics or diagnostics.

## Core Rule

Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index, do not
add new `.hpp` files, and do not move memory state ownership out of
`BirFunctionLowerer`.

## Read First

- `ideas/open/03_bir-memory-coordinator-dispatch-split.md`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`

## Current Targets

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- Existing memory implementation files under
  `src/backend/bir/lir_to_bir/memory/`
- New `.cpp` files only when they represent a real instruction-family
  boundary
- `src/backend/bir/lir_to_bir/lowering.hpp` only for private method
  declarations needed by split families

## Non-Goals

- Do not create new `.hpp` files.
- Do not redesign BIR memory semantics.
- Do not introduce a standalone memory state owner.
- Do not combine this with helper or header extraction.
- Do not downgrade or weaken testcase expectations.
- Do not add named-case shortcuts or testcase-shaped branches.

## Working Model

- `lower_scalar_or_local_memory_inst` should become the dispatch point, not the
  place where every memory-related lowering rule is implemented.
- Instruction-family handlers may live in existing memory `.cpp` files or new
  family `.cpp` files when the boundary is real and reviewable.
- Shared memory vocabulary and pure helper logic stay behind
  `memory_types.hpp` and `memory_helpers.hpp`.
- Mutable map updates, provenance mutation, and lowerer state ownership stay on
  `BirFunctionLowerer` and its implementation methods.

## Execution Rules

- Move one instruction family or tightly coupled pair per code-changing step.
- Keep each step behavior-preserving unless a separate source idea explicitly
  authorizes a semantic fix.
- Prefer semantic family boundaries over one-file-per-case splits.
- Keep `coordinator.cpp` thin but still readable as the dispatch map.
- For code-changing steps, run at least:
  `cmake --build --preset default --target c4c_backend`.
- When a moved family spans multiple memory paths or dispatcher behavior, also
  run:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Run `git diff --check` before handing a code slice back.

## Steps

### Step 1: Split Scalar-Only Cases

Goal: move scalar compare, select, binop, and cast handling out of the memory
coordinator body when those cases do not depend on memory state.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- A scalar-family `.cpp` file if an existing file is not the right semantic
  home
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:
- Inspect the scalar-only cases currently handled by
  `lower_scalar_or_local_memory_inst`.
- Extract a private scalar-family handler with declarations kept in
  `lowering.hpp`.
- Leave memory-dependent cast or address-int behavior in the coordinator or a
  later address-int family if it depends on provenance or pointer maps.
- Keep `lower_scalar_or_local_memory_inst` as the dispatcher entry.

Completion check:
- Scalar-only cases no longer bulk up the memory coordinator implementation.
- No new `.hpp` files are added.
- `cmake --build --preset default --target c4c_backend` passes.
- `git diff --check` passes.

### Step 2: Split Alloca And Local-Slot Handling

Goal: move local memory declaration and slot setup into one coherent local
memory family.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:
- Group alloca, aggregate slot declaration, scratch slot setup, and local slot
  creation paths behind local-memory family handlers.
- Keep state ownership on `BirFunctionLowerer`; do not introduce a separate
  memory state object.
- Preserve existing diagnostics and unsupported-case behavior.

Completion check:
- Alloca and local-slot setup are reviewable independently from the main
  coordinator dispatch.
- `cmake --build --preset default --target c4c_backend` passes.
- `git diff --check` passes.

### Step 3: Split GEP Handling

Goal: route local slot GEP, local aggregate GEP, global GEP, and dynamic array
projection through GEP-family handlers.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:
- Extract GEP dispatch into private family methods while reusing existing
  memory vocabulary and helper functions.
- Keep local, global, and dynamic array projection branches explicit.
- Do not add testcase-shaped shortcuts for known GEP cases.

Completion check:
- GEP lowering is separated from unrelated coordinator cases and preserves the
  existing local/global/dynamic array behavior.
- `cmake --build --preset default --target c4c_backend` passes.
- Backend subset passes when the moved paths span multiple memory files.
- `git diff --check` passes.

### Step 4: Split Load And Store Handling

Goal: move load/store materialization into a family boundary that keeps
provenance, local slot, and dynamic array paths explicit.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:
- Extract load and store handling into private family methods or a tightly
  coupled load/store implementation file.
- Preserve pointer provenance updates, local slot materialization, and dynamic
  array load/store behavior.
- Keep alias/provenance mutation visible in implementation files rather than
  hiding it in generic helpers.

Completion check:
- Load/store lowering can be reviewed without reading the full coordinator
  implementation.
- `cmake --build --preset default --target c4c_backend` passes.
- Backend subset passes.
- `git diff --check` passes.

### Step 5: Split Runtime Memory Intrinsic Handling

Goal: separate runtime memory intrinsic dispatch when `memcpy`, `memset`, or
related paths remain coupled to the coordinator.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- Existing memory implementation files that already own the relevant helper
  behavior
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:
- Inspect remaining intrinsic-like memory cases after the earlier splits.
- Move only cohesive runtime memory intrinsic handling behind private family
  methods.
- Reuse existing local/global memory helpers without expanding the coordinator
  again.

Completion check:
- Runtime memory intrinsic paths, if present in the coordinator, have a clear
  family boundary.
- `cmake --build --preset default --target c4c_backend` passes.
- `git diff --check` passes.

### Step 6: Validate Coordinator Boundary

Goal: prove the split satisfies the source idea without semantic drift.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/*.cpp`

Actions:
- Confirm `lower_scalar_or_local_memory_inst` is a thin dispatcher.
- Confirm instruction-family handlers are independently reviewable.
- Confirm no new `.hpp` files were created.
- Confirm `lowering.hpp` remains the complete private index.
- Confirm `BirFunctionLowerer` still owns memory state.
- Confirm no expectation rewrites were used as proof.
- Run:
  `cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Run `git diff --check`.

Completion check:
- Backend build and backend subset pass.
- Structural checks satisfy the source idea acceptance criteria.
- `todo.md` records close-readiness or any remaining source-idea gap.
