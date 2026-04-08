# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Activated from: ideas/closed/40_target_profile_and_execution_domain_foundation.md

## Purpose

Shift the execution plan from "keep trimming legacy seams" to "finish the BIR
contract that legacy seams are still covering". The immediate priority is to
identify and start migrating the remaining emitter-facing `BackendModule`
semantics into BIR so legacy IR deletion becomes safe instead of premature.

## Goal

Leave the repo with a concrete BIR parity map that answers:

- which `BackendModule`-owned semantics are still required by x86 emitters
- which `BackendModule`-owned semantics are still required by aarch64 emitters
- which shared parser/helper seams in `call_decode` still assume legacy IR
- which of those semantics belong in `bir.hpp` versus temporary glue

This round is successful when the next implementation slices can be framed as
"add/migrate BIR shape X" instead of "poke at legacy IR until it breaks".

## Current State

Already complete on the current branch:

- shared backend public `emit_module(const BackendModule&, ...)` entry removed
- [`src/backend/backend.cpp`](/workspaces/c4c/src/backend/backend.cpp) no longer directly includes
  `lir_to_backend_ir.hpp`
- [`src/backend/backend.hpp`](/workspaces/c4c/src/backend/backend.hpp) no longer carries a
  `BackendModule` forward declaration
- x86 emitter no longer depends on `LirAdapterError`
- aarch64 emitter no longer depends on legacy lowering/printer/validator
  includes and has shed some thin `BackendModule` pass-through wrappers
- parked legacy backend-IR test files are deleted from the tree

Still blocking source-idea closure:

- x86 and aarch64 emitters still have large private `BackendModule` helper
  surfaces
- `call_decode.hpp` still exposes many `parse_backend_minimal_*_module(...)`
  helpers that take `BackendModule`
- `lir_to_backend_ir.*` still exists as live lowering glue
- `ir.hpp`, `ir_printer.*`, and `ir_validate.*` still exist and still have
  active code/test references
- LLVM rescue behavior still exists in `src/codegen/llvm/llvm_codegen.cpp` and
  `src/apps/c4cll.cpp`

## Parallel Execution

### Group A

Mission:
- inventory the x86 emitter's remaining `BackendModule` dependency and identify
  the BIR data/shape needed to replace it

Write ownership:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp) if needed for notes

Expected outcomes:
- group the remaining x86 `BackendModule` helper surface into bounded semantic
  clusters
- identify which clusters are pure plumbing, which are parser helpers, and
  which require real module/function metadata
- annotate or lightly refactor only when it materially clarifies the required
  BIR replacement shape

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/x86/codegen/emit.cpp.o`

Out of scope:
- aarch64 emitter changes
- shared `bir.hpp` expansion
- deleting legacy IR files

### Group B

Mission:
- inventory the aarch64 emitter's remaining `BackendModule` dependency and
  identify the BIR data/shape needed to replace it

Write ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed for notes

Expected outcomes:
- group the remaining aarch64 `BackendModule` helper surface into bounded
  semantic clusters
- identify which clusters are pure plumbing, which are parser helpers, and
  which require real module/function metadata
- annotate or lightly refactor only when it materially clarifies the required
  BIR replacement shape

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Out of scope:
- x86 emitter changes
- shared `bir.hpp` expansion
- deleting legacy IR files

### Group C

Mission:
- map the shared legacy seam to concrete BIR parity work, starting from
  `call_decode` and the remaining `BackendModule` parser surface

Write ownership:
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp)
- [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp) if needed
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp) and adjacent BIR helpers only if a
  narrowly scoped parity addition is obvious and bounded

Expected outcomes:
- identify which `call_decode` / lowering helpers still require
  `BackendModule`
- distinguish "needs BIR structure expansion" from "needs emitter-local
  rewrite"
- if safe, add the smallest BIR-facing abstraction or helper signature change
  that reduces future `BackendModule` coupling

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/call_decode.cpp.o`
- or `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/lowering/lir_to_backend_ir.cpp.o`

Out of scope:
- deep emitter-local refactors owned by Group A or B
- deleting `ir.*` in this round
- LLVM rescue removal

### Group D

Mission:
- parked for this round

State:
- the parked legacy adapter test family is already gone
- do not dispatch a worker lane here unless a later round discovers a new
  bounded test-only follow-up

## Mainline Integration

Mainline stays responsible for:

- maintaining [`plan.md`](/workspaces/c4c/plan.md), [`todo.md`](/workspaces/c4c/todo.md), and
  [`todoA.md`](/workspaces/c4c/todoA.md) through [`todoD.md`](/workspaces/c4c/todoD.md)
- integrating the just-landed `backend.cpp` / `backend.hpp` cleanup with the
  next parity-focused wave
- resolving overlaps between emitter inventory and shared parity work
- running reintegration validation:
  `cmake -S . -B build`
  `cmake --build build -j8`
  targeted backend tests
  `ctest --test-dir build -R backend -j --output-on-failure` when appropriate

## Close Readiness

This plan is not ready to close until all of the following are true:

- the remaining emitter-facing `BackendModule` semantics have BIR equivalents
- emitter/helper code consumes BIR-first structures instead of `BackendModule`
- `lir_to_backend_ir.*` and `ir.*` are dead or trivially deletable
- active tests no longer depend on `ir_printer.*` / `ir_validate.*`
- LLVM rescue behavior is removed or explicitly split into a separate active
  idea before closing idea 41

## This Round Exit Criteria

This round is successful when:

- each active A/B/C lane can name the remaining `BackendModule` usage in terms
  of concrete missing BIR shape
- mainline can restate the next wave as explicit BIR additions and consumer
  migrations
- the repo is closer to "BIR parity first, deletion second" than it was at the
  start of the round
