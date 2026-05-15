Status: Active
Source Idea Path: ideas/open/246_prepared_aggregate_va_arg_access_plan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Print And Prove Prepared Aggregate Access Plans

# Current Packet

## Just Finished

Plan Step 3 confirmed prepared-printer coverage for aggregate `va_arg` access
plans without adding selected AArch64 aggregate machine-node consumption. The
existing focused prepared-printer test asserts populated aggregate access-plan
fields for the supported AAPCS64 path, `aggregate_access_plan=<none>` for the
incomplete path, and the missing fact
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`. Existing selected
AArch64 dispatch coverage still fails closed on that same missing fact.

## Suggested Next

Execute Step 4 by validating the prerequisite handoff and asking the supervisor
to close idea 246 or reactivate idea 243 when lifecycle acceptance is ready.

## Watchouts

- No Step 3 code changes were needed; committed tests already covered the
  prepared-printer and fail-closed diagnostic contracts requested by the packet.
- Keep selected aggregate `va_arg` machine-node consumption parked in idea 243
  until the supervisor accepts and closes this prerequisite.
- The fail-closed selected AArch64 diagnostic remains
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed. `test_after.log` is the proof log.
