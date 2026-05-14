Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Prepared Variadic Entry Carrier Types

# Current Packet

## Just Finished

Step 2, Define Prepared Variadic Entry Carrier Types, completed the prepared
shared carrier slice. `PreparedBirModule` now owns a separate
`PreparedVariadicEntryPlans` collection keyed by `FunctionNameId`, with typed
subrecords for named GP/FP counts, register-save-area facts, overflow-area
facts, `va_list` layout fields, and helper resource needs.

Implementation notes:
- `populate_variadic_entry_plans` publishes carriers only for variadic function
  definitions, not variadic declarations or callsites.
- Named GP/FP counts are populated from existing BIR parameter ABI metadata
  when available; target-specific register-save-area, overflow-area, `va_list`,
  and scratch-resource facts remain explicit optional/default fields rather
  than target-local inference.
- The prepared printer now emits a distinct
  `--- prepared-variadic-entry-plans ---` section and a function-summary
  `variadic_entry=yes/no` signal.
- Focused tests prove same-module variadic definitions publish entry carriers,
  variadic callsite metadata remains separate from caller entry carriers, and
  variadic extern declarations do not create function-entry carriers.

## Suggested Next

Start the next packet by wiring AArch64-side fail-closed observation against the
new prepared variadic-entry carrier, without consuming or fabricating target
lowering semantics.

## Watchouts

- The new carrier intentionally does not make AArch64 target lowering consume
  register-save-area, overflow-area, `va_list`, or helper scratch details yet.
- `PreparedCallPlan::variadic_fpr_arg_register_count` remains call-boundary
  metadata only; do not treat it as callee-entry state.
- Optional/default fields are structural placeholders for later AAPCS64 facts;
  the next packet should not backfill them with target-local inference.

## Proof

Ran the delegated proof command:

`(cmake --build build --target backend_prepare_liveness_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and three-test
CTest run.
