Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Facts For Aggregate `va_arg`

# Current Packet

## Just Finished

Step 4 was tested against aggregate `va_arg` helper lowering and stopped at the
missing prepared/shared fact required for semantic consumption:
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`. AArch64 dispatch
now fails closed with that exact diagnostic instead of selecting a generic
deferred helper node when only destination/source homes are present.

## Suggested Next

Ask the plan owner to split or activate the prerequisite prepared/shared carrier
for aggregate `va_arg` access planning before retrying Step 4 machine-node
consumption.

## Watchouts

- The current prepared aggregate operand homes expose destination payload and
  source `va_list` homes, but not aggregate size/alignment, payload source
  selection, register-save access, overflow progression, or copy extent.
- Do not reconstruct those AAPCS64 aggregate facts in AArch64 target lowering;
  Step 4 should resume only after a prepared aggregate access-plan fact exists.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed: backend subset 139/139. Proof log: `test_after.log`.
