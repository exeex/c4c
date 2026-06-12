Status: Active
Source Idea Path: ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Prepared Stack-Home Handoff Semantics

# Current Packet

## Just Finished

Absorbed `review/234_step2_dirty_route_review.md` for idea 234 Step 2.

Lifecycle decision: keep idea 234 active, but reject the dirty Step 2 route as
too broad. The `src/backend/mir/x86/module/module.cpp` compare-join consumer
changes are plausibly aligned with idea 234 but still need isolated proof. The
`src/backend/prealloc/call_plans.cpp` Route 6 consumed-plan repair is out of
scope for idea 234 and is now tracked by
`ideas/open/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md`.
The later pointer-backed same-module global selected-value-chain metadata
failure is also out of scope for idea 234 and is now tracked by
`ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md`.

## Suggested Next

Supervisor should isolate/reprove idea 234 Step 2 as a narrow x86
compare-join stack-home packet:

- Keep or rework only the `src/backend/mir/x86/module/module.cpp`
  compare-join consumer changes that repair authoritative prepared stack-home
  handoff through compare-join entry and return.
- Do not accept the `src/backend/prealloc/call_plans.cpp` Route 6
  consumed-plan mutation as idea 234 progress.
- Do not continue into pointer-backed selected-value-chain metadata under idea
  234; use idea 236 when that initiative is activated.
- Re-run the supervisor-selected narrow proof for
  `backend_x86_handoff_boundary`, `backend_x86_route_debug`, and
  `backend_prepared_lookup_helper` after the idea 234 diff is isolated.

## Watchouts

- Treat `backend_x86_handoff_boundary` as a broad exposing test, not as a queue
  of every failure that appears after the current assertion advances.
- Idea 234 progress must be about stack-backed parameter-home handoff in the
  x86 compare-join consumer, not Route 6 consumed-plan facts or
  selected-value-chain metadata.
- Do not change Route 6 route-debug row spelling, expected strings, fallback
  behavior, wrappers, prepared call/debug output, or helper-oracle behavior.
- Do not weaken or mark unsupported the `backend_x86_handoff_boundary`
  stack-backed parameter-home assertion.
- Do not testcase-match `branch_join_adjust_then_xor`, labels, assertion text,
  or expected strings. The fix should follow prepared stack-home authority, not
  the shape of one named case.

## Proof

No new code validation was run by the plan owner. The reviewer report recorded
the last dirty proof command:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

That proof is not acceptance proof for idea 234 because it included
out-of-scope Route 6 consumed-plan changes and then failed on an out-of-scope
selected-value-chain assertion.
