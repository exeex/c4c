Status: Active
Source Idea Path: ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate the Selected Boundary

# Current Packet

## Just Finished

Step 1 located the selected x86 Route 6 scalar `i32` route-debug /
`ConsumedPlans` compatibility boundary.

Selected route-debug row:

```text
route6 scalar arg call#0 block=entry inst#2 callee=addip0 arg#0 source=%t1 kind=ArgumentValue
```

Selected fixture/test surface:

- Positive fixture: `legalize_single_block_same_module_scalar_call_wrapper_miss_module()` in `tests/backend/bir/backend_x86_route_debug_test.cpp`.
- Selected BIR call: function `single_block_same_module_scalar_call_wrapper_miss`, block `entry`, instruction `2`, callee `addip0`, argument `0`, named `i32` value `%t1`.
- Prepared selector helpers: `find_selected_route6_scalar_arg_call(...)` and `find_selected_prepared_scalar_arg(...)`.
- Positive expected-string assertions: `selected_route6_scalar_arg_row` and the `single_block_same_module_scalar_call_wrapper_miss_trace` `expect_contains(...)` check.
- Fail-closed route-debug fixtures: `selected_route6_scalar_arg_without_route6_facts()`, `selected_route6_scalar_arg_with_invalid_source_id()`, `selected_route6_scalar_arg_with_duplicate_conflict()`, and `selected_route6_scalar_arg_with_prepared_mismatch()`.

Implementation/proof surfaces found:

- Route 6 producer/index surface: `src/backend/bir/bir.cpp` / `src/backend/bir/bir.hpp` functions `route6_call_argument_source_record(...)`, `route6_build_call_use_source_index(...)`, and `route6_find_call_argument_source(...)`. Statuses to preserve include `Available`, `MissingCall`, `WrongCall`, `MissingArgument`, `MissingSourceRelationship`, `MissingSourceProducer`, `AbiBoundExcluded`, `DuplicateRelationship`, and `NoMatch`.
- x86 `ConsumedPlans` adapter: `src/backend/mir/x86/x86.hpp` struct `ConsumedPlans`, `consume_plans(...)`, `shared_route6_call_use_source_index()`, `find_consumed_call_argument_plan(...)`, and `find_consumed_scalar_i32_call_argument_source(...)`.
- Current source-agreement consumer gate: `find_consumed_scalar_i32_call_argument_source(...)` requires a prepared call argument, Route 6 call-use source index, named `i32` argument, Route 6 `ArgumentValue` source kind, valid Route 6 source id, valid prepared source id, and id agreement.
- Route-debug row generation: `src/backend/mir/x86/debug/debug.cpp` function `append_route6_scalar_call_argument_sources(...)`, called from the trace path in `render_report(...)`; summary output remains on `append_grouped_authority(...)`.
- Direct-call/wrapper consumer surfaces to preserve: `src/backend/mir/x86/module/module.cpp` `append_prepared_direct_extern_call_argument(...)`, `append_prepared_direct_extern_call_return_function(...)`, and the bounded same-module/direct extern call path that calls `find_consumed_scalar_i32_call_argument_source(...)` before falling back to prepared homes.
- Prepared printer/debug fallback surfaces to preserve: `src/backend/prealloc/prepared_printer/calls.cpp` `append_call_plans(...)`, especially call plan rows with `wrapper_kind`, direct callee names, argument `source_encoding`, `source_value_id`, and memory-return/wrapper detail.
- Helper-oracle surface: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` Route 6 call-argument source producer/materialization checks around `verify_bir_call_argument_source_producer_materialization_lookup()`, including positive source identity/materialization, missing source producer, missing relationship, unnamed source, wrong-call/no-match, ABI-bound exclusion, and duplicate relationship fail-closed behavior.

Nearby proof cases to keep in the Step 2 implementation packet:

- Positive agreement: `single_block_same_module_scalar_call_wrapper_miss_trace` contains the exact selected Route 6 scalar arg row.
- Absent Route 6 fact: `selected_route6_scalar_arg_without_route6_facts()` keeps trace/function rows but omits the selected row.
- Invalid Route 6 source id: `selected_route6_scalar_arg_with_invalid_source_id()` keeps fallback trace/function rows but omits the selected row.
- Duplicate/conflict: `selected_route6_scalar_arg_with_duplicate_conflict()` keeps fallback trace/function rows but omits the selected row.
- Route 6/prepared mismatch: `selected_route6_scalar_arg_with_prepared_mismatch()` keeps fallback trace/function rows but omits the selected row.
- Non-agreement/unsupported/helper-oracle: `backend_prepared_lookup_helper` Route 6 source producer/materialization tests cover missing source producer, missing relationship, unnamed/non-source argument, wrong call, no match, ABI-bound exclusion, duplicate relationship, and prepared/BIR materialization compatibility.
- Wrapper/prepared string stability: `backend_x86_route_debug` checks same-module scalar-call wrapper trace rows plus direct extern memory-return prepared dump rows; `backend_prepared_lookup_helper` protects helper statuses and fallback semantics.

Recommended narrow proof command candidate for the supervisor:

```bash
set -o pipefail && cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log
```

## Suggested Next

Execute Step 2 by changing only the selected x86 Route 6 scalar `i32` path so
the route-debug/direct-call compatibility gate consumes the Route 6 scalar
source agreement fact semantically through `ConsumedPlans`, while preserving
prepared fallback and byte-stable route-debug/helper/wrapper strings.

## Watchouts

- Keep the route narrow to the selected trace row; summary rows, broad x86 wrappers, ABI placement, register/frame policy, and route-wide migration are outside this packet.
- `ConsumedPlans` remains the compatibility adapter: no deletion, bypass, helper rename, expected-string rewrite, supported-path downgrade, wrapper-output relabeling, or baseline refresh should be treated as progress.
- The current gate compares Route 6 `source_value_id` with the prepared argument `source_value_id`; Step 2 should preserve fail-closed behavior for absent Route 6 index, absent prepared call plan, invalid source id, duplicate/conflicting Route 6 records, non-`ArgumentValue`, non-`i32`, and mismatch paths.
- Direct-call/wrapper code already consumes the same helper as route-debug; any change there needs the same byte-stability proof and should not move call-wrapper policy into Route 6 ownership.
- The x86-enabled proof directory exists as `build-x86` and currently lists both `backend_x86_route_debug` and `backend_prepared_lookup_helper`.

## Proof

Analysis-only packet. No build or CTest was required and no new `test_after.log`
was written.

Validation commands run:

```bash
ctest --test-dir build-x86 -N -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$'
```
