Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared Facts For Scalar `va_arg`

# Current Packet

## Just Finished

Step 3 stopped at a lifecycle blocker: scalar `va_arg` has prepared entry
storage, helper scratch, source `va_list` home, and scalar result home facts,
but no prepared/shared scalar access plan that selects GP/FP register-save
versus overflow source, preserves per-access size/alignment, and records
`va_list` progression. AArch64 dispatch and direct call-record construction now
fail closed with the exact missing fact
`helper_operand_homes.va_arg.scalar_access_plan` instead of reconstructing those
facts in target lowering.

## Suggested Next

Route a lifecycle/prepared-authority packet to add a shared scalar `va_arg`
access-plan fact covering source classification, value size/alignment, and
overflow/register-save progression before retrying Step 3 machine-node
consumption.

## Watchouts

- Existing prepared helper operand-home records for scalar `va_arg` are useful
  but not sufficient for selected lowering; they identify `%ap` and the scalar
  result home only.
- The missing fact must be shared/prepared authority, not an AArch64-local
  reconstruction from helper name, result type, named register counts, or
  legacy AAPCS64 layout rules.
- `va_start` selected lowering and incomplete prepared-fact diagnostics remain
  unchanged.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. Build succeeded and CTest reported 139/139 backend tests passed.
Proof log: `test_after.log`.
