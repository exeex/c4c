Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Proof And Consumer Confirmation
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Plan Step 4: the delegated combined proof passed from the already-landed
grouped prealloc authority surfaces, confirming both the direct
`backend_prepare_frame_stack_call_contract` contract assertions and the x86
consumer-facing `backend_x86_route_debug` report path without reopening Step 3
or adding duplicate grouped fixtures.

## Suggested Next

Supervisor or plan-owner should evaluate the active plan for lifecycle
close/deactivate/switch handling, because the current Step 4 proof and consumer
confirmation packet is complete and no further executor-side test expansion is
currently justified.

## Watchouts

- Keep the route target-independent.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.
- Do not reopen Step 3 just to duplicate grouped assertions in
  `backend_x86_route_debug_test.cpp`; the grouped contract surface already
  lives in `backend_prepare_frame_stack_call_contract_test.cpp`.
- `backend_x86_route_debug` only exists when
  `C4C_ENABLE_X86_BACKEND_TESTS=ON`, so direct proof still needs the side
  build rather than the normal `default` preset.
- If lifecycle stays open, any new packet should start from a concrete consumer
  regression rather than speculative extra grouped-authority fixtures.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$' && cmake -S . -B build-x86-tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86-tests --target backend_x86_route_debug_test -j8 && ctest --test-dir build-x86-tests -j --output-on-failure -R '^backend_x86_route_debug$'`
passed exactly as delegated; proof log: `test_after.log`.
