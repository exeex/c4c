Status: Active
Source Idea Path: ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Fallback And Nearby Same-Feature Stability

# Current Packet

## Just Finished

Completed semantic inspection for `plan.md` Step 2, `Use Route 6 Agreement Only
Under Prepared Compatibility`.

No implementation or test semantic gap was found in the owned files. The
selected row already consumes Route 6 scalar `i32` argument-source evidence only
through `find_consumed_scalar_i32_call_argument_source(...)` in
`src/backend/mir/x86/x86.hpp`:

- `append_route6_scalar_call_argument_sources(...)` in
  `src/backend/mir/x86/debug/debug.cpp` calls `consume_plans(...)` and emits the
  selected row only when
  `find_consumed_scalar_i32_call_argument_source(...)` returns a value.
- The helper requires an existing prepared call-argument plan from
  `find_consumed_call_argument_plan(...)`, a shared non-empty
  `ConsumedPlans::route6_call_use_sources` index, a named `i32` argument,
  an available `bir::route6_find_call_argument_source(...)` record for the same
  argument pointer, `Route6CallUseSourceKind::ArgumentValue`, present Route
  6/prepared source ids, and equal Route 6/prepared source ids.
- Production callers found by AST/text inspection are the route-debug emitter
  and the two x86 prepared module emission paths in `module.cpp`; both consume
  the same helper and remain subordinate to prepared `ConsumedPlans`.

Confirmed fail-closed coverage and no-change boundaries:

- Route-debug tests cover the positive selected row plus absent Route 6 facts,
  invalid source id, duplicate/conflicting source relationship, and
  Route 6/prepared source-id mismatch while preserving the function row.
- Handoff boundary coverage proves the helper threads an agreed direct-extern
  scalar `i32` source through `ConsumedPlans`, fails closed when Route 6 facts
  are absent, preserves the prepared call argument selector, and keeps prepared
  fallback asm unchanged.
- Prepared lookup helper coverage proves available records, absent Route 6
  facts, duplicate relationship, missing argument, wrong call, missing source
  relationship, missing producer, ABI-bound exclusion, source-id/prepared
  mismatch, public-fallback/index status behavior, and helper-oracle stability.
- No row text, expected strings, wrappers, prepared call/debug output,
  direct-call/helper-oracle strings, supported/unsupported contracts, public
  fallback behavior, or baselines were changed.

## Suggested Next

Proceed with `plan.md` Step 3, `Prove Fallback And Nearby Same-Feature
Stability`, focusing on fallback and nearby same-feature stability without
changing row text, expected strings, wrappers, prepared call/debug output,
helper-oracle strings, supported/unsupported contracts, public fallback
behavior, or baselines.

## Watchouts

`backend_x86_handoff_boundary_test` is currently blocked by an unrelated
compile error in
`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`; treat
that aggregate target as a broader Step 3/4 proof blocker unless it is repaired
or explicitly scoped out by the supervisor.

## Proof

Accepted adjusted Step 2 proof command:

```sh
cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: passed 2/2 (`backend_prepared_lookup_helper`,
`backend_x86_route_debug`).
