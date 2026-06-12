Status: Active
Source Idea Path: ideas/open/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Route 6 Consumed-Plan Gap

# Current Packet

## Just Finished

Step 1 discovery reproduced the Route 6 consumed scalar i32 call-use boundary
failure without implementation edits. The delegated proof still fails only in
`backend_x86_handoff_boundary` at:
`x86 Route 6 call-use boundary: scalar call argument source did not thread through ConsumedPlans`.

The failing fixture is `check_consumed_plans_threads_route6_scalar_call_argument_source`
in `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`.
It prepares `make_x86_direct_extern_call_lane_module()`, consumes plans for
`main`, and queries the `printf` call at block `0`, instruction `1`, arg `1`
for named i32 `%t0`.

The minimal consumed-plan lookup owner is
`find_consumed_scalar_i32_call_argument_source(...)` in
`src/backend/mir/x86/x86.hpp`. Its prepared-call selector,
`find_consumed_call_argument_plan(consumed, 0, 1, 1)`, is the expected
`ConsumedPlans` entry point, but the scalar Route 6 helper also requires a
Route 6 source record built from `CallInst::arg_sources` and rejects unless
the Route 6 `source_value_id` agrees with
`PreparedCallArgumentPlan::source_value_id`.

Authoritative facts safe for Step 2 are the prepared call-argument facts for a
single named i32 argument when they are explicit and consistent:
`PreparedCallArgumentPlan::source_encoding`,
`PreparedCallArgumentPlan::source_value_id`, and the prepared source value
name captured while planning the argument source. Those can backfill the BIR
Route 6 `CallArgumentSourceRelationship` only for the same call/arg and only
when the relationship is absent, the argument is named i32, the prepared source
id is present, the source name is present, and the prepared source encoding is
representable as a Route 6 call-argument source encoding.

The currently green synthetic guard,
`backend_prepared_lookup_helper`, proves the x86 consumed helper already works
when the prepared argument and Route 6 relationship agree. The production gap
is therefore the missing handoff from prepared call-plan authority into the
Route 6 call-use source relationship for this named i32 path, not the debug
printer or x86 module renderer.

## Suggested Next

Execute Step 2 in `src/backend/prealloc/call_plans.cpp`: while building each
`PreparedCallArgumentPlan`, narrowly publish a missing BIR
`CallArgumentSourceRelationship` from prepared call-plan authority for named
i32 arguments only. Keep the existing x86 consumer join in
`src/backend/mir/x86/x86.hpp` fail-closed; do not loosen the Route 6/prepared
source-id agreement check.

## Watchouts

- Keep this packet scoped to Route 6 named scalar i32 call-argument source
  facts in `ConsumedPlans`.
- Fail closed when Route 6 facts already exist, when there are duplicate
  source relationships, when the prepared source id is absent, when the
  prepared source name is absent, when the BIR argument is not a named i32
  value, when source encodings are unsupported or ABI-bound, when prepared ids
  disagree with Route 6 ids, or when the lookup is for another call/arg.
- Guard surfaces that should remain unchanged: route-debug expected strings,
  helper-oracle output, prepared call/debug output, wrappers, baselines, and
  the existing `find_consumed_scalar_i32_call_argument_source(...)` equality
  check.
- Do not change x86 compare-join stack-home behavior from idea 234.
- Do not change prepared compare-join selected-value-chain metadata from idea
  236.
- Do not rewrite route-debug expected strings, baselines, wrappers,
  helper-oracle output, or prepared call/debug output as proof.

## Proof

Ran the supervisor-selected proof command:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: failed with exit `8`, expected for Step 1 discovery. Passing:
`backend_prepared_lookup_helper`, `backend_x86_route_debug`. Failing:
`backend_x86_handoff_boundary` at the Route 6 scalar call-argument source
assertion above. `test_after.log` is the proof log path and should remain the
recommended Step 2 proof command/log target.
