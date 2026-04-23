Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Grouped Storage, Spill, Reload, And Clobber Semantics
Plan Review Counter: 7 / 6
# Current Packet

## Just Finished

Plan Step 3: repaired the dormant standalone `backend_x86_route_debug` proof
by updating `tests/backend/backend_x86_route_debug_test.cpp` to the current
`x86/debug` public header surface and report contract, then passed the
x86-enabled direct consumer test in a side build.

## Suggested Next

Decide whether Step 3 is now complete enough to move the active packet to Step
4 proof/consumer confirmation, or add a grouped-register fixture inside
`backend_x86_route_debug_test.cpp` if the route needs direct grouped-assertion
coverage beyond the existing consumer-surface sanity check.

## Watchouts

- Keep the route target-independent.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.
- `backend_x86_route_debug` only exists when
  `C4C_ENABLE_X86_BACKEND_TESTS=ON`, so direct proof still needs the side
  build rather than the normal `default` preset.

## Proof

`cmake -S . -B build-x86-tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86-tests --target backend_x86_route_debug_test -j8 && ctest --test-dir build-x86-tests -j --output-on-failure -R '^backend_x86_route_debug$'`
passed in the x86-enabled side build.
