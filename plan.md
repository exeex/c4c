# RV64 Object Scalar Fragment Helper Cleanup Runbook

Status: Active
Source Idea: ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md

## Purpose

Peel RV64 object-route scalar helper ownership out of
`object_emission.cpp` into the prepared scalar emission module without pulling
in select-edge publication, predecessor publication movement, broad dispatch,
or behavior changes.

## Goal

Extract scalar binary, cast, compare-branch, move-to-register/location, and
simple return helpers into `prepared_scalar_emit.*` while preserving emitted
bytes, diagnostics, compare normalization, and branch-label fixup behavior.

## Core Rule

This is a behavior-preserving helper cleanup. Do not weaken tests,
expectations, unsupported markers, branch behavior, diagnostics, or object byte
contracts to claim progress.

## Read First

- `ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- Nearby extracted modules:
  - `src/backend/mir/riscv/codegen/prepared_local_memory_emit.*`
  - `src/backend/mir/riscv/codegen/prepared_global_memory_emit.*`
  - `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.*`
  - `src/backend/mir/riscv/codegen/prepared_call_emit.*`

## Current Targets And Scope

Owned files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- `todo.md`

Scalar-owned helper families:

- scalar binary fragments and prepared pointer-add fragments
- scalar casts
- compare value and compare branch fragments
- move-to-register and move-to-location helpers for scalar values
- simple return fragments
- narrow support helpers required only by those scalar fragments

## Non-Goals

- Do not move select-edge publication or predecessor publication movement.
- Do not move before-return move bundles or full terminator dispatch.
- Do not move `fragment_for_prepared_instruction` or turn
  `prepared_scalar_emit.*` into a dispatch facade.
- Do not change branch semantics, compare predicate normalization, branch-label
  fixups, diagnostics, emitted bytes, gcc_torture expectations, or unsupported
  markers.
- Do not absorb call emission, local/global memory ownership, object-data
  symbol/fixup ownership, or function traversal ownership.

## Working Model

`object_emission.cpp` should keep object-file emission, shared low-level
encoding, and genuinely cross-cutting wrappers. `prepared_scalar_emit.*` should
own scalar fragment construction behind explicit function declarations. Shared
helpers that are still used by select publication, edge movement, call
emission, function traversal, or object-data handling should stay parked until
a matching source idea owns them.

## Execution Rules

- Keep each step narrow and update `todo.md` after each packet.
- Use AST-backed caller checks where helpful before deleting wrappers.
- Preserve call compatibility for live public helpers used by extracted modules.
- If a helper still has direct callers outside scalar-owned paths, leave it
  parked and record why in `todo.md`.
- Any expectation rewrite, unsupported downgrade, or named-case-only shortcut is
  route drift and must be rejected.

## Steps

### Step 1: Map Scalar Helper Ownership

Goal: identify exactly which scalar helpers move, which wrappers stay parked,
and which declarations/includes are needed.

Actions:

- Inspect scalar-related helpers in `object_emission.cpp` and
  `prepared_scalar_emit.*`.
- Classify helpers as:
  - move into `prepared_scalar_emit.*`
  - remain parked because select/publication, call, memory, object-data, or
    dispatch ownership still uses them
  - candidate dead wrapper for Step 3 caller proof
- Record the move set, parked set, include/declaration prerequisites, and exact
  Step 2 validation command in `todo.md`.
- Do not edit implementation files in this step.

Completion check:

- `todo.md` names the current scalar helper map and the Step 2 proof command.

### Step 2: Extract Scalar Fragment Helpers

Goal: move the scalar-owned fragment helpers into `prepared_scalar_emit.*`
without changing behavior.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`

Actions:

- Move scalar binary, prepared pointer-add, cast, compare branch/value,
  move-to-register/location, and simple return helpers that Step 1 classifies
  as scalar-owned.
- Add only the declarations and includes required by the extracted helpers.
- Keep select publication, edge publication movement, before-return move
  bundles, broad instruction dispatch, and `fragment_for_prepared_instruction`
  out of the scalar module.
- Preserve compare predicate normalization, branch-label fixups, diagnostics,
  and emitted assembly/object bytes.
- Update `todo.md` with changed files, proof, and any parked helper rationale.

Completion check:

- Run:

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'" > test_after.log 2>&1
```

- The command passes, `test_after.log` exists, and `todo.md` records the proof.

### Step 3: Prune Dead Scalar Wrappers

Goal: remove only wrappers proven dead after scalar extraction.

Actions:

- Use `rg` and, where useful, `c4c-clang-tool-ccdb function-callers` to confirm
  direct callers for candidate wrappers from Step 1.
- Delete only helpers with no direct callers and no intended public boundary.
- Retain helpers that remain shared with select/publication, call, memory,
  object-data, function traversal, or dispatch ownership.
- Update `todo.md` with removed and retained helper rationale.

Completion check:

- Run the same Step 2 validation command into `test_after.log`.
- `todo.md` records caller-check evidence and proof.

### Step 4: Close-Readiness Review

Goal: decide whether this runbook satisfies the source idea and is ready for
plan-owner closure evaluation.

Actions:

- Review the final diff against
  `ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md`.
- Confirm scalar ownership is narrower and not a catch-all monolith.
- Confirm select/publication and broad dispatch remain separate.
- Confirm no tests, expectations, unsupported markers, diagnostics, branch
  behavior, or object byte contracts were weakened.
- Record close-readiness or blockers in `todo.md`.

Completion check:

- `todo.md` states whether the runbook is ready for closure evaluation and
  names any remaining parked helper work that belongs to later ideas.
