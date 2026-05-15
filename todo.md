Status: Active
Source Idea Path: ideas/open/246_prepared_aggregate_va_arg_access_plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define Prepared Aggregate Access Plan Carrier

# Current Packet

## Just Finished

Plan Step 1 defined the prepared/shared aggregate `va_arg` access-plan carrier
and completeness predicates. The carrier now hangs off
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`, can represent
source class, payload size/alignment, source coordinates, destination payload
home, copy extent, and `va_list` progression fields, and incomplete/absent
plans are reported as the explicit missing fact
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.

## Suggested Next

Execute Step 2 by populating complete aggregate `va_arg` access plans in shared
preparation for the representative supported AAPCS64 aggregate path, still
without selected AArch64 aggregate machine-node consumption.

## Watchouts

- Do not reconstruct aggregate source selection, size/alignment, copy extent,
  or `va_list` progression in AArch64 target lowering.
- Keep selected aggregate `va_arg` machine-node consumption parked in idea 243
  until this prerequisite closes.
- Step 1 intentionally does not populate aggregate access plans; current
  prepared output prints `aggregate_access_plan=<none>` and records the missing
  fact until Step 2 supplies the shared preparation data.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed. `test_after.log` is the proof log.
