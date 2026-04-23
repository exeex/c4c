Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Proof And Consumer Confirmation
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Plan Step 3: direct grouped saved/preserved/clobber/storage assertions now
exist in `tests/backend/backend_prepare_frame_stack_call_contract_test.cpp`
around the x86 route-debug contract surface, and the dormant standalone
`backend_x86_route_debug` proof was repaired against the current `x86/debug`
public report contract and passed in the x86-enabled side build.

## Suggested Next

Run Step 4 proof and consumer confirmation from the already-landed grouped
authority surfaces: keep the existing focused `backend_prepare_frame_stack_call_contract`
coverage, rerun the repaired x86 side-build consumer proof as needed, and only
add more tests if a consumer still reconstructs grouped span/storage facts
instead of reading published prealloc authority.

## Watchouts

- Keep the route target-independent.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.
- Do not reopen Step 3 just to duplicate grouped assertions in
  `backend_x86_route_debug_test.cpp` when the direct grouped contract coverage
  already lives in `backend_prepare_frame_stack_call_contract_test.cpp`.
- `backend_x86_route_debug` only exists when
  `C4C_ENABLE_X86_BACKEND_TESTS=ON`, so direct proof still needs the side
  build rather than the normal `default` preset.

## Proof

`cmake -S . -B build-x86-tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86-tests --target backend_x86_route_debug_test -j8 && ctest --test-dir build-x86-tests -j --output-on-failure -R '^backend_x86_route_debug$'`
passed in the x86-enabled side build.
