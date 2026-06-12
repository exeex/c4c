Status: Active
Source Idea Path: ideas/open/214_route6_x86_scalar_source_route_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route-Native Row Selection

# Current Packet

## Just Finished

Step 2: Add Route-Native Row Selection completed the route-native positive row selection for active idea 214.

```text
    route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue
```

Changed files:

- `src/backend/mir/x86/debug/debug.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`

Completed work:

- `debug.cpp` now emits the Route 6 scalar argument-source trace row family generically after `find_consumed_scalar_i32_call_argument_source(...)` accepts the current `ConsumedPlans`/Route 6 agreement for a scalar `i32` call argument.
- The rejected testcase-shaped production predicate for `single_block_same_module_scalar_call_wrapper_miss` / `entry` / inst `2` / `addip0` / `%t1` was removed.
- The selected route-debug fixture now carries the single Route 6 argument-source relationship for `%t1`; the prepared argument source id for `%t1` is `7`, and source-name-only Route 6 data remains fail-closed under the helper.
- The targeted x86 route-debug test asserts the byte-stable positive row without changing prepared output, wrapper kind, ABI placement, moves, call-plan ownership, result/lane/direct-global/publication-source handling, or unrelated expected strings.
- No additional `route6 scalar arg` rows appeared in the existing targeted x86 route-debug test surface; `backend_x86_route_debug_test.cpp` currently has no other Route 6 call-argument source relationships for this row family.

## Suggested Next

Proceed to Step 3: add focused route-debug fallback/debug-text coverage for absent Route 6 facts, invalid references, duplicate/conflict, mismatch, and compatibility fallback around the same selected scalar row, without weakening existing expected strings.

## Watchouts

- Keep this to one route-debug row.
- Do not migrate x86 wrappers, call plans, call printer output, ABI policy, or `ConsumedPlans`.
- Treat expected-string rewrites, helper renames, unsupported downgrades, and narrow testcase-shaped matching as blockers.
- Do not broaden from the selected `i32` argument row to `i64`, aggregate, result, lane, direct-global, or publication-source Route 6 families.
- The selected fixture requires `source_value_id=7` for `%t1`; omitting the id leaves the current helper fail-closed and the row absent.
- Production row selection must remain generic over accepted scalar `i32` Route 6/prepared agreement; do not reintroduce function, block, callee, instruction, argument-name, or fixture-specific matching.
- Step 3 still needs x86 route-debug expected-string proof for absent, invalid, duplicate/conflict, mismatch, and compatibility fallback behavior.

## Proof

Passed with `test_after.log` using the delegated x86-enabled command:

```text
cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log
```

Result: `backend_prepared_lookup_helper` and `backend_x86_route_debug` passed.
