Status: Active
Source Idea Path: ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Prepared Stack-Home Handoff Semantics

# Current Packet

## Just Finished

Completed Step 1 discovery for the x86 compare-join stack-home handoff
follow-up.

- Reproduced the delegated failure shape with the exact x86 proof command:
  `backend_prepared_lookup_helper` passed, `backend_x86_route_debug` passed, and
  only `backend_x86_handoff_boundary` failed.
- Exact failing case: `backend_x86_handoff_boundary`, case context
  `scalar-control-flow compare-against-zero compare-join lane with stack-backed parameter home`.
  The failed assertion is
  `x86 prepared-module consumer stopped following the authoritative prepared stack home through compare-join entry and return`.
- The failing assertion is reached through
  `check_compare_join_stack_home_consumes_prepared_entry_and_return_home(...)`
  in `tests/backend/bir/backend_x86_handoff_boundary_compare_branch_test.cpp`.
  The selected row uses
  `make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module()` with
  `expected_stack_home_zero_branch_joined_add_or_sub_then_xor_asm("branch_join_adjust_then_xor", "is_nonzero", 4, 0, 5, 1, 3)`.
- Observed wrong handoff state: the test rewrites the authoritative prepared
  home for `p.x` to `PreparedValueHomeKind::StackSlot`, clears the register
  home, sets `slot_id = 0`, `offset_bytes = 0`, and sets the prepared frame size
  to `4`; the x86 prepared-module consumer then emits assembly that does not
  match the expected stack-home compare-entry plus return behavior.
- Prepared compare-join authority is already traced through
  `src/backend/prealloc/control_flow.hpp`:
  `find_prepared_param_zero_materialized_compare_join_branches(...)` and
  `find_prepared_resolved_materialized_compare_join_render_contract(...)`
  provide the render contract consumed by the x86 path.
- Suspected handoff owner is the x86 prepared-module consumer in
  `src/backend/mir/x86/module/module.cpp`:
  `append_prepared_i32_param_zero_compare_join_return_function(...)` consumes
  the prepared compare-join contract, the compare entry reads the prepared zero
  test source, and `append_prepared_i32_compare_join_return_arm(...)` re-reads
  the selected parameter home through `render_prepared_i32_value_home_operand(...)`.
  The gap is local to preserving the same authoritative stack home through the
  compare-join entry/return lane, not to Route 6 output.
- Minimal Step 2 implementation target: start in
  `src/backend/mir/x86/module/module.cpp`. Treat
  `src/backend/prealloc/control_flow.hpp` as reference unless Step 2 proves the
  prepared render contract itself is wrong. Touch
  `tests/backend/bir/backend_x86_handoff_boundary_compare_branch_test.cpp` only
  if a focused assertion needs to be tightened after the semantic fix.
- Route 6 regression guard surfaces are
  `tests/backend/bir/backend_x86_route_debug_test.cpp` for selected Route 6 row
  spelling and fallback behavior, plus
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for Route 6
  consumed-plan helper agreement/fallback coverage.

## Suggested Next

Run Step 2 against `src/backend/mir/x86/module/module.cpp`: repair the x86
prepared compare-join stack-home handoff so the stack-backed `p.x` home remains
authoritative through the compare entry and return arms, while leaving Route 6
route-debug spelling/fallback behavior and all output contracts unchanged.

## Watchouts

- Keep this plan scoped to x86 compare-join stack-home handoff semantics.
- Do not change Route 6 route-debug row spelling, expected strings, fallback
  behavior, wrappers, prepared call/debug output, or helper-oracle behavior.
- Do not weaken or mark unsupported the `backend_x86_handoff_boundary`
  stack-backed parameter-home assertion.
- Do not testcase-match `branch_join_adjust_then_xor`, labels, assertion text,
  or expected strings. The fix should follow prepared stack-home authority, not
  the shape of this one named case.
- `backend_x86_handoff_boundary` is the known semantic failure; in this Step 1
  reproduction, `backend_x86_route_debug` and `backend_prepared_lookup_helper`
  stayed green and should remain the Route 6 guard surfaces for Step 2.

## Proof

Ran the delegated reproduction/proof command:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: command exited `8`, accepted for Step 1 discovery because the expected
known semantic failure reproduced in `backend_x86_handoff_boundary` while
`backend_x86_route_debug` and `backend_prepared_lookup_helper` passed.
`test_after.log` is the preserved proof log.

Recommended Step 2 proof command is the same command above, with the expected
result changing to a full pass after the implementation fix.
