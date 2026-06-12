Status: Active
Source Idea Path: ideas/open/214_route6_x86_scalar_source_route_debug_row.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Debug Text and Fallbacks

# Current Packet

## Just Finished

Step 3: Prove Debug Text and Fallbacks completed focused route-debug fallback coverage for the active idea 214 Route 6 scalar argument-source row.

```text
    route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue
```

Changed files:

- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `todo.md`

Completed work:

- `backend_x86_route_debug_test.cpp` now keeps the positive selected scalar row assertion and adds cloned-fixture route-debug mutations around only the selected `addip0` call.
- The route-debug text now proves the selected row is absent while the trace header and selected function row remain for absent Route 6 facts, an invalid source-id record, a duplicate/conflicting Route 6 relationship, and a Route 6/prepared source-id mismatch.
- No production changes were needed; `src/backend/mir/x86/debug/debug.cpp` was not edited.
- The detailed status taxonomy is still covered at helper level in `backend_prepared_lookup_helper`: absent Route 6 facts with prepared fallback, Route 6/prepared id mismatch with prepared fallback, duplicate Route 6 source records, wrong-call/no-match lookup failures, missing relationships, non-named arguments, and related fail-closed source-producer/materialization cases.

## Suggested Next

Proceed to Step 4: prove the wrapper/backend behavior remains unchanged by the Route 6 scalar argument-source debug row work, including the residual wrapper no-change proof around the selected scalar row family.

## Watchouts

- Keep Step 4 to wrapper/backend no-change proof; the Step 3 route-debug text coverage is complete for the selected scalar row family.
- Treat expected-string rewrites, helper renames, unsupported downgrades, and narrow testcase-shaped matching as blockers.
- Do not broaden from the selected `i32` argument row to `i64`, aggregate, result, lane, direct-global, or publication-source Route 6 families.
- The selected fixture still requires `source_value_id=7` for `%t1`; omitting the id leaves the helper fail-closed and the route-debug row absent.
- Production row selection remains generic over accepted scalar `i32` Route 6/prepared agreement; do not reintroduce function, block, callee, instruction, argument-name, or fixture-specific matching.
- Step 4 still needs residual wrapper no-change proof; this packet did not touch wrapper implementation, call plans, ABI policy, prepared printer output, or `ConsumedPlans`.

## Proof

Passed with `test_after.log` using the delegated x86-enabled command:

```text
cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log
```

Result: `backend_prepared_lookup_helper` and `backend_x86_route_debug` passed.
