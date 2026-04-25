# BIR Local GEP Family Extraction Runbook

Status: Active
Source Idea: ideas/open/07_bir-local-gep-family-extraction.md

## Purpose

Move the local GEP family out of
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` into a dedicated
implementation file while preserving BIR lowering behavior.

## Goal

`memory/local_gep.cpp` should own local address/projection lowering, while
`local_slots.cpp` keeps local slot declaration plus local load/store
materialization.

## Core Rule

This is an implementation-placement cleanup only. Do not change BIR output,
diagnostics, supported-path contracts, or test expectations.

## Read First

- `ideas/open/07_bir-local-gep-family-extraction.md`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`

## Current Targets

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Do not add new `.hpp` files. Keep `lowering.hpp` as the complete private
member index.

## Non-Goals

- Do not redesign local GEP semantics.
- Do not split local load/store behavior.
- Do not move alloca or local slot declaration code.
- Do not introduce `MemoryLoweringState`.
- Do not move state ownership out of `BirFunctionLowerer`.
- Do not convert member functions to free functions taking
  `BirFunctionLowerer& self`.
- Do not rewrite test expectations.

## Working Model

- `local_slots.cpp` should own alloca, local slot declaration, local load/store,
  and dynamic aggregate load/store behavior.
- `local_gep.cpp` should own the local GEP address/projection family.
- `lowering.hpp` should continue to declare the moved
  `BirFunctionLowerer` members.
- File-private helpers move only when they are exclusively needed by the local
  GEP family.
- Shared projection facts should come from existing memory helper surfaces when
  needed; this plan does not create new helper headers.

## Execution Rules

- Move code mechanically before improving naming or structure.
- Keep includes minimal and explicit in the new `.cpp`.
- Preserve helper behavior, diagnostics, and fallback paths exactly.
- Prefer existing helpers over copying logic.
- Do not broaden into shared address-projection consolidation; that belongs to
  `ideas/open/08_bir-address-projection-model-consolidation.md`.
- For every code-changing step, build before accepting the slice.
- Relevant proof must include local GEP and local-memory tests, including
  aggregate, pointer-array, dynamic aggregate, and base-pointer cases when
  available.

## Steps

### Step 1: Inventory The Local GEP Family

Goal: identify the exact move boundary before editing implementation files.

Primary target:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`

Actions:

- Locate the local GEP family named in the source idea:
  `resolve_local_aggregate_gep_slot`,
  `resolve_local_aggregate_pointer_array_slots`,
  `resolve_local_aggregate_dynamic_pointer_array_access`,
  `resolve_local_aggregate_gep_target`,
  `resolve_local_dynamic_aggregate_array_access`,
  `build_dynamic_local_aggregate_array_access`,
  `resolve_dynamic_local_aggregate_gep_projection`,
  `try_lower_dynamic_local_aggregate_gep_projection`,
  `try_lower_local_slot_pointer_gep`,
  `try_lower_local_array_slot_gep`,
  `try_lower_local_pointer_array_base_gep`, and
  `try_lower_local_pointer_slot_base_gep`.
- Identify file-private helpers, structs, and includes used only by this
  family.
- Identify helpers and includes that must remain with alloca, local slot
  declaration, load/store, or dynamic aggregate load/store.

Completion check:

- The executor can list the functions and exclusive helpers that will move.
- No code has been rewritten to change behavior.

### Step 2: Create `memory/local_gep.cpp`

Goal: add the dedicated implementation file and move only required includes.

Primary target:

- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`

Actions:

- Create `local_gep.cpp`.
- Include `../lowering.hpp`.
- Include `memory_helpers.hpp` only if the moved code uses shared projection
  helpers from that header.
- Add any standard or project includes required by the moved local GEP family.
- Do not add a new header file.

Completion check:

- `local_gep.cpp` exists with minimal includes and no unrelated lowering code.

### Step 3: Move The Local GEP Members

Goal: relocate the local GEP family without changing implementation behavior.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`

Actions:

- Move the local GEP member function definitions from `local_slots.cpp` to
  `local_gep.cpp`.
- Move file-private helpers only if they are exclusively used by those moved
  definitions.
- Keep alloca, local slot declaration, load/store, and dynamic aggregate
  load/store code in `local_slots.cpp` unless a tiny exclusive helper must move
  with the GEP family.
- Preserve function bodies and diagnostics while moving.

Completion check:

- `local_gep.cpp` defines the local GEP family.
- `local_slots.cpp` no longer contains the moved local GEP member definitions.
- The moved functions remain `BirFunctionLowerer` members.

### Step 4: Preserve The Private Index And Build Wiring

Goal: keep declarations and build integration aligned with the new file.

Primary targets:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- the existing build file that lists BIR lowering sources

Actions:

- Confirm `lowering.hpp` still declares the moved members exactly once.
- Do not move declarations to a new header.
- Add `memory/local_gep.cpp` to the existing build source list if required by
  the build system.
- Remove now-unused includes from `local_slots.cpp` only when the build or
  compiler diagnostics make the dependency clearly obsolete.

Completion check:

- The build system compiles `local_gep.cpp`.
- `lowering.hpp` remains the complete private index.
- No new `.hpp` files were added.

### Step 5: Build And Narrow Proof

Goal: prove the extraction did not change lowering behavior.

Actions:

- Build `c4c_codegen`.
- Run relevant BIR/LIR-to-BIR GEP and local-memory tests.
- Include coverage for aggregate local GEP, local pointer arrays, dynamic
  aggregate local access, local array slot GEP, and local pointer slot/base GEP
  paths when available.
- Do not rewrite expectations.

Completion check:

- `c4c_codegen` builds.
- The selected local GEP/local memory tests pass.
- Any failure is investigated as a behavior-preservation issue, not masked by
  expectation changes.

### Step 6: Final Lifecycle Readiness Check

Goal: make the completed slice easy for the supervisor to validate and commit.

Actions:

- Confirm `memory/local_gep.cpp` owns the local GEP family.
- Confirm `memory/local_slots.cpp` still owns local slot declaration and
  local load/store responsibilities.
- Confirm no new `.hpp` files were added.
- Confirm the moved functions are still `BirFunctionLowerer` members declared
  in `lowering.hpp`.
- Confirm no test expectations changed.

Completion check:

- All source acceptance criteria are satisfied, or remaining issues are
  recorded in `todo.md` for supervisor routing.
