Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Facts For Aggregate `va_arg`

# Current Packet

## Just Finished

Step 4 completed aggregate `va_arg` selected machine-node consumption from
prepared/shared `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`
facts. AArch64 now records aggregate source class, destination payload home,
source `va_list` home, source field/slot coordinates, copy extent, storage
coordinates, helper resources, and progression directly from prepared facts,
with missing/incomplete aggregate access-plan authority still fail-closed.

## Suggested Next

Execute Step 5 by adding `va_copy` selected machine-node consumption from
prepared source/destination `va_list` homes, layout/storage facts, and helper
resources.

## Watchouts

- `va_copy` should not reuse aggregate/scalar `va_arg` progression or payload
  copy assumptions; it needs its own prepared source/destination layout facts.
- Aggregate `va_arg` still depends on complete prepared entry storage and
  helper resource facts when building the selected record.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
passed: 139/139 backend tests green. `test_after.log` is the proof log.
