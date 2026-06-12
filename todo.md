Status: Active
Source Idea Path: ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Fallback And Nearby Same-Feature Stability

# Current Packet

## Just Finished

Attempted `plan.md` Step 3, `Prove Fallback And Nearby Same-Feature
Stability`.

Completed the validation-enabling compile-signature repair in
`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`: the
three stale
`find_prepared_param_zero_materialized_compare_join_context(...)` calls now
pass `prepared.module.names` to the current block/param overload. This was only
a handoff aggregate compile repair; no Route 6 route-debug behavior, row text,
expected strings, wrappers, prepared call/debug output, helper-oracle strings,
supported/unsupported contracts, public fallback behavior, or baselines were
changed.

The delegated proof now builds all three requested targets and runs the ctest
subset. `backend_prepared_lookup_helper` and `backend_x86_route_debug` pass,
covering the selected positive row, absent/invalid/duplicate-conflict/mismatch
fallbacks, `ConsumedPlans` compatibility fallback, and no-change helper/debug
surfaces already present in those tests.

Step 3 is not complete because `backend_x86_handoff_boundary` still fails after
the compile repair with an existing nearby same-feature scalar source handoff
assertion:

```text
scalar-control-flow compare-against-zero compare-join lane with stack-backed parameter home:
x86 prepared-module consumer stopped following the authoritative prepared stack home through compare-join entry and return
```

## Suggested Next

Delegate a follow-up packet for the remaining `backend_x86_handoff_boundary`
semantic failure in the compare-branch/parameter-home handoff surface, or have
the supervisor rescope the Step 3 proof if that aggregate failure is outside
this Route 6 route-debug slice.

## Watchouts

The remaining failure is not in the joined-branch compile-repair call sites; it
comes from the aggregate `backend_x86_handoff_boundary` test after linking
succeeds. Production implementation files and the compare-branch test source
were outside this packet's ownership, so I stopped instead of changing
semantics or expectation contracts.

## Proof

Delegated Step 3 proof command:

```sh
cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_x86_handoff_boundary|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: build passed; ctest failed 2/3 with
`backend_x86_handoff_boundary` failing. `test_after.log` is the canonical proof
log.

Supervisor build-only validation for the compile-signature repair:

```sh
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test
```

Result: passed / no work to do after the repair. Step 3 remains
blocked/incomplete on the recorded `backend_x86_handoff_boundary` semantic
CTest failure above.
