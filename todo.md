Status: Active
Source Idea Path: ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared Helper Operand Homes

# Current Packet

## Just Finished

Step 3 completed for `src/backend/prealloc/variadic.hpp` and `src/backend/mir/aarch64/codegen/variadic.cpp`: `va_start` and `va_copy` helper operand-home completeness now has shared prepared authority through `has_complete_prepared_variadic_va_start_operand_homes` and `has_complete_prepared_variadic_va_copy_operand_homes`.

`has_complete_prepared_variadic_entry_helper_operand_homes` now owns the shared completeness dispatch for all `PreparedVariadicEntryHelperOperandHomes` helper kinds. AArch64 `variadic_helper_operand_homes_complete` delegates to that prepared authority, and the va_start/va_copy record builders consume the shared helper-home predicates while target-local entry-layout guards and ABI emission remain local.

## Suggested Next

Supervisor should review whether Step 3 is now exhausted or whether another prepared-helper consumer remains before selecting the next packet.

## Watchouts

- Do not move AArch64 va_list layout spelling, save-area instruction selection, concrete stack/register addressing, aggregate copy chunking, or va_arg/va_copy record schemas into shared prealloc code.
- The reduced helper-consumption guard exists because `lower_call_instruction` rejects missing/incomplete helper operand homes before `CallInstructionRecord` machine-node selection can report local record-builder status.
- The va_start/va_copy record builders still keep target-local entry layout, field-size, scratch, register-save, overflow, and field-copy guards by design.
- Do not weaken variadic expectations or add named-case shortcuts.

## Proof

Passed: `cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log`.

Proof log: `test_after.log`.
