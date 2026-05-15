Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared Facts For Scalar `va_arg`

# Current Packet

## Just Finished

Step 3 completed scalar `va_arg` selected machine-node consumption from prepared
`helper_operand_homes.va_arg.scalar_access_plan` facts. The AArch64 lowering now
fails closed when scalar access-plan facts are absent or incomplete, and selects
structured GP, FP, and overflow-backed scalar `va_arg` records with printer
output from prepared storage, helper resources, source `va_list` homes, and
result homes.

## Suggested Next

Execute Step 4 by consuming prepared facts for aggregate `va_arg` helper
effects, keeping aggregate payload source selection and destination copy
semantics separate from the scalar path.

## Watchouts

- Scalar `va_arg` lowering is now selected only when the prepared scalar access
  plan is complete; missing or partial fields still report
  `helper_operand_homes.va_arg.scalar_access_plan`.
- Step 4 should not reuse scalar shortcuts for aggregate payload transport.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed: backend subset 139/139. Proof log: `test_after.log`.
