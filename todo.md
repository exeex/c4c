Status: Active
Source Idea Path: ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consume Route 6 Scalar Source Agreement

# Current Packet

## Just Finished

Step 2 changed the selected x86 Route 6 scalar `i32` route-debug/direct-call
compatibility path to consume a Route 6 `ArgumentValue` source-record
compatibility check before accepting the prepared `ConsumedPlans` source-id
agreement.

Reviewer route-quality repair completed: the helper no longer presents itself
as a general semantic identity predicate, and it now requires the same call
argument object through the Route 6 source record before x86 applies prepared
source-id agreement.

Code surfaces changed:

- `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp` now expose
  `route6_call_argument_source_matches_argument_value_record(...)`, a shared
  BIR compatibility check that requires an available Route 6 `ArgumentValue`
  source record, the same call argument object, the same Route 1 source object,
  and matching optional Route 6 source spelling.
- `src/backend/mir/x86/x86.hpp`
  `find_consumed_scalar_i32_call_argument_source(...)` now requires the shared
  BIR record-compatibility check, then keeps the existing prepared
  `source_value_id` equality check through `ConsumedPlans`.
- `tests/backend/bir/backend_x86_route_debug_test.cpp` adds a same-id but
  source-name-mismatched selected-row fixture proving the route-debug row fails
  closed when Route 6 source-record compatibility is absent.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` asserts the
  positive helper result satisfies the BIR record-compatibility check and
  rejects the same-id/source-name mismatch through the x86 `ConsumedPlans`
  helper.

Preserved selected positive row:

```text
route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue
```

## Suggested Next

Execute Step 3 by using the now-passing narrow proof to record the fail-closed
and compatibility matrix for absent Route 6 facts, invalid source ids,
duplicate/conflict records, prepared mismatch, same-id source-record mismatch,
prepared fallback, wrapper output, route-debug strings, and helper-oracle
behavior.

## Watchouts

- This slice intentionally keeps the prepared source-id agreement check in the
  x86 `ConsumedPlans` adapter; the BIR helper is only a Route 6 `ArgumentValue`
  source-record compatibility check, while prepared plans remain the
  source-id/fallback oracle.
- The helper is shared in BIR and may be attractive to AArch64 later, but this
  packet did not migrate AArch64 or broaden wrapper ownership.
- No expected strings, wrapper labels, helper names, fallback policies, or
  supported/unsupported classifications were downgraded.

## Proof

Delegated proof was rerun after the reviewer-repair changes and wrote
`test_after.log`:

```bash
set -o pipefail && cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log
```

Result: `2/2` tests passed: `backend_prepared_lookup_helper` and
`backend_x86_route_debug`.
