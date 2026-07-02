# RV64 Object Global Address Helper Cleanup Runbook

Status: Active
Source Idea: ideas/open/538_rv64_object_global_address_helper_cleanup.md

## Purpose

Separate RV64 object-route global address helper ownership from the remaining object emission shell while preserving symbol, fixup, relocation-target, and object-byte behavior.

## Goal

Move global load/store and direct symbol/address materialization helper families that produce per-instruction fragments into `prepared_global_memory_emit.*` without moving final object data or module assembly.

## Core Rule

This is a behavior-preserving ownership cleanup. Do not change symbol spelling, visibility, relocation target behavior, object bytes, tests, unsupported markers, gcc_torture expectations, or data-object contracts.

## Read First

- `ideas/open/538_rv64_object_global_address_helper_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`

## Non-Goals

- Do not move `append_rv64_prepared_data_objects`, final `.rodata`/`.data`/`.bss` section selection, global object symbol definition, data relocation assembly, text module assembly, or ELF writing.
- Do not repair RV64 capabilities, BIR inference, prepared data-object contracts, target-side inference, or gcc_torture behavior.
- Do not change tests, expectation files, unsupported markers, symbol visibility, link-name spelling, relocation target contracts, or object bytes.
- Do not merge this cleanup into local memory, scalar fragment, select edge, call, traversal, or data-symbol cleanup work.

## Working Model

- `prepared_global_memory_emit.*` should own global-address helpers that materialize per-instruction fragments for loads, stores, and direct symbol/address references.
- `object_emission.cpp` should keep final object data assembly, module assembly, object section selection, and ELF-writing ownership.
- Local memory helpers and global address helpers must remain distinct. Use the boundary established by the preceding local-memory cleanup as a guardrail, not as permission to merge ownership.
- Keep public object-route wrappers in `object_emission.cpp` only when they preserve dispatch compatibility or prevent data-object/module ownership from leaking into the prepared global helper layer.

## Execution Rules

- Prefer narrow, reviewable moves over broad rewrites.
- Preserve helper names unless a local rename is needed to avoid collision after extraction.
- If a helper mixes per-instruction global-address fragments with data-object, module assembly, relocation assembly, or ELF-writing ownership, split only the per-instruction portion when that split is straightforward; otherwise park it and record the reason in `todo.md`.
- Do not route through text assembly parsing or hide AUIPC/LO12/fixup contracts behind opaque strings.
- Record parked helper decisions in `todo.md`; do not edit the source idea unless durable source intent changes.
- For code-changing steps, run the delegated proof command after the build succeeds:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_aggregate_global|codegen_route_riscv64_global_store|obj_runtime_rv64_global_store_prepared_value_preserves_source|rv64_runtime_global_load)'`

## Ordered Steps

### Step 1: Map the global-address boundary

Goal: identify the exact global load/store and direct symbol/address helper set that can move without pulling in final object data or module assembly ownership.

Actions:
- Inspect `object_emission.cpp` for encoded global memory helpers, direct symbol/address materialization helpers, AUIPC/LO12 fixup helpers, and global load/store fragment builders.
- Compare those helpers against existing declarations and private helpers in `prepared_global_memory_emit.*`.
- Classify helpers as:
  - owned by prepared global memory now
  - safe to move in this runbook
  - parked because they are shared with final data-object assembly, module assembly, text assembly, ELF writing, local memory, scalar fragments, dispatch, or relocation-assembly paths
- Note the include and declaration prerequisites for the code-changing extraction step.
- Do not edit implementation files in this step unless the supervisor explicitly delegates code work.

Completion Check:
- `todo.md` records the move set, parked set, prerequisites, and the exact Step 2 validation command.

### Step 2: Extract the global-address fragment helper family

Goal: move in-scope per-instruction global load/store and direct symbol/address helpers into `prepared_global_memory_emit.*` while keeping emitted object behavior unchanged.

Primary Target:
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:
- Move global load/store fragment helpers and their direct global-only support routines from `object_emission.cpp` into `prepared_global_memory_emit.cpp`.
- Move direct symbol/address materialization support only when it emits per-instruction fragments and preserves explicit fixup target contracts.
- Add only the minimal public declarations needed by object emission call sites.
- Keep `append_rv64_prepared_data_objects`, object section selection, global object symbol definition, data relocation assembly, text module assembly, and ELF writing in `object_emission.cpp`.
- Keep wrapper functions in `object_emission.cpp` when they preserve a stable object-route compatibility boundary.

Completion Check:
- The delegated proof command passes and writes the canonical proof log requested by the supervisor.
- No symbol spelling, visibility, relocation targets, object bytes, tests, expectations, unsupported markers, or data-object contracts were changed.
- The extracted API names and includes make the global-address boundary clear.

### Step 3: Prune only genuinely dead global-address wrappers

Goal: remove or park post-extraction wrappers based on actual call-site ownership.

Actions:
- Inspect remaining object-route global/address wrappers and declarations after Step 2.
- Delete only wrappers that are now unused and are not preserving a public or compatibility boundary.
- Leave high-traffic or mixed-ownership wrappers parked, especially if they cross dispatch, local memory, scalar fragments, data-object assembly, module assembly, relocation assembly, or ELF-writing paths.
- Record retained wrappers and the reason for retention in `todo.md`.

Completion Check:
- The delegated proof command passes if code changes were made.
- `todo.md` explains any retained wrappers and why they should stay parked.

### Step 4: Close-readiness review

Goal: decide whether the active runbook and source idea are satisfied.

Actions:
- Compare the final helper boundary against `ideas/open/538_rv64_object_global_address_helper_cleanup.md`.
- Confirm global/address helper ownership is separated from local memory, final object data, and module assembly concerns.
- Confirm symbol spelling, fixup target contracts, visibility, relocation targets, and emitted object behavior are preserved.
- Confirm no testcase-shaped shortcuts, expectation downgrades, unsupported marker changes, or RV64 capability repairs were mixed into this cleanup.
- Identify whether any remaining parked helpers belong to a later open cleanup idea rather than this one.

Completion Check:
- `todo.md` records whether the runbook is ready for plan-owner closure evaluation or requires a follow-up split.
