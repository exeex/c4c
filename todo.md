Status: Active
Source Idea Path: ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate va_arg And va_copy Consumption Boundaries

# Current Packet

## Just Finished

Step 4 completed for `src/backend/mir/aarch64/codegen/variadic.cpp`: `make_variadic_scalar_va_arg_record` now consumes `prepare::has_complete_prepared_variadic_scalar_va_arg_access_plan(homes)` instead of locally rechecking scalar access-plan completeness.

Aggregate `va_arg` and `va_copy` consumption boundaries were traced: aggregate `va_arg` consumes `prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(homes)` and still keeps AArch64 register-save-area, overflow-area, scratch, and record-field guards local; `va_copy` consumes `prepare::has_complete_prepared_variadic_va_copy_operand_homes(homes)` and still keeps AArch64 va_list layout, scratch, and field-copy guards local.

## Suggested Next

Supervisor should review Step 4 as exhausted and select lifecycle review, closure, or the next plan-owned packet.

## Watchouts

- Do not move AArch64 va_list layout spelling, save-area instruction selection, concrete stack/register addressing, aggregate copy chunking, or va_arg/va_copy record schemas into shared prealloc code.
- The reduced helper-consumption guard exists because `lower_call_instruction` rejects missing/incomplete helper operand homes before `CallInstructionRecord` machine-node selection can report local record-builder status.
- The va_start, scalar va_arg, aggregate va_arg, and va_copy record builders still keep target-local entry layout, field-size, scratch, register-save, overflow, and field-copy guards by design.
- Do not weaken variadic expectations or add named-case shortcuts.

## Proof

Passed: `cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log`.

Proof log: `test_after.log`.
