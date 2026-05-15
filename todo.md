Status: Active
Source Idea Path: ideas/open/246_prepared_aggregate_va_arg_access_plan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Populate Aggregate Access Plans In Shared Preparation

# Current Packet

## Just Finished

Plan Step 2 populated complete prepared/shared aggregate `va_arg` access plans
for the supported AAPCS64 sret aggregate helper shape. Shared preparation now
fills `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` from
prepared helper operand homes plus explicit sret payload ABI metadata, carrying
overflow source selection, payload size/alignment, source coordinates, copy
extent, destination payload home, and overflow `va_list` progression. Aggregate
helper calls without the required payload metadata still fail closed with
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`.

## Suggested Next

Execute Step 3 by keeping the populated aggregate access plan inspectable
through prepared printer coverage and confirming selected AArch64 aggregate
machine-node consumption remains deferred to idea 243.

## Watchouts

- Aggregate plan population currently supports the explicit sret payload
  aggregate helper shape; calls missing that ABI metadata intentionally retain
  the exact missing prepared fact.
- Do not reconstruct aggregate source selection, size/alignment, copy extent,
  or `va_list` progression in AArch64 target lowering.
- Keep selected aggregate `va_arg` machine-node consumption parked in idea 243
  until this prerequisite closes.
- This packet did not add selected AArch64 aggregate machine-node consumption.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed. `test_after.log` is the proof log.
