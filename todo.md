Status: Active
Source Idea Path: ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Prepared Stack-Home Handoff Semantics

# Current Packet

## Just Finished

Completed idea 234 Step 2 cleanup: isolated the x86 compare-join stack-home
consumer repair in `src/backend/mir/x86/module/module.cpp` and removed the
out-of-scope Route 6 consumed-plan mutation from
`src/backend/prealloc/call_plans.cpp`.

The isolated x86 repair validates prepared compare-join edge-publication facts,
rejects drifted move-bundle destinations, and skips dead non-i32 carrier moves
while preserving valid i32 register publication moves. The original stack-home
handoff assertion stayed advanced after the Route 6 mutation was removed.

## Suggested Next

Supervisor can review and commit the isolated idea 234 Step 2 slice if the
remaining `src/backend/mir/x86/module/module.cpp` diff is acceptable. The next
implementation packet should use idea 235 for the Route 6 consumed scalar i32
call-argument source failure.

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
- `src/backend/prealloc/call_plans.cpp` is intentionally clean in this slice;
  the observed Route 6 failure is split to idea 235.

## Proof

Ran the supervisor-selected proof:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: failed with exit 8 after a successful build. `backend_prepared_lookup_helper`
and `backend_x86_route_debug` passed. `backend_x86_handoff_boundary` advanced
past the original idea 234 stack-home assertion and failed at the split-out
idea 235 assertion:

`x86 Route 6 call-use boundary: scalar call argument source did not thread through ConsumedPlans`

Proof log: `test_after.log`.
