Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Facts For `va_start`

# Current Packet

## Just Finished

Step 2 completed: AArch64 `va_start` helper calls now consume prepared variadic
entry facts and helper operand-home records into a structured selected
`VariadicVaStartRecord`, expose prepared destination/register-save/overflow/
layout/scratch facts on the machine node, and print pseudo `va.start` records
from that structured payload. Existing incomplete prepared/shared fact paths
still fail closed with explicit diagnostics.

## Suggested Next

Delegate an executor packet for Step 3: consume prepared/shared facts for
scalar `va_arg` lowering, covering at least one register-save-area-backed and
one overflow-backed scalar path if the prepared operand-home/result facts are
complete.

## Watchouts

- `va_start` selection is intentionally limited to records derivable from
  `PreparedVariadicEntryPlanFunction` plus the matching helper operand-home
  record; other variadic helpers still defer outside the selected `va_start`
  subset.
- `VariadicVaStartRecord` stores prepared layout/storage/scratch snapshots; do
  not infer field offsets, named register counts, stack offsets, or scratch
  policy in AArch64 lowering.
- Keep incomplete fact handling fail-closed with explicit diagnostics when
  extending scalar `va_arg`.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. Build succeeded and CTest reported 139/139 backend tests passed.
Proof log: `test_after.log`.
