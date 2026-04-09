# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2 architecture reset, still in `Phase 1: Ownership wiring`:
  move shared lowering ownership out of monolithic
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/{cfg,types,memory,calls,phi,aggregates}.cpp`
- build/test work is intentionally ignored in this phase per `plan.md`
  until the lane explicitly switches to `Phase 2: Compile recovery`

## Next Slice

- finish the remaining call-side ownership move out of the monolith:
  keep pushing direct/declared-call metadata and helper logic into
  `src/backend/lowering/lir_to_bir/calls.cpp`
- continue the memory/address lane after that:
  make more GEP/address formation paths consume split memory helpers instead of
  rebuilding shape logic in `lir_to_bir.cpp`
- once the current ownership-wiring burst is coherent, start a dedicated
  compile-recovery pass for the split seam files before any new targeted tests

## Recently Completed

- expanded `src/backend/bir.hpp` so BIR can carry backend-owned type, memory,
  and call metadata needed by the split lowering seam
- added `src/backend/lowering/lir_to_bir/passes.hpp` and wired the translated
  split lowering scaffolds into the tree and build lists
- wrapped the old `try_lower_to_bir(...)` path as a legacy fallback behind
  `try_lower_to_bir_with_options(...)`
- moved type/legalization helper ownership into
  `src/backend/lowering/lir_to_bir/types.cpp`
- moved first memory/materialization and repeated GEP matcher ownership into
  `src/backend/lowering/lir_to_bir/memory.cpp`
- moved first direct-call construction, argument lowering, and basic call
  metadata ownership into `src/backend/lowering/lir_to_bir/calls.cpp`
- split direct-call metadata further so the split call seam now owns:
  `arg_types`, placeholder `arg_abi/result_abi`,
  target-derived `calling_convention`, and declared-function `is_variadic`

## Blockers

- no active blocker on ownership wiring
- later compile/test recovery still has known unrelated backend test noise, so
  not every existing backend test executable is currently a clean regression
  signal for this lane

## Notes To Resume

- do not add any new testcase-shaped x86 direct-LIR matcher while this reset
  is active
- the current goal is structural ownership, not behavioral completion
- when this lane switches to compile recovery, start narrow and seam-local
  before restoring broader regression discipline
