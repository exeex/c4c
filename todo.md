# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 restored the x86 prepared multi-function rejection guard after the
helper-prefix cleanup. Multi-defined prepared modules now reject instead of
falling back to contract-first stubs when a defined function falls outside the
supported prepared scalar lanes, which closes the global-function-pointer plus
indirect variadic-runtime acceptance hole while leaving single-function
unsupported scalar parking intact.

The route-debug surface now reports the same module-level bounded
multi-function rejection for the global-function-pointer/indirect-variadic
boundary. The retired helper-prefix/local-byval renderer shortcut remains
retired; the active multi-defined call path still keeps only the producer
byval stack-destination authority coverage.

## Suggested Next

Supervisor should review the retained producer byval authority slice plus this
restored multi-function rejection guard as one Step 4 cleanup/recovery slice.
The next executable Step 4 packet remains the prepared control-flow
producer-publication route already named in `plan.md`: publish or surface the
missing two-segment loop-countdown branch-condition, join-transfer,
edge-transfer, and predecessor-owned parallel-copy bundle identities, or record
the missing identity as a semantic unsupported boundary.

## Watchouts

`src/backend/mir/x86/debug/debug.cpp` was touched to keep the route-debug
surface aligned with the restored renderer guard; it was necessary for the
owned rejection test's debug contract, though it was not in the initial owned
file list. Step 4 remains about prepared control-flow identity. Do not resume
f32 or wider local-byval helper-prefix rendering from the retired shortcut.

## Proof

Ran the supervisor-selected proof exactly:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_prepare_frame_stack_call_contract_test backend_prepare_liveness_test backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepare_liveness|backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' > test_after.log 2>&1`.

Configure and build passed. CTest passed
`backend_prepare_frame_stack_call_contract`, `backend_prepare_liveness`,
`backend_x86_handoff_boundary`, and
`backend_x86_prepared_handoff_label_authority`. Canonical proof log:
`test_after.log`.
