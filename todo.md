Status: Active
Source Idea Path: ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Entry Plans

# Current Packet

## Just Finished

Step 2 completed for `src/backend/mir/aarch64/codegen/variadic.cpp`: `require_prepared_variadic_entry_plan` now uses `PreparedVariadicEntryPlanFunction::missing_required_facts` as the prepared entry-plan completeness authority instead of rebuilding the full entry-plan checklist locally.

The obsolete local `missing_variadic_entry_facts` field re-enumeration was removed. A reduced AArch64 helper-consumption guard remains for `helper_resources.scratch_register_count`, `helper_resources.scratch_stack_bytes`, and `helper_operand_homes` because the current AArch64 call path must reject those before it can reach target-local record construction. The existing va_start/scalar va_arg/aggregate va_arg/va_copy record-builder guards and AArch64 save-area/va_list emission paths remain local.

## Suggested Next

Step 3 packet: reduce helper operand-home completeness reconstruction in `variadic_helper_operand_homes_complete`, starting with any prepared-authority helper surface needed for scalar `va_arg` access-plan completeness.

## Watchouts

- Do not move AArch64 va_list layout spelling, save-area instruction selection, concrete stack/register addressing, aggregate copy chunking, or va_arg/va_copy record schemas into shared prealloc code.
- The reduced helper-consumption guard exists because `lower_call_instruction` rejects missing/ incomplete helper operand homes before `CallInstructionRecord` machine-node selection can report local record-builder status.
- For Step 3, add or expose a shared scalar access-plan completeness helper before removing scalar completeness reconstruction in `variadic_helper_operand_homes_complete`.
- Do not weaken variadic expectations or add named-case shortcuts.

## Proof

Passed: `cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log`.

Proof log: `test_after.log`.
