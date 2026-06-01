Status: Active
Source Idea Path: ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared Helper Operand Homes

# Current Packet

## Just Finished

Step 3 completed for `src/backend/prealloc/variadic.hpp` and `src/backend/mir/aarch64/codegen/variadic.cpp`: scalar `va_arg` access-plan completeness now has shared prepared authority through `is_complete_prepared_variadic_scalar_va_arg_access_plan` and `has_complete_prepared_variadic_scalar_va_arg_access_plan`.

`variadic_helper_operand_homes_complete` now consumes the shared scalar prepared helper instead of locally reconstructing scalar access-plan completeness. Target-local record-builder guards and AArch64 ABI emission remain local.

## Suggested Next

Supervisor should review and select the next Step 3 cleanup packet, likely another helper operand-home consumer that can move to prepared authority without moving target-local record construction or ABI emission.

## Watchouts

- Do not move AArch64 va_list layout spelling, save-area instruction selection, concrete stack/register addressing, aggregate copy chunking, or va_arg/va_copy record schemas into shared prealloc code.
- The reduced helper-consumption guard exists because `lower_call_instruction` rejects missing/incomplete helper operand homes before `CallInstructionRecord` machine-node selection can report local record-builder status.
- The scalar record builder still keeps target-local entry/register-save/overflow/scratch guards by design.
- Do not weaken variadic expectations or add named-case shortcuts.

## Proof

Passed: `cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log`.

Proof log: `test_after.log`.
