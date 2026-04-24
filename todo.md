Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Validate Coordinator Boundary

# Current Packet

## Just Finished

Completed `Step 6: Validate Coordinator Boundary` by validating the current
BIR memory coordinator split against the source-idea boundary checks.
`lower_scalar_or_local_memory_inst` is now a thin dispatcher for the split
memory instruction families: alloca/local-slot setup, GEP, load/store, and
runtime memory intrinsic lowering delegate to private family handlers. The
remaining address-int cast, pointer subtraction, and call-lowering branches are
unchanged coordinator responsibilities from the current runbook route.

Structural checks passed:
- instruction-family handlers are independently reviewable in existing
  semantic homes (`scalar.cpp`, `memory/addressing.cpp`, and
  `memory/local_slots.cpp`)
- no new `.hpp` files were created under `src/backend/bir/lir_to_bir/memory`
- `lowering.hpp` remains the complete private `BirFunctionLowerer` index
- `BirFunctionLowerer` still owns memory state
- no test expectation rewrites were used as proof
- no remaining source-idea gap was found; the plan is close-ready

## Suggested Next

Ask the plan owner to close the active plan if supervisor review accepts this
validation packet.

## Watchouts

- Close review should preserve the current boundary: address-int casts, pointer
  subtraction, and call lowering are still in the coordinator, while extracted
  memory families stay behind their private handlers.
- No implementation/header/test file was touched in this validation packet.

## Proof

`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed: 97 backend tests passed, 0 failed, 12 disabled not run. Proof log:
`test_after.log`. `git diff --check` passed.
