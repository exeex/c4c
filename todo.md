Status: Active
Source Idea Path: ideas/open/06_bir-call-lowering-exit-from-memory-coordinator.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reduce Coordinator To Dispatch

# Current Packet

## Just Finished

Steps 2 and 3 moved the existing `LirCallOp` lowering body from
`src/backend/bir/lir_to_bir/memory/coordinator.cpp` into
`BirFunctionLowerer::lower_call_inst(...)` in
`src/backend/bir/lir_to_bir/calling.cpp`, then reduced the coordinator call
branch to `return lower_call_inst(*call, lowered_insts);`.

## Suggested Next

Continue with Step 5 in `plan.md`: final lifecycle readiness check for the
placement cleanup.

## Watchouts

The delegated proof passed after an extra default build compiled the touched
translation units. No test expectations or memory intrinsic implementation
files were changed.

## Proof

Ran `cmake --build --preset default --target c4c_codegen && ctest --test-dir
build -j --output-on-failure -R '^backend_'`; build succeeded and all enabled
backend tests passed. Proof output is preserved in `test_after.log`.
