# RV64 Object Local Memory Helper Cleanup Runbook

Status: Active
Source Idea: ideas/open/537_rv64_object_local_memory_helper_cleanup.md

## Purpose

Separate RV64 object-route local memory helper ownership from the remaining object emission shell while preserving prepared access facts and diagnostics.

## Goal

Move local frame-slot load/store, local pointer materialization, and pointer-value base-plus-offset helper families into `prepared_local_memory_emit.*` without changing behavior.

## Core Rule

This is a behavior-preserving ownership cleanup. Do not change local-array semantics, pointer provenance, prepared-memory facts, tests, unsupported markers, or gcc_torture expectations.

## Read First

- `ideas/open/537_rv64_object_local_memory_helper_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`

## Non-Goals

- Do not move global symbol materialization, prepared data-object emission, `.rodata`/`.data`/`.bss` assembly, object symbol definition, relocation handling, or ELF writing.
- Do not repair BIR inference, prepared memory facts, target-side inference, or RV64 capability gaps.
- Do not change local-array behavior, pointer provenance, diagnostics, tests, or expectation files.
- Do not expand this cleanup into later sequence items such as global address, scalar fragment, select edge, call, traversal, or data-symbol cleanup.

## Working Model

- `prepared_local_memory_emit.*` already owns simple textual local load/store helpers.
- This runbook should extend that same boundary to the encoded RV64 object-route local memory helpers currently parked in `object_emission.cpp`.
- Keep APIs structured around existing prepared access facts, stack-layout facts, value-home lookups, and frame helper functions.
- Leave public object-route wrappers in place when they still provide compatibility for object emission call sites or avoid widening the extraction.

## Execution Rules

- Prefer narrow, reviewable moves over broad rewrites.
- Preserve helper names unless a local rename is needed to avoid collision after extraction.
- If a helper serves both local-memory and global/object-data paths, either keep it parked or split only the local-memory portion.
- Record parked helper decisions in `todo.md`; do not edit the source idea unless source intent changes.
- For code-changing steps, run the delegated proof command after the build succeeds:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_prepared_local_array|obj_runtime_rv64_local_temp|obj_runtime_rv64_large_fixed_frame_slot_access|rv64_runtime_riscv64_pointer_to_pointer_local_address)'`

## Ordered Steps

### Step 1: Map the local-memory boundary

Goal: identify the exact local-memory helper set that can move without pulling in global/address or object-data ownership.

Actions:
- Inspect `object_emission.cpp` for encoded local memory helpers, especially `fragment_for_prepared_store_local`, `fragment_for_prepared_load_local`, local frame-slot offset helpers, local pointer materialization helpers, and pointer-value base-plus-offset helpers.
- Compare those helpers against existing declarations and private helpers in `prepared_local_memory_emit.*`.
- Classify helpers as:
  - owned by prepared local memory now
  - safe to move in this runbook
  - parked because they are shared with global/address, object data, module assembly, relocation, or dispatch paths
- Note the include and declaration prerequisites for the code-changing extraction step.
- Do not edit implementation files in this step unless the supervisor explicitly delegates code work.

Completion Check:
- `todo.md` records the move set, parked set, prerequisites, and the exact Step 2 validation command.

### Step 2: Extract the encoded local-memory helper family

Goal: move the in-scope encoded local-memory helpers into `prepared_local_memory_emit.*` while keeping object emission behavior unchanged.

Primary Target:
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:
- Move local frame-slot load/store fragment helpers and their direct local-only support routines from `object_emission.cpp` into `prepared_local_memory_emit.cpp`.
- Move local pointer materialization support that is specific to frame-slot or local pointer values.
- Move pointer-value base-plus-offset load/store support only when it is local-memory-specific and uses existing prepared memory access facts.
- Add only the minimal public declarations needed by object emission call sites.
- Keep global symbol materialization, prepared data objects, relocation handling, and module assembly in `object_emission.cpp`.
- Keep wrapper functions in `object_emission.cpp` when they preserve a stable object-route compatibility boundary.

Completion Check:
- The delegated proof command passes and writes the canonical proof log requested by the supervisor.
- No tests, expectations, unsupported markers, or prepared facts were weakened.
- The extracted API names and includes make the local-memory boundary clear.

### Step 3: Prune only genuinely dead local-memory wrappers

Goal: remove or park post-extraction wrappers based on actual call-site ownership.

Actions:
- Inspect remaining object-route local-memory wrappers and declarations after Step 2.
- Delete only wrappers that are now unused and are not preserving a public or compatibility boundary.
- Leave high-traffic or mixed-ownership wrappers parked, especially if they cross dispatch, frame, scalar, global/address, or object-data paths.
- Record retained wrappers and the reason for retention in `todo.md`.

Completion Check:
- The delegated proof command passes if code changes were made.
- `todo.md` explains any retained wrappers and why they should stay parked.

### Step 4: Close-readiness review

Goal: decide whether the active runbook and source idea are satisfied.

Actions:
- Compare the final helper boundary against `ideas/open/537_rv64_object_local_memory_helper_cleanup.md`.
- Confirm local memory helper ownership is separated from global/address and object-data concerns.
- Confirm prepared access facts and diagnostics are preserved.
- Confirm no testcase-shaped shortcuts, expectation downgrades, unsupported marker changes, or semantic local-array/pointer changes were introduced.
- Identify whether any remaining parked helpers belong to a later open cleanup idea rather than this one.

Completion Check:
- `todo.md` records whether the runbook is ready for plan-owner closure evaluation or requires a follow-up split.
