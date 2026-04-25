Status: Active
Source Idea Path: ideas/open/06_bir-call-lowering-exit-from-memory-coordinator.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Final Lifecycle Readiness Check

# Current Packet

## Just Finished

Step 5 completed the final lifecycle readiness check for the BIR call lowering
placement cleanup. `memory/coordinator.cpp` now only dispatches `LirCallOp`
lowering through `lower_call_inst(*call, lowered_insts)`;
`lower_call_inst(...)` remains a `BirFunctionLowerer` member declared in
`lowering.hpp` and defined in `calling.cpp`; `memory/intrinsics.cpp` remains
the `memcpy`/`memset` specialization owner. No new `.hpp` files or test
expectation changes were found in the working tree.

## Suggested Next

Supervisor can run lifecycle completion/commit routing for the completed
placement cleanup slice.

## Watchouts

No blockers found. `test_after.log` contains the fresh delegated proof output.

## Proof

Ran `cmake --build --preset default --target c4c_codegen && ctest --test-dir
build -j --output-on-failure -R '^backend_'`; build succeeded and all enabled
backend tests passed: `100% tests passed, 0 tests failed out of 97`. Proof
output is preserved in `test_after.log`.
