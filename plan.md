# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Activated from: ideas/closed/40_target_profile_and_execution_domain_foundation.md

## Purpose

Shift the execution plan from "keep trimming legacy seams" to "expand the BIR
contract so legacy seams can disappear safely". The immediate priority is to
add the missing call / extern / global / string / local-slot shape that still
forces emitter and parser code to depend on `BackendModule`.

## Goal

Leave the repo with concrete BIR contract expansion underway:

- bounded BIR shape additions exist for the highest-priority missing contracts
- emitter/shared code can point at specific new BIR fields/helpers instead of
  abstract "future parity"
- the next migrations can target real BIR consumers rather than more inventory

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
- parity inventory has already identified the first obvious missing BIR shape
  clusters: call / extern / global / string / local-slot

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
- prepare x86 to consume the first new BIR contract additions, focusing on the
  already-identified high-value clusters

Write ownership:
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/emit.hpp`](/workspaces/c4c/src/backend/x86/codegen/emit.hpp) if needed

Expected outcomes:
- convert the current x86 inventory into a migration-ready cut list
- identify the first x86 helpers that should switch once call / extern /
  global / string / local-slot shape lands in BIR
- make only bounded clarifying edits that reduce future migration friction

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/x86/codegen/emit.cpp.o`

Out of scope:
- aarch64 emitter changes
- owning shared `bir.hpp` schema design
- deleting legacy IR files

### Group B

Mission:
- prepare aarch64 to consume the first new BIR contract additions, focusing on
  the already-identified high-value clusters

Write ownership:
- [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [`src/backend/aarch64/codegen/emit.hpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.hpp) if needed

Expected outcomes:
- convert the current aarch64 inventory into a migration-ready cut list
- identify the first aarch64 helpers that should switch once call / extern /
  global / string / local-slot shape lands in BIR
- make only bounded clarifying edits that reduce future migration friction

Worker-local validation:
- `cmake --build build -j8 --target CMakeFiles/c4cll.dir/src/backend/aarch64/codegen/emit.cpp.o`

Out of scope:
- x86 emitter changes
- owning shared `bir.hpp` schema design
- deleting legacy IR files

### Group C

Mission:
- expand `bir.hpp` and adjacent shared helpers with the first bounded contract
  additions needed to replace `BackendModule` parser/emitter seams

Write ownership:
- [`src/backend/bir.hpp`](/workspaces/c4c/src/backend/bir.hpp)
- [`src/backend/bir.cpp`](/workspaces/c4c/src/backend/bir.cpp) if needed
- [`src/backend/bir_printer.hpp`](/workspaces/c4c/src/backend/bir_printer.hpp) / [`src/backend/bir_printer.cpp`](/workspaces/c4c/src/backend/bir_printer.cpp) if needed
- [`src/backend/bir_validate.hpp`](/workspaces/c4c/src/backend/bir_validate.hpp) / [`src/backend/bir_validate.cpp`](/workspaces/c4c/src/backend/bir_validate.cpp) if needed
- [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- [`src/backend/lowering/call_decode.cpp`](/workspaces/c4c/src/backend/lowering/call_decode.cpp) if needed
- [`src/backend/lowering/lir_to_backend_ir.hpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.hpp) /
  [`src/backend/lowering/lir_to_backend_ir.cpp`](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp) if needed

Expected outcomes:
- add the smallest useful BIR shape for one or more of:
  call / extern / global / string / local-slot
- update shared helpers or notes so the new shape is explicitly connected to
  the remaining `BackendModule` seams
- keep additions bounded and migration-oriented rather than trying to finish
  all parity in one slice

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
  next BIR contract-expansion wave
- resolving overlaps between emitter migration prep and shared BIR shape work
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

- each active A/B/C lane can point to concrete BIR contract additions or the
  immediate consumer migrations they unlock
- mainline can restate the next wave as "switch consumer X to BIR shape Y"
- the repo is materially closer to "BIR contract expansion first, deletion
  second" than it was at the start of the round
