Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Grouped Storage, Spill, Reload, And Clobber Semantics
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Plan Step 3: added grouped x86 route-debug consumer reporting in
`src/backend/mir/x86/debug/debug.cpp` and proved through
`backend_prepare_frame_stack_call_contract_test` that the downstream debug
surface reads grouped saved-register, preserved-value, clobber, and storage
authority directly from prepared plans.

## Suggested Next

If the supervisor wants direct standalone proof for the dormant x86 debug
binary, reconfigure with `C4C_ENABLE_X86_BACKEND_TESTS=ON` and audit the
existing `backend_x86_route_debug` target separately from this Step 3 slice.

## Watchouts

- Keep the route target-independent.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.
- The delegated regex only exercised `backend_prepare_frame_stack_call_contract`
  in the current `default` build because `backend_x86_route_debug` is not
  registered when `C4C_ENABLE_X86_BACKEND_TESTS=OFF`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_x86_route_debug)$'`
passed for the registered subset in the current `default` build
(`test_after.log`).
