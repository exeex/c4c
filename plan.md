# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Activated from: ideas/closed/40_target_profile_and_execution_domain_foundation.md

## Purpose

Finish the cutover to BIR-only backend ownership by removing the remaining
legacy backend-IR production seams, then retire the now-stale legacy tests and
build wiring.

## Goal

Leave the repo with one backend IR path:

- production/backend routing accepts only LIR entry or prelowered BIR
- x86/aarch64 emitters consume only direct LIR probes or BIR-native paths
- legacy backend-IR code and legacy backend-IR-centric tests are deleted or
  fully parked out of active use

## Current State

Already done in the current branch:

- x86 legacy `BackendModule` emitter entry hard-locked
- aarch64 legacy `BackendModule` emitter entry hard-locked
- shared backend `BackendModule` public emission removed
- legacy backend-IR-centric test targets removed from active CMake / ctest wiring

Remaining work is now best handled as four parallel workstreams plus one mainline
integration lane.

## Parallel Execution

### Group A

Mission:
- finish the x86 emitter-side conversion away from legacy backend IR internals

Write ownership:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp) if needed

Expected outcomes:
- remove dead `BackendModule`-typed helper paths where possible
- trim obsolete legacy includes when helper/type dependencies are gone
- keep x86 emitter entry behavior unchanged from the newly locked boundary

Worker-local validation:
- compile only the representative production object:
  `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/x86/codegen/emit.cpp.o`

Out of scope:
- shared backend routing
- aarch64 emitter
- legacy test-file deletion

### Group B

Mission:
- finish the aarch64 emitter-side conversion away from legacy backend IR internals

Write ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed

Expected outcomes:
- remove dead `BackendModule`-typed helper paths where possible
- trim obsolete legacy includes when helper/type dependencies are gone
- keep aarch64 emitter entry behavior unchanged from the newly locked boundary

Worker-local validation:
- compile only the representative production object:
  `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Out of scope:
- shared backend routing
- x86 emitter
- legacy test-file deletion

### Group C

Mission:
- remove the remaining shared lowering/legacy-IR production surfaces

Write ownership:
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- legacy IR printer/validator headers or sources only if required by the same deletion batch

Expected outcomes:
- shrink or remove production ownership of `lower_lir_to_backend_module(...)`
- delete dead compatibility helpers that only exist for legacy backend IR
- keep any still-needed bridge code explicitly temporary and documented

Worker-local validation:
- compile only touched representative objects:
  `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`
- or
  `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`

Out of scope:
- x86 emitter-local helper cleanup
- aarch64 emitter-local helper cleanup
- deleting parked legacy tests unless directly required by the same seam removal

### Group D

Mission:
- retire or migrate the parked legacy backend-IR-centric tests and scaffolding

Write ownership:
- [`tests/backend/backend_lir_adapter_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_tests.cpp)
- [`tests/backend/backend_lir_adapter_aarch64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_aarch64_tests.cpp)
- [`tests/backend/backend_lir_adapter_x86_64_tests.cpp`](/workspaces/c4c/tests/backend/backend_lir_adapter_x86_64_tests.cpp)
- [`tests/backend/backend_x86_64_extracted_tests.cpp`](/workspaces/c4c/tests/backend/backend_x86_64_extracted_tests.cpp)
- [`tests/backend/backend_module_tests.cpp`](/workspaces/c4c/tests/backend/backend_module_tests.cpp)
- directly related test support files only if required

Expected outcomes:
- delete files that are now pure legacy scaffolding
- migrate any still-valuable assertions into surviving BIR/native test families
- keep test coverage aligned with BIR-only architecture

Worker-local validation:
- compile only the touched representative test object, for example:
  `cmake --build build -j8 --target CMakeFiles/backend_bir_tests.dir/tests/backend/backend_bir_pipeline_tests.cpp.o`
- if editing a parked legacy file temporarily, use its matching object target only

Out of scope:
- production emitter cleanup
- shared lowering cleanup unless required for a migrated assertion

## Mainline Integration

Mainline stays responsible for:

- updating [`plan.md`](/workspaces/c4c/plan.md), [`todo.md`](/workspaces/c4c/todo.md), and
  [`todoA.md`](/workspaces/c4c/todoA.md) through [`todoD.md`](/workspaces/c4c/todoD.md)
- conflict resolution and commit ordering across A/B/C/D
- broad validation after worker slices land
- deciding when to escalate from parked legacy state to physical deletion

Mainline validation after merging worker output:

- `cmake -S . -B build`
- `cmake --build build -j8`
- targeted surviving backend tests as appropriate
- broader `ctest -R backend` only after reintegration, not inside each worker lane

## Completion Check

This plan is complete when:

- no live production path depends on legacy `BackendModule` emission
- no active backend test/build path depends on legacy backend-IR-only seams
- emitter and lowering ownership is BIR-first rather than legacy-IR-first
- remaining legacy files are either deleted or clearly documented as temporary
