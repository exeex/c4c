# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Activated from: ideas/closed/40_target_profile_and_execution_domain_foundation.md

## Purpose

Finish the transition from "legacy backend IR is locked behind hard stops" to
"legacy backend IR is no longer load-bearing". This plan stays optimized for
parallel subagent execution while mainline owns reintegration, broad
validation, and final commit sequencing.

## Goal

Reach a state where the remaining closure work for idea 41 is narrow and
mechanical:

- x86 and aarch64 emitter internals no longer depend on legacy
  `BackendModule`-typed helpers for active behavior
- shared lowering no longer presents legacy backend-IR production seams as
  normal public workflow
- parked backend-IR-centric tests are either deleted or migrated into surviving
  BIR/native coverage
- mainline has a clean map of the final legacy files still blocking close
  (`ir.*`, `lir_to_backend_ir.*`, LLVM rescue paths, and any residual tests)

## Current State

Already complete on the current branch:

- shared backend public `emit_module(const BackendModule&, ...)` entry removed
- x86 and aarch64 public emitter headers no longer expose legacy
  `BackendModule` emission entry points
- legacy backend-module test targets are removed from active CMake / ctest
  wiring
- two pure parked legacy test scaffolds have already been deleted
- backend-only reintegration validation has passed after the first cleanup wave

Not yet complete relative to the source idea:

- x86 emitter internals still contain many `BackendModule` helpers and
  `LirAdapterError`-based fallback branches
- aarch64 emitter internals still contain many `BackendModule` helpers even
  after the public shim/includes cleanup
- `lir_to_backend_ir.*` still exists and still exports
  `lower_lir_to_backend_module(...)`, `render_module(...)`, and
  `LirAdapterError`
- `ir.hpp`, `ir_printer.*`, and `ir_validate.*` still exist and are still
  referenced by active code/tests
- LLVM rescue behavior in `src/codegen/llvm/llvm_codegen.cpp` and
  `src/apps/c4cll.cpp` still exists, so idea 41 cannot close yet

## Parallel Execution

### Group A

Mission:
- reduce x86 emitter-local ownership of legacy backend IR until the remaining
  blockers are explicit and minimal

Write ownership:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp) if needed

Expected outcomes:
- remove dead internal `BackendModule` entry helpers that are no longer
  reachable after boundary locking
- isolate or annotate any remaining x86 `BackendModule` helpers that still
  block full BIR-native emitter migration
- trim obsolete legacy includes once the file no longer needs them

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/x86/codegen/emit.cpp.o`

Out of scope:
- aarch64 emitter internals
- shared lowering / `lir_to_backend_ir.*`
- parked test deletion
- final broad validation

### Group B

Mission:
- reduce aarch64 emitter-local ownership of legacy backend IR until the
  remaining blockers are explicit and minimal

Write ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed

Expected outcomes:
- remove dead internal `BackendModule` entry helpers that are no longer
  reachable after boundary locking
- isolate or annotate any remaining aarch64 `BackendModule` helpers that still
  block full BIR-native emitter migration
- trim obsolete legacy includes once the file no longer needs them

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Out of scope:
- x86 emitter internals
- shared lowering / `lir_to_backend_ir.*`
- parked test deletion
- final broad validation

### Group C

Mission:
- shrink the shared legacy lowering seam so it is no longer presented as a
  normal backend path

Write ownership:
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- directly adjacent shared backend headers only if required by the same seam
  reduction

Expected outcomes:
- reduce public exposure of `lower_lir_to_backend_module(...)`
- remove dead compatibility helpers that exist only for legacy backend IR
- leave any still-required bridge code explicitly temporary and easy to delete

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`
- or `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`

Out of scope:
- emitter-local helper cleanup owned by Group A or B
- parked legacy tests unless required by the same API reduction
- LLVM rescue removal

### Group D

Mission:
- parked for this round

State:
- the `backend_lir_adapter*` legacy test family has already been deleted in the
  second wave
- do not dispatch a worker lane here unless a later round discovers a new
  bounded test-only follow-up

## Mainline Integration

Mainline stays responsible for:

- maintaining [`plan.md`](/workspaces/c4c/plan.md), [`todo.md`](/workspaces/c4c/todo.md), and
  [`todoA.md`](/workspaces/c4c/todoA.md) through [`todoD.md`](/workspaces/c4c/todoD.md)
- dispatching subagents with strict file ownership and single-object builds
- resolving overlaps, patch conflicts, and partial reductions across groups
- running reintegration validation:
  `cmake -S . -B build`
  `cmake --build build -j8`
  targeted backend tests
  `ctest --test-dir build -R backend -j --output-on-failure` when appropriate
- deciding when remaining work is small enough to shift from parallel lanes to
  a final closeout lane

## Close Readiness

This plan is not ready to close until all of the following are true:

- no active production path depends on emitter-internal `BackendModule`
  ownership
- `lir_to_backend_ir.*` is deleted or reduced to a clearly temporary,
  non-public bridge with an agreed follow-up deletion step
- `ir.*` support files are deleted or fully retired from active code/tests
- parked legacy backend-IR tests are deleted or migrated
- LLVM rescue behavior is removed or explicitly split into a separate active
  idea before closing idea 41

## This Round Exit Criteria

This round is successful when:

- each active A/B/C lane can hand mainline a bounded patch with no cross-lane
  write conflicts
- the remaining legacy blockers are fewer, more localized, and easier to name
- mainline can restate the post-round close gap in a shorter checklist than the
  current one
