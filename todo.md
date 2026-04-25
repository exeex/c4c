Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Direct Method And Member Probes Through Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by routing the direct operator-deref return-type
probe in `infer_generic_ctrl_type` through
`find_struct_method_return_type`.

The `NK_DEREF` branch now asks the helper for `operator_deref` with
`is_const_obj == false`, preserving the previous base-before-const preference
while leaving pointer and array fallback behavior unchanged.

## Suggested Next

Supervisor should choose the next packet. This slice leaves helper authority,
method registration, ref-overload registration, tests, expectations, and the
rendered maps unchanged.

## Watchouts

- The targeted `NK_DEREF` branch no longer directly probes
  `struct_method_ret_types_`.
- The helper call intentionally passes `false` to preserve the non-const
  class-specific lookup route before the `_const` fallback.
- Pointer and array dereference fallback still runs when no struct
  `operator_deref` return type is found.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_(positive_sema_operator_deref|hir_expr_operator_member_helper)' > test_after.log`

Proof log: `test_after.log`.
